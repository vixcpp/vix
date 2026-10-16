#pragma once

#include <vix/network/Operation.hpp>
#include <vix/network/TcpListener.hpp>
#include <vix/Result.hpp>
#include <cstddef>
#include <memory>

namespace vix::network
{
  class Endpoint;
  class TcpConnection;
  class TcpListener;
  namespace detail { class NetworkAccess; }
  class Provider
  {
  public:
    static vix::Result<Provider> create();
    // Creates a thread-free provider. Operations advance only when poll() is
    // called; the provider and all of its resources are thread-affine to the
    // creating thread. poll() is nonblocking and may not be called recursively.
    static vix::Result<Provider> create_cooperative();
    Provider(Provider &&) noexcept = default;
    Provider(const Provider &) = delete;
    Provider &operator=(Provider &&) = delete;
    Provider &operator=(const Provider &) = delete;
    ~Provider();
    vix::Result<void> shutdown();
    // Performs one nonblocking progress turn: queued commands, expired
    // deadlines, ready native events, then eligible coroutine resumptions.
    // Returns the number of progress actions observed. Valid only for a
    // cooperative provider; a background provider is already self-driven.
    vix::Result<std::size_t> poll();
    Operation<TcpConnection> connect(const Endpoint &, OperationOptions = {});
    vix::Result<TcpListener> listen(const Endpoint &, ListenOptions = {});
  private:
    explicit Provider(std::shared_ptr<void> core) noexcept : core_(std::move(core)) {}
    std::shared_ptr<void> core_;
    friend class detail::NetworkAccess;
  };
}
