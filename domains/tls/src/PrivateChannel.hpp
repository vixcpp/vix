#pragma once

#include "OpenSslDriver.hpp"

#include "../../network/src/Engine.hpp"

#include <vix/Completion.hpp>
#include <vix/Cancellation.hpp>
#include <vix/Deadline.hpp>
#include <vix/network/Provider.hpp>
#include <vix/network/TcpConnection.hpp>

#include <array>
#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <utility>

namespace vix::tls::detail
{
  class PrivateChannel final
  {
  public:
    struct State;
    struct ShutdownDiagnostics
    {
      OpenSslDriver::ShutdownDiagnostics engine;
      std::size_t network_ciphertext_generated{};
      std::size_t network_ciphertext_sent{};
    };
    static vix::Result<std::shared_ptr<PrivateChannel>> create(
        vix::network::detail::TcpTransportLease lease,
        SSL_CTX *context, bool server_role);
    static vix::Result<std::shared_ptr<PrivateChannel>> create(
        vix::network::detail::TcpTransportLease lease,
        std::unique_ptr<OpenSslDriver> driver);
    static vix::Result<std::shared_ptr<PrivateChannel>> prepare(
        std::unique_ptr<OpenSslDriver> driver);
    bool attach(vix::network::detail::TcpTransportLease lease) noexcept;

    PrivateChannel(const PrivateChannel &) = delete;
    PrivateChannel &operator=(const PrivateChannel &) = delete;
    ~PrivateChannel();

    // Experimental handshake operation only. The returned Network operation
    // intentionally retains Network's non-abandonable started-await contract.
    vix::network::Operation<void> handshake(vix::CancellationToken cancellation = {},
                                            std::optional<vix::Deadline> deadline = {});
    vix::network::Operation<std::size_t> read(std::span<std::byte> output,
                                              vix::CancellationToken cancellation = {},
                                              std::optional<vix::Deadline> deadline = {});
    vix::network::Operation<void> write(std::span<const std::byte> input,
                                        vix::CancellationToken cancellation = {},
                                        std::optional<vix::Deadline> deadline = {});
    vix::network::Operation<void> shutdown(vix::CancellationToken cancellation = {},
                                          std::optional<vix::Deadline> deadline = {});
    [[nodiscard]] ShutdownDiagnostics
    shutdown_diagnostics_for_test() const noexcept;
    [[nodiscard]] std::shared_ptr<vix::network::detail::ProviderCore>
    provider_for_http_composition() const noexcept;
    void close() noexcept;

  private:
    explicit PrivateChannel(std::shared_ptr<State> state) noexcept
        : state_(std::move(state)) {}
    std::shared_ptr<State> state_;
  };
}
