#include "Reactor.hpp"
#include <vix/network/Errors.hpp>
#include <algorithm>
#include <cerrno>
#include <climits>
#include <limits>
#include <new>
#include <poll.h>
#include <unordered_map>
#include <vector>

namespace vix::network::detail
{
  struct Reactor::Impl
  {
    struct Registration
    {
      std::uint64_t token{};
      bool read{};
      bool write{};
    };
    std::unordered_map<NativeSocket, Registration> registrations;
    std::uint64_t adds{}, modifies{}, removes{};
    std::size_t cursor{};
  };

  Reactor::Reactor(std::unique_ptr<Impl> impl) noexcept : impl_(std::move(impl)) {}
  Reactor::~Reactor() = default;

  vix::Result<std::unique_ptr<Reactor>> Reactor::create()
  {
    std::unique_ptr<Impl> impl{new (std::nothrow) Impl};
    if (!impl)
      return vix::Error{vix::network::resource_exhausted, "cannot allocate WASI network reactor"};
    std::unique_ptr<Reactor> reactor{new (std::nothrow) Reactor(std::move(impl))};
    if (!reactor)
      return vix::Error{vix::network::resource_exhausted, "cannot allocate WASI network reactor"};
    return reactor;
  }

  // Cooperative providers have no sleeping reactor thread. Commands are
  // synchronized in ProviderCore and consumed by its next caller-owned poll.
  bool Reactor::wake() noexcept { return true; }

  bool Reactor::add(NativeSocket socket, std::uint64_t token, bool read, bool write) noexcept
  {
    if (socket > static_cast<NativeSocket>(INT_MAX) || (!read && !write))
      return false;
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
    try
    {
#endif
      if (!impl_->registrations.emplace(socket, Impl::Registration{token, read, write}).second)
        return false;
      ++impl_->adds;
      return true;
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
    }
    catch (...)
    {
      return false;
    }
#endif
  }

  bool Reactor::modify(NativeSocket socket, std::uint64_t token, bool read, bool write) noexcept
  {
    auto it = impl_->registrations.find(socket);
    if (it == impl_->registrations.end())
      return false;
    if (!read && !write)
    {
      impl_->registrations.erase(it);
      ++impl_->removes;
      return true;
    }
    it->second = {token, read, write};
    ++impl_->modifies;
    return true;
  }

  bool Reactor::remove(NativeSocket socket) noexcept
  {
    if (impl_->registrations.erase(socket) == 0)
      return false;
    ++impl_->removes;
    return true;
  }

  int Reactor::wait(ReactorEvent *events, int capacity, int timeout_ms) noexcept
  {
    // Provider::poll is deliberately a nonblocking operation. A positive or
    // infinite timeout would violate its public cooperative progress contract.
    if (!events || capacity <= 0 || timeout_ms != 0)
      return -1;
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
    try
    {
#endif
      std::vector<pollfd> descriptors;
      std::vector<std::uint64_t> tokens;
      descriptors.reserve(impl_->registrations.size());
      tokens.reserve(impl_->registrations.size());
      for (const auto &[socket, registration] : impl_->registrations)
      {
        pollfd descriptor{};
        descriptor.fd = static_cast<int>(socket);
        if (registration.read) descriptor.events = static_cast<short>(descriptor.events | POLLIN);
        if (registration.write) descriptor.events = static_cast<short>(descriptor.events | POLLOUT);
        descriptors.push_back(descriptor);
        tokens.push_back(registration.token);
      }
      if (descriptors.empty())
        return 0;
      const auto start = impl_->cursor % descriptors.size();
      if (start != 0)
      {
        std::rotate(descriptors.begin(), descriptors.begin() + static_cast<std::ptrdiff_t>(start), descriptors.end());
        std::rotate(tokens.begin(), tokens.begin() + static_cast<std::ptrdiff_t>(start), tokens.end());
      }
      int ready;
      if (descriptors.size() > static_cast<std::size_t>(std::numeric_limits<nfds_t>::max()))
        return -1;
      do
      {
        ready = ::poll(descriptors.data(), static_cast<nfds_t>(descriptors.size()), 0);
      } while (ready < 0 && errno == EINTR);
      if (ready < 0)
        return -1;
      int output{};
      std::size_t last_ready{};
      for (std::size_t i = 0; i < descriptors.size() && output < capacity; ++i)
      {
        const short flags = descriptors[i].revents;
        if (!flags)
          continue;
        std::uint32_t translated{};
        if (flags & (POLLIN | POLLPRI)) translated |= reactor_readable;
        if (flags & POLLOUT) translated |= reactor_writable;
        if (flags & (POLLERR | POLLNVAL)) translated |= reactor_error;
        if (flags & POLLHUP) translated |= reactor_hangup;
        events[output++] = {tokens[i], translated};
        last_ready = i;
      }
      if (output)
        impl_->cursor = (start + last_ready + 1) % descriptors.size();
      return output;
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
    }
    catch (...)
    {
      return -1;
    }
#endif
  }

  std::uint64_t Reactor::control_count(ReactorControl operation) const noexcept
  {
    switch (operation)
    {
    case ReactorControl::add: return impl_->adds;
    case ReactorControl::modify: return impl_->modifies;
    case ReactorControl::remove: return impl_->removes;
    }
    return 0;
  }
  std::uint64_t Reactor::wake_events() const noexcept { return 0; }
  std::uint64_t Reactor::wake_writes() const noexcept { return 0; }
}
