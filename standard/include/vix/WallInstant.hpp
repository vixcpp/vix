/**
 * @file WallInstant.hpp
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

#ifndef VIX_WALL_INSTANT_HPP
#define VIX_WALL_INSTANT_HPP

#include <chrono>
#include <compare>
#include <cstdint>
#include <optional>

#include <vix/Duration.hpp>
#include <vix/detail/TimeContract.hpp>

namespace vix
{
  /**
   * @brief Represents a point in wall-clock time.
   *
   * @details
   * WallInstant represents an absolute point on the Unix
   * timeline using signed nanoseconds since January 1, 1970,
   * at 00:00:00 UTC.
   *
   * It is suitable for:
   * - Recording timestamps in logs.
   * - Storing creation and modification times.
   * - Persisting timestamps in databases.
   * - Comparing real-world points in time.
   *
   * WallInstant supports arithmetic with Duration,
   * chronological comparisons, and interoperability
   * with std::chrono::sys_time.
   *
   * @par Example
   * @code{.cpp}
   * auto created = vix::WallInstant::from_unix_nanoseconds(0);
   *
   * auto updated = created + vix::Duration::seconds(10);
   *
   * auto elapsed = updated - created;
   *
   * if (updated > created)
   * {
   *     // The updated timestamp is later.
   * }
   * @endcode
   *
   * @par Arithmetic safety
   * Use try_add() and try_subtract() when an arithmetic
   * overflow must be handled.
   *
   * Ordinary arithmetic operators enforce representable
   * results. Unrepresentable results terminate the process.
   *
   * @par Wall-clock behavior
   * Wall-clock time is not monotonic.
   *
   * System clock adjustments, synchronization, or manual
   * changes can cause observed timestamps to move backward
   * or forward.
   *
   * Do not use WallInstant for measuring elapsed execution
   * time or enforcing operational deadlines.
   *
   * @par Representation
   * WallInstant stores a signed 64-bit nanosecond count.
   *
   * Negative values represent instants before the Unix epoch.
   * A default-constructed WallInstant represents the epoch.
   *
   * WallInstant does not store a timezone or calendar format.
   *
   * @note
   * For elapsed-time measurements and deadlines, prefer
   * MonotonicInstant and Deadline.
   *
   * @see Duration
   * @see MonotonicInstant
   * @see Deadline
   */
  class WallInstant
  {
  public:
    /**
     * @brief Constructs an instant at the Unix epoch.
     *
     * The initial timestamp is zero nanoseconds since
     * January 1, 1970, at 00:00:00 UTC.
     */
    constexpr WallInstant() noexcept = default;

    /**
     * @brief Creates an instant from a Unix timestamp.
     *
     * @param count Signed nanoseconds since the Unix epoch.
     *
     * @return The corresponding WallInstant.
     *
     * @note
     * Negative values represent instants before the epoch.
     *
     * @par Example
     * @code{.cpp}
     * auto epoch = vix::WallInstant::from_unix_nanoseconds(0);
     * @endcode
     */
    [[nodiscard]] static constexpr WallInstant from_unix_nanoseconds(
        std::int64_t count) noexcept
    {
      return WallInstant(count, raw_tag{});
    }

    /**
     * @brief Creates an instant from a chrono system timestamp.
     *
     * @param value A std::chrono::sys_time value
     *              with nanosecond resolution.
     *
     * @return The equivalent WallInstant.
     *
     * @see to_chrono()
     */
    [[nodiscard]] static constexpr WallInstant from_chrono(
        std::chrono::sys_time<std::chrono::nanoseconds> value) noexcept
    {
      return from_unix_nanoseconds(value.time_since_epoch().count());
    }

    /**
     * @brief Returns the Unix timestamp in nanoseconds.
     *
     * @return Signed nanoseconds since the Unix epoch.
     *
     * @note
     * The result may be negative for instants before
     * January 1, 1970.
     */
    [[nodiscard]] constexpr std::int64_t unix_nanosecond_count() const noexcept
    {
      return count_;
    }

    /**
     * @brief Converts this instant to a chrono system timestamp.
     *
     * @return The equivalent std::chrono::sys_time
     *         with nanosecond resolution.
     *
     * @par Example
     * @code{.cpp}
     * auto instant = vix::WallInstant::from_unix_nanoseconds(0);
     *
     * auto timestamp = instant.to_chrono();
     * @endcode
     *
     * @see from_chrono()
     */
    [[nodiscard]] constexpr std::chrono::sys_time<std::chrono::nanoseconds>
    to_chrono() const noexcept
    {
      return std::chrono::sys_time<std::chrono::nanoseconds>{
          std::chrono::nanoseconds{count_}};
    }

    /**
     * @brief Safely advances an instant by a duration.
     *
     * @param duration The duration to add.
     *
     * @return The resulting WallInstant, or std::nullopt
     *         if the timestamp would be unrepresentable.
     *
     * @par Example
     * @code{.cpp}
     * auto start = vix::WallInstant::from_unix_nanoseconds(0);
     *
     * auto later = start.try_add(vix::Duration::hours(2));
     *
     * if (later)
     * {
     *     // The new timestamp is valid.
     * }
     * @endcode
     *
     * @see try_subtract()
     */
    [[nodiscard]] constexpr std::optional<WallInstant> try_add(Duration duration) const noexcept
    {
      const auto result = detail::try_add_time(count_, duration.nanosecond_count());
      return result ? std::optional<WallInstant>{from_unix_nanoseconds(*result)} : std::nullopt;
    }

    /**
     * @brief Safely moves an instant backward by a duration.
     *
     * @param duration The duration to subtract.
     *
     * @return The resulting WallInstant, or std::nullopt
     *         if the timestamp would be unrepresentable.
     *
     * @note
     * Subtracting a negative duration moves the timestamp
     * forward, when the result is representable.
     *
     * @see try_add()
     */
    [[nodiscard]] constexpr std::optional<WallInstant> try_subtract(Duration duration) const noexcept
    {
      const auto result = detail::try_subtract_time(count_, duration.nanosecond_count());
      return result ? std::optional<WallInstant>{from_unix_nanoseconds(*result)} : std::nullopt;
    }

    /**
     * @brief Advances an instant by a duration.
     *
     * @param instant The starting instant.
     * @param duration The duration to add.
     *
     * @return The resulting WallInstant.
     *
     * @warning
     * Arithmetic overflow terminates the process.
     * Use try_add() when overflow must be handled.
     */
    [[nodiscard]] friend constexpr WallInstant operator+(WallInstant instant,
                                                         Duration duration) noexcept
    {
      return from_unix_nanoseconds(detail::require_time_value(
          detail::try_add_time(instant.count_, duration.nanosecond_count())));
    }

    /**
     * @brief Subtracts a duration from an instant.
     *
     * @param instant The starting instant.
     * @param duration The duration to subtract.
     *
     * @return The resulting WallInstant.
     *
     * @warning
     * Arithmetic overflow terminates the process.
     * Use try_subtract() when overflow must be handled.
     */
    [[nodiscard]] friend constexpr WallInstant operator-(WallInstant instant,
                                                         Duration duration) noexcept
    {
      return from_unix_nanoseconds(detail::require_time_value(
          detail::try_subtract_time(instant.count_, duration.nanosecond_count())));
    }

    /**
     * @brief Calculates the duration between two instants.
     *
     * @param left The first instant.
     * @param right The instant to subtract.
     *
     * @return The signed duration from right to left.
     *
     * @details
     * The result is positive when left is later than right,
     * negative when left is earlier, and zero when equal.
     *
     * @par Example
     * @code{.cpp}
     * auto start = vix::WallInstant::from_unix_nanoseconds(0);
     * auto end = start + vix::Duration::seconds(5);
     *
     * auto difference = end - start;
     * @endcode
     *
     * @warning
     * An unrepresentable duration terminates the process.
     *
     * @note
     * Wall-clock differences can be affected by system
     * clock adjustments. Use monotonic time to measure
     * actual elapsed execution time.
     */
    [[nodiscard]] friend constexpr Duration operator-(WallInstant left,
                                                      WallInstant right) noexcept
    {
      return Duration::nanoseconds(detail::require_time_value(
          detail::try_subtract_time(left.count_, right.count_)));
    }

    /**
     * @brief Compares two wall-clock instants.
     *
     * @return The chronological ordering of the instants.
     *
     * @details
     * Supports ==, !=, <, <=, > and >= comparisons.
     *
     * An instant with a larger Unix timestamp represents
     * a later point in time.
     */
    [[nodiscard]] friend constexpr auto operator<=>(WallInstant,
                                                    WallInstant) noexcept = default;

  private:
    struct raw_tag
    {
    };

    constexpr WallInstant(std::int64_t count, raw_tag) noexcept
        : count_(count)
    {
    }

    std::int64_t count_{0};
  };
} // namespace vix

#endif // VIX_WALL_INSTANT_HPP
