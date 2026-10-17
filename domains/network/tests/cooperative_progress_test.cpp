#include <vix/network/Address.hpp>
#include <vix/network/Endpoint.hpp>
#include <vix/network/Errors.hpp>
#include <vix/network/Provider.hpp>
#include <vix/network/TcpConnection.hpp>
#include <vix/network/TcpListener.hpp>
#include "CoroutineTask.hpp"
#include "Engine.hpp"
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <coroutine>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <thread>
#include <utility>

using namespace vix::network;

template <class T>
struct Observation
{
  std::mutex mutex;
  std::optional<vix::Completion<T>> completion;
  std::atomic<unsigned> resumes{};
  std::atomic<bool> reentrant_poll_rejected{};
};

template <class T>
struct Task
{
  struct promise_type
  {
    std::mutex final_mutex;
    std::condition_variable final_cv;
    bool final_suspended{};
    Task get_return_object() { return Task{std::coroutine_handle<promise_type>::from_promise(*this)}; }
    std::suspend_never initial_suspend() noexcept { return {}; }
    vix::network::test_detail::FinalSuspendAwaiter<promise_type> final_suspend() noexcept { return {}; }
    void return_void() noexcept {}
    void unhandled_exception() noexcept { std::terminate(); }
  };
  std::coroutine_handle<promise_type> handle;
  ~Task() { vix::network::test_detail::destroy_after_final_suspend(handle); }
  void abandon_pending() noexcept
  {
    auto owned = std::exchange(handle, {});
    if (owned)
      owned.destroy();
  }
};

template <class T>
Task<void> await_operation(Operation<T> operation, std::shared_ptr<Observation<T>> observation,
                           Provider *reentrant_provider = nullptr)
{
  auto completion = co_await std::move(operation);
  if (reentrant_provider)
  {
    auto nested = reentrant_provider->poll();
    observation->reentrant_poll_rejected = !nested && nested.error().identity() == operation_in_progress;
  }
  {
    std::lock_guard lock(observation->mutex);
    observation->completion.emplace(std::move(completion));
  }
  ++observation->resumes;
}

template <class T>
bool finished(const std::shared_ptr<Observation<T>> &observation)
{
  return observation->resumes.load() != 0;
}

template <class T>
vix::Completion<T> take_completion(const std::shared_ptr<Observation<T>> &observation)
{
  std::lock_guard lock(observation->mutex);
  assert(observation->completion);
  return std::move(*observation->completion);
}

static void drive_until(Provider &first, Provider &second, const auto &predicate)
{
  for (unsigned attempt = 0; attempt < 5000 && !predicate(); ++attempt)
  {
    assert(first.poll());
    assert(second.poll());
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  assert(predicate());
}

int main()
{
  auto address = Address::parse("127.0.0.1");
  assert(address);
  auto any_endpoint = Endpoint::create(address.take(), 0);
  assert(any_endpoint);

  auto server_result = Provider::create_cooperative();
  auto client_result = Provider::create_cooperative();
  assert(server_result && client_result);
  auto server = server_result.take();
  auto client = client_result.take();
  auto idle = server.poll();
  assert(idle && idle.value() == 0); // no pending work is a valid no-op
  assert(server.poll());
  std::atomic<bool> foreign_thread_rejected{};
  std::thread foreign_driver([&]
  {
    auto result = server.poll();
    foreign_thread_rejected = !result && result.error().identity() == operation_in_progress;
  });
  foreign_driver.join();
  assert(foreign_thread_rejected);

  auto listener_result = server.listen(any_endpoint.value());
  assert(listener_result);
  auto listener = listener_result.take();
  auto local = listener.local_endpoint();
  assert(local);

  auto accepted_observation = std::make_shared<Observation<TcpConnection>>();
  auto connected_observation = std::make_shared<Observation<TcpConnection>>();
  [[maybe_unused]] auto accept_task = await_operation(listener.accept(), accepted_observation, &server);
  [[maybe_unused]] auto connect_task = await_operation(client.connect(local.value()), connected_observation);
  assert(!finished(accepted_observation) && !finished(connected_observation));
  drive_until(server, client, [&] { return finished(accepted_observation) && finished(connected_observation); });
  assert(accepted_observation->reentrant_poll_rejected);
  assert(accepted_observation->resumes == 1 && connected_observation->resumes == 1);
  auto accepted_completion = take_completion(accepted_observation);
  auto connected_completion = take_completion(connected_observation);
  assert(accepted_completion.status() == vix::CompletionStatus::Success);
  assert(connected_completion.status() == vix::CompletionStatus::Success);
  auto accepted = accepted_completion.take();
  auto connected = connected_completion.take();

  // Read and write are both suspended at once. Explicit progress dispatches
  // readiness, terminalizes each operation once, then resumes both coroutines.
  std::array<std::byte, 4> received{};
  const std::array<std::byte, 4> payload{std::byte{'p'}, std::byte{'o'}, std::byte{'l'}, std::byte{'l'}};
  auto read_observation = std::make_shared<Observation<std::size_t>>();
  auto write_observation = std::make_shared<Observation<void>>();
  [[maybe_unused]] auto read_task = await_operation(accepted.receive(received), read_observation);
  [[maybe_unused]] auto write_task = await_operation(connected.send(payload), write_observation);
  assert(!finished(read_observation) && !finished(write_observation));
  drive_until(server, client, [&] { return finished(read_observation) && finished(write_observation); });
  auto read_completion = take_completion(read_observation);
  auto write_completion = take_completion(write_observation);
  assert(read_completion.status() == vix::CompletionStatus::Success && read_completion.take() == payload.size());
  assert(write_completion.status() == vix::CompletionStatus::Success);
  assert(received == payload);
  assert(server.poll() && client.poll());
  assert(read_observation->resumes == 1 && write_observation->resumes == 1);

  // Cancellation with no ready data is observed on the next explicit turn and
  // does not close the TCP connection.
  vix::CancellationSource cancellation;
  std::array<std::byte, 1> byte{};
  auto cancelled_observation = std::make_shared<Observation<std::size_t>>();
  [[maybe_unused]] auto cancelled_task = await_operation(accepted.receive(byte, {cancellation.token(), {}}), cancelled_observation);
  assert(!finished(cancelled_observation));
  assert(!server.shutdown()); // pending operations keep the provider alive
  assert(cancellation.request_cancel());
  for (unsigned attempt = 0; attempt < 8 && !finished(cancelled_observation); ++attempt)
    assert(server.poll());
  assert(finished(cancelled_observation) && cancelled_observation->resumes == 1);
  assert(take_completion(cancelled_observation).status() == vix::CompletionStatus::Cancelled);

  // If readiness is processed first, a later cancellation cannot replace the
  // already-published successful terminal result.
  const std::array<std::byte, 1> marker{std::byte{'!'}};
  auto marker_write_observation = std::make_shared<Observation<void>>();
  [[maybe_unused]] auto marker_write_task = await_operation(connected.send(marker), marker_write_observation);
  for (unsigned attempt = 0; attempt < 8 && !finished(marker_write_observation); ++attempt)
    assert(client.poll());
  assert(finished(marker_write_observation));
  vix::CancellationSource late_cancellation;
  auto ready_read_observation = std::make_shared<Observation<std::size_t>>();
  [[maybe_unused]] auto ready_read_task = await_operation(accepted.receive(byte, {late_cancellation.token(), {}}), ready_read_observation);
  for (unsigned attempt = 0; attempt < 8 && !finished(ready_read_observation); ++attempt)
    assert(server.poll());
  assert(finished(ready_read_observation));
  assert(late_cancellation.request_cancel());
  auto ready_read_completion = take_completion(ready_read_observation);
  assert(ready_read_completion.status() == vix::CompletionStatus::Success);
  assert(ready_read_completion.take() == marker.size() && byte[0] == marker[0]);

  auto deadline_observation = std::make_shared<Observation<std::size_t>>();
  [[maybe_unused]] auto deadline_task = await_operation(
      accepted.receive(byte, {{}, vix::Deadline::at(vix::MonotonicClock::now())}), deadline_observation);
  for (unsigned attempt = 0; attempt < 8 && !finished(deadline_observation); ++attempt)
    assert(server.poll());
  assert(finished(deadline_observation) && deadline_observation->resumes == 1);
  auto deadline_completion = take_completion(deadline_observation);
  assert(deadline_completion.status() == vix::CompletionStatus::Failure);
  assert(deadline_completion.error().identity() == timed_out);

  // Hold a cooperative resume after terminalization, destroy its suspended
  // awaiter, then prove the queued ticket is safely discarded by a later poll.
  auto abandoned_observation = std::make_shared<Observation<void>>();
  const std::array<std::byte, 1> abandoned_payload{std::byte{'x'}};
  auto abandoned_task = await_operation(connected.send(abandoned_payload), abandoned_observation);
  auto client_core = detail::NetworkAccess::core_for_test(client);
  client_core->hold_cooperative_resumes_for_test(true);
  assert(client.poll());
  assert(abandoned_observation->resumes == 0);
  abandoned_task.abandon_pending();
  client_core->hold_cooperative_resumes_for_test(false);
  assert(client.poll());
  assert(abandoned_observation->resumes == 0);

  assert(accepted.close());
  assert(connected.close());
  assert(listener.close());
  assert(server.shutdown());
  assert(client.shutdown());
}
