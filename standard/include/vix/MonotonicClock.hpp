#ifndef VIX_MONOTONIC_CLOCK_HPP
#define VIX_MONOTONIC_CLOCK_HPP

#include <chrono>

#include <vix/MonotonicInstant.hpp>

namespace vix
{
  /**
   * @brief Source of non-decreasing process-local elapsed-time instants.
   *
   * now() is thread-safe and allocation-free. It provides ordering suitable
   * for elapsed time and Deadline semantics, subject to platform resolution;
   * it does not imply timer precision or hard real-time scheduling.
   */
  class MonotonicClock
  {
  public:
    MonotonicClock() = delete;

    [[nodiscard]] static MonotonicInstant now() noexcept
    {
      static_assert(std::chrono::steady_clock::is_steady,
                    "Vix MonotonicClock requires std::chrono::steady_clock.");

      const auto current = std::chrono::steady_clock::now();
      const auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(
          current.time_since_epoch());
      return MonotonicInstant(nanos.count(), MonotonicInstant::raw_tag{});
    }
  };
} // namespace vix

#endif // VIX_MONOTONIC_CLOCK_HPP
