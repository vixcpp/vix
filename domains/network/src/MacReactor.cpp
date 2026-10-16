#include "Reactor.hpp"
#include <vix/network/Errors.hpp>
#include <sys/event.h>
#include <sys/socket.h>
#include <unistd.h>
#include <array>
#include <atomic>
#include <cerrno>
#include <memory>
#include <time.h>

namespace vix::network::detail
{
  namespace
  {
    constexpr std::uintptr_t wake_ident = 1;
    void change(struct kevent &event, NativeSocket socket, std::uint64_t token,
                short filter, unsigned short flags) noexcept
    {
      EV_SET(&event, static_cast<uintptr_t>(socket), filter, flags, 0, 0,
             reinterpret_cast<void *>(static_cast<uintptr_t>(token)));
    }
  }

  struct Reactor::Impl
  {
    int poller{-1};
    std::atomic<std::uint64_t> adds{}, modifies{}, removes{}, wake_reads{}, wake_writes{};
    ~Impl() { if (poller >= 0) (void)::close(poller); }
  };

  Reactor::Reactor(std::unique_ptr<Impl> impl) noexcept : impl_(std::move(impl)) {}
  Reactor::~Reactor() = default;

  vix::Result<std::unique_ptr<Reactor>> Reactor::create()
  {
    auto impl = std::make_unique<Impl>();
    impl->poller = ::kqueue();
    if (impl->poller < 0)
      return vix::Error{vix::network::resource_exhausted, "cannot create kqueue network reactor"};
    struct kevent wake_event{};
    EV_SET(&wake_event, wake_ident, EVFILT_USER, EV_ADD | EV_CLEAR, 0, 0, nullptr);
    if (::kevent(impl->poller, &wake_event, 1, nullptr, 0, nullptr) != 0)
    {
      return vix::Error{vix::network::io_failure, "cannot register kqueue reactor wakeup"};
    }
    return std::unique_ptr<Reactor>(new Reactor(std::move(impl)));
  }

  bool Reactor::wake() noexcept
  {
    struct kevent trigger{};
    EV_SET(&trigger, wake_ident, EVFILT_USER, 0, NOTE_TRIGGER, 0, nullptr);
    int result;
    do
    {
      result = ::kevent(impl_->poller, &trigger, 1, nullptr, 0, nullptr);
    } while (result < 0 && errno == EINTR);
    if (result == 0)
    {
      ++impl_->wake_writes;
      return true;
    }
    return false;
  }

  bool Reactor::add(NativeSocket socket, std::uint64_t token, bool read, bool write) noexcept
  {
    struct kevent changes[2]{};
    int count{};
    if (read) change(changes[count++], socket, token, EVFILT_READ, EV_ADD | EV_ENABLE | EV_ONESHOT);
    if (write) change(changes[count++], socket, token, EVFILT_WRITE, EV_ADD | EV_ENABLE | EV_ONESHOT);
    if (!count || ::kevent(impl_->poller, changes, count, nullptr, 0, nullptr) != 0)
    {
      (void)remove(socket);
      return false;
    }
    ++impl_->adds;
    return true;
  }

  bool Reactor::modify(NativeSocket socket, std::uint64_t token, bool read, bool write) noexcept
  {
    if (!remove(socket) || !add(socket, token, read, write))
      return false;
    ++impl_->modifies;
    return true;
  }

  bool Reactor::remove(NativeSocket socket) noexcept
  {
    bool removed{};
    for (const short filter : std::array<short, 2>{EVFILT_READ, EVFILT_WRITE})
    {
      struct kevent event{};
      change(event, socket, 0, filter, EV_DELETE);
      if (::kevent(impl_->poller, &event, 1, nullptr, 0, nullptr) == 0)
        removed = true;
      else if (errno != ENOENT && errno != EBADF)
        return false;
    }
    if (removed)
      ++impl_->removes;
    return true;
  }

  int Reactor::wait(ReactorEvent *events, int capacity, int timeout_ms) noexcept
  {
    if (!events || capacity <= 0)
      return -1;
    struct kevent native[16]{};
    const int limit = capacity < 16 ? capacity : 16;
    struct timespec timeout{};
    struct timespec *timeout_pointer = nullptr;
    if (timeout_ms >= 0)
    {
      timeout.tv_sec = timeout_ms / 1000;
      timeout.tv_nsec = static_cast<long>(timeout_ms % 1000) * 1000000L;
      timeout_pointer = &timeout;
    }
    int count;
    do
    {
      count = ::kevent(impl_->poller, nullptr, 0, native, limit, timeout_pointer);
    } while (count < 0 && errno == EINTR);
    if (count < 0)
      return count;
    for (int i = 0; i < count; ++i)
    {
      if (native[i].filter == EVFILT_USER && native[i].ident == wake_ident)
      {
        ++impl_->wake_reads;
        events[i] = {0, 0};
        continue;
      }
      std::uint32_t translated{};
      if (native[i].filter == EVFILT_READ) translated |= reactor_readable;
      if (native[i].filter == EVFILT_WRITE) translated |= reactor_writable;
      if (native[i].flags & EV_ERROR) translated |= reactor_error;
      if (native[i].flags & EV_EOF) translated |= reactor_hangup;
      events[i] = {static_cast<std::uint64_t>(reinterpret_cast<uintptr_t>(native[i].udata)), translated};
    }
    return count;
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
