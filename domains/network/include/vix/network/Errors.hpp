#pragma once

#include <vix/Error.hpp>

namespace vix::network
{
  inline constexpr auto resource_exhausted = vix::ErrorIdentity::define<"vix.network">(17);
  inline constexpr auto io_failure = vix::ErrorIdentity::define<"vix.network">(18);
  inline constexpr auto close_failed = vix::ErrorIdentity::define<"vix.network">(19);
  inline constexpr auto provider_in_use = vix::ErrorIdentity::define<"vix.network">(20);
  inline constexpr auto timed_out = vix::ErrorIdentity::define<"vix.network">(21);
  inline constexpr auto provider_stopped = vix::ErrorIdentity::define<"vix.network">(22);
  inline constexpr auto permission_denied = vix::ErrorIdentity::define<"vix.network">(23);
  inline constexpr auto address_unavailable = vix::ErrorIdentity::define<"vix.network">(24);
  inline constexpr auto unsupported = vix::ErrorIdentity::define<"vix.network">(25);
  inline constexpr auto connection_refused = vix::ErrorIdentity::define<"vix.network">(26);
  inline constexpr auto connection_reset = vix::ErrorIdentity::define<"vix.network">(27);
  inline constexpr auto connection_aborted = vix::ErrorIdentity::define<"vix.network">(28);
  inline constexpr auto network_unreachable = vix::ErrorIdentity::define<"vix.network">(29);
  inline constexpr auto host_unreachable = vix::ErrorIdentity::define<"vix.network">(30);
  inline constexpr auto peer_closed = vix::ErrorIdentity::define<"vix.network">(31);
  inline constexpr auto operation_in_progress = vix::ErrorIdentity::define<"vix.network">(32);
  inline constexpr auto write_shutdown = vix::ErrorIdentity::define<"vix.network">(33);
  inline constexpr auto address_in_use = vix::ErrorIdentity::define<"vix.network">(34);
  // The TCP transport has been exclusively transferred to a private Network
  // owner; an already-created ordinary operation cannot use it.
  inline constexpr auto transport_owned = vix::ErrorIdentity::define<"vix.network">(35);
}
