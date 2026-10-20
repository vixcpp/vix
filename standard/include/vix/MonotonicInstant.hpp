/**
 * @file MonotonicInstant.hpp
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

#ifndef VIX_MONOTONIC_INSTANT_HPP
#define VIX_MONOTONIC_INSTANT_HPP

#include <compare>
#include <cstdint>
#include <optional>

#include <vix/Duration.hpp>
#include <vix/detail/TimeContract.hpp>

namespace vix
{
  /**
   * @brief Represents a point on Vix's monotonic clock.
   *
   * @details
   * MonotonicInstant represents a point in time within
   * the process-local monotonic clock domain.
   *
   * Unlike WallInstant, monotonic time is intended for
   * measuring elapsed time rather than representing
   * real-world dates and timestamps.
   *
   * Typical uses include:
   * - Measuring execution duration.
   * - Tracking elapsed time.
   * - Calculating operation time limits.
   * - Establishing Deadline boundaries.
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
   * @endcode
   *
   * @par Monotonic clock domain
   * The numeric origin of MonotonicInstant is intentionally
   * opaque and has no calendar meaning.
   *
   * Instants may be compared or subtracted when they belong
   * to the same process-local monotonic clock domain.
   *
   * Do not persist monotonic instants or compare values
   * obtained from different processes.
   *
   * @par Arithmetic safety
   * Use try_add() and try_subtract() when arithmetic
   * overflow must be handled.
   *
   * Ordinary arithmetic operators require representable
   * results. Invalid arithmetic terminates the process.
   *
   * @par Value semantics
   * MonotonicInstant is a lightweight value type backed
   * by a signed 64-bit nanosecond representation.
   *
   * It has no dynamic allocation or shared state.
   *
   * @note
   * A default-constructed MonotonicInstant has an internal
   * count of zero. It does not capture the current time.
   *
   * @warning
   * Do not use MonotonicInstant for logging timestamps,
   * persistent records, calendar dates, or comparisons
   * across processes.
   *
   * @see MonotonicClock
   * @see Duration
   * @see Deadline
   * @see WallInstant
   */
  class MonotonicInstant
  {
  public:
    /**
     * @brief Constructs a zero-valued monotonic instant.
     *
     * @details
     * Creates an instant whose internal representation
     * is zero.
     *
     * This does not read the monotonic clock.
     *
     * @note
     * Use MonotonicClock::now() to obtain the current
     * monotonic time.
     */
    constexpr MonotonicInstant() noexcept = default;

    /**
     * @brief Returns the smallest representable instant.
     *
     * @return A MonotonicInstant containing the minimum
     *         signed 64-bit nanosecond value.
     *
     * @note
     * This is a representation boundary, not a timestamp
     * corresponding to a real-world date.
     */
    [[nodiscard]] static constexpr MonotonicInstant min() noexcept
    {
      return MonotonicInstant(std::numeric_limits<std::int64_t>::min(), raw_tag{});
    }

    /**
     * @brief Returns the largest representable instant.
     *
     * @return A MonotonicInstant containing the maximum
     *         signed 64-bit nanosecond value.
     *
     * @note
     * This is a representation boundary, not a timestamp
     * corresponding to a real-world date.
     */
    [[nodiscard]] static constexpr MonotonicInstant max() noexcept
    {
      return MonotonicInstant(std::numeric_limits<std::int64_t>::max(), raw_tag{});
    }

    /**
     * @brief Safely advances an instant by a duration.
     *
     * @param duration Duration to add.
     *
     * @return The resulting instant, or std::nullopt
     *         if the result is unrepresentable.
     *
     * @details
     * A positive duration moves the instant forward.
     * A negative duration moves it backward.
     *
     * @par Example
     * @code{.cpp}
     * auto now = vix::MonotonicClock::now();
     *
     * auto later = now.try_add(vix::Duration::seconds(5));
     *
     * if (later)
     * {
     *     // The calculated instant is representable.
     * }
     * @endcode
     *
     * @see try_subtract()
     */
    [[nodiscard]] constexpr std::optional<MonotonicInstant>
    try_add(Duration duration) const noexcept
    {
      const auto result = detail::try_add_time(count_, duration.nanosecond_count());
      return result ? std::optional<MonotonicInstant>{MonotonicInstant(*result, raw_tag{})}
                    : std::nullopt;
    }

    /**
     * @brief Safely subtracts a duration from an instant.
     *
     * @param duration Duration to subtract.
     *
     * @return The resulting instant, or std::nullopt
     *         if the result is unrepresentable.
     *
     * @details
     * A positive duration moves the instant backward.
     * A negative duration moves it forward.
     *
     * @see try_add()
     */
    [[nodiscard]] constexpr std::optional<MonotonicInstant>
    try_subtract(Duration duration) const noexcept
    {
      const auto result = detail::try_subtract_time(count_, duration.nanosecond_count());
      return result ? std::optional<MonotonicInstant>{MonotonicInstant(*result, raw_tag{})}
                    : std::nullopt;
    }

    /**
     * @brief Advances a monotonic instant by a duration.
     *
     * @param instant Starting monotonic instant.
     * @param duration Duration to add.
     *
     * @return The resulting monotonic instant.
     *
     * @warning
     * Arithmetic overflow terminates the process.
     * Use try_add() when overflow must be handled.
     *
     * @par Example
     * @code{.cpp}
     * auto start = vix::MonotonicClock::now();
     * auto later = start + vix::Duration::seconds(10);
     * @endcode
     */
    [[nodiscard]] friend constexpr MonotonicInstant operator+(
        MonotonicInstant instant,
        Duration duration) noexcept
    {
      return MonotonicInstant(detail::require_time_value(
                                  detail::try_add_time(instant.count_, duration.nanosecond_count())),
                              raw_tag{});
    }

    /**
     * @brief Subtracts a duration from a monotonic instant.
     *
     * @param instant Starting monotonic instant.
     * @param duration Duration to subtract.
     *
     * @return The resulting monotonic instant.
     *
     * @warning
     * Arithmetic overflow terminates the process.
     * Use try_subtract() when overflow must be handled.
     */
    [[nodiscard]] friend constexpr MonotonicInstant operator-(
        MonotonicInstant instant,
        Duration duration) noexcept
    {
      return MonotonicInstant(detail::require_time_value(
                                  detail::try_subtract_time(instant.count_, duration.nanosecond_count())),
                              raw_tag{});
    }

    /**
     * @brief Calculates the duration between two monotonic instants.
     *
     * @param left First instant.
     * @param right Instant to subtract.
     *
     * @return The signed duration from right to left.
     *
     * @details
     * The result is:
     * - Positive when left is later than right.
     * - Negative when left is earlier than right.
     * - Zero when the instants are equal.
     *
     * @par Example
     * @code{.cpp}
     * auto start = vix::MonotonicClock::now();
     *
     * // Perform an operation.
     *
     * auto elapsed = vix::MonotonicClock::now() - start;
     * @endcode
     *
     * @pre
     * Both instants must belong to the same monotonic
     * clock domain.
     *
     * @warning
     * An unrepresentable duration terminates the process.
     */
    [[nodiscard]] friend constexpr Duration operator-(MonotonicInstant left,
                                                      MonotonicInstant right) noexcept
    {
      return Duration::nanoseconds(detail::require_time_value(
          detail::try_subtract_time(left.count_, right.count_)));
    }

    /**
     * @brief Compares two monotonic instants.
     *
     * @return Ordering of their internal time values.
     *
     * @details
     * Supports ==, !=, <, <=, > and >= comparisons.
     *
     * @pre
     * Both values must belong to the same monotonic
     * clock domain for a meaningful time comparison.
     *
     * @note
     * Comparison does not perform a system clock read.
     */
    [[nodiscard]] friend constexpr auto operator<=>(MonotonicInstant,
                                                    MonotonicInstant) noexcept = default;

  private:
    friend class MonotonicClock;

    struct raw_tag
    {
    };

    constexpr MonotonicInstant(std::int64_t count, raw_tag) noexcept
        : count_(count)
    {
    }

    std::int64_t count_{0};
  };
} // namespace vix

#endif // VIX_MONOTONIC_INSTANT_HPP
