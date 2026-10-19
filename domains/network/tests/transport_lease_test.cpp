#include "CoroutineTask.hpp"
#include "Engine.hpp"

#include <vix/network/Provider.hpp>
#include <vix/network/TcpConnection.hpp>

#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>

#include <array>
#include <atomic>
#include <barrier>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <coroutine>
#include <cstring>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <utility>

using namespace vix::network;
using namespace vix::network::detail;

template <class T>
struct LeaseObservation
{
  std::mutex mutex;
  std::condition_variable cv;
  std::optional<vix::Completion<T>> completion;
  std::atomic<bool> done{};
  std::atomic<unsigned> terminal_observations{};
};

template <class T>
struct LeaseTask
{
  struct promise_type
  {
    std::shared_ptr<vix::network::test_detail::FinalSuspendState> final_suspend_state =
        std::make_shared<vix::network::test_detail::FinalSuspendState>();
    LeaseTask get_return_object() { return LeaseTask{std::coroutine_handle<promise_type>::from_promise(*this)}; }
    std::suspend_never initial_suspend() noexcept { return {}; }
    vix::network::test_detail::FinalSuspendAwaiter<promise_type> final_suspend() noexcept { return {}; }
    void return_void() noexcept {}
    void unhandled_exception() noexcept { std::terminate(); }
  };

  std::coroutine_handle<promise_type> handle;
  explicit LeaseTask(std::coroutine_handle<promise_type> value) : handle(value) {}
  LeaseTask(LeaseTask &&other) noexcept : handle(std::exchange(other.handle, {})) {}
  LeaseTask(const LeaseTask &) = delete;
  ~LeaseTask() { vix::network::test_detail::destroy_after_final_suspend(handle); }
};

struct ResumeObservation
{
  std::mutex mutex;
  std::condition_variable cv;
  std::atomic<unsigned> resumes{};
  std::atomic<bool> finished{};

  bool wait_finished(std::chrono::milliseconds timeout = std::chrono::seconds(3))
  {
    std::unique_lock lock(mutex);
    return cv.wait_for(lock, timeout, [&]
                       { return finished.load(std::memory_order_acquire); });
  }
};

struct SuspendedOperationTask
{
  struct promise_type
  {
    explicit promise_type(vix::network::Operation<void> &,
                          ResumeObservation *observation_arg) noexcept
        : observation(observation_arg) {}
    SuspendedOperationTask get_return_object() noexcept
    {
      return SuspendedOperationTask{
          std::coroutine_handle<promise_type>::from_promise(*this)};
    }
    std::suspend_always initial_suspend() const noexcept { return {}; }
    struct FinalAwaiter
    {
      ResumeObservation *observation;
      bool await_ready() const noexcept { return false; }
      void await_suspend(std::coroutine_handle<>) const noexcept
      {
        observation->finished.store(true, std::memory_order_release);
        observation->cv.notify_all();
      }
      void await_resume() const noexcept {}
    };
    FinalAwaiter final_suspend() const noexcept { return {observation}; }
    void return_void() const noexcept {}
    void unhandled_exception() const noexcept { std::terminate(); }
    ResumeObservation *observation;
  };

  std::coroutine_handle<promise_type> handle;
  explicit SuspendedOperationTask(std::coroutine_handle<promise_type> value) noexcept
      : handle(value) {}
  SuspendedOperationTask(SuspendedOperationTask &&other) noexcept
      : handle(std::exchange(other.handle, {})) {}
  SuspendedOperationTask(const SuspendedOperationTask &) = delete;
  ~SuspendedOperationTask()
  {
    if (handle)
      handle.destroy();
  }
  void resume() { handle.resume(); }
  void destroy() noexcept
  {
    if (handle)
      handle.destroy();
    handle = {};
  }
};

static SuspendedOperationTask observe_for_abandonment(
    vix::network::Operation<void> operation, ResumeObservation *observation)
{
  const auto completion = co_await std::move(operation);
  if (completion.status() == vix::CompletionStatus::Success)
    observation->resumes.fetch_add(1, std::memory_order_release);
}

template <class T>
LeaseTask<void> observe(vix::network::Operation<T> operation, std::shared_ptr<LeaseObservation<T>> observation)
{
  auto completion = co_await std::move(operation);
  {
    std::lock_guard lock(observation->mutex);
    observation->completion.emplace(std::move(completion));
    observation->terminal_observations.fetch_add(1, std::memory_order_relaxed);
    observation->done.store(true, std::memory_order_release);
  }
  observation->cv.notify_all();
}

template <class T>
bool wait_done(const std::shared_ptr<LeaseObservation<T>> &observation,
               std::chrono::milliseconds timeout = std::chrono::seconds(2))
{
  std::unique_lock lock(observation->mutex);
  return observation->cv.wait_for(lock, timeout, [&]
                                  { return observation->done.load(std::memory_order_acquire); });
}

template <class T>
vix::Completion<T> take_completion(const std::shared_ptr<LeaseObservation<T>> &observation)
{
  std::lock_guard lock(observation->mutex);
  assert(observation->completion);
  return std::move(*observation->completion);
}

struct SocketPair
{
  SocketPair()
  {
    const int result = ::socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0, descriptors);
    assert(result == 0);
  }
  ~SocketPair()
  {
    if (descriptors[0] >= 0)
      (void)::close(descriptors[0]);
    if (descriptors[1] >= 0)
      (void)::close(descriptors[1]);
  }
  int descriptors[2]{-1, -1};
};

struct NativeCallGate
{
  void pause()
  {
    std::unique_lock lock(mutex);
    entered = true;
    cv.notify_all();
    cv.wait(lock, [&]
            { return released; });
  }
  bool wait_until_entered()
  {
    std::unique_lock lock(mutex);
    return cv.wait_for(lock, std::chrono::seconds(2), [&]
                       { return entered; });
  }
  void release()
  {
    {
      std::lock_guard lock(mutex);
      released = true;
    }
    cv.notify_all();
  }
  std::mutex mutex;
  std::condition_variable cv;
  bool entered{};
  bool released{};
};

struct TerminalGate
{
  std::mutex mutex;
  std::condition_variable cv;
  bool published{};
  static void notify(void *context) noexcept
  {
    auto &self = *static_cast<TerminalGate *>(context);
    {
      std::lock_guard lock(self.mutex);
      self.published = true;
    }
    self.cv.notify_all();
  }
  bool wait()
  {
    std::unique_lock lock(mutex);
    return cv.wait_for(lock, std::chrono::seconds(2), [&] { return published; });
  }
};

static void pause_gate(void *context) noexcept
{
  static_cast<NativeCallGate *>(context)->pause();
}

static void queued_start_terminalization_prevents_native_io(bool deadline,
                                                             bool cooperative)
{
  auto provider_result = cooperative ? Provider::create_cooperative() : Provider::create();
  assert(provider_result);
  auto provider = provider_result.take();
  auto core = NetworkAccess::core_for_test(provider);
  SocketPair sockets;
  auto connection = NetworkAccess::adopt_tcp_for_test(provider, sockets.descriptors[0]);
  sockets.descriptors[0] = -1;

  std::array<std::byte, 1> input{};
  auto observed = std::make_shared<LeaseObservation<std::size_t>>();
  std::shared_ptr<OperationRecord<std::size_t>> record;
  vix::CancellationSource cancellation;
  OperationOptions options;
  if (deadline)
    options.deadline = vix::Deadline::after(vix::Duration::zero());
  else
    options.cancellation = cancellation.token();

  NativeCallGate enqueue_gate;
  TerminalGate terminal_gate;
  if (!cooperative)
    NetworkAccess::set_before_start_enqueue_hook_for_test(connection, pause_gate,
                                                           &enqueue_gate);

  std::optional<LeaseTask<void>> task;
  std::thread starter;
  if (!cooperative)
  {
    starter = std::thread([&]
    {
      NetworkAccess::capture_next_receive_record_for_test(connection, record);
      auto operation = connection.receive(input, options);
      assert(record);
      record->set_terminal_hook_for_test(TerminalGate::notify, &terminal_gate);
      task.emplace(observe(std::move(operation), observed));
    });
    assert(enqueue_gate.wait_until_entered());
    if (!deadline)
      (void)cancellation.request_cancel();
    assert(terminal_gate.wait());
    assert(record->phase() == OperationRecord<std::size_t>::Phase::completed_before_suspend);
    assert(core->active_operations_for_test() == 0);
    auto counters = NetworkAccess::transport_counters_for_test(connection);
    assert(counters.native_recv_attempts == 0);
    assert(counters.descriptor_arm_attempts == 0);
    assert(counters.read_buffer_accesses == 0);
    enqueue_gate.release();
    starter.join();
  }
  else
  {
    if (!deadline)
      (void)cancellation.request_cancel();
    NetworkAccess::capture_next_receive_record_for_test(connection, record);
    auto operation = connection.receive(input, options);
    assert(record);
    task.emplace(observe(std::move(operation), observed));
    assert(core->active_operations_for_test() == 1);
    assert(NetworkAccess::transport_counters_for_test(connection).native_recv_attempts == 0);
    assert(provider.poll());
    assert(wait_done(observed));
  }

  assert(wait_done(observed));
  assert(observed->terminal_observations.load(std::memory_order_relaxed) == 1);
  auto completion = take_completion(observed);
  if (deadline)
  {
    assert(completion.status() == vix::CompletionStatus::Failure);
    assert(completion.error().identity() == timed_out);
  }
  else
    assert(completion.status() == vix::CompletionStatus::Cancelled);
  const auto counters = NetworkAccess::transport_counters_for_test(connection);
  assert(counters.native_recv_attempts == 0);
  assert(counters.descriptor_arm_attempts == 0);
  assert(counters.read_buffer_accesses == 0);
  assert(counters.read_slot_releases == 1);
  assert(core->active_operations_for_test() == 0);
  assert(connection.close());
  assert(core->active_resources_for_test() == 0);
  assert(provider.shutdown());
}

static void provider_shutdown_racing_accepted_queued_start(bool cooperative)
{
  auto provider_result = cooperative ? Provider::create_cooperative() : Provider::create();
  assert(provider_result);
  auto provider = provider_result.take();
  auto core = NetworkAccess::core_for_test(provider);
  SocketPair sockets;
  auto connection = NetworkAccess::adopt_tcp_for_test(provider, sockets.descriptors[0]);
  sockets.descriptors[0] = -1;
  std::array<std::byte, 1> input{};
  auto observed = std::make_shared<LeaseObservation<std::size_t>>();

  NativeCallGate execute_gate;
  if (!cooperative)
    NetworkAccess::set_before_start_execute_hook_for_test(connection, pause_gate,
                                                           &execute_gate);
  [[maybe_unused]] auto task = observe(connection.receive(input), observed);

  if (cooperative)
  {
    assert(core->active_operations_for_test() == 1);
    auto shutdown = provider.shutdown();
    assert(!shutdown && shutdown.error().identity() == provider_in_use);
    const std::byte byte{0x5A};
    assert(::send(sockets.descriptors[1], &byte, 1, MSG_NOSIGNAL) == 1);
    auto polled = provider.poll();
    assert(polled);
  }
  else
  {
    assert(execute_gate.wait_until_entered());
    assert(core->active_operations_for_test() == 1);
    auto shutdown = provider.shutdown();
    assert(!shutdown && shutdown.error().identity() == provider_in_use);
    execute_gate.release();
    const std::byte byte{0x5A};
    assert(::send(sockets.descriptors[1], &byte, 1, MSG_NOSIGNAL) == 1);
  }

  assert(wait_done(observed));
  assert(observed->terminal_observations.load(std::memory_order_relaxed) == 1);
  auto completion = take_completion(observed);
  assert(completion.status() == vix::CompletionStatus::Success);
  assert(completion.take() == 1);
  assert(input[0] == std::byte{0x5A});
  assert(core->active_operations_for_test() == 0);
  const auto counters = NetworkAccess::transport_counters_for_test(connection);
  assert(counters.native_recv_attempts == 1);
  assert(counters.read_slot_releases == 1);
  assert(connection.close());
  assert(core->active_resources_for_test() == 0);
  assert(provider.shutdown());
}

static Provider make_cooperative_provider()
{
  auto result = Provider::create_cooperative();
  assert(result);
  return result.take();
}

static void background_connection_descriptor_arming_runs_on_provider()
{
  auto provider_result = Provider::create();
  assert(provider_result);
  auto provider = provider_result.take();
  SocketPair sockets;
  auto connection = NetworkAccess::adopt_tcp_for_test(provider,
                                                       sockets.descriptors[0]);
  sockets.descriptors[0] = -1;

  std::array<std::byte, 1> input{};
  auto received = std::make_shared<LeaseObservation<std::size_t>>();
  [[maybe_unused]] auto receive_task = observe(connection.receive(input), received);
  const std::byte incoming{0x41};
  assert(::send(sockets.descriptors[1], &incoming, 1, MSG_NOSIGNAL) == 1);
  assert(wait_done(received));
  auto receive_completion = std::move(*received->completion);
  assert(receive_completion.status() == vix::CompletionStatus::Success);
  assert(receive_completion.take() == 1);
  assert(input[0] == incoming);

  const std::array<std::byte, 1> outgoing{std::byte{0x42}};
  auto sent = std::make_shared<LeaseObservation<void>>();
  [[maybe_unused]] auto send_task = observe(connection.send(outgoing), sent);
  assert(wait_done(sent));
  auto send_completion = std::move(*sent->completion);
  assert(send_completion.status() == vix::CompletionStatus::Success);
  std::byte observed{};
  assert(::recv(sockets.descriptors[1], &observed, 1, 0) == 1);
  assert(observed == outgoing[0]);

  const auto counters = NetworkAccess::transport_counters_for_test(connection);
  assert(counters.descriptor_arm_attempts == 2);
  assert(counters.descriptor_arm_wrong_thread == 0);
  assert(connection.close());
  assert(provider.shutdown());
}

static void idle_claim_and_provider_lifetime()
{
  auto provider = make_cooperative_provider();
  auto core = NetworkAccess::core_for_test(provider);
  SocketPair sockets;
  auto connection = NetworkAccess::adopt_tcp_for_test(provider, sockets.descriptors[0]);
  sockets.descriptors[0] = -1;
  {
    auto claimed = NetworkAccess::claim_tcp_for_tls(std::move(connection));
    assert(claimed);
    auto lease = claimed.take();
    assert(core->active_resources_for_test() == 1);
    auto stopped = provider.shutdown();
    assert(!stopped && stopped.error().identity() == provider_in_use);
  }
  assert(core->active_resources_for_test() == 0);
  std::byte byte{};
  assert(::recv(sockets.descriptors[1], &byte, 1, 0) == 0);
  assert(provider.shutdown());
}

static void cooperative_claim_preserves_owner_thread_affinity()
{
  auto provider = make_cooperative_provider();
  SocketPair sockets;
  auto connection = NetworkAccess::adopt_tcp_for_test(provider, sockets.descriptors[0]);
  sockets.descriptors[0] = -1;
  std::optional<vix::ErrorIdentity> claim_error;
  std::thread wrong_thread([&]
                           {
    auto rejected = NetworkAccess::claim_tcp_for_tls(std::move(connection));
    assert(!rejected);
    claim_error.emplace(rejected.error().identity()); });
  wrong_thread.join();
  assert(claim_error && *claim_error == operation_in_progress);

  std::array<std::byte, 1> received{};
  auto observation = std::make_shared<LeaseObservation<std::size_t>>();
  [[maybe_unused]] auto task = observe(connection.receive(received), observation);
  constexpr std::byte payload{0x39};
  assert(::send(sockets.descriptors[1], &payload, 1, MSG_NOSIGNAL) == 1);
  for (unsigned attempt = 0; attempt < 16 && !observation->done; ++attempt)
    assert(provider.poll());
  assert(wait_done(observation));
  auto completion = take_completion(observation);
  assert(completion.status() == vix::CompletionStatus::Success);
  assert(completion.take() == 1 && received[0] == payload);
  auto claimed = NetworkAccess::claim_tcp_for_tls(std::move(connection));
  assert(claimed);
  {
    auto lease = claimed.take();
  }
  assert(provider.shutdown());
}

static void active_receive_claim_rejected_and_connection_remains_usable()
{
  auto provider = make_cooperative_provider();
  SocketPair sockets;
  auto connection = NetworkAccess::adopt_tcp_for_test(provider, sockets.descriptors[0]);
  sockets.descriptors[0] = -1;
  std::array<std::byte, 1> received{};
  auto observation = std::make_shared<LeaseObservation<std::size_t>>();
  [[maybe_unused]] auto task = observe(connection.receive(received), observation);

  auto rejected = NetworkAccess::claim_tcp_for_tls(std::move(connection));
  assert(!rejected && rejected.error().identity() == operation_in_progress);

  constexpr std::byte payload{0x51};
  assert(::send(sockets.descriptors[1], &payload, 1, MSG_NOSIGNAL) == 1);
  for (unsigned attempt = 0; attempt < 16 && !observation->done; ++attempt)
    assert(provider.poll());
  assert(wait_done(observation));
  auto completion = take_completion(observation);
  assert(completion.status() == vix::CompletionStatus::Success);
  assert(completion.take() == 1 && received[0] == payload);

  auto claimed = NetworkAccess::claim_tcp_for_tls(std::move(connection));
  assert(claimed);
  {
    auto lease = claimed.take();
  }
  assert(provider.shutdown());
}

static void active_send_claim_rejected_and_connection_remains_usable()
{
  auto provider = make_cooperative_provider();
  SocketPair sockets;
  auto connection = NetworkAccess::adopt_tcp_for_test(provider, sockets.descriptors[0]);
  sockets.descriptors[0] = -1;
  constexpr std::array<std::byte, 4> payload{
      std::byte{'l'}, std::byte{'e'}, std::byte{'a'}, std::byte{'s'}};
  auto observation = std::make_shared<LeaseObservation<void>>();
  [[maybe_unused]] auto task = observe(connection.send(payload), observation);

  auto rejected = NetworkAccess::claim_tcp_for_tls(std::move(connection));
  assert(!rejected && rejected.error().identity() == operation_in_progress);
  for (unsigned attempt = 0; attempt < 16 && !observation->done; ++attempt)
    assert(provider.poll());
  assert(wait_done(observation));
  auto completion = take_completion(observation);
  assert(completion.status() == vix::CompletionStatus::Success);
  std::array<std::byte, payload.size()> received{};
  assert(::recv(sockets.descriptors[1], received.data(), received.size(), 0) ==
         static_cast<ssize_t>(received.size()));
  assert(received == payload);
  auto claimed = NetworkAccess::claim_tcp_for_tls(std::move(connection));
  assert(claimed);
  {
    auto lease = claimed.take();
  }
  assert(provider.shutdown());
}

static void dormant_operations_cannot_bypass_claim()
{
  auto provider = make_cooperative_provider();
  SocketPair sockets;
  auto connection = NetworkAccess::adopt_tcp_for_test(provider, sockets.descriptors[0]);
  sockets.descriptors[0] = -1;
  std::array<std::byte, 1> receive_buffer{};
  constexpr std::array<std::byte, 1> send_buffer{std::byte{'x'}};
  auto dormant_receive = connection.receive(receive_buffer);
  auto dormant_send = connection.send(send_buffer);
  auto claimed = NetworkAccess::claim_tcp_for_tls(std::move(connection));
  assert(claimed);
  {
    auto lease = claimed.take();

    {
      auto receive_observation = std::make_shared<LeaseObservation<std::size_t>>();
      [[maybe_unused]] auto receive_task = observe(std::move(dormant_receive), receive_observation);
      assert(wait_done(receive_observation));
      auto receive_completion = take_completion(receive_observation);
      assert(receive_completion.status() == vix::CompletionStatus::Failure);
      assert(receive_completion.error().identity() == transport_owned);

      auto send_observation = std::make_shared<LeaseObservation<void>>();
      [[maybe_unused]] auto send_task = observe(std::move(dormant_send), send_observation);
      assert(wait_done(send_observation));
      auto send_completion = take_completion(send_observation);
      assert(send_completion.status() == vix::CompletionStatus::Failure);
      assert(send_completion.error().identity() == transport_owned);
      const auto counters = NetworkAccess::transport_counters_for_test(lease);
      assert(counters.native_recv_attempts == 0 && counters.native_send_attempts == 0);
      assert(counters.read_buffer_accesses == 0 && counters.write_buffer_accesses == 0);
      std::byte byte{};
      assert(::recv(sockets.descriptors[1], &byte, 1, 0) == -1);

      std::array<std::byte, 1> lease_buffer{};
      auto lease_receive_observation = std::make_shared<LeaseObservation<std::size_t>>();
      [[maybe_unused]] auto lease_receive_task = observe(lease.receive(lease_buffer), lease_receive_observation);
      constexpr std::byte incoming{0x45};
      assert(::send(sockets.descriptors[1], &incoming, 1, MSG_NOSIGNAL) == 1);
      for (unsigned attempt = 0; attempt < 16 && !lease_receive_observation->done; ++attempt)
        assert(provider.poll());
      assert(wait_done(lease_receive_observation));
      auto lease_receive_completion = take_completion(lease_receive_observation);
      assert(lease_receive_completion.status() == vix::CompletionStatus::Success);
      assert(lease_receive_completion.take() == 1 && lease_buffer[0] == incoming);

      constexpr std::array<std::byte, 1> outgoing{std::byte{0x46}};
      auto lease_send_observation = std::make_shared<LeaseObservation<void>>();
      [[maybe_unused]] auto lease_send_task = observe(lease.send(outgoing), lease_send_observation);
      for (unsigned attempt = 0; attempt < 16 && !lease_send_observation->done; ++attempt)
        assert(provider.poll());
      assert(wait_done(lease_send_observation));
      auto lease_send_completion = take_completion(lease_send_observation);
      assert(lease_send_completion.status() == vix::CompletionStatus::Success);
      std::byte outgoing_received{};
      assert(::recv(sockets.descriptors[1], &outgoing_received, 1, 0) == 1);
      assert(outgoing_received == outgoing[0]);
    }
    // The provider must remain alive while the lease owns its transport.
    auto still_owned = provider.shutdown();
    assert(!still_owned && still_owned.error().identity() == provider_in_use);
  }
  assert(provider.shutdown());
}

static void claim_and_precreated_starter_race()
{
  auto provider_result = Provider::create();
  assert(provider_result);
  auto provider = provider_result.take();
  SocketPair sockets;
  auto connection = NetworkAccess::adopt_tcp_for_test(provider, sockets.descriptors[0]);
  sockets.descriptors[0] = -1;
  std::array<std::byte, 1> receive_buffer{};
  auto dormant = connection.receive(receive_buffer);
  auto observation = std::make_shared<LeaseObservation<std::size_t>>();
  std::optional<TcpTransportLease> lease;
  std::optional<vix::ErrorIdentity> claim_error;
  std::optional<LeaseTask<void>> task;
  std::barrier start{3};
  std::thread starter([&]
                      {
    start.arrive_and_wait();
    task.emplace(observe(std::move(dormant), observation)); });
  std::thread claimer([&]
                      {
    start.arrive_and_wait();
    auto result = NetworkAccess::claim_tcp_for_tls(std::move(connection));
    if (result)
      lease.emplace(result.take());
    else
      claim_error.emplace(result.error().identity()); });
  start.arrive_and_wait();
  starter.join();
  claimer.join();

  if (lease)
  {
    assert(wait_done(observation));
    auto completion = take_completion(observation);
    assert(completion.status() == vix::CompletionStatus::Failure);
    assert(completion.error().identity() == transport_owned);
  }
  else
  {
    assert(claim_error && *claim_error == operation_in_progress);
    constexpr std::byte payload{0x62};
    assert(::send(sockets.descriptors[1], &payload, 1, MSG_NOSIGNAL) == 1);
    assert(wait_done(observation));
    auto completion = take_completion(observation);
    assert(completion.status() == vix::CompletionStatus::Success);
    assert(completion.take() == 1 && receive_buffer[0] == payload);
    auto retry = NetworkAccess::claim_tcp_for_tls(std::move(connection));
    assert(retry);
    lease.emplace(retry.take());
  }
  lease.reset();
  assert(provider.shutdown());
}

static void claim_rejected_at_native_receive_boundary()
{
  auto provider_result = Provider::create();
  assert(provider_result);
  auto provider = provider_result.take();
  SocketPair sockets;
  auto connection = NetworkAccess::adopt_tcp_for_test(provider, sockets.descriptors[0]);
  sockets.descriptors[0] = -1;
  NativeCallGate gate;
  NetworkAccess::set_before_native_receive_hook_for_test(
      connection,
      [](void *context) noexcept
      { static_cast<NativeCallGate *>(context)->pause(); },
      &gate);
  std::array<std::byte, 1> receive_buffer{};
  auto observation = std::make_shared<LeaseObservation<std::size_t>>();
  [[maybe_unused]] auto task = observe(connection.receive(receive_buffer), observation);
  assert(gate.wait_until_entered());

  auto rejected = NetworkAccess::claim_tcp_for_tls(std::move(connection));
  assert(!rejected && rejected.error().identity() == operation_in_progress);
  constexpr std::byte payload{0x73};
  assert(::send(sockets.descriptors[1], &payload, 1, MSG_NOSIGNAL) == 1);
  NetworkAccess::set_before_native_receive_hook_for_test(connection, nullptr, nullptr);
  gate.release();
  assert(wait_done(observation));
  auto completion = take_completion(observation);
  assert(completion.status() == vix::CompletionStatus::Success);
  assert(completion.take() == 1 && receive_buffer[0] == payload);
  auto claimed = NetworkAccess::claim_tcp_for_tls(std::move(connection));
  assert(claimed);
  {
    auto lease = claimed.take();
  }
  assert(provider.shutdown());
}

static void cancellation_and_deadline_keep_active_slot_until_terminal()
{
  {
    auto provider = make_cooperative_provider();
    SocketPair sockets;
    auto connection = NetworkAccess::adopt_tcp_for_test(provider, sockets.descriptors[0]);
    sockets.descriptors[0] = -1;
    std::array<std::byte, 1> buffer{};
    vix::CancellationSource cancellation;
    auto observation = std::make_shared<LeaseObservation<std::size_t>>();
    [[maybe_unused]] auto task = observe(
        connection.receive(buffer, {cancellation.token(), {}}), observation);
    assert(cancellation.request_cancel());
    auto premature = NetworkAccess::claim_tcp_for_tls(std::move(connection));
    assert(!premature && premature.error().identity() == operation_in_progress);
    assert(provider.poll());
    assert(wait_done(observation));
    assert(take_completion(observation).status() == vix::CompletionStatus::Cancelled);
    auto claimed = NetworkAccess::claim_tcp_for_tls(std::move(connection));
    assert(claimed);
    {
      auto lease = claimed.take();
    }
    assert(provider.shutdown());
  }
  {
    auto provider = make_cooperative_provider();
    SocketPair sockets;
    auto connection = NetworkAccess::adopt_tcp_for_test(provider, sockets.descriptors[0]);
    sockets.descriptors[0] = -1;
    std::array<std::byte, 1> buffer{};
    auto observation = std::make_shared<LeaseObservation<std::size_t>>();
    [[maybe_unused]] auto task = observe(
        connection.receive(buffer, {{}, vix::Deadline::at(vix::MonotonicClock::now())}), observation);
    auto premature = NetworkAccess::claim_tcp_for_tls(std::move(connection));
    assert(!premature && premature.error().identity() == operation_in_progress);
    assert(provider.poll());
    assert(wait_done(observation));
    auto completion = take_completion(observation);
    assert(completion.status() == vix::CompletionStatus::Failure);
    assert(completion.error().identity() == timed_out);
    auto claimed = NetworkAccess::claim_tcp_for_tls(std::move(connection));
    assert(claimed);
    {
      auto lease = claimed.take();
    }
    assert(provider.shutdown());
  }
}

static void lease_destruction_closes_even_with_stale_operation_state()
{
  auto provider = make_cooperative_provider();
  auto core = NetworkAccess::core_for_test(provider);
  SocketPair sockets;
  auto connection = NetworkAccess::adopt_tcp_for_test(provider, sockets.descriptors[0]);
  const int leased_descriptor = NetworkAccess::native_fd_for_test(connection);
  sockets.descriptors[0] = -1;
  std::array<std::byte, 1> buffer{};
  auto stale = connection.receive(buffer);
  {
    auto claimed = NetworkAccess::claim_tcp_for_tls(std::move(connection));
    assert(claimed);
    auto lease = claimed.take();
    assert(core->active_resources_for_test() == 1);
  }
  assert(core->active_resources_for_test() == 0);
  std::byte byte{};
  assert(::recv(sockets.descriptors[1], &byte, 1, 0) == 0);
  assert(::dup2(sockets.descriptors[1], leased_descriptor) == leased_descriptor);
  auto observation = std::make_shared<LeaseObservation<std::size_t>>();
  [[maybe_unused]] auto task = observe(std::move(stale), observation);
  assert(wait_done(observation));
  auto completion = take_completion(observation);
  assert(completion.status() == vix::CompletionStatus::Failure);
  assert(completion.error().identity() == transport_owned);
  assert(::fcntl(leased_descriptor, F_GETFD) >= 0);
  (void)::close(leased_descriptor);
  assert(provider.shutdown());
}

static void lease_destruction_defers_close_until_cancelled_child_retires()
{
  auto provider = make_cooperative_provider();
  auto core = NetworkAccess::core_for_test(provider);
  SocketPair sockets;
  auto connection = NetworkAccess::adopt_tcp_for_test(provider, sockets.descriptors[0]);
  sockets.descriptors[0] = -1;
  std::array<std::byte, 1> buffer{};
  vix::CancellationSource cancellation;
  auto observation = std::make_shared<LeaseObservation<std::size_t>>();
  std::optional<LeaseTask<void>> task;
  {
    auto claimed = NetworkAccess::claim_tcp_for_tls(std::move(connection));
    assert(claimed);
    auto lease = claimed.take();
    task.emplace(observe(lease.receive(buffer, {cancellation.token(), {}}), observation));
  }
  assert(core->active_resources_for_test() == 1);
  auto still_active = provider.shutdown();
  assert(!still_active && still_active.error().identity() == provider_in_use);
  std::byte byte{};
  assert(::recv(sockets.descriptors[1], &byte, 1, 0) == -1);
  assert(cancellation.request_cancel());
  assert(provider.poll());
  assert(wait_done(observation));
  assert(take_completion(observation).status() == vix::CompletionStatus::Cancelled);
  task.reset();
  assert(core->active_resources_for_test() == 0);
  assert(::recv(sockets.descriptors[1], &byte, 1, 0) == 0);
  assert(provider.shutdown());
}

static void lease_destruction_defers_close_until_deadline_child_retires()
{
  auto provider = make_cooperative_provider();
  auto core = NetworkAccess::core_for_test(provider);
  SocketPair sockets;
  auto connection = NetworkAccess::adopt_tcp_for_test(provider, sockets.descriptors[0]);
  sockets.descriptors[0] = -1;
  std::array<std::byte, 1> buffer{};
  auto observation = std::make_shared<LeaseObservation<std::size_t>>();
  std::optional<LeaseTask<void>> task;
  {
    auto claimed = NetworkAccess::claim_tcp_for_tls(std::move(connection));
    assert(claimed);
    auto lease = claimed.take();
    task.emplace(observe(
        lease.receive(buffer, {{}, vix::Deadline::at(vix::MonotonicClock::now())}), observation));
  }
  assert(core->active_resources_for_test() == 1);
  auto still_active = provider.shutdown();
  assert(!still_active && still_active.error().identity() == provider_in_use);
  assert(provider.poll());
  assert(wait_done(observation));
  auto completion = take_completion(observation);
  assert(completion.status() == vix::CompletionStatus::Failure);
  assert(completion.error().identity() == timed_out);
  task.reset();
  assert(core->active_resources_for_test() == 0);
  std::byte byte{};
  assert(::recv(sockets.descriptors[1], &byte, 1, 0) == 0);
  assert(provider.shutdown());
}

static void http_claim_idle_provider_and_owned_io()
{
  auto provider = make_cooperative_provider();
  auto core = NetworkAccess::core_for_test(provider);
  SocketPair sockets;
  auto connection = NetworkAccess::adopt_tcp_for_test(provider, sockets.descriptors[0]);
  sockets.descriptors[0] = -1;
  auto claimed = NetworkAccess::claim_tcp_for_http(std::move(connection));
  assert(claimed);
  {
    auto lease = claimed.take();
    assert(NetworkAccess::provider_for_http_lease(lease) == core);
    assert(!NetworkAccess::provider_for_tls_lease(lease));
    assert(core->active_resources_for_test() == 1);

    std::array<std::byte, 2> received{};
    auto receive_observation = std::make_shared<LeaseObservation<std::size_t>>();
    [[maybe_unused]] auto receive_task = observe(lease.receive(received), receive_observation);
    constexpr std::array<std::byte, 2> incoming{std::byte{'H'}, std::byte{'!'}};
    assert(::send(sockets.descriptors[1], incoming.data(), incoming.size(), MSG_NOSIGNAL) ==
           static_cast<ssize_t>(incoming.size()));
    for (unsigned attempt = 0; attempt < 16 && !receive_observation->done; ++attempt)
      assert(provider.poll());
    assert(wait_done(receive_observation));
    auto received_completion = take_completion(receive_observation);
    assert(received_completion.status() == vix::CompletionStatus::Success);
    assert(received_completion.take() == incoming.size() && received == incoming);

    constexpr std::array<std::byte, 2> outgoing{std::byte{'O'}, std::byte{'K'}};
    auto send_observation = std::make_shared<LeaseObservation<void>>();
    [[maybe_unused]] auto send_task = observe(lease.send(outgoing), send_observation);
    for (unsigned attempt = 0; attempt < 16 && !send_observation->done; ++attempt)
      assert(provider.poll());
    assert(wait_done(send_observation));
    assert(take_completion(send_observation).status() == vix::CompletionStatus::Success);
    std::array<std::byte, outgoing.size()> peer_bytes{};
    assert(::recv(sockets.descriptors[1], peer_bytes.data(), peer_bytes.size(), 0) ==
           static_cast<ssize_t>(peer_bytes.size()));
    assert(peer_bytes == outgoing);
    auto still_owned = provider.shutdown();
    assert(!still_owned && still_owned.error().identity() == provider_in_use);
  }
  assert(core->active_resources_for_test() == 0);
  std::byte byte{};
  assert(::recv(sockets.descriptors[1], &byte, 1, 0) == 0);
  assert(provider.shutdown());
}

static void http_claim_rejects_active_receive_and_send()
{
  {
    auto provider = make_cooperative_provider();
    SocketPair sockets;
    auto connection = NetworkAccess::adopt_tcp_for_test(provider, sockets.descriptors[0]);
    sockets.descriptors[0] = -1;
    std::array<std::byte, 1> buffer{};
    auto observation = std::make_shared<LeaseObservation<std::size_t>>();
    [[maybe_unused]] auto task = observe(connection.receive(buffer), observation);
    auto rejected = NetworkAccess::claim_tcp_for_http(std::move(connection));
    assert(!rejected && rejected.error().identity() == operation_in_progress);
    constexpr std::byte incoming{0x31};
    assert(::send(sockets.descriptors[1], &incoming, 1, MSG_NOSIGNAL) == 1);
    for (unsigned attempt = 0; attempt < 16 && !observation->done; ++attempt)
      assert(provider.poll());
    assert(wait_done(observation));
    auto completion = take_completion(observation);
    assert(completion.status() == vix::CompletionStatus::Success);
    assert(completion.take() == 1 && buffer[0] == incoming);
    {
      auto retry = NetworkAccess::claim_tcp_for_http(std::move(connection));
      assert(retry);
    }
    assert(provider.shutdown());
  }
  {
    auto provider = make_cooperative_provider();
    SocketPair sockets;
    auto connection = NetworkAccess::adopt_tcp_for_test(provider, sockets.descriptors[0]);
    sockets.descriptors[0] = -1;
    constexpr std::array<std::byte, 3> payload{std::byte{'w'}, std::byte{'r'}, std::byte{'t'}};
    auto observation = std::make_shared<LeaseObservation<void>>();
    [[maybe_unused]] auto task = observe(connection.send(payload), observation);
    auto rejected = NetworkAccess::claim_tcp_for_http(std::move(connection));
    assert(!rejected && rejected.error().identity() == operation_in_progress);
    for (unsigned attempt = 0; attempt < 16 && !observation->done; ++attempt)
      assert(provider.poll());
    assert(wait_done(observation));
    assert(take_completion(observation).status() == vix::CompletionStatus::Success);
    std::array<std::byte, payload.size()> peer_bytes{};
    assert(::recv(sockets.descriptors[1], peer_bytes.data(), peer_bytes.size(), 0) ==
           static_cast<ssize_t>(peer_bytes.size()));
    assert(peer_bytes == payload);
    {
      auto retry = NetworkAccess::claim_tcp_for_http(std::move(connection));
      assert(retry);
    }
    assert(provider.shutdown());
  }
}

static void http_claim_rejects_dormant_operations_and_arbitrates_starter()
{
  {
    auto provider = make_cooperative_provider();
    SocketPair sockets;
    auto connection = NetworkAccess::adopt_tcp_for_test(provider, sockets.descriptors[0]);
    sockets.descriptors[0] = -1;
    std::array<std::byte, 1> receive_buffer{};
    constexpr std::array<std::byte, 1> send_buffer{std::byte{0x52}};
    auto dormant_receive = connection.receive(receive_buffer);
    auto dormant_send = connection.send(send_buffer);
    auto claimed = NetworkAccess::claim_tcp_for_http(std::move(connection));
    assert(claimed);
    auto lease = claimed.take();
    auto receive_observation = std::make_shared<LeaseObservation<std::size_t>>();
    [[maybe_unused]] auto receive_task = observe(std::move(dormant_receive), receive_observation);
    assert(wait_done(receive_observation));
    auto receive_completion = take_completion(receive_observation);
    assert(receive_completion.status() == vix::CompletionStatus::Failure);
    assert(receive_completion.error().identity() == transport_owned);
    auto send_observation = std::make_shared<LeaseObservation<void>>();
    [[maybe_unused]] auto send_task = observe(std::move(dormant_send), send_observation);
    assert(wait_done(send_observation));
    auto send_completion = take_completion(send_observation);
    assert(send_completion.status() == vix::CompletionStatus::Failure);
    assert(send_completion.error().identity() == transport_owned);
    const auto counters = NetworkAccess::transport_counters_for_test(lease);
    assert(counters.native_recv_attempts == 0 && counters.native_send_attempts == 0);
    assert(counters.read_buffer_accesses == 0 && counters.write_buffer_accesses == 0);
    lease.close();
    assert(provider.shutdown());
  }

  auto provider_result = Provider::create();
  assert(provider_result);
  auto provider = provider_result.take();
  SocketPair sockets;
  auto connection = NetworkAccess::adopt_tcp_for_test(provider, sockets.descriptors[0]);
  sockets.descriptors[0] = -1;
  std::array<std::byte, 1> receive_buffer{};
  auto dormant = connection.receive(receive_buffer);
  auto observation = std::make_shared<LeaseObservation<std::size_t>>();
  std::optional<TcpTransportLease> lease;
  std::optional<vix::ErrorIdentity> claim_error;
  std::optional<LeaseTask<void>> task;
  std::barrier start{3};
  std::thread starter([&]
                      { start.arrive_and_wait(); task.emplace(observe(std::move(dormant), observation)); });
  std::thread claimer([&]
                      {
    start.arrive_and_wait();
    auto result = NetworkAccess::claim_tcp_for_http(std::move(connection));
    if (result) lease.emplace(result.take());
    else claim_error.emplace(result.error().identity()); });
  start.arrive_and_wait();
  starter.join();
  claimer.join();
  if (lease)
  {
    assert(wait_done(observation));
    const auto completion = take_completion(observation);
    assert(completion.status() == vix::CompletionStatus::Failure);
    assert(completion.error().identity() == transport_owned);
    assert(NetworkAccess::transport_counters_for_test(*lease).native_recv_attempts == 0);
  }
  else
  {
    assert(claim_error && *claim_error == operation_in_progress);
    constexpr std::byte incoming{0x63};
    assert(::send(sockets.descriptors[1], &incoming, 1, MSG_NOSIGNAL) == 1);
    assert(wait_done(observation));
    auto completion = take_completion(observation);
    assert(completion.status() == vix::CompletionStatus::Success);
    assert(completion.take() == 1 && receive_buffer[0] == incoming);
    auto retry = NetworkAccess::claim_tcp_for_http(std::move(connection));
    assert(retry);
    lease.emplace(retry.take());
  }
  lease.reset();
  task.reset();
  assert(provider.shutdown());
}

static void tls_and_http_claims_are_mutually_exclusive()
{
  auto provider = make_cooperative_provider();
  SocketPair sockets;
  auto connection = NetworkAccess::adopt_tcp_for_test(provider, sockets.descriptors[0]);
  sockets.descriptors[0] = -1;
  std::optional<TcpConnection> tls_candidate{NetworkAccess::alias_tcp_for_test(connection)};
  std::optional<TcpConnection> http_candidate{NetworkAccess::alias_tcp_for_test(connection)};
  auto tls_claim = NetworkAccess::claim_tcp_for_tls(std::move(*tls_candidate));
  assert(tls_claim);
  auto tls_lease = tls_claim.take();
  auto http_claim = NetworkAccess::claim_tcp_for_http(std::move(*http_candidate));
  assert(!http_claim && http_claim.error().identity() == transport_owned);
  tls_candidate.reset();
  http_candidate.reset();
  tls_lease.close();
  assert(provider.shutdown());
}

static void http_claim_preserves_cooperative_affinity_and_defers_close()
{
  auto provider = make_cooperative_provider();
  auto core = NetworkAccess::core_for_test(provider);
  SocketPair sockets;
  auto connection = NetworkAccess::adopt_tcp_for_test(provider, sockets.descriptors[0]);
  sockets.descriptors[0] = -1;
  std::optional<vix::ErrorIdentity> thread_error;
  std::thread wrong_thread([&]
                           {
    auto rejected = NetworkAccess::claim_tcp_for_http(std::move(connection));
    assert(!rejected);
    thread_error.emplace(rejected.error().identity()); });
  wrong_thread.join();
  assert(thread_error && *thread_error == operation_in_progress);

  auto claimed = NetworkAccess::claim_tcp_for_http(std::move(connection));
  assert(claimed);
  auto lease = claimed.take();
  std::array<std::byte, 1> buffer{};
  vix::CancellationSource cancellation;
  auto observation = std::make_shared<LeaseObservation<std::size_t>>();
  [[maybe_unused]] auto task = observe(lease.receive(buffer, {cancellation.token(), {}}), observation);
  lease.close();
  assert(core->active_resources_for_test() == 1);
  auto still_owned = provider.shutdown();
  assert(!still_owned && still_owned.error().identity() == provider_in_use);
  std::byte byte{};
  assert(::recv(sockets.descriptors[1], &byte, 1, 0) == -1);
  assert(cancellation.request_cancel());
  assert(provider.poll());
  assert(wait_done(observation));
  assert(take_completion(observation).status() == vix::CompletionStatus::Cancelled);
  assert(core->active_resources_for_test() == 0);
  assert(::recv(sockets.descriptors[1], &byte, 1, 0) == 0);
  assert(provider.shutdown());
}

static void completion_before_suspend_publishes_before_await_resume()
{
  auto made = ProviderCore::create();
  assert(made);
  auto provider = made.take();
  const auto wait_for_phase = [](const auto &record, OperationRecord<void>::Phase phase)
  {
    for (unsigned attempt = 0; attempt < 2000; ++attempt)
    {
      if (record->phase() == phase)
        return true;
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return false;
  };

  {
    auto record = std::make_shared<OperationRecord<void>>(provider);
    record->hold_completion_publication_for_test(true);
    assert(record->begin(std::noop_coroutine()));
    std::thread completion([record]
                           { record->complete(vix::Completion<void>::success()); });
    assert(wait_for_phase(record,
                          OperationRecord<void>::Phase::completing_before_suspend));
    // Completion has won but is deliberately not stored yet. The awaiter must
    // commit suspension and resume only after terminal publication finishes.
    assert(record->commit_suspend());
    record->hold_completion_publication_for_test(false);
    completion.join();
    assert(record->terminal_trace_for_test().size() == 8);
    assert(record->phase() == OperationRecord<void>::Phase::completed_scheduled);
    assert(record->take_completion().status() == vix::CompletionStatus::Success);
  }

  {
    auto record = std::make_shared<OperationRecord<void>>(provider);
    record->hold_completion_publication_for_test(true);
    assert(record->begin(std::noop_coroutine()));
    std::thread completion([record]
                           { record->complete(vix::Completion<void>::success()); });
    assert(wait_for_phase(record,
                          OperationRecord<void>::Phase::completing_before_suspend));
    record->hold_completion_publication_for_test(false);
    completion.join();
    assert(record->phase() == OperationRecord<void>::Phase::completed_before_suspend);
    // Returning false from await_suspend is safe only once both the value and
    // provider-operation retirement are visible.
    assert(!record->commit_suspend());
    assert(record->take_completion().status() == vix::CompletionStatus::Success);
  }
  assert(provider->active_operations_for_test() == 0);
  assert(provider->shutdown());
}

static void suspended_awaiter_abandonment_arbitrates_ticket_lifetime()
{
  auto provider_result = Provider::create_cooperative();
  assert(provider_result);
  auto provider = provider_result.take();
  auto core = NetworkAccess::core_for_test(provider);

  // A terminal completion before await_suspend commits is consumed inline;
  // it does not need a scheduled resume ticket.
  {
    std::shared_ptr<OperationRecord<void>> record;
    auto operation = OperationFactory<void>::make(
        core, [&record](auto candidate)
        {
          record = candidate;
          record->complete(vix::Completion<void>::success());
        });
    ResumeObservation observation;
    auto task = observe_for_abandonment(std::move(operation), &observation);
    task.resume();
    assert(record);
    assert(observation.finished.load(std::memory_order_acquire));
    assert(observation.resumes.load(std::memory_order_acquire) == 1);
    assert(record->phase() == OperationRecord<void>::Phase::consumed);
  }

  // A terminal winner pauses before completion/ticket publication. Destroying
  // the already-suspended parent wins abandonment; publication must not later
  // enqueue a resume of its destroyed frame.
  {
    std::shared_ptr<OperationRecord<void>> record;
    auto operation = OperationFactory<void>::make(
        core, [&record](auto candidate)
        {
          record = candidate;
          record->hold_completion_publication_for_test(true);
        });
    ResumeObservation observation;
    auto task = observe_for_abandonment(std::move(operation), &observation);
    task.resume();
    assert(record && record->phase() == OperationRecord<void>::Phase::suspended);
    std::thread completion([record]
                           { record->complete(vix::Completion<void>::success()); });
    bool reached_terminalizing = false;
    for (unsigned attempt = 0; attempt < 2000; ++attempt)
    {
      if (record->phase() == OperationRecord<void>::Phase::completing_suspended)
      {
        reached_terminalizing = true;
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    assert(reached_terminalizing);
    task.destroy();
    record->hold_completion_publication_for_test(false);
    completion.join();
    assert(record->phase() == OperationRecord<void>::Phase::abandoned);
    assert(core->active_operations_for_test() == 0);
    assert(observation.resumes.load(std::memory_order_acquire) == 0);
    assert(provider.poll());
    assert(observation.resumes.load(std::memory_order_acquire) == 0);
  }

  // Once a ticket is published but cooperative dispatch is held, destroying
  // the suspended frame atomically abandons the queued ticket.
  {
    core->hold_cooperative_resumes_for_test(true);
    std::shared_ptr<OperationRecord<void>> record;
    auto operation = OperationFactory<void>::make(
        core, [&record](auto candidate) { record = candidate; });
    ResumeObservation observation;
    auto task = observe_for_abandonment(std::move(operation), &observation);
    task.resume();
    assert(record && record->phase() == OperationRecord<void>::Phase::suspended);
    record->complete(vix::Completion<void>::success());
    assert(record->phase() == OperationRecord<void>::Phase::completed_scheduled);
    assert(!observation.finished.load(std::memory_order_acquire));
    task.destroy();
    assert(record->phase() == OperationRecord<void>::Phase::abandoned);
    core->hold_cooperative_resumes_for_test(false);
    (void)provider.poll();
    assert(observation.resumes.load(std::memory_order_acquire) == 0);
    assert(core->active_operations_for_test() == 0);
  }

  // Cooperative completion is queued, not resumed inline. Polling is the only
  // progress point, and the operation resumes exactly once on the owner.
  {
    std::shared_ptr<OperationRecord<void>> record;
    auto operation = OperationFactory<void>::make(
        core, [&record](auto candidate) { record = candidate; });
    ResumeObservation observation;
    auto task = observe_for_abandonment(std::move(operation), &observation);
    task.resume();
    record->complete(vix::Completion<void>::success());
    assert(!observation.finished.load(std::memory_order_acquire));
    auto polled = provider.poll();
    assert(polled && polled.value() > 0);
    assert(observation.wait_finished());
    assert(observation.resumes.load(std::memory_order_acquire) == 1);
    assert(record->phase() == OperationRecord<void>::Phase::consumed);
  }

  assert(core->active_operations_for_test() == 0);
  assert(provider.shutdown());
}

static void resume_ticket_claim_and_abandon_are_mutually_exclusive()
{
  std::barrier start{3};
  ResumeTicket ticket{std::noop_coroutine()};
  std::atomic<bool> claimed{};
  std::atomic<bool> abandoned{};
  std::thread scheduler([&]
                        {
    start.arrive_and_wait();
    claimed.store(ticket.claim_resume(), std::memory_order_release); });
  std::thread destroyer([&]
                         {
    start.arrive_and_wait();
    abandoned.store(ticket.abandon(), std::memory_order_release); });
  start.arrive_and_wait();
  scheduler.join();
  destroyer.join();
  assert(claimed.load(std::memory_order_acquire) !=
         abandoned.load(std::memory_order_acquire));
}

static void background_resume_and_abandonment_lifecycle()
{
  auto provider_result = Provider::create();
  assert(provider_result);
  auto provider = provider_result.take();
  auto core = NetworkAccess::core_for_test(provider);

  // Background dispatch resumes a published operation exactly once.
  {
    std::shared_ptr<OperationRecord<void>> record;
    auto operation = OperationFactory<void>::make(
        core, [&record](auto candidate) { record = candidate; });
    ResumeObservation observation;
    auto task = observe_for_abandonment(std::move(operation), &observation);
    task.resume();
    assert(record && record->phase() == OperationRecord<void>::Phase::suspended);
    record->complete(vix::Completion<void>::success());
    assert(observation.wait_finished());
    assert(observation.resumes.load(std::memory_order_acquire) == 1);
    assert(record->phase() == OperationRecord<void>::Phase::consumed);
  }

  // Frame destruction wins while terminal publication is deliberately held.
  // The background dispatcher must never receive a ticket for that frame.
  {
    std::shared_ptr<OperationRecord<void>> record;
    auto operation = OperationFactory<void>::make(
        core, [&record](auto candidate)
        {
          record = candidate;
          record->hold_completion_publication_for_test(true);
        });
    ResumeObservation observation;
    auto task = observe_for_abandonment(std::move(operation), &observation);
    task.resume();
    std::thread completion([record]
                           { record->complete(vix::Completion<void>::success()); });
    bool terminalizing = false;
    for (unsigned attempt = 0; attempt < 2000; ++attempt)
    {
      if (record->phase() == OperationRecord<void>::Phase::completing_suspended)
      {
        terminalizing = true;
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    assert(terminalizing);
    task.destroy();
    record->hold_completion_publication_for_test(false);
    completion.join();
    assert(record->phase() == OperationRecord<void>::Phase::abandoned);
    assert(core->active_operations_for_test() == 0);
    assert(observation.resumes.load(std::memory_order_acquire) == 0);
  }

  assert(provider.shutdown());
}

int main()
{
  background_connection_descriptor_arming_runs_on_provider();
  queued_start_terminalization_prevents_native_io(false, false);
  queued_start_terminalization_prevents_native_io(true, false);
  queued_start_terminalization_prevents_native_io(false, true);
  provider_shutdown_racing_accepted_queued_start(false);
  provider_shutdown_racing_accepted_queued_start(true);
  idle_claim_and_provider_lifetime();
  cooperative_claim_preserves_owner_thread_affinity();
  active_receive_claim_rejected_and_connection_remains_usable();
  active_send_claim_rejected_and_connection_remains_usable();
  dormant_operations_cannot_bypass_claim();
  claim_and_precreated_starter_race();
  claim_rejected_at_native_receive_boundary();
  cancellation_and_deadline_keep_active_slot_until_terminal();
  lease_destruction_closes_even_with_stale_operation_state();
  lease_destruction_defers_close_until_cancelled_child_retires();
  lease_destruction_defers_close_until_deadline_child_retires();
  http_claim_idle_provider_and_owned_io();
  http_claim_rejects_active_receive_and_send();
  http_claim_rejects_dormant_operations_and_arbitrates_starter();
  tls_and_http_claims_are_mutually_exclusive();
  http_claim_preserves_cooperative_affinity_and_defers_close();
  completion_before_suspend_publishes_before_await_resume();
  suspended_awaiter_abandonment_arbitrates_ticket_lifetime();
  resume_ticket_claim_and_abandon_are_mutually_exclusive();
  background_resume_and_abandonment_lifecycle();
}
