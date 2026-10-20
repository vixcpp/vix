
/**
 * @file cooperative.cpp
 * @brief Runs a TCP echo exchange with a thread-free Vix Network provider.
 *
 * This example is self-contained: both TCP endpoints run in one process.
 * run() advances the cooperative provider without a background reactor thread.
 */

#include <vix/network/Provider.hpp>

#include <array>
#include <cstddef>
#include <iostream>
#include <span>
#include <string>

int main()
{
  using namespace vix::network;

  auto created = Provider::create_cooperative();
  if (!created)
  {
    std::cerr << "Cannot create cooperative provider: " << created.error().message() << '\n';
    return 1;
  }
  auto provider = created.take();

  // Loopback + port 0 asks the OS to choose an available local port.
  auto address = Address::parse("127.0.0.1");
  if (!address)
    return 1;
  auto endpoint = Endpoint::create(address.take(), 0);
  if (!endpoint)
    return 1;

  auto listening = provider.listen(endpoint.value());
  if (!listening)
  {
    std::cerr << "Listen failed: " << listening.error().message() << '\n';
    return 1;
  }
  auto listener = listening.take();

  auto bound = listener.local_endpoint();
  if (!bound)
    return 1;
  std::cout << "Listening on port " << bound.value().port() << '\n';

  // run() drives the cooperative reactor until this connection completes.
  auto connected = run(provider, provider.connect(bound.value()));
  if (!connected)
  {
    std::cerr << "Runner failed: " << connected.error().message() << '\n';
    return 1;
  }
  auto connection_result = connected.take();
  if (connection_result.status() != vix::CompletionStatus::Success)
  {
    std::cerr << "Connect did not succeed.\n";
    return 1;
  }
  auto client = connection_result.take();

  auto accepted = run(provider, listener.accept());
  if (!accepted)
  {
    std::cerr << "Accept runner failed: " << accepted.error().message() << '\n';
    return 1;
  }
  auto accept_result = accepted.take();
  if (accept_result.status() != vix::CompletionStatus::Success)
  {
    std::cerr << "Accept did not succeed.\n";
    return 1;
  }
  auto server = accept_result.take();

  const std::string message = "Hello from cooperative Vix Network!\n";
  auto sent = run(provider, client.send(std::as_bytes(std::span{message.data(), message.size()})));
  if (!sent || sent.value().status() != vix::CompletionStatus::Success)
  {
    std::cerr << "Client send failed.\n";
    return 1;
  }

  // Tell the server no more input will arrive, while keeping the read side open.
  if (!client.shutdown_write())
    return 1;

  // Read until EOF, echoing each chunk. TCP does not preserve message boundaries.
  std::array<std::byte, 128> buffer{};
  while (true)
  {
    auto received = run(provider, server.receive(buffer));
    if (!received)
      return 1;
    auto outcome = received.take();
    if (outcome.status() != vix::CompletionStatus::Success)
      return 1;
    const auto count = outcome.take();
    if (count == 0)
      break;

    auto echoed = run(provider, server.send(std::span<const std::byte>{buffer.data(), count}));
    if (!echoed || echoed.value().status() != vix::CompletionStatus::Success)
      return 1;
  }
  if (!server.shutdown_write())
    return 1;

  std::cout << "Echo: ";
  while (true)
  {
    auto received = run(provider, client.receive(buffer));
    if (!received)
      return 1;
    auto outcome = received.take();
    if (outcome.status() != vix::CompletionStatus::Success)
      return 1;
    const auto count = outcome.take();
    if (count == 0)
      break;

    std::cout.write(reinterpret_cast<const char *>(buffer.data()),
                    static_cast<std::streamsize>(count));
  }

  if (!server.close() || !client.close() || !listener.close())
  {
    std::cerr << "Could not close all TCP resources.\n";
    return 1;
  }
  if (!provider.shutdown())
  {
    std::cerr << "Provider shutdown failed.\n";
    return 1;
  }
  return 0;
}
