#include <vix/http/Http.hpp>

#include <vix/network/Address.hpp>
#include <vix/network/Endpoint.hpp>
#include <vix/network/Provider.hpp>
#include <vix/network/TcpListener.hpp>

#include <atomic>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <type_traits>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace
{
  static_assert(std::is_copy_constructible_v<vix::http::Request>);
  static_assert(std::is_copy_constructible_v<vix::http::Response>);
  static_assert(!std::is_copy_constructible_v<vix::http::ServerSession>);

  void require(bool condition, const char *message)
  {
    if (!condition)
    {
      std::cerr << "HTTP public API test failed: " << message << '\n';
      std::abort();
    }
  }

  std::vector<std::byte> bytes(std::string_view value)
  {
    std::vector<std::byte> result;
    for (const unsigned char c : value)
      result.push_back(static_cast<std::byte>(c));
    return result;
  }

  std::string string_of(const std::vector<std::byte> &value)
  {
    std::string result;
    for (const auto c : value)
      result.push_back(static_cast<char>(std::to_integer<unsigned char>(c)));
    return result;
  }

  void client_server_exchange()
  {
    auto provider_result = vix::network::Provider::create();
    require(static_cast<bool>(provider_result), "create provider");
    auto provider = provider_result.take();
    auto address_result = vix::network::Address::parse("127.0.0.1");
    require(static_cast<bool>(address_result), "parse loopback address");
    auto endpoint_result = vix::network::Endpoint::create(address_result.take(), 0);
    require(static_cast<bool>(endpoint_result), "create listener endpoint");
    auto listener_result = provider.listen(endpoint_result.value());
    if (!listener_result)
    {
      std::cerr << "HTTP listen failed: " << listener_result.error().message() << '\n';
      std::abort();
    }
    auto listener = listener_result.take();
    auto local_result = listener.local_endpoint();
    require(static_cast<bool>(local_result), "read bound endpoint");
    const auto endpoint = local_result.take();

    std::atomic<bool> server_ok{false};
    std::thread server([&]
    {
      bool all_ok = true;
      for (int index = 0; index < 2 && all_ok; ++index)
      {
        auto accept_operation = vix::http::accept(provider, listener);
        if (!accept_operation)
          return;
        auto accepted = vix::network::run(provider, accept_operation.take());
        if (!accepted || accepted.value().status() != vix::CompletionStatus::Success)
          return;
        auto session = accepted.take().take();
        const auto premature_response = session.send_response(vix::http::Response{});
        if (premature_response ||
            premature_response.error().identity() != vix::http::invalid_session_state)
          return;
        auto request_operation = session.receive_request();
        if (!request_operation)
          return;
        auto request = vix::network::run(provider, request_operation.take());
        if (!request || request.value().status() != vix::CompletionStatus::Success)
          return;
        auto received = request.take().take();
        vix::http::Response response;
        response.status_code = 200;
        response.reason = "OK";
        if (index == 0)
        {
          if (received.method != "POST" || received.target != "/snapshot" ||
              string_of(received.body) != "request-body")
            return;
          response.body = bytes("response-body");
        }
        else
        {
          if (received.method != "HEAD" || received.target != "/head")
            return;
          // This is the selected representation; HTTP must not transmit it for HEAD.
          response.body = bytes("hidden-representation");
        }
        auto response_operation = session.send_response(response);
        if (!response_operation)
          return;
        if (index == 0)
        {
          // Sending owns a serialized snapshot, not a borrow from response.
          response.body = bytes("changed-after-start");
        }
        auto sent = vix::network::run(provider, response_operation.take());
        all_ok = sent && sent.value().status() == vix::CompletionStatus::Success;
      }
      server_ok.store(all_ok, std::memory_order_release);
    });

    vix::http::Request outgoing;
    outgoing.method = "POST";
    outgoing.target = "/snapshot";
    outgoing.headers.push_back({"Host", "localhost"});
    outgoing.body = bytes("request-body");
    auto exchange_operation = vix::http::exchange(provider, endpoint, outgoing);
    require(static_cast<bool>(exchange_operation), "prepare HTTP exchange");
    outgoing.target = "/changed";
    outgoing.body = bytes("changed-after-start");
    auto response = vix::network::run(provider, exchange_operation.take());
    require(static_cast<bool>(response), "runner admission");
    require(response.value().status() == vix::CompletionStatus::Success,
            "HTTP exchange terminal success");
    auto received = response.take().take();
    require(received.status_code == 200, "response status");
    if (string_of(received.body) != "response-body")
      std::cerr << "unexpected response body size=" << received.body.size()
                << " text=" << string_of(received.body) << '\n';
    require(string_of(received.body) == "response-body", "response snapshot body");

    vix::http::Request head_request;
    head_request.method = "HEAD";
    head_request.target = "/head";
    head_request.headers.push_back({"Host", "localhost"});
    auto head_operation = vix::http::exchange(provider, endpoint, head_request);
    require(static_cast<bool>(head_operation), "prepare HEAD exchange");
    auto head_result = vix::network::run(provider, head_operation.take());
    require(head_result && head_result.value().status() == vix::CompletionStatus::Success,
            "HEAD exchange terminal success");
    const auto head_response = head_result.take().take();
    require(head_response.status_code == 200, "HEAD response status");
    require(head_response.body.empty(), "HEAD response representation is not transmitted");
    server.join();
    require(server_ok.load(std::memory_order_acquire), "server exchange completed");
    require(static_cast<bool>(listener.close()), "close listener");
    require(static_cast<bool>(provider.shutdown()), "shutdown provider");
  }

  void cooperative_client_exchange()
  {
    const int server_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    require(server_fd >= 0, "create native cooperative peer");
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = 0;
    require(::bind(server_fd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) == 0,
            "bind native cooperative peer");
    require(::listen(server_fd, 1) == 0, "listen native cooperative peer");
    socklen_t address_size = sizeof(address);
    require(::getsockname(server_fd, reinterpret_cast<sockaddr *>(&address), &address_size) == 0,
            "read native cooperative peer port");
    const auto port = ntohs(address.sin_port);
    std::thread peer([server_fd]
    {
      const int connection = ::accept(server_fd, nullptr, nullptr);
      if (connection >= 0)
      {
        char request[4096]{};
        (void)::recv(connection, request, sizeof(request), 0);
        constexpr char reply[] = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\nConnection: close\r\n\r\nOK";
        std::size_t offset = 0;
        while (offset < sizeof(reply) - 1)
        {
          const auto count = ::send(connection, reply + offset, sizeof(reply) - 1 - offset, 0);
          if (count <= 0) break;
          offset += static_cast<std::size_t>(count);
        }
        (void)::close(connection);
      }
      (void)::close(server_fd);
    });

    auto provider_result = vix::network::Provider::create_cooperative();
    require(static_cast<bool>(provider_result), "create cooperative provider");
    auto provider = provider_result.take();
    auto address_result = vix::network::Address::parse("127.0.0.1");
    require(static_cast<bool>(address_result), "parse cooperative peer address");
    auto endpoint = vix::network::Endpoint::create(address_result.take(), port);
    require(static_cast<bool>(endpoint), "create cooperative peer endpoint");
    vix::http::Request request;
    request.headers.push_back({"Host", "localhost"});
    auto operation = vix::http::exchange(provider, endpoint.value(), request);
    require(static_cast<bool>(operation), "prepare cooperative HTTP client");
    auto outcome = vix::network::run(provider, operation.take());
    require(outcome && outcome.value().status() == vix::CompletionStatus::Success,
            "cooperative HTTP client completes through runner polling");
    require(outcome.take().take().status_code == 200, "cooperative HTTP response parsed");
    peer.join();
    require(static_cast<bool>(provider.shutdown()), "shutdown cooperative provider");
  }

  void preparation_rejects_invalid_request()
  {
    auto provider_result = vix::network::Provider::create();
    require(static_cast<bool>(provider_result), "create provider for validation");
    auto provider = provider_result.take();
    auto address = vix::network::Address::parse("127.0.0.1");
    require(static_cast<bool>(address), "parse address for validation");
    auto endpoint = vix::network::Endpoint::create(address.take(), 1);
    require(static_cast<bool>(endpoint), "endpoint for validation");
    vix::http::Request request;
    auto result = vix::http::exchange(provider, endpoint.value(), request);
    require(!result && result.error().identity() == vix::http::invalid_message,
            "missing HTTP/1.1 Host rejected before connection start");
    require(static_cast<bool>(provider.shutdown()), "shutdown validation provider");
  }
}

int main()
{
  client_server_exchange();
  cooperative_client_exchange();
  preparation_rejects_invalid_request();
}
