#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include "Reactor.hpp"
#include <vix/network/Errors.hpp>
#include <algorithm>
#include <array>
#include <atomic>
#include <memory>
#include <unordered_map>
#include <vector>

namespace vix::network::detail
{
  struct Reactor::Impl
  {
    struct Registration { std::uint64_t token{}; bool read{}, write{}; };
    SOCKET wake_receiver{INVALID_SOCKET};
    SOCKET wake_sender{INVALID_SOCKET};
    bool winsock_started{};
    std::unordered_map<NativeSocket, Registration> registrations;
    std::size_t cursor{};
    std::atomic<std::uint64_t> adds{}, modifies{}, removes{}, wake_reads{}, wake_writes{};

    ~Impl()
    {
      if (wake_sender != INVALID_SOCKET) (void)::closesocket(wake_sender);
      if (wake_receiver != INVALID_SOCKET) (void)::closesocket(wake_receiver);
      if (winsock_started) (void)::WSACleanup();
    }
  };

  Reactor::Reactor(std::unique_ptr<Impl> impl) noexcept : impl_(std::move(impl)) {}
  Reactor::~Reactor() = default;

  vix::Result<std::unique_ptr<Reactor>> Reactor::create()
  {
    auto impl = std::make_unique<Impl>();
    WSADATA data{};
    const int startup = ::WSAStartup(MAKEWORD(2, 2), &data);
    if (startup != 0)
      return vix::Error{vix::network::resource_exhausted, "cannot initialize Winsock"};
    impl->winsock_started = true;

    impl->wake_receiver = ::WSASocketW(AF_INET, SOCK_DGRAM, IPPROTO_UDP, nullptr, 0, WSA_FLAG_OVERLAPPED);
    impl->wake_sender = ::WSASocketW(AF_INET, SOCK_DGRAM, IPPROTO_UDP, nullptr, 0, WSA_FLAG_OVERLAPPED);
    if (impl->wake_receiver == INVALID_SOCKET || impl->wake_sender == INVALID_SOCKET)
      return vix::Error{vix::network::resource_exhausted, "cannot create Winsock reactor wake sockets"};
    u_long nonblocking = 1;
    if (::ioctlsocket(impl->wake_receiver, FIONBIO, &nonblocking) != 0 ||
        ::ioctlsocket(impl->wake_sender, FIONBIO, &nonblocking) != 0)
      return vix::Error{vix::network::io_failure, "cannot configure Winsock reactor wake sockets"};

    sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (::bind(impl->wake_receiver, reinterpret_cast<const sockaddr *>(&local), sizeof local) == SOCKET_ERROR)
      return vix::Error{vix::network::io_failure, "cannot bind Winsock reactor wake socket"};
    int length = sizeof local;
    if (::getsockname(impl->wake_receiver, reinterpret_cast<sockaddr *>(&local), &length) == SOCKET_ERROR ||
        ::connect(impl->wake_sender, reinterpret_cast<const sockaddr *>(&local), length) == SOCKET_ERROR)
      return vix::Error{vix::network::io_failure, "cannot connect Winsock reactor wake sockets"};
    return std::unique_ptr<Reactor>(new Reactor(std::move(impl)));
  }

  bool Reactor::wake() noexcept
  {
    constexpr char byte = 1;
    for (;;)
    {
      const int sent = ::send(impl_->wake_sender, &byte, 1, 0);
      if (sent == 1 || (sent == SOCKET_ERROR && ::WSAGetLastError() == WSAEWOULDBLOCK))
      {
        ++impl_->wake_writes;
        return true;
      }
      if (sent == SOCKET_ERROR && ::WSAGetLastError() == WSAEINTR)
        continue;
      return false;
    }
  }

  bool Reactor::add(NativeSocket socket, std::uint64_t token, bool read, bool write) noexcept
  {
    if (socket == invalid_socket) return false;
    const auto [it, inserted] = impl_->registrations.emplace(socket, Impl::Registration{token, read, write});
    (void)it;
    if (inserted) ++impl_->adds;
    return inserted;
  }
  bool Reactor::modify(NativeSocket socket, std::uint64_t token, bool read, bool write) noexcept
  {
    const auto found = impl_->registrations.find(socket);
    if (found == impl_->registrations.end()) return false;
    found->second = {token, read, write};
    ++impl_->modifies;
    return true;
  }
  bool Reactor::remove(NativeSocket socket) noexcept
  {
    if (impl_->registrations.erase(socket) == 0) return false;
    ++impl_->removes;
    return true;
  }

  int Reactor::wait(ReactorEvent *events, int capacity, int timeout_ms) noexcept
  {
    if (!events || capacity <= 0) return -1;
    std::vector<WSAPOLLFD> pollfds;
    std::vector<std::uint64_t> tokens;
    try
    {
      pollfds.reserve(impl_->registrations.size() + 1);
      tokens.reserve(impl_->registrations.size());
      pollfds.push_back({impl_->wake_receiver, POLLRDNORM, 0});
      for (const auto &[socket, registration] : impl_->registrations)
      {
        short interest{};
        if (registration.read) interest |= POLLRDNORM;
        if (registration.write) interest |= POLLWRNORM;
        pollfds.push_back({static_cast<SOCKET>(socket), interest, 0});
        tokens.push_back(registration.token);
      }
    }
    catch (...)
    {
      return -1;
    }

    const int ready = ::WSAPoll(pollfds.data(), static_cast<ULONG>(pollfds.size()), timeout_ms);
    if (ready == SOCKET_ERROR) return -1;
    int output{};
    if (pollfds[0].revents)
    {
      std::array<char, 128> bytes{};
      while (::recv(impl_->wake_receiver, bytes.data(), static_cast<int>(bytes.size()), 0) > 0) {}
      ++impl_->wake_reads;
      events[output++] = {0, 0};
    }

    const std::size_t count = tokens.size();
    if (!count) return output;
    const std::size_t start = impl_->cursor++ % count;
    for (std::size_t offset = 0; offset < count && output < capacity; ++offset)
    {
      const std::size_t index = (start + offset) % count;
      const auto &native = pollfds[index + 1];
      if (!native.revents) continue;
      std::uint32_t translated{};
      if (native.revents & POLLRDNORM) translated |= reactor_readable;
      if (native.revents & POLLWRNORM) translated |= reactor_writable;
      if (native.revents & (POLLERR | POLLNVAL)) translated |= reactor_error;
      if (native.revents & POLLHUP) translated |= reactor_hangup;
      events[output++] = {tokens[index], translated};
    }
    return output;
  }

  std::uint64_t Reactor::control_count(ReactorControl operation) const noexcept
  {
    switch (operation)
    {
    case ReactorControl::add: return impl_->adds.load();
    case ReactorControl::modify: return impl_->modifies.load();
    case ReactorControl::remove: return impl_->removes.load();
    }
    return 0;
  }
  std::uint64_t Reactor::wake_events() const noexcept { return impl_->wake_reads.load(); }
  std::uint64_t Reactor::wake_writes() const noexcept { return impl_->wake_writes.load(); }
}
