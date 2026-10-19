#include "HttpTlsAccess.hpp"

#include "PrivateChannel.hpp"

namespace vix::tls::detail
{
  std::shared_ptr<vix::network::detail::ProviderCore>
  HttpTlsAccess::provider(const TlsChannel &channel) noexcept
  {
    return channel.channel_ ? channel.channel_->provider_for_http_composition()
                            : nullptr;
  }
}
