#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include "SocketPlatform.hpp"
#include <vix/network/Errors.hpp>
#include <algorithm>
#include <climits>
#include <string>

namespace vix::network::detail
{
  namespace
  {
    bool winsock_call_begin() noexcept
    {
      WSADATA data{};
      return ::WSAStartup(MAKEWORD(2, 2), &data) == 0;
    }
    void winsock_call_end() noexcept { (void)::WSACleanup(); }
    int native_family(AddressFamily family) noexcept { return family == AddressFamily::ipv4 ? AF_INET : AF_INET6; }
    SOCKET native_socket(NativeSocket socket) noexcept { return static_cast<SOCKET>(socket); }
    bool configure_socket(SOCKET socket) noexcept
    {
      u_long nonblocking = 1;
      return ::ioctlsocket(socket, FIONBIO, &nonblocking) == 0;
    }
  }

  NativeAddress make_native_address(AddressFamily family, const std::array<std::uint8_t, 16> &bytes,
                                    std::uint32_t scope, std::uint16_t port) noexcept
  {
    NativeAddress out{};
    if (family == AddressFamily::ipv4)
    {
      auto *address = reinterpret_cast<sockaddr_in *>(out.storage.data());
      address->sin_family = AF_INET;
      address->sin_port = htons(port);
      std::copy_n(bytes.data(), 4, reinterpret_cast<std::uint8_t *>(&address->sin_addr));
      out.length = sizeof(sockaddr_in);
    }
    else
    {
      auto *address = reinterpret_cast<sockaddr_in6 *>(out.storage.data());
      address->sin6_family = AF_INET6;
      address->sin6_port = htons(port);
      address->sin6_scope_id = scope;
      std::copy_n(bytes.data(), 16, reinterpret_cast<std::uint8_t *>(&address->sin6_addr));
      out.length = sizeof(sockaddr_in6);
    }
    return out;
  }

  vix::Result<Endpoint> endpoint_from_native_address(const NativeAddress &address)
  {
    char text[numeric_address_text_size + 12]{};
    std::uint16_t port{};
    if (address.length >= static_cast<int>(sizeof(sockaddr_in)) &&
        reinterpret_cast<const sockaddr *>(address.storage.data())->sa_family == AF_INET)
    {
      const auto &native = *reinterpret_cast<const sockaddr_in *>(address.storage.data());
      if (!::InetNtopA(AF_INET, const_cast<IN_ADDR *>(&native.sin_addr), text, sizeof text))
        return vix::Error{io_failure, "getsockname"};
      port = ntohs(native.sin_port);
    }
    else if (address.length >= static_cast<int>(sizeof(sockaddr_in6)) &&
             reinterpret_cast<const sockaddr *>(address.storage.data())->sa_family == AF_INET6)
    {
      const auto &native = *reinterpret_cast<const sockaddr_in6 *>(address.storage.data());
      if (!::InetNtopA(AF_INET6, const_cast<IN6_ADDR *>(&native.sin6_addr), text, sizeof text))
        return vix::Error{io_failure, "getsockname"};
      port = ntohs(native.sin6_port);
      if (native.sin6_scope_id)
      {
        const auto length = std::char_traits<char>::length(text);
        text[length] = '%';
        auto scope = std::to_string(native.sin6_scope_id);
        std::copy(scope.begin(), scope.end(), text + length + 1);
      }
    }
    else
      return vix::Error{io_failure, "getsockname returned unsupported address"};
    auto parsed = Address::parse(text);
    if (!parsed) return parsed.error();
    return Endpoint::create(parsed.take(), port);
  }

  vix::Result<NativeSocket> create_tcp_socket(AddressFamily family) noexcept
  {
    const SOCKET socket = ::WSASocketW(native_family(family), SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
    if (socket == INVALID_SOCKET) return connect_error(::WSAGetLastError(), "socket");
    if (!configure_socket(socket))
    {
      const int error = ::WSAGetLastError();
      (void)::closesocket(socket);
      return connect_error(error, "configure socket");
    }
    return static_cast<NativeSocket>(socket);
  }

  NativeSocket accept_tcp_socket(NativeSocket listener, int &native_error) noexcept
  {
    const SOCKET socket = ::accept(native_socket(listener), nullptr, nullptr);
    if (socket == INVALID_SOCKET)
    {
      native_error = ::WSAGetLastError();
      return invalid_socket;
    }
    if (!configure_socket(socket))
    {
      native_error = ::WSAGetLastError();
      (void)::closesocket(socket);
      return invalid_socket;
    }
    native_error = 0;
    return static_cast<NativeSocket>(socket);
  }
  int bind_socket(NativeSocket socket, const NativeAddress &address) noexcept
  { return ::bind(native_socket(socket), reinterpret_cast<const sockaddr *>(address.storage.data()), address.length); }
  int listen_socket(NativeSocket socket, int backlog) noexcept { return ::listen(native_socket(socket), backlog); }
  int connect_socket(NativeSocket socket, const NativeAddress &address) noexcept
  { return ::connect(native_socket(socket), reinterpret_cast<const sockaddr *>(address.storage.data()), address.length); }
  int connection_error(NativeSocket socket, int &error) noexcept
  { int size = sizeof error; return ::getsockopt(native_socket(socket), SOL_SOCKET, SO_ERROR, reinterpret_cast<char *>(&error), &size); }
  vix::Result<NativeAddress> local_socket_address(NativeSocket socket) noexcept
  {
    NativeAddress out{};
    int length = static_cast<int>(out.storage.size());
    if (::getsockname(native_socket(socket), reinterpret_cast<sockaddr *>(out.storage.data()), &length) == SOCKET_ERROR)
      return connect_error(::WSAGetLastError(), "getsockname");
    out.length = length;
    return out;
  }
  std::ptrdiff_t receive_socket(NativeSocket socket, std::span<std::byte> buffer) noexcept
  {
    const int length = static_cast<int>(std::min<std::size_t>(buffer.size(), INT_MAX));
    return ::recv(native_socket(socket), reinterpret_cast<char *>(buffer.data()), length, 0);
  }
  std::ptrdiff_t send_socket(NativeSocket socket, std::span<const std::byte> buffer) noexcept
  {
    const int length = static_cast<int>(std::min<std::size_t>(buffer.size(), INT_MAX));
    return ::send(native_socket(socket), reinterpret_cast<const char *>(buffer.data()), length, 0);
  }
  int shutdown_socket_write(NativeSocket socket) noexcept { return ::shutdown(native_socket(socket), SD_SEND); }
  int last_socket_error() noexcept { return ::WSAGetLastError(); }
  bool socket_error_interrupted(int error) noexcept { return error == WSAEINTR; }
  bool socket_error_would_block(int error) noexcept { return error == WSAEWOULDBLOCK; }
  bool socket_error_accept_retry(int error) noexcept { return error == WSAEINTR || error == WSAECONNRESET; }
  bool socket_error_connect_pending(int error) noexcept { return error == WSAEWOULDBLOCK || error == WSAEINPROGRESS || error == WSAEALREADY; }
  bool socket_error_address_in_use(int error) noexcept { return error == WSAEADDRINUSE; }
  int socket_error_peer_closed_code() noexcept { return WSAESHUTDOWN; }
  int close_native_socket(NativeSocket socket) noexcept { return ::closesocket(native_socket(socket)); }
  bool parse_numeric_address(AddressFamily family, const char *text, std::uint8_t *bytes) noexcept
  {
    if (!winsock_call_begin()) return false;
    const bool parsed = ::InetPtonA(native_family(family), text, bytes) == 1;
    winsock_call_end();
    return parsed;
  }
  bool format_numeric_address(AddressFamily family, const std::uint8_t *bytes, char *text, std::size_t size) noexcept
  {
    if (!winsock_call_begin()) return false;
    const bool formatted = ::InetNtopA(native_family(family), const_cast<std::uint8_t *>(bytes), text, static_cast<DWORD>(size)) != nullptr;
    winsock_call_end();
    return formatted;
  }

  vix::Error transport_error(int error, const char *where) noexcept
  {
    auto identity = io_failure;
    switch (error)
    {
    case WSAECONNRESET: identity = connection_reset; break;
    case WSAECONNABORTED: identity = connection_aborted; break;
    case WSAESHUTDOWN: case WSAENOTCONN: identity = peer_closed; break;
    case WSAENETUNREACH: identity = network_unreachable; break;
    case WSAEHOSTUNREACH: identity = host_unreachable; break;
    case WSAETIMEDOUT: identity = timed_out; break;
    case WSAEACCES: identity = permission_denied; break;
    case WSAENOBUFS: case WSA_NOT_ENOUGH_MEMORY: identity = resource_exhausted; break;
    default: break;
    }
    return vix::Error{identity, where};
  }
  vix::Error connect_error(int error, const char *where) noexcept
  {
    auto identity = transport_error(error, where).identity();
    switch (error)
    {
    case WSAECONNREFUSED: identity = connection_refused; break;
    case WSAEADDRNOTAVAIL: identity = address_unavailable; break;
    case WSAEAFNOSUPPORT: case WSAEPROTONOSUPPORT: case WSAESOCKTNOSUPPORT: identity = unsupported; break;
    case WSAEMFILE: case WSAENOBUFS: identity = resource_exhausted; break;
    default: break;
    }
    return vix::Error{identity, where};
  }
}
