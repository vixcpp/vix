#pragma once

#include <vix/Result.hpp>
#include <vix/network/Operation.hpp>
#include <cstddef>
#include <memory>
#include <span>

namespace vix::network
{
  namespace detail { class TcpConnectionState; class NetworkAccess; }
  class TcpConnection
  {
  public:
    TcpConnection(TcpConnection &&) noexcept;
    TcpConnection(const TcpConnection &) = delete;
    TcpConnection &operator=(TcpConnection &&) = delete;
    TcpConnection &operator=(const TcpConnection &) = delete;
    ~TcpConnection();
    vix::Result<void> close();
    // Half-closes only the local write direction; receiving remains available.
    vix::Result<void> shutdown_write();
    // The supplied storage is borrowed until the operation reaches terminal completion.
    // EOF is reported as Success(0).
    Operation<std::size_t> receive(std::span<std::byte>, OperationOptions = {});
    // The supplied bytes are borrowed and must not change until terminal completion.
    // Success means the complete span was accepted by the socket; a non-success
    // may have transmitted a prefix.
    Operation<void> send(std::span<const std::byte>, OperationOptions = {});
  private:
    explicit TcpConnection(std::shared_ptr<detail::TcpConnectionState>) noexcept;
    std::shared_ptr<detail::TcpConnectionState> state_;
    friend class detail::NetworkAccess;
  };
}
