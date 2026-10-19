#pragma once

#include <vix/Result.hpp>
#include <vix/ErrorIdentity.hpp>
#include <vix/network/Address.hpp>
#include <vix/network/Operation.hpp>
#include <vix/network/Provider.hpp>
#include <vix/network/TcpListener.hpp>

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace vix::http
{
  inline constexpr auto invalid_message = vix::ErrorIdentity::define<"vix.http">(10);
  inline constexpr auto message_too_large = vix::ErrorIdentity::define<"vix.http">(11);
  inline constexpr auto invalid_session_state = vix::ErrorIdentity::define<"vix.http">(12);
  inline constexpr auto unsupported_message = vix::ErrorIdentity::define<"vix.http">(13);

  struct HeaderField
  {
    std::string name;
    std::string value;
  };

  // Message values own their fields and body. Body is deliberately buffered
  // and bounded in this experimental API; public streaming is not yet offered.
  struct Request
  {
    std::string method{"GET"};
    std::string target{"/"};
    std::string version{"HTTP/1.1"};
    std::vector<HeaderField> headers;
    std::vector<std::byte> body;
    std::vector<HeaderField> trailers;
  };

  struct Response
  {
    std::string version{"HTTP/1.1"};
    unsigned status_code{200};
    std::string reason{"OK"};
    std::vector<HeaderField> headers;
    std::vector<std::byte> body;
    std::vector<HeaderField> trailers;
  };

  struct Limits
  {
    std::size_t request_body_bytes{65536};
    std::size_t response_body_bytes{65536};
    std::size_t serialized_message_bytes{8192};
  };

  struct ExchangeOptions
  {
    vix::network::OperationOptions operation{};
    Limits limits{};
  };

  struct ServerOptions
  {
    vix::network::OperationOptions operation{};
    Limits limits{};
  };

  namespace detail { class SessionState; struct SessionAccess; }

  // A one-request, one-response protocol session. The application decides
  // what the request means; this type does not dispatch or route it.
  class ServerSession final
  {
  public:
    ServerSession(ServerSession &&) noexcept;
    ServerSession &operator=(ServerSession &&) noexcept;
    ServerSession(const ServerSession &) = delete;
    ServerSession &operator=(const ServerSession &) = delete;
    ~ServerSession();

    [[nodiscard]] vix::Result<vix::network::Operation<Request>> receive_request(
        vix::network::OperationOptions = {});
    [[nodiscard]] vix::Result<vix::network::Operation<void>> send_response(
        const Response &, vix::network::OperationOptions = {});
    void close() noexcept;

  private:
    explicit ServerSession(std::shared_ptr<detail::SessionState>) noexcept;
    std::shared_ptr<detail::SessionState> state_;
    friend struct detail::SessionAccess;
    friend vix::Result<vix::network::Operation<ServerSession>> accept(
        vix::network::Provider &, vix::network::TcpListener &, ServerOptions);
  };

  // Synchronously snapshots and validates request before returning an
  // operation. The operation connects and performs one bounded exchange.
  [[nodiscard]] vix::Result<vix::network::Operation<Response>> exchange(
      vix::network::Provider &, const vix::network::Endpoint &,
      const Request &, ExchangeOptions = {});

  // Accepts one transport and returns an HTTP session operation. A session
  // accepts exactly one request and sends at most one response.
  [[nodiscard]] vix::Result<vix::network::Operation<ServerSession>> accept(
      vix::network::Provider &, vix::network::TcpListener &,
      ServerOptions = {});
}
