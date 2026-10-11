#ifndef VIX_WALL_CLOCK_HPP
#define VIX_WALL_CLOCK_HPP

#include <chrono>

#include <vix/WallInstant.hpp>

namespace vix
{
  /**
   * @brief Source of adjustable civil/system-clock instants.
   *
   * now() is thread-safe and allocation-free. Consecutive observations may
   * move in either direction when the platform adjusts civil time.
   */
  class WallClock
  {
  public:
    WallClock() = delete;

    [[nodiscard]] static WallInstant now() noexcept
    {
      const auto current = std::chrono::system_clock::now();
      const auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(
          current.time_since_epoch());
      return WallInstant::from_unix_nanoseconds(nanos.count());
    }
  };
} // namespace vix

#endif // VIX_WALL_CLOCK_HPP
