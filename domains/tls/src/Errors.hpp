#pragma once

#include <vix/ErrorIdentity.hpp>
#include <vix/tls/Errors.hpp>

namespace vix::tls::detail
{
  inline constexpr auto backend_failure = vix::tls::backend_failure;
  inline constexpr auto transport_failure = vix::tls::transport_failure;
  inline constexpr auto peer_truncated = vix::tls::peer_truncated;
  inline constexpr auto operation_conflict = vix::tls::operation_conflict;
  inline constexpr auto buffer_limit = vix::tls::resource_limit;
  inline constexpr auto channel_closed = vix::tls::channel_closed;
  inline constexpr auto peer_closed = vix::tls::peer_closed;
  inline constexpr auto authentication_failed = vix::tls::authentication_failed;
  inline constexpr auto peer_identity_mismatch = vix::tls::peer_identity_mismatch;
}
