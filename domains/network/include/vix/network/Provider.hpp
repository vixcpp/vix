#pragma once

#include <vix/network/Operation.hpp>
#include <vix/Result.hpp>
#include <memory>

namespace vix::network
{
  class Endpoint;
  class TcpConnection;
  namespace detail { class NetworkAccess; }
  class Provider
  {
  public:
    static vix::Result<Provider> create();
    Provider(Provider &&) noexcept = default;
    Provider(const Provider &) = delete;
    Provider &operator=(Provider &&) = delete;
    Provider &operator=(const Provider &) = delete;
    ~Provider();
    vix::Result<void> shutdown();
    Operation<TcpConnection> connect(const Endpoint &, OperationOptions = {});
  private:
    explicit Provider(std::shared_ptr<void> core) noexcept : core_(std::move(core)) {}
    std::shared_ptr<void> core_;
    friend class detail::NetworkAccess;
  };
}
