#include "Reactor.hpp"
#include <vix/network/Errors.hpp>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <cerrno>
#include <atomic>
#include <memory>

namespace vix::network::detail
{
  struct Reactor::Impl
  {
    int poller{-1};
    int wake_fd{-1};
    std::atomic<std::uint64_t> adds{}, modifies{}, removes{}, wake_reads{}, wake_writes{};
    ~Impl()
    {
      if (wake_fd >= 0) (void)::close(wake_fd);
      if (poller >= 0) (void)::close(poller);
    }
  };

  Reactor::Reactor(std::unique_ptr<Impl> impl) noexcept : impl_(std::move(impl)) {}
  Reactor::~Reactor() = default;

  vix::Result<std::unique_ptr<Reactor>> Reactor::create()
  {
    auto impl = std::make_unique<Impl>();
    impl->poller = ::epoll_create1(EPOLL_CLOEXEC);
    impl->wake_fd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (impl->poller < 0 || impl->wake_fd < 0)
    {
      return vix::Error{vix::network::resource_exhausted, "cannot create Linux network reactor"};
    }
    epoll_event event{};
    event.events = EPOLLIN;
    event.data.u64 = 0;
    if (::epoll_ctl(impl->poller, EPOLL_CTL_ADD, impl->wake_fd, &event) != 0)
    {
      return vix::Error{vix::network::io_failure, "cannot register Linux reactor wakeup"};
    }
    return std::unique_ptr<Reactor>(new Reactor(std::move(impl)));
  }

  bool Reactor::wake() noexcept
  {
    const std::uint64_t one = 1;
    for (;;)
    {
      const auto written = ::write(impl_->wake_fd, &one, sizeof one);
      if (written == sizeof one)
      {
        ++impl_->wake_writes;
        return true;
      }
      if (written < 0 && errno == EINTR)
        continue;
      return written < 0 && errno == EAGAIN;
    }
  }

  bool Reactor::add(NativeSocket socket, std::uint64_t token, bool read, bool write) noexcept
  {
    epoll_event event{};
    event.events = EPOLLONESHOT | EPOLLERR | EPOLLHUP;
#ifdef EPOLLRDHUP
    event.events |= EPOLLRDHUP;
#endif
    if (read) event.events |= EPOLLIN;
    if (write) event.events |= EPOLLOUT;
    event.data.u64 = token;
    if (::epoll_ctl(impl_->poller, EPOLL_CTL_ADD, static_cast<int>(socket), &event) != 0)
      return false;
    ++impl_->adds;
    return true;
  }

  bool Reactor::modify(NativeSocket socket, std::uint64_t token, bool read, bool write) noexcept
  {
    epoll_event event{};
    event.events = EPOLLONESHOT | EPOLLERR | EPOLLHUP;
#ifdef EPOLLRDHUP
    event.events |= EPOLLRDHUP;
#endif
    if (read) event.events |= EPOLLIN;
    if (write) event.events |= EPOLLOUT;
    event.data.u64 = token;
    if (::epoll_ctl(impl_->poller, EPOLL_CTL_MOD, static_cast<int>(socket), &event) != 0)
      return false;
    ++impl_->modifies;
    return true;
  }

  bool Reactor::remove(NativeSocket socket) noexcept
  {
    if (::epoll_ctl(impl_->poller, EPOLL_CTL_DEL, static_cast<int>(socket), nullptr) != 0)
      return false;
    ++impl_->removes;
    return true;
  }

  int Reactor::wait(ReactorEvent *events, int capacity, int timeout_ms) noexcept
  {
    if (!events || capacity <= 0)
      return -1;
    epoll_event native[16]{};
    const int limit = capacity < 16 ? capacity : 16;
    int count;
    do
    {
      count = ::epoll_wait(impl_->poller, native, limit, timeout_ms);
    } while (count < 0 && errno == EINTR);
    if (count < 0)
      return count;
    int output{};
    for (int i = 0; i < count; ++i)
    {
      if (native[i].data.u64 == 0)
      {
        std::uint64_t value{};
        for (;;)
        {
          const auto got = ::read(impl_->wake_fd, &value, sizeof value);
          if (got == sizeof value) continue;
          if (got < 0 && errno == EINTR) continue;
          if (got < 0 && errno == EAGAIN) break;
          return -1;
        }
        ++impl_->wake_reads;
        events[output++] = {0, 0};
        continue;
      }
      std::uint32_t translated{};
      if (native[i].events & EPOLLIN) translated |= reactor_readable;
      if (native[i].events & EPOLLOUT) translated |= reactor_writable;
      if (native[i].events & EPOLLERR) translated |= reactor_error;
      if (native[i].events & (EPOLLHUP
#ifdef EPOLLRDHUP
                              | EPOLLRDHUP
#endif
                              )) translated |= reactor_hangup;
      events[output++] = {native[i].data.u64, translated};
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
