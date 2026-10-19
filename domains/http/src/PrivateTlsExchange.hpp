#pragma once

#include "PrivateExchange.hpp"

#include <vix/tls/TlsChannel.hpp>

namespace vix::http::detail
{
  // Private test-only observation seam. It reports exchange progression but
  // never participates in scheduling, cancellation, or completion.
  struct PrivateExchangeTestObserver
  {
    void (*phase)(void *, const char *) noexcept{};
    void *context{};
  };

  // Private opt-in HTTPS composition proof. TlsChannel is consumed and retains
  // exclusive ownership of its Network TCP lease for the whole exchange.
  [[nodiscard]] vix::Result<vix::network::Operation<ExchangeResponse>>
  start_private_tls_exchange(vix::tls::TlsChannel &&,
                             std::span<const std::byte> serialized_request,
                             vix::network::OperationOptions = {},
                             vix::Duration exchange_lifetime =
                                 default_exchange_lifetime,
                             std::size_t receive_chunk_for_test =
                                 default_exchange_receive_chunk,
                             PrivateExchangeTestObserver = {},
                             bool establish_default_deadline = true,
                             std::size_t response_body_limit = 65536);
}
