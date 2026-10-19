#pragma once

#include <vix/Completion.hpp>
#include <vix/Result.hpp>
#include <vix/network/Operation.hpp>
#include <vix/network/TcpConnection.hpp>
#include <vix/tls/TlsConfig.hpp>
#include <vix/tls/Errors.hpp>

#include <memory>
#include <optional>
#include <span>
#include <cstddef>
#include <utility>

namespace vix::tls::detail
{
  class PrivateChannel;
  class HttpTlsAccess;
}

namespace vix::tls
{
  // Experimental, opt-in secure stream. This header and target are not part
  // of the stable installed SDK until TLS is qualified.
  class TlsChannel final
  {
  public:
    static vix::Result<TlsChannel> create_client(
        vix::network::TcpConnection &&, const TlsClientConfig &);
    static vix::Result<TlsChannel> create_server(
        vix::network::TcpConnection &&, const TlsServerConfig &);

    TlsChannel(TlsChannel &&) noexcept = default;
    TlsChannel(const TlsChannel &) = delete;
    TlsChannel &operator=(TlsChannel &&) noexcept = default;
    TlsChannel &operator=(const TlsChannel &) = delete;
    ~TlsChannel() = default;

    vix::network::Operation<void> handshake(
        vix::network::OperationOptions = {});
    // output is borrowed until terminal completion. Clean peer close_notify is
    // reported as peer_closed; raw TCP EOF without it is peer_truncated.
    vix::network::Operation<std::size_t> receive(
        std::span<std::byte>, vix::network::OperationOptions = {});
    // input is borrowed until terminal completion. Success means all emitted
    // ciphertext was accepted by Network, not that the peer consumed it.
    vix::network::Operation<void> send(
        std::span<const std::byte>, vix::network::OperationOptions = {});
    // Graceful close_notify exchange; failure/cancellation closes the channel.
    vix::network::Operation<void> shutdown(
        vix::network::OperationOptions = {});
    // Immediate abortive close. All started operations remain retained until
    // their Network children reach terminal completion.
    void close() noexcept;

  private:
    explicit TlsChannel(std::shared_ptr<detail::PrivateChannel> channel) noexcept
        : channel_(std::move(channel)) {}
    std::shared_ptr<detail::PrivateChannel> channel_;
    friend class detail::HttpTlsAccess;
  };
}
