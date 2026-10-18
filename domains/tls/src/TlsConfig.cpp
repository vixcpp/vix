#include <vix/tls/TlsConfig.hpp>
#include <vix/tls/Errors.hpp>

#include "TlsContext.hpp"

#include <openssl/ssl.h>

#include <memory>
#include <new>
#include <utility>

namespace vix::tls
{
  namespace
  {
    int openssl_version(TlsVersion version) noexcept
    {
      switch (version)
      {
      case TlsVersion::tls_1_2:
        return TLS1_2_VERSION;
      case TlsVersion::tls_1_3:
        return TLS1_3_VERSION;
      }
      return 0;
    }

    bool configure_versions(SSL_CTX *context, TlsVersion minimum,
                            TlsVersion maximum) noexcept
    {
      const auto min_value = openssl_version(minimum);
      const auto max_value = openssl_version(maximum);
      return min_value != 0 && max_value != 0 && min_value <= max_value &&
             SSL_CTX_set_min_proto_version(context, min_value) == 1 &&
             SSL_CTX_set_max_proto_version(context, max_value) == 1;
    }
  }

  vix::Result<TlsClientConfig> TlsClientConfig::create(TlsClientOptions options)
  {
    if (options.expected_peer.value.empty() ||
        options.expected_peer.value.find('\0') != std::string::npos ||
        (options.expected_peer.kind != PeerIdentityKind::dns_name &&
         options.expected_peer.kind != PeerIdentityKind::ip_address) ||
        (!options.use_system_trust && !options.ca_file) ||
        (options.server_name &&
         (options.server_name->empty() ||
          options.server_name->find('\0') != std::string::npos)) ||
        (options.ca_file && options.ca_file->find('\0') != std::string::npos))
      return vix::Error{invalid_configuration,
                        "TLS client requires a peer identity and trust source"};

    std::unique_ptr<SSL_CTX, decltype(&SSL_CTX_free)> context{
        SSL_CTX_new(TLS_client_method()), SSL_CTX_free};
    if (!context)
      return vix::Error{backend_failure, "OpenSSL client context creation failed"};
    if (!configure_versions(context.get(), options.minimum_version,
                            options.maximum_version))
      return vix::Error{invalid_configuration, "TLS protocol version range is invalid"};
    SSL_CTX_set_verify(context.get(), SSL_VERIFY_PEER, nullptr);
    if (options.use_system_trust && SSL_CTX_set_default_verify_paths(context.get()) != 1)
      return vix::Error{authentication_failed, "system TLS trust roots are unavailable"};
    if (options.ca_file &&
        SSL_CTX_load_verify_locations(context.get(), options.ca_file->c_str(), nullptr) != 1)
      return vix::Error{authentication_failed, "configured TLS CA file could not be loaded"};

    try
    {
      auto state = std::make_shared<detail::TlsContextState>(context.get());
      context.release();
      state->server_role = false;
      state->identity_is_ip = options.expected_peer.kind == PeerIdentityKind::ip_address;
      state->expected_identity = std::move(options.expected_peer.value);
      if (options.server_name)
        state->server_name = std::move(*options.server_name);
      else if (!state->identity_is_ip)
        state->server_name = state->expected_identity;
      return TlsClientConfig{std::move(state)};
    }
    catch (const std::bad_alloc &)
    {
      return vix::Error{resource_limit, "TLS client configuration allocation failed"};
    }
  }

  vix::Result<TlsServerConfig> TlsServerConfig::create(TlsServerOptions options)
  {
    if (options.certificate_chain_file.empty() || options.private_key_file.empty() ||
        options.certificate_chain_file.find('\0') != std::string::npos ||
        options.private_key_file.find('\0') != std::string::npos)
      return vix::Error{invalid_configuration,
                        "TLS server requires a certificate chain and private key"};

    std::unique_ptr<SSL_CTX, decltype(&SSL_CTX_free)> context{
        SSL_CTX_new(TLS_server_method()), SSL_CTX_free};
    if (!context)
      return vix::Error{backend_failure, "OpenSSL server context creation failed"};
    if (!configure_versions(context.get(), options.minimum_version,
                            options.maximum_version))
      return vix::Error{invalid_configuration, "TLS protocol version range is invalid"};
    if (SSL_CTX_use_certificate_chain_file(context.get(),
                                           options.certificate_chain_file.c_str()) != 1 ||
        SSL_CTX_use_PrivateKey_file(context.get(), options.private_key_file.c_str(),
                                    SSL_FILETYPE_PEM) != 1 ||
        SSL_CTX_check_private_key(context.get()) != 1)
      return vix::Error{invalid_configuration,
                        "TLS server certificate and private key are invalid or mismatched"};

    try
    {
      auto state = std::make_shared<detail::TlsContextState>(context.get());
      context.release();
      state->server_role = true;
      return TlsServerConfig{std::move(state)};
    }
    catch (const std::bad_alloc &)
    {
      return vix::Error{resource_limit, "TLS server configuration allocation failed"};
    }
  }
}
