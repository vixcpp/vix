#pragma once

#include <vix/tls/TlsChannel.hpp>

#include "../../network/src/Engine.hpp"

#include <memory>

namespace vix::tls::detail
{
  // Private cross-Domain seam for the experimental HTTP composition proof.
  // It exposes only provider binding; TCP ownership and TLS state remain
  // encapsulated by TlsChannel/PrivateChannel.
  class HttpTlsAccess final
  {
  public:
    [[nodiscard]] static std::shared_ptr<vix::network::detail::ProviderCore>
    provider(const TlsChannel &) noexcept;
  };
}
