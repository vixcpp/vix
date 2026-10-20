/**
 * @file MonotonicClock.hpp
 * @author Gaspard Kirira
 *
 * Copyright 2026, Gaspard Kirira.
 * All rights reserved.
 * https://github.com/vixcpp/vix
 *
 * Use of this source code is governed by an MIT license
 * that can be found in the LICENSE file.
 *
 * Vix.cpp
 */

#ifndef VIX_MONOTONIC_CLOCK_HPP
#define VIX_MONOTONIC_CLOCK_HPP

#include <chrono>

#include <vix/MonotonicInstant.hpp>

namespace vix
{
  /**
   * @brief Provides a monotonic clock for measuring elapsed time.
   *
   * @details
   * MonotonicClock provides access to a process-local clock
   * that does not move backward.
   *
   * Unlike a wall clock, it is not affected by adjustments
   * to the system's civil time.
   *
   * Use MonotonicClock when you need to:
   * - Measure how long an operation takes.
   * - Calculate elapsed execution time.
   * - Establish operation time limits.
   * - Create monotonic deadline boundaries.
   *
   * @par Example
   * @code{.cpp}
   * auto start = vix::MonotonicClock::now();
   *
   * // Perform an operation.
   *
   * auto end = vix::MonotonicClock::now();
   *
   * vix::Duration elapsed = end - start;
   *
   * auto nanoseconds = elapsed.nanosecond_count();
   * @endcode
   *
   * @par Clock behavior
   * MonotonicClock uses std::chrono::steady_clock.
   *
   * Successive clock observations are non-decreasing,
   * subject to the platform's clock resolution.
   *
   * The clock's origin is unspecified and has no
   * relationship to the Unix epoch.
   *
   * @par Thread safety
   * now() is thread-safe and performs no dynamic allocation.
   *
   * @note
   * A nanosecond representation does not guarantee
   * nanosecond measurement accuracy or resolution.
   *
   * @warning
   * MonotonicClock does not guarantee precise timer
   * scheduling or hard real-time execution.
   *
   * Do not use its instants as calendar timestamps,
   * persistent timestamps, or values to compare
   * across processes.
   *
   * @see MonotonicInstant
   * @see Duration
   * @see Deadline
   * @see WallClock
   */
  class MonotonicClock
  {
  public:
    /**
     * @brief Construction is not supported.
     *
     * MonotonicClock is a utility class providing
     * static clock operations.
     *
     * Use now() without creating an instance.
     */
    MonotonicClock() = delete;

    /**
     * @brief Returns the current monotonic time.
     *
     * @return A MonotonicInstant representing the
     *         current process-local monotonic time.
     *
     * @details
     * Reads std::chrono::steady_clock and converts
     * its current time to a nanosecond representation.
     *
     * The returned instant can be compared with other
     * instants from the same monotonic clock domain.
     *
     * @par Example
     * @code{.cpp}
     * auto started = vix::MonotonicClock::now();
     *
     * // Perform an operation.
     *
     * auto elapsed =
     *     vix::MonotonicClock::now() - started;
     * @endcode
     *
     * @note
     * This function is thread-safe, allocation-free,
     * and does not modify the system clock.
     *
     * @note
     * Two consecutive calls may return equal values
     * depending on the platform's clock resolution.
     *
     * @see MonotonicInstant
     */
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
