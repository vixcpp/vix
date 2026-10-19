#include "Http1.hpp"
#include "PrivateExchange.hpp"
#include "CoroutineTask.hpp"
#include "Engine.hpp"

#include <vix/network/Address.hpp>
#include <vix/network/Endpoint.hpp>
#include <vix/network/Errors.hpp>
#include <vix/network/Provider.hpp>
#include <vix/network/TcpConnection.hpp>
#include <vix/network/TcpListener.hpp>
#include <vix/Deadline.hpp>
#include <vix/Duration.hpp>
#include <vix/MonotonicClock.hpp>

#include <array>
#include <algorithm>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <coroutine>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <thread>
#include <utility>

namespace
{
  using namespace vix::network;
  using namespace vix::http::detail;

  std::mutex progress_mutex;
  std::condition_variable progress_changed;

  template <class T>
  struct Observation
  {
    std::mutex mutex;
    std::condition_variable changed;
    std::optional<vix::Completion<T>> completion;
    bool done{};
    unsigned publications{};

    void publish(vix::Completion<T> value)
    {
      {
        std::lock_guard lock(mutex);
        assert(!done);
        completion.emplace(std::move(value));
        done = true;
        ++publications;
      }
      changed.notify_all();
      progress_changed.notify_all();
    }

    bool finished()
    {
      std::lock_guard lock(mutex);
      return done;
    }

    vix::Completion<T> take()
    {
      std::lock_guard lock(mutex);
      assert(done && completion);
      return std::move(*completion);
    }
  };

  struct TestTask
  {
    struct promise_type
    {
      std::shared_ptr<vix::network::test_detail::FinalSuspendState> final_suspend_state =
          std::make_shared<vix::network::test_detail::FinalSuspendState>();
      TestTask get_return_object()
      {
        return TestTask{std::coroutine_handle<promise_type>::from_promise(*this)};
      }
      std::suspend_never initial_suspend() const noexcept { return {}; }
      vix::network::test_detail::FinalSuspendAwaiter<promise_type>
      final_suspend() const noexcept { return {}; }
      void return_void() const noexcept {}
      void unhandled_exception() const noexcept { std::terminate(); }
    };

    std::coroutine_handle<promise_type> handle;
    explicit TestTask(std::coroutine_handle<promise_type> value) : handle(value) {}
    ~TestTask() { reset(); }
    void reset() { vix::network::test_detail::destroy_after_final_suspend(handle); }
    TestTask(TestTask &&other) noexcept : handle(std::exchange(other.handle, {})) {}
    TestTask(const TestTask &) = delete;
  };

  template <class T>
  TestTask observe(vix::network::Operation<T> operation,
                   const std::shared_ptr<Observation<T>> &observation)
  {
    observation->publish(co_await std::move(operation));
  }

  bool as_bytes(std::string_view text, std::vector<std::byte> &output)
  {
    output.resize(text.size());
    std::transform(text.begin(), text.end(), output.begin(),
                   [](char value) { return static_cast<std::byte>(value); });
    return true;
  }

  enum class ExchangeMode
  {
    normal,
    truncated,
    cancelled_before_send,
    expired_before_send,
    cancelled_during_receive,
    expired_during_receive,
    informational_coalesced,
    informational_limit_exceeded,
    informational_fragmented,
    informational_only,
    default_deadline_during_receive,
    switching,
    head_response,
    connect_success,
    post_response
  };

  TestTask serve_one_request(TcpConnection connection,
                             const std::shared_ptr<Observation<bool>> &observation,
                             bool truncate_response, bool hold_after_request,
                             const std::shared_ptr<std::optional<TcpConnection>> &retained,
                             ExchangeMode mode = ExchangeMode::normal,
                             std::string expected_method = "GET")
  {
    Http1Parser parser;
    std::array<std::byte, 1024> input{};
    bool request_complete = false;
    for (unsigned turn = 0; turn < 32 && !request_complete; ++turn)
    {
      auto step = parser.next(MessageKind::request, {}, {});
      if (step.status == ParseStatus::message_complete)
      {
        request_complete = true;
        break;
      }
      if (step.status == ParseStatus::protocol_error)
        break;
      auto received = co_await connection.receive(input);
      if (received.status() != vix::CompletionStatus::Success)
        break;
      const auto count = received.take();
      if (count == 0 || parser.append(std::span<const std::byte>{input.data(), count}) != count)
        break;
      step = parser.next(MessageKind::request, {}, {});
      if (step.status == ParseStatus::headers_complete)
        step = parser.next(MessageKind::request, {}, {});
      request_complete = step.status == ParseStatus::message_complete;
      if (step.status == ParseStatus::protocol_error)
        break;
    }

    bool sent_response = false;
    if (request_complete && hold_after_request)
    {
      retained->emplace(std::move(connection));
      observation->publish(vix::Completion<bool>{true});
      co_return;
    }
    const auto expected_target = expected_method == "CONNECT"
                                     ? std::string_view{"localhost:443"}
                                     : std::string_view{"/"};
    if (request_complete && parser.head().method == expected_method &&
        parser.head().target == expected_target)
    {
      if (mode == ExchangeMode::informational_fragmented)
        vix::network::detail::NetworkAccess::limit_send_chunk_for_test(connection, 3);
      std::vector<std::byte> response;
      std::string wire;
      if (mode == ExchangeMode::informational_coalesced)
      {
        for (std::size_t i = 0; i < max_informational_responses; ++i)
          wire += "HTTP/1.1 100 Continue\r\n\r\n";
        wire += "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n"
                "Connection: close\r\n\r\nOK";
      }
      else if (mode == ExchangeMode::informational_limit_exceeded)
      {
        const auto count = max_informational_responses +
                           1U;
        for (std::size_t i = 0; i < count; ++i)
          wire += "HTTP/1.1 100 Continue\r\n\r\n";
        wire += "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nOK";
      }
      else if (mode == ExchangeMode::informational_fragmented)
      {
        as_bytes("HTTP/1.1 103 Early Hints\r\n\r\n", response);
        auto first = co_await connection.send(response);
        if (first.status() == vix::CompletionStatus::Success)
        {
          response.clear();
          as_bytes("HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nOK", response);
          auto second = co_await connection.send(response);
          sent_response = second.status() == vix::CompletionStatus::Success;
        }
      }
      else if (mode == ExchangeMode::informational_only)
        wire = "HTTP/1.1 100 Continue\r\n\r\n";
      else if (mode == ExchangeMode::switching)
        wire = "HTTP/1.1 101 Switching Protocols\r\nConnection: Upgrade\r\n"
               "Upgrade: example\r\n\r\n";
      else if (mode == ExchangeMode::connect_success)
        wire = "HTTP/1.1 200 Connection Established\r\n\r\n";
      else
      {
        const std::array<HeaderField, 1> fields{{{"Connection", "close"}}};
        std::string head;
        if (serialize_response_head(200, "OK", fields,
                                    {OutgoingFraming::Kind::fixed_length, 2}, head))
          wire = head + (mode == ExchangeMode::head_response
                             ? ""
                             : (truncate_response ? "O" : "OK"));
      }
      if (!wire.empty())
      {
        response.clear();
        as_bytes(wire, response);
        auto result = co_await connection.send(response);
        sent_response = result.status() == vix::CompletionStatus::Success;
      }
    }
    (void)connection.close();
    observation->publish(vix::Completion<bool>{sent_response});
  }

  bool progress_until(Provider &provider, bool cooperative,
                      const auto &complete)
  {
    if (cooperative)
    {
      const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
      while (!complete() && std::chrono::steady_clock::now() < deadline)
      {
        auto result = provider.poll();
        assert(result);
        std::this_thread::yield();
      }
      return complete();
    }

    std::unique_lock lock(progress_mutex);
    return progress_changed.wait_for(lock, std::chrono::seconds(5), complete);
  }

  bool run_exchange(bool cooperative, ExchangeMode mode,
                    std::string request_method = "GET")
  {
    auto provider_result = cooperative ? Provider::create_cooperative() : Provider::create();
    assert(provider_result);
    auto provider = provider_result.take();

    auto address = Address::parse("127.0.0.1");
    assert(address);
    auto wildcard = Endpoint::create(address.take(), 0);
    assert(wildcard);
    auto listener_result = provider.listen(wildcard.take());
    assert(listener_result);
    auto listener = listener_result.take();
    auto endpoint = listener.local_endpoint();
    assert(endpoint);

    auto accepted = std::make_shared<Observation<TcpConnection>>();
    auto connected = std::make_shared<Observation<TcpConnection>>();
    [[maybe_unused]] auto accept_task = observe(listener.accept(), accepted);
    [[maybe_unused]] auto connect_task = observe(provider.connect(endpoint.value()), connected);
    assert(progress_until(provider, cooperative,
                          [&] { return accepted->finished() && connected->finished(); }));
    auto accepted_result = accepted->take();
    auto connected_result = connected->take();
    assert(accepted_result.status() == vix::CompletionStatus::Success);
    assert(connected_result.status() == vix::CompletionStatus::Success);

    vix::CancellationSource cancellation;
    OperationOptions options;
    if (mode == ExchangeMode::cancelled_before_send)
    {
      assert(cancellation.request_cancel());
      options.cancellation = cancellation.token();
    }
    else if (mode == ExchangeMode::cancelled_during_receive)
      options.cancellation = cancellation.token();
    else if (mode == ExchangeMode::expired_before_send)
      options.deadline = vix::Deadline::at(vix::MonotonicClock::now());
    else if (mode == ExchangeMode::expired_during_receive)
      options.deadline = vix::Deadline::after(vix::Duration::seconds(3));

    auto server_result = std::make_shared<Observation<bool>>();
    const bool truncate_response = mode == ExchangeMode::truncated;
    const bool hold_after_request =
        mode == ExchangeMode::cancelled_during_receive ||
        mode == ExchangeMode::expired_during_receive ||
        mode == ExchangeMode::default_deadline_during_receive;
    auto retained_server_connection = std::make_shared<std::optional<TcpConnection>>();
    [[maybe_unused]] auto server_task = serve_one_request(
        accepted_result.take(), server_result, truncate_response, hold_after_request,
        retained_server_connection, mode, request_method);

    std::string request;
    const std::array<HeaderField, 2> request_fields{{
        {"Host", "localhost"}, {"Connection", "close"}}};
    const auto request_target = request_method == "CONNECT"
                                    ? std::string_view{"localhost:443"}
                                    : std::string_view{"/"};
    assert(serialize_request_head(request_method, request_target, request_fields,
                                  {OutgoingFraming::Kind::no_body, 0}, request));
    std::vector<std::byte> request_bytes;
    as_bytes(request, request_bytes);

    auto client_connection = connected_result.take();
    std::optional<TcpConnection> client_state_for_test{
        vix::network::detail::NetworkAccess::alias_tcp_for_test(client_connection)};
    if (mode == ExchangeMode::normal)
      vix::network::detail::NetworkAccess::limit_send_chunk_for_test(
          client_connection, 3);
    auto exchange = start_private_exchange(
        std::move(client_connection), request_bytes, std::move(options),
        mode == ExchangeMode::default_deadline_during_receive
            ? vix::Duration::milliseconds(200)
            : default_exchange_lifetime,
        mode == ExchangeMode::informational_coalesced
            ? max_exchange_receive_chunk
            : default_exchange_receive_chunk);
    assert(exchange);
    auto client = std::make_shared<Observation<ExchangeResponse>>();
    auto client_task = observe(exchange.take(), client);
    if (hold_after_request)
    {
      assert(progress_until(provider, cooperative,
                            [&] { return server_result->finished(); }));
      assert(server_result->take().take());
      if (client->finished())
      {
        const auto early = client->take();
        std::fprintf(stderr, "HTTP parent completed before cancellation: status=%u",
                     static_cast<unsigned>(early.status()));
        if (early.status() == vix::CompletionStatus::Failure)
          std::fprintf(stderr, " error=%.*s:%u message=%.*s",
                       static_cast<int>(early.error().identity().domain().size()),
                       early.error().identity().domain().data(),
                       early.error().identity().code(),
                       static_cast<int>(early.error().message().size()),
                       early.error().message().data());
        std::fputc('\n', stderr);
        assert(false);
      }
      assert(!client->finished());
      if (mode == ExchangeMode::cancelled_during_receive)
        assert(cancellation.request_cancel());
    }
    assert(progress_until(provider, cooperative,
                          [&] { return client->finished() && server_result->finished(); }));

    if (!hold_after_request)
    {
      auto server_completion = server_result->take();
      assert(server_completion.status() == vix::CompletionStatus::Success);
      if (mode == ExchangeMode::normal || mode == ExchangeMode::truncated ||
          mode == ExchangeMode::informational_coalesced ||
          mode == ExchangeMode::informational_limit_exceeded ||
          mode == ExchangeMode::informational_fragmented ||
          mode == ExchangeMode::informational_only ||
          mode == ExchangeMode::switching || mode == ExchangeMode::head_response ||
          mode == ExchangeMode::connect_success || mode == ExchangeMode::post_response)
      {
        const auto server_sent = server_completion.take();
        if (!server_sent)
          std::fprintf(stderr, "HTTP test server failed to send response, mode=%u\n",
                       static_cast<unsigned>(mode));
        assert(server_sent);
      }
      else
        assert(!server_completion.take());
    }
    auto client_completion = client->take();
    assert(client->publications == 1 && server_result->publications == 1);
    if (mode == ExchangeMode::cancelled_before_send)
      assert(client_completion.status() == vix::CompletionStatus::Cancelled);
    else if (mode == ExchangeMode::cancelled_during_receive)
      assert(client_completion.status() == vix::CompletionStatus::Cancelled);
    else if (mode == ExchangeMode::expired_before_send)
      assert(client_completion.status() == vix::CompletionStatus::Failure &&
             client_completion.error().identity() == vix::network::timed_out);
    else if (mode == ExchangeMode::expired_during_receive)
      assert(client_completion.status() == vix::CompletionStatus::Failure &&
             client_completion.error().identity() == vix::network::timed_out);
    else if (mode == ExchangeMode::default_deadline_during_receive)
      assert(client_completion.status() == vix::CompletionStatus::Failure &&
             client_completion.error().identity() == vix::network::timed_out);
    else if (truncate_response)
    {
      assert(client_completion.status() == vix::CompletionStatus::Failure);
      assert(client_completion.error().identity() == malformed_response);
    }
    else if (mode == ExchangeMode::switching ||
             mode == ExchangeMode::connect_success ||
             mode == ExchangeMode::informational_only)
    {
      assert(client_completion.status() == vix::CompletionStatus::Failure);
      assert(client_completion.error().identity() == malformed_response);
    }
    else if (mode == ExchangeMode::informational_limit_exceeded)
    {
      assert(client_completion.status() == vix::CompletionStatus::Failure);
      assert(client_completion.error().identity() ==
             informational_response_limit_exceeded);
    }
    else
    {
      if (client_completion.status() != vix::CompletionStatus::Success)
      {
        std::fprintf(stderr, "HTTP exchange failure: status=%u", static_cast<unsigned>(client_completion.status()));
        if (client_completion.status() == vix::CompletionStatus::Failure)
          std::fprintf(stderr, " error=%.*s:%u message=%.*s",
                       static_cast<int>(client_completion.error().identity().domain().size()),
                       client_completion.error().identity().domain().data(),
                       client_completion.error().identity().code(),
                       static_cast<int>(client_completion.error().message().size()),
                       client_completion.error().message().data());
        std::fputc('\n', stderr);
      }
      assert(client_completion.status() == vix::CompletionStatus::Success);
      auto response = client_completion.take();
      assert(response.status_code == 200);
      const bool no_body = mode == ExchangeMode::head_response;
      assert(response.body.size() == (no_body ? 0U : 2U));
      if (!no_body)
        assert(response.body[0] == std::byte{'O'} && response.body[1] == std::byte{'K'});
      const auto counters =
          vix::network::detail::NetworkAccess::transport_counters_for_test(
              *client_state_for_test);
      if (mode == ExchangeMode::normal)
      {
        assert(counters.native_send_attempts > 1);
        assert(counters.native_recv_attempts > 1);
      }
    }

    server_task.reset();
    if (*retained_server_connection)
    {
      (void)(*retained_server_connection)->close();
      retained_server_connection->reset();
    }
    client_state_for_test.reset();
    client_task.reset();
    connect_task.reset();
    accept_task.reset();

    // Background lease close is posted to the Network reactor. Wait for its
    // resource accounting to retire before checking provider shutdown.
    auto core = vix::network::detail::NetworkAccess::core_for_test(provider);
    for (unsigned turn = 0; turn < 5000 && core->active_resources_for_test() != 1; ++turn)
      std::this_thread::yield();
    (void)listener.close();
    for (unsigned turn = 0; turn < 5000 && core->active_resources_for_test() != 0; ++turn)
    {
      if (cooperative)
        assert(provider.poll());
      else
        std::this_thread::yield();
    }
    assert(core->active_operations_for_test() == 0);
    assert(core->active_resources_for_test() == 0);
    assert(provider.shutdown());
    return true;
  }

  void cooperative_wrong_thread_is_rejected()
  {
    auto provider_result = Provider::create_cooperative();
    assert(provider_result);
    auto provider = provider_result.take();
    auto address = Address::parse("127.0.0.1");
    assert(address);
    auto any_endpoint = Endpoint::create(address.take(), 0);
    assert(any_endpoint);
    auto listener_result = provider.listen(any_endpoint.take());
    assert(listener_result);
    auto listener = listener_result.take();
    auto endpoint = listener.local_endpoint();
    assert(endpoint);

    auto accepted = std::make_shared<Observation<TcpConnection>>();
    auto connected = std::make_shared<Observation<TcpConnection>>();
    [[maybe_unused]] auto accept_task = observe(listener.accept(), accepted);
    [[maybe_unused]] auto connect_task = observe(provider.connect(endpoint.value()), connected);
    assert(progress_until(provider, true,
                          [&] { return accepted->finished() && connected->finished(); }));
    auto accepted_completion = accepted->take();
    auto connected_completion = connected->take();
    assert(accepted_completion.status() == vix::CompletionStatus::Success);
    assert(connected_completion.status() == vix::CompletionStatus::Success);

    std::string request;
    const std::array<HeaderField, 1> fields{{{"Host", "localhost"}}};
    assert(serialize_request_head("GET", "/", fields,
                                  {OutgoingFraming::Kind::no_body, 0}, request));
    std::vector<std::byte> request_bytes;
    as_bytes(request, request_bytes);
    auto exchange = start_private_exchange(connected_completion.take(), request_bytes);
    assert(exchange);
    auto observation = std::make_shared<Observation<ExchangeResponse>>();
    std::thread foreign_starter(
        [operation = exchange.take(), observation]() mutable
        {
          auto task = observe(std::move(operation), observation);
          task.reset();
        });
    foreign_starter.join();
    auto completion = observation->take();
    assert(completion.status() == vix::CompletionStatus::Failure);
    assert(completion.error().identity() == operation_in_progress);
    assert(provider.poll()); // retires the rejected claim on its owner thread

    assert(accepted_completion.take().close());
    assert(listener.close());
    auto core = vix::network::detail::NetworkAccess::core_for_test(provider);
    assert(core->active_operations_for_test() == 0);
    assert(core->active_resources_for_test() == 0);
    assert(provider.shutdown());
  }
} // namespace

int main()
{
  InformationalResponseCounter counter;
  for (std::size_t i = 0; i + 1 < max_informational_responses; ++i)
    assert(counter.consume());
  assert(counter.count == max_informational_responses - 1);
  assert(counter.consume()); // The configured limit itself is accepted.
  assert(counter.count == max_informational_responses);
  assert(!counter.consume()); // Only the next response is rejected.

  assert(run_exchange(false, ExchangeMode::normal));
  assert(run_exchange(true, ExchangeMode::normal));
  assert(run_exchange(false, ExchangeMode::truncated));
  assert(run_exchange(true, ExchangeMode::truncated));
  assert(run_exchange(false, ExchangeMode::cancelled_before_send));
  assert(run_exchange(true, ExchangeMode::cancelled_before_send));
  assert(run_exchange(false, ExchangeMode::expired_before_send));
  assert(run_exchange(true, ExchangeMode::expired_before_send));
  assert(run_exchange(false, ExchangeMode::cancelled_during_receive));
  assert(run_exchange(true, ExchangeMode::cancelled_during_receive));
  assert(run_exchange(false, ExchangeMode::expired_during_receive));
  assert(run_exchange(true, ExchangeMode::expired_during_receive));
  assert(run_exchange(true, ExchangeMode::default_deadline_during_receive));
  assert(run_exchange(false, ExchangeMode::informational_coalesced));
  assert(run_exchange(true, ExchangeMode::informational_limit_exceeded));
  assert(run_exchange(false, ExchangeMode::informational_fragmented));
  assert(run_exchange(true, ExchangeMode::informational_fragmented));
  assert(run_exchange(true, ExchangeMode::informational_only));
  assert(run_exchange(false, ExchangeMode::switching));
  assert(run_exchange(true, ExchangeMode::switching));
  assert(run_exchange(false, ExchangeMode::head_response, "HEAD"));
  assert(run_exchange(true, ExchangeMode::head_response, "HEAD"));
  assert(run_exchange(false, ExchangeMode::connect_success, "CONNECT"));
  assert(run_exchange(true, ExchangeMode::connect_success, "CONNECT"));
  assert(run_exchange(false, ExchangeMode::post_response, "POST"));
  assert(run_exchange(true, ExchangeMode::post_response, "POST"));
  cooperative_wrong_thread_is_rejected();
}
