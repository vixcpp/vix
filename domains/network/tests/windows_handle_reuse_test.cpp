#include "Engine.hpp"
#include "SocketPlatform.hpp"
#include <vix/network/Address.hpp>
#include <cassert>
#include <cstdint>

int main()
{
  using namespace vix::network::detail;
  auto created = ProviderCore::create();
  assert(created);
  auto provider = created.take();

  auto first_socket = create_tcp_socket(AddressFamily::ipv4);
  assert(first_socket);
  const NativeSocket retired_handle = first_socket.value();
  const auto retired_token = provider->register_descriptor(retired_handle, true, false);
  assert(retired_token != 0);
  provider->retire(retired_token);

  // Deterministically reuse the same live SOCKET value for a new reactor
  // registration before closing it. The generation/token must not be reused.
  const auto same_handle_token = provider->register_descriptor(retired_handle, true, false);
  assert(same_handle_token != 0 && same_handle_token != retired_token);
  assert(!provider->accepts(retired_token));
  assert(!provider->dispatch_for_test(retired_token, reactor_readable));
  provider->retire(same_handle_token);
  assert(close_native_socket(retired_handle) == 0);

  NativeSocket replacement = invalid_socket;
  bool handle_reused = false;
  for (unsigned attempt = 0; attempt < 32 && !handle_reused; ++attempt)
  {
    auto socket = create_tcp_socket(AddressFamily::ipv4);
    assert(socket);
    if (socket.value() == retired_handle)
    {
      replacement = socket.value();
      handle_reused = true;
    }
    else
      assert(close_native_socket(socket.value()) == 0);
  }

  // Native handle reuse is allocator-dependent, but stale logical tokens must
  // be rejected in either case. If Winsock reused the value, the new token
  // must still identify a distinct registration generation.
  if (!handle_reused)
  {
    auto socket = create_tcp_socket(AddressFamily::ipv4);
    assert(socket);
    replacement = socket.value();
  }
  const auto replacement_token = provider->register_descriptor(replacement, true, false);
  assert(replacement_token != 0 && replacement_token != retired_token && replacement_token != same_handle_token);
  assert(!provider->accepts(retired_token));
  assert(!provider->dispatch_for_test(retired_token, reactor_readable));
  provider->retire(replacement_token);
  assert(close_native_socket(replacement) == 0);
  assert(provider->shutdown());
  return 0;
}
