#pragma once

#include <vix/Result.hpp>
#include "SocketPlatform.hpp"
#include <cstddef>
#include <cstdint>
#include <memory>

namespace vix::network::detail
{
  inline constexpr std::uint32_t reactor_readable = 1u << 0;
  inline constexpr std::uint32_t reactor_writable = 1u << 1;
  inline constexpr std::uint32_t reactor_error = 1u << 2;
  inline constexpr std::uint32_t reactor_hangup = 1u << 3;
  enum class ReactorControl : std::uint8_t { add, modify, remove };

  struct ReactorEvent
  {
    std::uint64_t token{}; // zero is reserved for the reactor wakeup
    std::uint32_t events{};
  };

  // Private readiness/wakeup boundary. The common provider owns dispatch,
  // operation state and deadlines; a selected platform source owns native
  // polling and wakeup details.
  class Reactor
  {
  public:
    static vix::Result<std::unique_ptr<Reactor>> create();
    ~Reactor();
    Reactor(const Reactor &) = delete;
    Reactor &operator=(const Reactor &) = delete;

    bool wake() noexcept;
    bool add(NativeSocket socket, std::uint64_t token, bool read, bool write) noexcept;
    bool modify(NativeSocket socket, std::uint64_t token, bool read, bool write) noexcept;
    bool remove(NativeSocket socket) noexcept;
    int wait(ReactorEvent *events, int capacity, int timeout_ms) noexcept;
    [[nodiscard]] std::uint64_t control_count(ReactorControl) const noexcept;
    [[nodiscard]] std::uint64_t wake_events() const noexcept;
    [[nodiscard]] std::uint64_t wake_writes() const noexcept;

  private:
    struct Impl;
    explicit Reactor(std::unique_ptr<Impl>) noexcept;
    std::unique_ptr<Impl> impl_;
  };
}
