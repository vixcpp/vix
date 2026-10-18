#pragma once

#include <vix/Result.hpp>

#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace vix::tls::detail
{
  struct TlsContextState;
}

namespace vix::tls
{
  enum class TlsVersion
  {
    tls_1_2,
    tls_1_3
  };

  enum class PeerIdentityKind
  {
    dns_name,
    ip_address
  };

  struct PeerIdentity
  {
    PeerIdentityKind kind{PeerIdentityKind::dns_name};
    std::string value;
  };

  struct TlsClientOptions
  {
    // Required reference identity; DNS names are verified independently from
    // SNI and IP literals use IP address verification.
    PeerIdentity expected_peer;
    // System roots are enabled by default. A CA file may add trust; disabling
    // system roots requires an explicit CA file.
    std::optional<std::string> ca_file;
    bool use_system_trust{true};
    // SNI is independent from the identity checked above. If omitted for a
    // DNS identity, the identity is used as the default SNI name.
    std::optional<std::string> server_name;
    TlsVersion minimum_version{TlsVersion::tls_1_2};
    TlsVersion maximum_version{TlsVersion::tls_1_3};
  };

  struct TlsServerOptions
  {
    std::string certificate_chain_file;
    std::string private_key_file;
    TlsVersion minimum_version{TlsVersion::tls_1_2};
    TlsVersion maximum_version{TlsVersion::tls_1_3};
  };

  class TlsClientConfig final
  {
  public:
    static vix::Result<TlsClientConfig> create(TlsClientOptions);
    TlsClientConfig(const TlsClientConfig &) noexcept = default;
    TlsClientConfig(TlsClientConfig &&) noexcept = default;
    TlsClientConfig &operator=(const TlsClientConfig &) noexcept = default;
    TlsClientConfig &operator=(TlsClientConfig &&) noexcept = default;

  private:
    explicit TlsClientConfig(std::shared_ptr<const detail::TlsContextState> state) noexcept
        : state_(std::move(state)) {}
    std::shared_ptr<const detail::TlsContextState> state_;
    friend class TlsChannel;
  };

  class TlsServerConfig final
  {
  public:
    static vix::Result<TlsServerConfig> create(TlsServerOptions);
    TlsServerConfig(const TlsServerConfig &) noexcept = default;
    TlsServerConfig(TlsServerConfig &&) noexcept = default;
    TlsServerConfig &operator=(const TlsServerConfig &) noexcept = default;
    TlsServerConfig &operator=(TlsServerConfig &&) noexcept = default;

  private:
    explicit TlsServerConfig(std::shared_ptr<const detail::TlsContextState> state) noexcept
        : state_(std::move(state)) {}
    std::shared_ptr<const detail::TlsContextState> state_;
    friend class TlsChannel;
  };
}
