#include "SocketPlatform.hpp"
#include <vix/network/Errors.hpp>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <algorithm>
#include <climits>
#include <string>

#if defined(__ANDROID__) && (!defined(__ANDROID_API__) || __ANDROID_API__ < 21)
#error "The Vix Android Network provider requires NDK API level 21 or newer."
#endif

namespace vix::network::detail
{
  namespace
  {
    int native_family(AddressFamily family) noexcept { return family == AddressFamily::ipv4 ? AF_INET : AF_INET6; }
    bool configure_socket(int socket) noexcept
    {
      const int status = ::fcntl(socket, F_GETFL, 0);
#if defined(__wasi__)
      // wasi-libc Preview 2 uses socket descriptors only as its private libc
      // adapter. WASI has no child-process inheritance, so descriptor
      // close-on-exec flags are neither required nor part of this provider.
      return status >= 0 && ::fcntl(socket, F_SETFL, status | O_NONBLOCK) == 0;
#else
      const int descriptor = ::fcntl(socket, F_GETFD, 0);
      if (status < 0 || descriptor < 0 || ::fcntl(socket, F_SETFL, status | O_NONBLOCK) != 0 ||
          ::fcntl(socket, F_SETFD, descriptor | FD_CLOEXEC) != 0)
        return false;
#if defined(__APPLE__)
      const int enabled = 1;
      if (::setsockopt(socket, SOL_SOCKET, SO_NOSIGPIPE, &enabled, sizeof enabled) != 0)
        return false;
#endif
      return true;
#endif
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
    char text[INET6_ADDRSTRLEN]{};
    std::uint16_t port{};
    if (address.length >= static_cast<int>(sizeof(sockaddr_in)) &&
        reinterpret_cast<const sockaddr *>(address.storage.data())->sa_family == AF_INET)
    {
      const auto &native = *reinterpret_cast<const sockaddr_in *>(address.storage.data());
      if (!::inet_ntop(AF_INET, &native.sin_addr, text, sizeof text))
        return vix::Error{io_failure, "getsockname"};
      port = ntohs(native.sin_port);
    }
    else if (address.length >= static_cast<int>(sizeof(sockaddr_in6)) &&
             reinterpret_cast<const sockaddr *>(address.storage.data())->sa_family == AF_INET6)
    {
      const auto &native = *reinterpret_cast<const sockaddr_in6 *>(address.storage.data());
      if (!::inet_ntop(AF_INET6, &native.sin6_addr, text, sizeof text))
        return vix::Error{io_failure, "getsockname"};
      port = ntohs(native.sin6_port);
      if (native.sin6_scope_id)
      {
        const auto length = std::char_traits<char>::length(text);
        if (length + 12 < sizeof text)
        {
          text[length] = '%';
          auto scope = std::to_string(native.sin6_scope_id);
          std::copy(scope.begin(), scope.end(), text + length + 1);
        }
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
#if defined(__linux__)
    const int socket = ::socket(native_family(family), SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
#else
    const int socket = ::socket(native_family(family), SOCK_STREAM, 0);
#endif
    if (socket < 0) return connect_error(errno, "socket");
#if !defined(__linux__)
    if (!configure_socket(socket))
    {
      const int error = errno;
      (void)::close(socket);
      return connect_error(error, "configure socket");
    }
#endif
    return static_cast<NativeSocket>(socket);
  }

  NativeSocket accept_tcp_socket(NativeSocket listener, int &native_error) noexcept
  {
#if defined(__linux__)
    const int socket = ::accept4(static_cast<int>(listener), nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
#else
    const int socket = ::accept(static_cast<int>(listener), nullptr, nullptr);
#endif
    if (socket < 0)
    {
      native_error = errno;
      return invalid_socket;
    }
#if !defined(__linux__)
    if (!configure_socket(socket))
    {
      native_error = errno;
      (void)::close(socket);
      return invalid_socket;
    }
#endif
    native_error = 0;
    return static_cast<NativeSocket>(socket);
  }

  int bind_socket(NativeSocket socket, const NativeAddress &address) noexcept
  { return ::bind(static_cast<int>(socket), reinterpret_cast<const sockaddr *>(address.storage.data()), static_cast<socklen_t>(address.length)); }
  int listen_socket(NativeSocket socket, int backlog) noexcept { return ::listen(static_cast<int>(socket), backlog); }
  int connect_socket(NativeSocket socket, const NativeAddress &address) noexcept
  { return ::connect(static_cast<int>(socket), reinterpret_cast<const sockaddr *>(address.storage.data()), static_cast<socklen_t>(address.length)); }
  int connection_error(NativeSocket socket, int &error) noexcept
  { socklen_t size = sizeof error; return ::getsockopt(static_cast<int>(socket), SOL_SOCKET, SO_ERROR, &error, &size); }
  vix::Result<NativeAddress> local_socket_address(NativeSocket socket) noexcept
  {
    NativeAddress out{};
    socklen_t length = static_cast<socklen_t>(out.storage.size());
    if (::getsockname(static_cast<int>(socket), reinterpret_cast<sockaddr *>(out.storage.data()), &length) != 0)
      return connect_error(errno, "getsockname");
    out.length = static_cast<int>(length);
    return out;
  }
  std::ptrdiff_t receive_socket(NativeSocket socket, std::span<std::byte> buffer) noexcept
  { return ::recv(static_cast<int>(socket), reinterpret_cast<char *>(buffer.data()), buffer.size(), 0); }
  std::ptrdiff_t send_socket(NativeSocket socket, std::span<const std::byte> buffer) noexcept
  {
#if defined(MSG_NOSIGNAL)
    constexpr int flags = MSG_NOSIGNAL;
#else
    constexpr int flags = 0; // macOS sockets are configured with SO_NOSIGPIPE.
#endif
    return ::send(static_cast<int>(socket), reinterpret_cast<const char *>(buffer.data()), buffer.size(), flags);
  }
  int shutdown_socket_write(NativeSocket socket) noexcept { return ::shutdown(static_cast<int>(socket), SHUT_WR); }
  int last_socket_error() noexcept { return errno; }
  bool socket_error_interrupted(int error) noexcept { return error == EINTR; }
  bool socket_error_would_block(int error) noexcept { return error == EAGAIN || error == EWOULDBLOCK; }
  bool socket_error_accept_retry(int error) noexcept { return error == EINTR || error == ECONNABORTED; }
  bool socket_error_connect_pending(int error) noexcept { return error == EINPROGRESS || error == EWOULDBLOCK; }
  bool socket_error_address_in_use(int error) noexcept { return error == EADDRINUSE; }
  int socket_error_peer_closed_code() noexcept { return EPIPE; }
  int close_native_socket(NativeSocket socket) noexcept { return ::close(static_cast<int>(socket)); }
  bool parse_numeric_address(AddressFamily family, const char *text, std::uint8_t *bytes) noexcept { return ::inet_pton(native_family(family), text, bytes) == 1; }
  bool format_numeric_address(AddressFamily family, const std::uint8_t *bytes, char *text, std::size_t size) noexcept { return ::inet_ntop(native_family(family), bytes, text, static_cast<socklen_t>(size)) != nullptr; }

  vix::Error transport_error(int error, const char *where) noexcept
  {
    auto identity = io_failure;
    switch (error)
    {
    case ECONNRESET: identity = connection_reset; break;
    case ECONNABORTED: identity = connection_aborted; break;
    case EPIPE: case ENOTCONN: identity = peer_closed; break;
    case ENETUNREACH: identity = network_unreachable; break;
    case EHOSTUNREACH: identity = host_unreachable; break;
    case ETIMEDOUT: identity = timed_out; break;
    case EACCES: case EPERM: identity = permission_denied; break;
    case ENOMEM: case ENOBUFS: identity = resource_exhausted; break;
    default: break;
    }
    return vix::Error{identity, where};
  }
  vix::Error connect_error(int error, const char *where) noexcept
  {
    auto identity = transport_error(error, where).identity();
    switch (error)
    {
    case ECONNREFUSED: identity = connection_refused; break;
    case EADDRNOTAVAIL: identity = address_unavailable; break;
    case EAFNOSUPPORT: case EPROTONOSUPPORT: case ESOCKTNOSUPPORT: identity = unsupported; break;
    case EMFILE: case ENFILE: identity = resource_exhausted; break;
    default: break;
    }
    return vix::Error{identity, where};
  }
}
