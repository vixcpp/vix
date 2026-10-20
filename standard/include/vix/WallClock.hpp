/**
 * @file WallClock.hpp
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

#ifndef VIX_WALL_CLOCK_HPP
#define VIX_WALL_CLOCK_HPP

#include <chrono>

#include <vix/WallInstant.hpp>

namespace vix
{
  /**
   * @brief Provides access to the current system clock.
   *
   * @details
   * WallClock provides the current real-world time as
   * a WallInstant.
   *
   * It is suitable for:
   * - Recording timestamps in application logs.
   * - Tracking creation and modification times.
   * - Storing timestamps in databases.
   * - Recording when events occur.
   * - Displaying time to users.
   *
   * @par Example
   * @code{.cpp}
   * auto now = vix::WallClock::now();
   *
   * auto timestamp = now.unix_nanosecond_count();
   *
   * // Store or log the timestamp.
   * @endcode
   *
   * @par Clock behavior
   * WallClock uses std::chrono::system_clock.
   *
   * Its time is based on the system's civil clock
   * and may be adjusted by the operating system,
   * time synchronization services, or the user.
   *
   * Consecutive observations may move forward
   * or backward.
   *
   * @par Thread safety
   * now() is thread-safe and performs no dynamic allocation.
   *
   * @par Precision
   * Timestamps are represented in nanoseconds.
   *
   * Actual clock resolution and accuracy depend
   * on the operating system and hardware.
   *
   * @warning
   * Do not use WallClock to measure elapsed execution
   * time, implement operation timeouts, or enforce
   * deadlines.
   *
   * Use MonotonicClock for elapsed-time measurements
   * and Deadline for operational time limits.
   *
   * @see WallInstant
   * @see MonotonicClock
   * @see MonotonicInstant
   * @see Deadline
   */
  class WallClock
  {
  public:
    /**
     * @brief Construction is not supported.
     *
     * WallClock exposes static clock operations.
     * Use now() without creating an instance.
     */
    WallClock() = delete;

    /**
     * @brief Returns the current system time.
     *
     * @return A WallInstant representing the current
     *         system-clock time as Unix-epoch nanoseconds.
     *
     * @details
     * Reads std::chrono::system_clock and converts
     * the observed timestamp to nanoseconds.
     *
     * The returned WallInstant can be used for
     * logging, persistence, and chronological comparisons.
     *
     * @par Example
     * @code{.cpp}
     * auto created_at = vix::WallClock::now();
     *
     * auto unix_ns = created_at.unix_nanosecond_count();
     *
     * auto chrono_time = created_at.to_chrono();
     * @endcode
     *
     * @note
     * This function is thread-safe, allocation-free,
     * and does not modify the system clock.
     *
     * @note
     * Two consecutive calls may return equal timestamps.
     * A later call may also return an earlier timestamp
     * if the system clock is adjusted.
     *
     * @warning
     * Nanosecond representation does not guarantee
     * nanosecond clock accuracy or resolution.
     *
     * @see WallInstant
     * @see MonotonicClock::now()
     */
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
