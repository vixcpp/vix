#include <vix/network/Address.hpp>
#include <vix/network/Endpoint.hpp>
#include <vix/network/Errors.hpp>
#include <vix/network/Provider.hpp>
#include <vix/network/TcpConnection.hpp>
#include <vix/network/TcpListener.hpp>
#include <vix/Cancellation.hpp>
#include <vix/Deadline.hpp>
#include <array>
#include <cassert>
#include <coroutine>
#include <cstdio>
#include <exception>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <utility>

using namespace vix::network;

template <class T>
struct Observation
{
  std::optional<vix::Completion<T>> completion;
  unsigned resumes{};
  bool reentrant_poll_rejected{};
};

template <class T>
struct Detached
{
  struct promise_type
  {
    Detached get_return_object() noexcept { return {}; }
    std::suspend_never initial_suspend() const noexcept { return {}; }
    std::suspend_never final_suspend() const noexcept { return {}; }
    void return_void() const noexcept {}
    void unhandled_exception() const noexcept { std::terminate(); }
  };
};

template <class T>
Detached<T> observe(Operation<T> operation, std::shared_ptr<Observation<T>> result,
                    Provider *reentrant_provider = nullptr)
{
  result->completion.emplace(co_await std::move(operation));
  ++result->resumes;
  if (reentrant_provider)
  {
    auto nested = reentrant_provider->poll();
    result->reentrant_poll_rejected = !nested && nested.error().identity() == operation_in_progress;
  }
}

static bool drive(Provider &first, Provider &second, const auto &done)
{
  for (unsigned turn = 0; turn < 100000 && !done(); ++turn)
  {
    if (!first.poll() || !second.poll())
      return false;
  }
  return done();
}

static int denied_network()
{
  auto address = Address::parse("127.0.0.1");
  if (!address) return 1;
  auto endpoint = Endpoint::create(address.take(), 9);
  if (!endpoint) return 1;
  auto provider_result = Provider::create_cooperative();
  if (!provider_result) return 1;
  auto provider = provider_result.take();
  auto result = std::make_shared<Observation<TcpConnection>>();
  observe(provider.connect(endpoint.value()), result);
  for (unsigned turn = 0; turn < 100000 && !result->completion; ++turn)
    if (!provider.poll()) return 1;
  if (!result->completion || result->completion->status() != vix::CompletionStatus::Failure ||
      result->completion->error().identity() != permission_denied)
  {
    std::fputs("WASI host-denied TCP connect did not report permission_denied\n", stderr);
    return 1;
  }
  return provider.shutdown() ? 0 : 1;
}

static int loopback_contract()
{
  auto address = Address::parse("127.0.0.1");
  if (!address) return 1;
  auto endpoint = Endpoint::create(address.take(), 0);
  if (!endpoint) return 1;
  auto server_result = Provider::create_cooperative();
  auto client_result = Provider::create_cooperative();
  if (!server_result || !client_result) return 1;
  auto server = server_result.take();
  auto client = client_result.take();
  auto listener_result = server.listen(endpoint.value());
  if (!listener_result) return 1;
  auto listener = listener_result.take();
  auto second_listener_result = server.listen(endpoint.value());
  if (!second_listener_result) return 1;
  auto second_listener = second_listener_result.take();
  auto local = listener.local_endpoint();
  auto second_local = second_listener.local_endpoint();
  if (!local || !second_local) return 1;

  auto accepted_result = std::make_shared<Observation<TcpConnection>>();
  auto connected_result = std::make_shared<Observation<TcpConnection>>();
  auto second_accepted_result = std::make_shared<Observation<TcpConnection>>();
  auto second_connected_result = std::make_shared<Observation<TcpConnection>>();
  observe(listener.accept(), accepted_result, &server);
  observe(second_listener.accept(), second_accepted_result);
  observe(client.connect(local.value()), connected_result);
  observe(client.connect(second_local.value()), second_connected_result);
  if (!drive(server, client, [&]
             { return accepted_result->completion && connected_result->completion &&
                      second_accepted_result->completion && second_connected_result->completion; }))
    return 1;
  if (accepted_result->completion->status() != vix::CompletionStatus::Success ||
      connected_result->completion->status() != vix::CompletionStatus::Success ||
      accepted_result->resumes != 1 || !accepted_result->reentrant_poll_rejected ||
      connected_result->resumes != 1 ||
      second_accepted_result->completion->status() != vix::CompletionStatus::Success ||
      second_connected_result->completion->status() != vix::CompletionStatus::Success)
  {
    std::fputs("WASI loopback connect/accept failed; Wasmtime must grant TCP and inherit-network\n", stderr);
    return 1;
  }
  auto accepted = accepted_result->completion->take();
  auto connected = connected_result->completion->take();
  auto second_accepted = second_accepted_result->completion->take();
  auto second_connected = second_connected_result->completion->take();

  const std::array<std::byte, 6> payload{std::byte{'v'}, std::byte{'i'}, std::byte{'x'},
                                         std::byte{'-'}, std::byte{'p'}, std::byte{'2'}};
  std::array<std::byte, 6> received{};
  auto send_result = std::make_shared<Observation<void>>();
  auto receive_result = std::make_shared<Observation<std::size_t>>();
  observe(connected.send(payload), send_result);
  observe(accepted.receive(received), receive_result);
  if (!drive(server, client, [&] { return send_result->completion && receive_result->completion; }))
    return 1;
  if (send_result->completion->status() != vix::CompletionStatus::Success ||
      receive_result->completion->status() != vix::CompletionStatus::Success ||
      receive_result->completion->take() != payload.size() || received != payload)
    return 1;

  // Cancellation is a queued fact: it becomes terminal only on the owner's
  // next explicit progress turn, without closing the connection.
  vix::CancellationSource cancellation;
  auto cancelled_result = std::make_shared<Observation<std::size_t>>();
  std::array<std::byte, 1> pending_byte{};
  observe(accepted.receive(pending_byte, {cancellation.token(), {}}), cancelled_result);
  if (!cancellation.request_cancel() || !server.poll() || !cancelled_result->completion ||
      cancelled_result->completion->status() != vix::CompletionStatus::Cancelled)
    return 1;

  auto expired_result = std::make_shared<Observation<std::size_t>>();
  observe(accepted.receive(pending_byte, {{}, vix::Deadline::at(vix::MonotonicClock::now())}), expired_result);
  if (!server.poll() || !expired_result->completion ||
      expired_result->completion->status() != vix::CompletionStatus::Failure ||
      expired_result->completion->error().identity() != timed_out)
    return 1;

  if (!connected.shutdown_write()) return 1;
  std::array<std::byte, 1> eof_byte{};
  auto eof_result = std::make_shared<Observation<std::size_t>>();
  observe(accepted.receive(eof_byte), eof_result);
  if (!drive(server, client, [&] { return bool(eof_result->completion); }) ||
      eof_result->completion->status() != vix::CompletionStatus::Success || eof_result->completion->take() != 0)
    return 1;

  if (!accepted.close() || !connected.close() || !second_accepted.close() ||
      !second_connected.close() || !listener.close() || !second_listener.close()) return 1;
  return server.shutdown() && client.shutdown() ? 0 : 1;
}

int main(int argc, char **argv)
{
  if (argc != 2) return 2;
  const std::string_view mode(argv[1]);
  if (mode == "denied") return denied_network();
  if (mode == "loopback") return loopback_contract();
  return 2;
}
