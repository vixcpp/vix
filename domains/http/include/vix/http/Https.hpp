#pragma once

#include <vix/http/Http.hpp>
#include <vix/tls/TlsConfig.hpp>
#include <vix/tls/Errors.hpp>

namespace vix::http
{
  namespace detail { class HttpsSessionState; struct HttpsSessionAccess; }

  // Opt-in experimental HTTPS operations. These declarations are installed
  // only with the HTTPSTLSExperimental package component.
  class HttpsServerSession final
  {
  public:
    HttpsServerSession(HttpsServerSession &&) noexcept;
    HttpsServerSession &operator=(HttpsServerSession &&) noexcept;
    HttpsServerSession(const HttpsServerSession &) = delete;
    HttpsServerSession &operator=(const HttpsServerSession &) = delete;
    ~HttpsServerSession();

    [[nodiscard]] vix::Result<vix::network::Operation<Request>> receive_request(
        vix::network::OperationOptions = {});
    [[nodiscard]] vix::Result<vix::network::Operation<void>> send_response(
        const Response &, vix::network::OperationOptions = {});
    void close() noexcept;

  private:
    explicit HttpsServerSession(std::shared_ptr<detail::HttpsSessionState>) noexcept;
    std::shared_ptr<detail::HttpsSessionState> state_;
    friend struct detail::HttpsSessionAccess;
    friend vix::Result<vix::network::Operation<HttpsServerSession>> https_accept(
        vix::network::Provider &, vix::network::TcpListener &,
        const vix::tls::TlsServerConfig &, ServerOptions);
  };

  // The client config is mandatory: failure to authenticate is terminal and
  // never falls back to plaintext HTTP.
  [[nodiscard]] vix::Result<vix::network::Operation<Response>> https_exchange(
      vix::network::Provider &, const vix::network::Endpoint &, const Request &,
      const vix::tls::TlsClientConfig &, ExchangeOptions = {});

  // Accepts one TCP connection, completes the authenticated TLS server
  // handshake, then returns a one-request/one-response HTTP session.
  [[nodiscard]] vix::Result<vix::network::Operation<HttpsServerSession>> https_accept(
      vix::network::Provider &, vix::network::TcpListener &,
      const vix::tls::TlsServerConfig &, ServerOptions = {});
}
