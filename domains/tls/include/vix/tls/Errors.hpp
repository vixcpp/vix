#pragma once

#include <vix/ErrorIdentity.hpp>

namespace vix::tls
{
  inline constexpr auto authentication_failed = vix::ErrorIdentity::define<"vix.tls">(1);
  inline constexpr auto peer_identity_mismatch = vix::ErrorIdentity::define<"vix.tls">(2);
  inline constexpr auto protocol_failure = vix::ErrorIdentity::define<"vix.tls">(3);
  inline constexpr auto peer_truncated = vix::ErrorIdentity::define<"vix.tls">(4);
  inline constexpr auto operation_conflict = vix::ErrorIdentity::define<"vix.tls">(5);
  inline constexpr auto resource_limit = vix::ErrorIdentity::define<"vix.tls">(6);
  inline constexpr auto channel_closed = vix::ErrorIdentity::define<"vix.tls">(7);
  inline constexpr auto peer_closed = vix::ErrorIdentity::define<"vix.tls">(8);
  inline constexpr auto backend_failure = vix::ErrorIdentity::define<"vix.tls">(9);
  inline constexpr auto invalid_configuration = vix::ErrorIdentity::define<"vix.tls">(10);
  inline constexpr auto transport_failure = vix::ErrorIdentity::define<"vix.tls">(11);
}
