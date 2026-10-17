#include "CoroutineTask.hpp"
#include <vix/network/Address.hpp>
#include <vix/network/Errors.hpp>
#include <vix/network/Provider.hpp>
#include <vix/network/TcpConnection.hpp>
#include <vix/network/TcpListener.hpp>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdio>
#include <exception>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <utility>
#include <string_view>
#include <vector>

namespace
{
  template <class T>
  struct Observation
  {
    std::mutex mutex;
    std::optional<vix::Completion<T>> completion;
    std::atomic<bool> finished{};
    std::atomic<unsigned> completions{};
  };

  template <class T>
  struct Task
  {
    struct promise_type
    {
      std::mutex final_mutex;
      std::condition_variable final_cv;
      bool final_suspended{};
      Task get_return_object() noexcept { return Task{std::coroutine_handle<promise_type>::from_promise(*this)}; }
      std::suspend_never initial_suspend() const noexcept { return {}; }
      vix::network::test_detail::FinalSuspendAwaiter<promise_type> final_suspend() noexcept { return {}; }
      void return_void() const noexcept {}
      void unhandled_exception() const noexcept { std::terminate(); }
    };
    std::coroutine_handle<promise_type> handle{};
    explicit Task(std::coroutine_handle<promise_type> value) noexcept : handle(value) {}
    Task(Task &&other) noexcept : handle(std::exchange(other.handle, {})) {}
    Task(const Task &) = delete;
    ~Task() { vix::network::test_detail::destroy_after_final_suspend(handle); }
  };

  template <class T>
  Task<T> observe(vix::network::Operation<T> operation, std::shared_ptr<Observation<T>> result)
  {
    auto completion = co_await std::move(operation);
    {
      std::lock_guard lock(result->mutex);
      result->completion.emplace(std::move(completion));
    }
    ++result->completions;
    result->finished = true;
  }

  bool wait(const std::atomic<bool> &finished)
  {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!finished.load(std::memory_order_acquire) && std::chrono::steady_clock::now() < deadline)
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    return finished.load(std::memory_order_acquire);
  }

}

int main(int argc, char **argv)
{
  using namespace vix::network;
  if (argc == 2 && std::string_view(argv[1]) == "--ipv6-only")
  {
    auto made = Provider::create();
    assert(made);
    auto provider = made.take();
    auto address = Address::parse("::1");
    assert(address);
    auto listener = provider.listen(Endpoint::create(address.take(), 0).take());
    if (!listener && (listener.error().identity() == unsupported || listener.error().identity() == address_unavailable))
    {
      std::fprintf(stderr, "IPv6 loopback is unavailable on this host.\n");
      assert(provider.shutdown());
      return 77;
    }
    assert(listener);
    auto owned_listener = listener.take();
    assert(owned_listener.local_endpoint());
    assert(owned_listener.close());
    assert(provider.shutdown());
    return 0;
  }
  assert(argc == 1);
  auto v4 = Address::parse("127.0.0.1");
  auto v6 = Address::parse("::1");
  assert(v4 && v4.value().display() == "127.0.0.1");
  assert(v6 && v6.value().display() == "::1");

  auto made = Provider::create();
  assert(made);
  auto provider = made.take();
  (void)v6;

  auto listen_result = provider.listen(Endpoint::create(v4.take(), 0).take());
  assert(listen_result);
  auto listener = listen_result.take();
  auto endpoint = listener.local_endpoint();
  assert(endpoint);

  auto accepted = std::make_shared<Observation<TcpConnection>>();
  auto accept_task = observe(listener.accept(), accepted);
  auto connected = std::make_shared<Observation<TcpConnection>>();
  auto connect_task = observe(provider.connect(endpoint.value()), connected);
  assert(wait(accepted->finished) && wait(connected->finished));
  std::optional<TcpConnection> server;
  std::optional<TcpConnection> client;
  {
    std::lock_guard lock(accepted->mutex);
    assert(accepted->completion->status() == vix::CompletionStatus::Success);
    server.emplace(accepted->completion->take());
  }
  {
    std::lock_guard lock(connected->mutex);
    assert(connected->completion->status() == vix::CompletionStatus::Success);
    client.emplace(connected->completion->take());
  }
  auto busy_shutdown = provider.shutdown();
  assert(!busy_shutdown && busy_shutdown.error().identity() == provider_in_use);

  // Keep several readiness registrations live concurrently. This exercises
  // WSAPoll's dynamically sized registration set rather than a small fixed
  // descriptor array.
  constexpr std::size_t simultaneous_connections = 4;
  std::vector<TcpListener> simultaneous_listeners;
  std::vector<Endpoint> simultaneous_endpoints;
  std::vector<std::shared_ptr<Observation<TcpConnection>>> simultaneous_accepts;
  std::vector<std::shared_ptr<Observation<TcpConnection>>> simultaneous_connects;
  std::vector<Task<TcpConnection>> simultaneous_accept_tasks;
  std::vector<Task<TcpConnection>> simultaneous_connect_tasks;
  std::vector<std::thread> simultaneous_launchers;
  std::mutex task_storage_mutex;
  simultaneous_listeners.reserve(simultaneous_connections);
  simultaneous_endpoints.reserve(simultaneous_connections);
  simultaneous_accepts.reserve(simultaneous_connections);
  simultaneous_connects.reserve(simultaneous_connections);
  simultaneous_accept_tasks.reserve(simultaneous_connections);
  simultaneous_connect_tasks.reserve(simultaneous_connections);
  simultaneous_launchers.reserve(simultaneous_connections);
  for (std::size_t i = 0; i < simultaneous_connections; ++i)
  {
    auto created = provider.listen(Endpoint::create(Address::parse("127.0.0.1").take(), 0).take());
    assert(created);
    simultaneous_listeners.push_back(created.take());
    simultaneous_endpoints.push_back(simultaneous_listeners.back().local_endpoint().take());
    simultaneous_accepts.push_back(std::make_shared<Observation<TcpConnection>>());
    simultaneous_connects.push_back(std::make_shared<Observation<TcpConnection>>());
  }
  for (std::size_t i = 0; i < simultaneous_connections; ++i)
  {
    simultaneous_launchers.emplace_back([&, i]
    {
      auto accept = observe(simultaneous_listeners[i].accept(), simultaneous_accepts[i]);
      auto connect = observe(provider.connect(simultaneous_endpoints[i]), simultaneous_connects[i]);
      std::lock_guard lock(task_storage_mutex);
      simultaneous_accept_tasks.push_back(std::move(accept));
      simultaneous_connect_tasks.push_back(std::move(connect));
    });
  }
  for (auto &launcher : simultaneous_launchers)
    launcher.join();
  for (std::size_t i = 0; i < simultaneous_connections; ++i)
  {
    assert(wait(simultaneous_accepts[i]->finished) && wait(simultaneous_connects[i]->finished));
    assert(simultaneous_accepts[i]->completions == 1 && simultaneous_connects[i]->completions == 1);
    std::optional<TcpConnection> accepted_connection;
    std::optional<TcpConnection> connected_connection;
    {
      std::lock_guard lock(simultaneous_accepts[i]->mutex);
      assert(simultaneous_accepts[i]->completion->status() == vix::CompletionStatus::Success);
      accepted_connection.emplace(simultaneous_accepts[i]->completion->take());
    }
    {
      std::lock_guard lock(simultaneous_connects[i]->mutex);
      assert(simultaneous_connects[i]->completion->status() == vix::CompletionStatus::Success);
      connected_connection.emplace(simultaneous_connects[i]->completion->take());
    }
    assert(connected_connection->close());
    assert(accepted_connection->close());
    assert(simultaneous_listeners[i].close());
  }

  constexpr std::array payload{std::byte{'V'}, std::byte{'I'}, std::byte{'X'}, std::byte{'!'}};
  auto sent = std::make_shared<Observation<void>>();
  auto send_task = observe(client->send(payload), sent);
  assert(wait(sent->finished));
  {
    std::lock_guard lock(sent->mutex);
    assert(sent->completion->status() == vix::CompletionStatus::Success);
  }
  std::array<std::byte, 2> first{};
  auto first_read = std::make_shared<Observation<std::size_t>>();
  auto first_read_task = observe(server->receive(first), first_read);
  assert(wait(first_read->finished));
  {
    std::lock_guard lock(first_read->mutex);
    assert(first_read->completion->status() == vix::CompletionStatus::Success);
    assert(first_read->completion->take() == first.size());
    if (first[0] != payload[0] || first[1] != payload[1])
      std::fprintf(stderr, "first TCP read mismatch: got %u %u, expected %u %u\n",
                   std::to_integer<unsigned>(first[0]), std::to_integer<unsigned>(first[1]),
                   std::to_integer<unsigned>(payload[0]), std::to_integer<unsigned>(payload[1]));
    assert(first[0] == payload[0] && first[1] == payload[1]);
  }
  std::array<std::byte, 2> second{};
  auto second_read = std::make_shared<Observation<std::size_t>>();
  auto second_read_task = observe(server->receive(second), second_read);
  assert(wait(second_read->finished));
  {
    std::lock_guard lock(second_read->mutex);
    assert(second_read->completion->status() == vix::CompletionStatus::Success);
    assert(second_read->completion->take() == second.size());
    if (second[0] != payload[2] || second[1] != payload[3])
      std::fprintf(stderr, "second TCP read mismatch: got %u %u, expected %u %u\n",
                   std::to_integer<unsigned>(second[0]), std::to_integer<unsigned>(second[1]),
                   std::to_integer<unsigned>(payload[2]), std::to_integer<unsigned>(payload[3]));
    assert(second[0] == payload[2] && second[1] == payload[3]);
  }
  assert(client->shutdown_write());
  std::array<std::byte, 1> eof_buffer{};
  auto eof = std::make_shared<Observation<std::size_t>>();
  auto eof_task = observe(server->receive(eof_buffer), eof);
  assert(wait(eof->finished));
  {
    std::lock_guard lock(eof->mutex);
    assert(eof->completion->status() == vix::CompletionStatus::Success);
    assert(eof->completion->take() == 0);
  }

  auto deadline_listener_result = provider.listen(Endpoint::create(Address::parse("127.0.0.1").take(), 0).take());
  assert(deadline_listener_result);
  auto deadline_listener = deadline_listener_result.take();
  vix::CancellationSource cancellation;
  OperationOptions cancellation_options;
  cancellation_options.cancellation = cancellation.token();
  auto cancelled = std::make_shared<Observation<TcpConnection>>();
  auto cancelled_task = observe(deadline_listener.accept(cancellation_options), cancelled);
  const auto pending_shutdown = provider.shutdown();
  assert(!pending_shutdown && pending_shutdown.error().identity() == provider_in_use);
  assert(cancellation.request_cancel());
  assert(wait(cancelled->finished));
  assert(cancelled->completions == 1);
  {
    std::lock_guard lock(cancelled->mutex);
    assert(cancelled->completion->status() == vix::CompletionStatus::Cancelled);
  }
  OperationOptions options;
  options.deadline = vix::Deadline::after(vix::Duration::milliseconds(50));
  auto timeout = std::make_shared<Observation<TcpConnection>>();
  auto timeout_task = observe(deadline_listener.accept(options), timeout);
  assert(wait(timeout->finished));
  assert(timeout->completions == 1);
  {
    std::lock_guard lock(timeout->mutex);
    assert(timeout->completion->status() == vix::CompletionStatus::Failure);
    assert(timeout->completion->error().identity() == timed_out);
  }

  // Race cancellation against a real incoming connect. Either terminal winner
  // is valid, but it must publish exactly one completion and retire cleanly.
  auto race_listener_result = provider.listen(Endpoint::create(Address::parse("127.0.0.1").take(), 0).take());
  assert(race_listener_result);
  auto race_listener = race_listener_result.take();
  auto race_endpoint = race_listener.local_endpoint();
  assert(race_endpoint);
  vix::CancellationSource race_cancellation;
  OperationOptions race_options;
  race_options.cancellation = race_cancellation.token();
  auto race_accept = std::make_shared<Observation<TcpConnection>>();
  auto race_accept_task = observe(race_listener.accept(race_options), race_accept);
  auto race_connect = std::make_shared<Observation<TcpConnection>>();
  auto race_connect_task = observe(provider.connect(race_endpoint.value()), race_connect);
  std::thread cancel_racer([&] { (void)race_cancellation.request_cancel(); });
  cancel_racer.join();
  assert(wait(race_accept->finished) && wait(race_connect->finished));
  assert(race_accept->completions == 1 && race_connect->completions == 1);
  {
    std::lock_guard lock(race_connect->mutex);
    assert(race_connect->completion->status() == vix::CompletionStatus::Success);
    auto connection = race_connect->completion->take();
    assert(connection.close());
  }
  {
    std::lock_guard lock(race_accept->mutex);
    const auto status = race_accept->completion->status();
    assert(status == vix::CompletionStatus::Cancelled || status == vix::CompletionStatus::Success);
    if (status == vix::CompletionStatus::Success)
    {
      auto connection = race_accept->completion->take();
      assert(connection.close());
    }
  }
  assert(race_listener.close());

  // A very short deadline races a native readiness event. A timeout or a
  // successful accept is valid; duplicated completion is never valid.
  auto deadline_race_listener_result = provider.listen(Endpoint::create(Address::parse("127.0.0.1").take(), 0).take());
  assert(deadline_race_listener_result);
  auto deadline_race_listener = deadline_race_listener_result.take();
  auto deadline_race_endpoint = deadline_race_listener.local_endpoint();
  assert(deadline_race_endpoint);
  OperationOptions deadline_race_options;
  deadline_race_options.deadline = vix::Deadline::after(vix::Duration::milliseconds(1));
  auto deadline_race_accept = std::make_shared<Observation<TcpConnection>>();
  auto deadline_race_accept_task = observe(deadline_race_listener.accept(deadline_race_options), deadline_race_accept);
  auto deadline_race_connect = std::make_shared<Observation<TcpConnection>>();
  auto deadline_race_connect_task = observe(provider.connect(deadline_race_endpoint.value()), deadline_race_connect);
  assert(wait(deadline_race_accept->finished) && wait(deadline_race_connect->finished));
  assert(deadline_race_accept->completions == 1 && deadline_race_connect->completions == 1);
  {
    std::lock_guard lock(deadline_race_connect->mutex);
    assert(deadline_race_connect->completion->status() == vix::CompletionStatus::Success);
    auto connection = deadline_race_connect->completion->take();
    assert(connection.close());
  }
  {
    std::lock_guard lock(deadline_race_accept->mutex);
    const auto status = deadline_race_accept->completion->status();
    assert(status == vix::CompletionStatus::Success || status == vix::CompletionStatus::Failure);
    if (status == vix::CompletionStatus::Success)
    {
      auto connection = deadline_race_accept->completion->take();
      assert(connection.close());
    }
    else
      assert(deadline_race_accept->completion->error().identity() == timed_out);
  }
  assert(deadline_race_listener.close());

  assert(client->close());
  assert(server->close());
  assert(listener.close());

  auto refused = std::make_shared<Observation<TcpConnection>>();
  auto refused_task = observe(provider.connect(endpoint.value()), refused);
  assert(wait(refused->finished));
  {
    std::lock_guard lock(refused->mutex);
    assert(refused->completion->status() == vix::CompletionStatus::Failure);
    assert(refused->completion->error().identity() == connection_refused);
  }

  assert(deadline_listener.close());
  assert(provider.shutdown());

  // Repeated rapid retire/create cycles encourage native SOCKET value reuse;
  // stale readiness must not complete a later operation for a retired socket.
  for (int i = 0; i < 16; ++i)
  {
    auto repeated = Provider::create();
    assert(repeated);
    auto instance = repeated.take();
    assert(instance.shutdown());
  }
  std::atomic<unsigned> lifecycle_failures{};
  std::vector<std::thread> provider_lifecycle_workers;
  provider_lifecycle_workers.reserve(8);
  for (unsigned worker = 0; worker < 8; ++worker)
    provider_lifecycle_workers.emplace_back([&]
    {
      for (unsigned iteration = 0; iteration < 3; ++iteration)
      {
        auto repeated = Provider::create();
        if (!repeated)
        {
          ++lifecycle_failures;
          continue;
        }
        auto instance = repeated.take();
        if (!instance.shutdown())
          ++lifecycle_failures;
      }
    });
  for (auto &worker : provider_lifecycle_workers)
    worker.join();
  assert(lifecycle_failures == 0);
  return 0;
}
