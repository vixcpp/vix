#pragma once

#include <openssl/ssl.h>

#include <memory>
#include <string>

namespace vix::tls::detail
{
  struct TlsContextState final
  {
    explicit TlsContextState(SSL_CTX *owned_context) noexcept : context(owned_context) {}
    ~TlsContextState() { SSL_CTX_free(context); }
    TlsContextState(const TlsContextState &) = delete;
    TlsContextState &operator=(const TlsContextState &) = delete;

    SSL_CTX *context{};
    bool server_role{};
    bool identity_is_ip{};
    std::string expected_identity;
    std::string server_name;
  };

  using SharedTlsContext = std::shared_ptr<const TlsContextState>;
}
