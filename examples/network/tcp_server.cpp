
/**
 * @file tcp_server.cpp
 * @brief Runs a single-client TCP echo server using Vix Network.
 *
 * Listens on 127.0.0.1:8080. Run examples/network/tcp_client.cpp
 * from another terminal to send data and receive the echoed bytes.
 * The server exits after serving one client.
 */

#include <vix/network/Provider.hpp>

#include <array>
#include <cstddef>
#include <iostream>
#include <span>

int main()
{
  using namespace vix::network;

  auto created = Provider::create();
  if (!created)
  {
    std::cerr << "Provider creation failed: " << created.error().message() << '\n';
    return 1;
  }
  auto provider = created.take();

  auto address = Address::parse("127.0.0.1");
  if (!address)
  {
    std::cerr << "Invalid address: " << address.error().message() << '\n';
    return 1;
  }
  auto endpoint = Endpoint::create(address.take(), 8080);
  if (!endpoint)
  {
    std::cerr << "Invalid endpoint: " << endpoint.error().message() << '\n';
    return 1;
  }

  auto listening = provider.listen(endpoint.value());
  if (!listening)
  {
    std::cerr << "Listen failed: " << listening.error().message() << '\n';
    return 1;
  }
  auto listener = listening.take();
  std::cout << "Listening on 127.0.0.1:8080 (one client)...\n"
            << std::flush;

  auto accepted = run(provider, listener.accept());
  if (!accepted)
  {
    std::cerr << "Accept runner failed: " << accepted.error().message() << '\n';
    return 1;
  }
  auto accepted_outcome = accepted.take();
  if (accepted_outcome.status() != vix::CompletionStatus::Success)
  {
    if (accepted_outcome.status() == vix::CompletionStatus::Failure)
      std::cerr << "Accept failed: " << accepted_outcome.error().message() << '\n';
    else
      std::cerr << "Accept was cancelled.\n";
    return 1;
  }
  auto client = accepted_outcome.take();
  std::cout << "Client connected. Echoing bytes...\n";

  std::array<std::byte, 4096> buffer{};
  while (true)
  {
    auto received = run(provider, client.receive(buffer));
    if (!received)
    {
      std::cerr << "Receive runner failed: " << received.error().message() << '\n';
      return 1;
    }
    auto receive_outcome = received.take();
    if (receive_outcome.status() != vix::CompletionStatus::Success)
    {
      std::cerr << "Receive did not complete successfully.\n";
      return 1;
    }
    const auto count = receive_outcome.take();
    if (count == 0)
      break; // Client half-closed its write direction.

    // Echo exactly the received chunk. TCP may split data across reads.
    auto sent = run(provider, client.send(std::span<const std::byte>{buffer.data(), count}));
    if (!sent)
    {
      std::cerr << "Send runner failed: " << sent.error().message() << '\n';
      return 1;
    }
    if (sent.value().status() != vix::CompletionStatus::Success)
    {
      std::cerr << "Send did not complete successfully.\n";
      return 1;
    }
  }

  // Tell the client no more echoed bytes are coming.
  if (!client.shutdown_write())
  {
    std::cerr << "Could not shut down client write direction.\n";
    return 1;
  }
  if (!client.close())
  {
    std::cerr << "Could not close TCP connection.\n";
    return 1;
  }
  if (!listener.close())
  {
    std::cerr << "Could not close TCP listener.\n";
    return 1;
  }
  if (!provider.shutdown())
  {
    std::cerr << "Could not shut down Network provider.\n";
    return 1;
  }

  std::cout << "Client served; server exiting.\n";
  return 0;
}
