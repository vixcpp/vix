#include <vix/tls/TlsChannel.hpp>

#include "OpenSslDriver.hpp"
#include "PrivateChannel.hpp"
#include "TlsContext.hpp"

#include "../../network/src/Engine.hpp"

#include <new>
#include <utility>

namespace vix::tls
{
  vix::Result<TlsChannel> TlsChannel::create_client(
      vix::network::TcpConnection &&connection, const TlsClientConfig &config)
  {
    try
    {
      if (!config.state_ || config.state_->server_role)
        return vix::Error{invalid_configuration, "TLS client configuration is invalid"};
      auto driver = detail::OpenSslDriver::create(
          config.state_->context, false, detail::OpenSslDriver::buffer_capacity,
          config.state_->expected_identity, config.state_->identity_is_ip,
          config.state_->server_name);
      if (!driver)
        return driver.error();
      auto channel = detail::PrivateChannel::prepare(driver.take());
      if (!channel)
        return channel.error();
      auto lease = vix::network::detail::NetworkAccess::claim_tcp_for_tls(
          std::move(connection));
      if (!lease)
        return lease.error();
      if (!channel.value()->attach(lease.take()))
        return vix::Error{transport_failure, "claimed TLS transport could not be attached"};
      return TlsChannel{channel.take()};
    }
    catch (const std::bad_alloc &)
    {
      return vix::Error{resource_limit, "TLS client channel allocation failed"};
    }
  }

  vix::Result<TlsChannel> TlsChannel::create_server(
      vix::network::TcpConnection &&connection, const TlsServerConfig &config)
  {
    try
    {
      if (!config.state_ || !config.state_->server_role)
        return vix::Error{invalid_configuration, "TLS server configuration is invalid"};
      auto driver = detail::OpenSslDriver::create(
          config.state_->context, true, detail::OpenSslDriver::buffer_capacity);
      if (!driver)
        return driver.error();
      auto channel = detail::PrivateChannel::prepare(driver.take());
      if (!channel)
        return channel.error();
      auto lease = vix::network::detail::NetworkAccess::claim_tcp_for_tls(
          std::move(connection));
      if (!lease)
        return lease.error();
      if (!channel.value()->attach(lease.take()))
        return vix::Error{transport_failure, "claimed TLS transport could not be attached"};
      return TlsChannel{channel.take()};
    }
    catch (const std::bad_alloc &)
    {
      return vix::Error{resource_limit, "TLS server channel allocation failed"};
    }
  }

  vix::network::Operation<void> TlsChannel::handshake(
      vix::network::OperationOptions options)
  {
    return channel_->handshake(std::move(options.cancellation), options.deadline);
  }

  vix::network::Operation<std::size_t> TlsChannel::receive(
      std::span<std::byte> output, vix::network::OperationOptions options)
  {
    return channel_->read(output, std::move(options.cancellation), options.deadline);
  }

  vix::network::Operation<void> TlsChannel::send(
      std::span<const std::byte> input, vix::network::OperationOptions options)
  {
    return channel_->write(input, std::move(options.cancellation), options.deadline);
  }

  vix::network::Operation<void> TlsChannel::shutdown(
      vix::network::OperationOptions options)
  {
    return channel_->shutdown(std::move(options.cancellation), options.deadline);
  }

  void TlsChannel::close() noexcept
  {
    if (channel_)
      channel_->close();
  }
}
