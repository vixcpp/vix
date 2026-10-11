#pragma once

#include <vix/Result.hpp>
#include <vix/network/Address.hpp>
#include <vix/network/Operation.hpp>
#include <cstdint>
#include <memory>

namespace vix::network
{
  class TcpConnection;
  struct ListenOptions { std::uint32_t backlog{128}; };
  namespace detail { class TcpListenerState; class NetworkAccess; }
  class TcpListener
  {
  public:
    TcpListener(TcpListener&&) noexcept;
    TcpListener(const TcpListener&) = delete;
    TcpListener& operator=(TcpListener&&) = delete;
    ~TcpListener();
    Operation<TcpConnection> accept(OperationOptions = {});
    vix::Result<Endpoint> local_endpoint() const;
    vix::Result<void> close();
  private:
    explicit TcpListener(std::shared_ptr<detail::TcpListenerState>) noexcept;
    std::shared_ptr<detail::TcpListenerState> state_;
    friend class Provider;
    friend class detail::NetworkAccess;
  };
}
