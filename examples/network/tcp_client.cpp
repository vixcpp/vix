/**
 * @file tcp_client.cpp
 * @brief Connects to the Vix TCP echo server and exchanges a byte stream.
 *
 * Start examples/network/tcp_server.cpp in another terminal first.
 * Uses the synchronous run() bridge over asynchronous Network operations.
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

  auto created = Provider::create();
  if (!created)
  {
    std::cerr << "Provider creation failed: " << created.error().message() << '\n';
    return 1;
  }
trash: /// controlled_wait.cpp
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

  // run() starts one asynchronous operation and waits for its Completion.
  auto connected = run(provider, provider.connect(endpoint.value()));
  if (!connected)
  {
    std::cerr << "Runner failed: " << connected.error().message() << '\n';
    return 1;
  }
  auto connection_outcome = connected.take();
  if (connection_outcome.status() != vix::CompletionStatus::Success)
  {
    if (connection_outcome.status() == vix::CompletionStatus::Failure)
      std::cerr << "Connect failed: " << connection_outcome.error().message() << '\n';
    else
      std::cerr << "Connect was cancelled.\n";
    return 1;
  }
  auto connection = connection_outcome.take();

  const std::string message = "Hello from Vix Network!\n";
  const auto bytes = std::as_bytes(std::span{message.data(), message.size()});
  auto sent = run(provider, connection.send(bytes));
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

  // Signal end-of-request without closing the receive direction.
  auto half_closed = connection.shutdown_write();
  if (!half_closed)
  {
    std::cerr << "Write shutdown failed: " << half_closed.error().message() << '\n';
    return 1;
  }

  std::cout << "Echo from server: ";
  std::array<std::byte, 4096> buffer{};
  while (true)
  {
    auto received = run(provider, connection.receive(buffer));
    if (!received)
    {
      std::cerr << "Receive runner failed: " << received.error().message() << '\n';
      return 1;
    }
    auto outcome = received.take();
    if (outcome.status() != vix::CompletionStatus::Success)
    {
      if (outcome.status() == vix::CompletionStatus::Failure)
        std::cerr << "Receive failed: " << outcome.error().message() << '\n';
      else
        std::cerr << "Receive was cancelled.\n";
      return 1;
    }

    const auto count = outcome.take();
    if (count == 0)
      break; // EOF for a non-empty buffer.

    // TCP is a byte stream: a receive may contain only part of the echo.
    std::cout.write(reinterpret_cast<const char *>(buffer.data()),
                    static_cast<std::streamsize>(count));
  }

  if (!connection.close())
  {
    std::cerr << "Could not close TCP connection.\n";
    return 1;
  }
  if (!provider.shutdown())
  {
    std::cerr << "Could not shut down Network provider.\n";
    return 1;
  }
  return 0;
}
