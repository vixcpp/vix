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

template <class T>
LeaseTask<void> observe(vix::network::Operation<T> operation, std::shared_ptr<LeaseObservation<T>> observation)
{
  auto completion = co_await std::move(operation);
  {
    std::lock_guard lock(observation->mutex);
    observation->completion.emplace(std::move(completion));
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
    assert(::socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0, descriptors) == 0);
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

static Provider make_cooperative_provider()
{
  auto result = Provider::create_cooperative();
  assert(result);
  return result.take();
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

int main()
{
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
}
