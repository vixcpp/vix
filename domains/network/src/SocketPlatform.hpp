#pragma once

#include <vix/network/Address.hpp>
#include <vix/network/Endpoint.hpp>
#include <vix/Result.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace vix::network::detail
{
  inline constexpr std::size_t numeric_address_text_size = 46;
  using NativeSocket = std::uintptr_t;
  inline constexpr NativeSocket invalid_socket = ~NativeSocket{};

  struct NativeAddress
  {
    alignas(std::max_align_t) std::array<std::byte, 128> storage{};
    int length{};
  };

  NativeAddress make_native_address(AddressFamily, const std::array<std::uint8_t, 16> &,
                                   std::uint32_t scope, std::uint16_t port) noexcept;
  vix::Result<Endpoint> endpoint_from_native_address(const NativeAddress &);
  vix::Result<NativeSocket> create_tcp_socket(AddressFamily) noexcept;
  NativeSocket accept_tcp_socket(NativeSocket, int &native_error) noexcept;
  int bind_socket(NativeSocket, const NativeAddress &) noexcept;
  int listen_socket(NativeSocket, int backlog) noexcept;
  int connect_socket(NativeSocket, const NativeAddress &) noexcept;
  int connection_error(NativeSocket, int &error) noexcept;
  vix::Result<NativeAddress> local_socket_address(NativeSocket) noexcept;
  std::ptrdiff_t receive_socket(NativeSocket, std::span<std::byte>) noexcept;
  std::ptrdiff_t send_socket(NativeSocket, std::span<const std::byte>) noexcept;
  int shutdown_socket_write(NativeSocket) noexcept;
  int last_socket_error() noexcept;
  bool socket_error_interrupted(int) noexcept;
  bool socket_error_would_block(int) noexcept;
  bool socket_error_accept_retry(int) noexcept;
  bool socket_error_connect_pending(int) noexcept;
  bool socket_error_address_in_use(int) noexcept;
  int socket_error_peer_closed_code() noexcept;
  int close_native_socket(NativeSocket) noexcept;
  vix::Error transport_error(int, const char *where) noexcept;
  vix::Error connect_error(int, const char *where) noexcept;
  bool parse_numeric_address(AddressFamily family, const char *text, std::uint8_t *bytes) noexcept;
  bool format_numeric_address(AddressFamily family, const std::uint8_t *bytes, char *text, std::size_t size) noexcept;
}
