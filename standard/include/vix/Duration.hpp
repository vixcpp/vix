/**
 * @file Duration.hpp
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

#ifndef VIX_DURATION_HPP
#define VIX_DURATION_HPP

#include <chrono>
#include <compare>
#include <cstdint>
#include <limits>
#include <optional>

#include <vix/detail/TimeContract.hpp>

namespace vix
{
  /**
   * @brief Represents a signed duration with nanosecond precision.
   *
   * @details
   * Duration represents elapsed time or the difference between
   * two points in time.
   *
   * It supports:
   * - Explicit construction from nanoseconds to days.
   * - Arithmetic with overflow checking.
   * - Safe arithmetic returning std::optional.
   * - Comparison between durations.
   * - Conversion to and from std::chrono::nanoseconds.
   *
   * Internally, Duration stores a signed 64-bit nanosecond count,
   * representing approximately -292 to +292 years.
   *
   * @par Example
   * @code{.cpp}
   * auto timeout = vix::Duration::seconds(5);
   * auto delay = vix::Duration::milliseconds(250);
   *
   * auto total = timeout + delay;
   *
   * if (auto extended = total.try_add(delay))
   * {
   *     // Use the calculated duration.
   * }
   * @endcode
   *
   * @par Arithmetic safety
   * Ordinary arithmetic operators enforce valid results.
   * Overflow, division by zero, or an unrepresentable result
   * terminates the process.
   *
   * Use try_add(), try_subtract(), try_multiply(), or
   * try_divide() when arithmetic failure must be handled.
   *
   * @par Negative durations
   * Negative durations are valid.
   * Individual domain APIs decide whether negative timeouts
   * or waiting periods are accepted.
   *
   * @par Value semantics
   * Duration is a lightweight value type with no dynamic
   * allocation or shared state.
   *
   * @note
   * Duration::days() represents fixed 24-hour periods,
   * not calendar days with timezone adjustments.
   */
  class Duration
  {
  public:
    /**
     * @brief Constructs a zero duration.
     */
    constexpr Duration() noexcept = default;

    /**
     * @brief Creates a duration from nanoseconds.
     *
     * @param count Signed number of nanoseconds.
     * @return The corresponding duration.
     */
    [[nodiscard]] static constexpr Duration nanoseconds(std::int64_t count) noexcept
    {
      return Duration(count, raw_tag{});
    }

    /**
     * @brief Creates a duration from microseconds.
     *
     * @param count Signed number of microseconds.
     * @return The corresponding duration.
     *
     * @warning Unrepresentable values terminate the process.
     */
    [[nodiscard]] static constexpr Duration microseconds(std::int64_t count) noexcept
    {
      return from_scaled(count, 1'000);
    }

    /**
     * @brief Creates a duration from milliseconds.
     *
     * @param count Signed number of milliseconds.
     * @return The corresponding duration.
     *
     * @warning Unrepresentable values terminate the process.
     */
    [[nodiscard]] static constexpr Duration milliseconds(std::int64_t count) noexcept
    {
      return from_scaled(count, 1'000'000);
    }

    /**
     * @brief Creates a duration from seconds.
     *
     * @param count Signed number of seconds.
     * @return The corresponding duration.
     *
     * @warning Unrepresentable values terminate the process.
     */
    [[nodiscard]] static constexpr Duration seconds(std::int64_t count) noexcept
    {
      return from_scaled(count, 1'000'000'000);
    }

    /**
     * @brief Creates a duration from minutes.
     *
     * @param count Signed number of minutes.
     * @return The corresponding duration.
     *
     * @warning Unrepresentable values terminate the process.
     */
    [[nodiscard]] static constexpr Duration minutes(std::int64_t count) noexcept
    {
      return from_scaled(count, 60'000'000'000LL);
    }

    /**
     * @brief Creates a duration from hours.
     *
     * @param count Signed number of hours.
     * @return The corresponding duration.
     *
     * @warning Unrepresentable values terminate the process.
     */
    [[nodiscard]] static constexpr Duration hours(std::int64_t count) noexcept
    {
      return from_scaled(count, 3'600'000'000'000LL);
    }

    /**
     * @brief Creates a duration from fixed 24-hour days.
     *
     * @param count Signed number of days.
     * @return The corresponding duration.
     *
     * @note A day represents exactly 86,400 seconds.
     * @warning Unrepresentable values terminate the process.
     */
    [[nodiscard]] static constexpr Duration days(std::int64_t count) noexcept
    {
      return from_scaled(count, 86'400'000'000'000LL);
    }

    /**
     * @brief Returns a duration of zero nanoseconds.
     *
     * @return Zero duration.
     */
    [[nodiscard]] static constexpr Duration zero() noexcept
    {
      return nanoseconds(0);
    }

    /**
     * @brief Returns the smallest representable duration.
     *
     * @return Minimum signed 64-bit nanosecond duration.
     */
    [[nodiscard]] static constexpr Duration min() noexcept
    {
      return nanoseconds(std::numeric_limits<std::int64_t>::min());
    }

    /**
     * @brief Returns the largest representable duration.
     *
     * @return Maximum signed 64-bit nanosecond duration.
     */
    [[nodiscard]] static constexpr Duration max() noexcept
    {
      return nanoseconds(std::numeric_limits<std::int64_t>::max());
    }

    /**
     * @brief Creates a Duration from std::chrono::nanoseconds.
     *
     * @param value Chrono duration to convert.
     * @return Equivalent Vix duration.
     *
     * @see to_chrono()
     */
    [[nodiscard]] static constexpr Duration from_chrono(
        std::chrono::nanoseconds value) noexcept
    {
      return nanoseconds(value.count());
    }

    /**
     * @brief Converts this duration to std::chrono::nanoseconds.
     *
     * @return Equivalent chrono duration.
     *
     * @see from_chrono()
     */
    [[nodiscard]] constexpr std::chrono::nanoseconds to_chrono() const noexcept
    {
      return std::chrono::nanoseconds(count_);
    }

    /**
     * @brief Returns the underlying nanosecond count.
     *
     * @return Signed number of nanoseconds.
     */
    [[nodiscard]] constexpr std::int64_t nanosecond_count() const noexcept
    {
      return count_;
    }

    /**
     * @brief Adds two durations with overflow checking.
     *
     * @param other Duration to add.
     * @return The sum, or std::nullopt on overflow.
     *
     * @par Example
     * @code{.cpp}
     * auto first = vix::Duration::seconds(5);
     * auto second = vix::Duration::seconds(3);
     *
     * auto result = first.try_add(second);
     *
     * if (result)
     * {
     *     // Addition succeeded.
     * }
     * @endcode
     */
    [[nodiscard]] constexpr std::optional<Duration> try_add(Duration other) const noexcept
    {
      const auto result = detail::try_add_time(count_, other.count_);
      return result ? std::optional<Duration>{nanoseconds(*result)} : std::nullopt;
    }

    /**
     * @brief Subtracts a duration with overflow checking.
     *
     * @param other Duration to subtract.
     * @return The difference, or std::nullopt on overflow.
     */
    [[nodiscard]] constexpr std::optional<Duration> try_subtract(Duration other) const noexcept
    {
      const auto result = detail::try_subtract_time(count_, other.count_);
      return result ? std::optional<Duration>{nanoseconds(*result)} : std::nullopt;
    }

    /**
     * @brief Multiplies a duration by an integer safely.
     *
     * @param factor Signed multiplication factor.
     * @return The product, or std::nullopt on overflow.
     */
    [[nodiscard]] constexpr std::optional<Duration> try_multiply(std::int64_t factor) const noexcept
    {
      const auto result = detail::try_multiply_time(count_, factor);
      return result ? std::optional<Duration>{nanoseconds(*result)} : std::nullopt;
    }

    /**
     * @brief Divides a duration by an integer safely.
     *
     * @param divisor Signed divisor.
     * @return The quotient, or std::nullopt if division is
     *         invalid or the result is unrepresentable.
     *
     * @note Integer division truncates toward zero.
     * @note Division by zero returns std::nullopt.
     */
    [[nodiscard]] constexpr std::optional<Duration> try_divide(std::int64_t divisor) const noexcept
    {
      const auto result = detail::try_divide_time(count_, divisor);
      return result ? std::optional<Duration>{nanoseconds(*result)} : std::nullopt;
    }

    /**
     * @brief Adds two durations.
     *
     * @param left First duration.
     * @param right Second duration.
     * @return Sum of both durations.
     *
     * @warning Arithmetic overflow terminates the process.
     * Use try_add() when overflow must be handled.
     */
    [[nodiscard]] friend constexpr Duration operator+(Duration left, Duration right) noexcept
    {
      return nanoseconds(detail::require_time_value(
          detail::try_add_time(left.count_, right.count_)));
    }

    /**
     * @brief Subtracts two durations.
     *
     * @param left First duration.
     * @param right Duration to subtract.
     * @return Difference between the durations.
     *
     * @warning Arithmetic overflow terminates the process.
     */
    [[nodiscard]] friend constexpr Duration operator-(Duration left, Duration right) noexcept
    {
      return nanoseconds(detail::require_time_value(
          detail::try_subtract_time(left.count_, right.count_)));
    }

    /**
     * @brief Negates a duration.
     *
     * @return Duration with the opposite sign.
     *
     * @warning Negating Duration::min() terminates the process.
     */
    [[nodiscard]] constexpr Duration operator-() const noexcept
    {
      return nanoseconds(detail::require_time_value(
          detail::try_subtract_time(0, count_)));
    }

    /**
     * @brief Multiplies a duration by an integer.
     *
     * @param value Duration to multiply.
     * @param factor Signed multiplication factor.
     * @return The multiplied duration.
     *
     * @warning Arithmetic overflow terminates the process.
     */
    [[nodiscard]] friend constexpr Duration operator*(Duration value, std::int64_t factor) noexcept
    {
      return nanoseconds(detail::require_time_value(
          detail::try_multiply_time(value.count_, factor)));
    }

    /**
     * @brief Multiplies an integer by a duration.
     *
     * @param factor Signed multiplication factor.
     * @param value Duration to multiply.
     * @return The multiplied duration.
     *
     * @warning Arithmetic overflow terminates the process.
     */
    [[nodiscard]] friend constexpr Duration operator*(std::int64_t factor, Duration value) noexcept
    {
      return value * factor;
    }

    /**
     * @brief Divides a duration by an integer.
     *
     * @param divisor Signed divisor.
     * @return The quotient duration.
     *
     * @note Integer division truncates toward zero.
     * @warning Division by zero or an unrepresentable
     * result terminates the process.
     */
    [[nodiscard]] constexpr Duration operator/(std::int64_t divisor) const noexcept
    {
      return nanoseconds(detail::require_time_value(
          detail::try_divide_time(count_, divisor)));
    }

    /**
     * @brief Adds another duration to this duration.
     *
     * @param other Duration to add.
     * @return Reference to this updated duration.
     *
     * @warning Arithmetic overflow terminates the process.
     */
    constexpr Duration &operator+=(Duration other) noexcept
    {
      count_ = (*this + other).count_;
      return *this;
    }

    /**
     * @brief Subtracts another duration from this duration.
     *
     * @param other Duration to subtract.
     * @return Reference to this updated duration.
     *
     * @warning Arithmetic overflow terminates the process.
     */
    constexpr Duration &operator-=(Duration other) noexcept
    {
      count_ = (*this - other).count_;
      return *this;
    }

    /**
     * @brief Multiplies this duration by an integer.
     *
     * @param factor Signed multiplication factor.
     * @return Reference to this updated duration.
     *
     * @warning Arithmetic overflow terminates the process.
     */
    constexpr Duration &operator*=(std::int64_t factor) noexcept
    {
      count_ = (*this * factor).count_;
      return *this;
    }

    /**
     * @brief Divides this duration by an integer.
     *
     * @param divisor Signed divisor.
     * @return Reference to this updated duration.
     *
     * @warning Invalid division terminates the process.
     */
    constexpr Duration &operator/=(std::int64_t divisor) noexcept
    {
      count_ = (*this / divisor).count_;
      return *this;
    }

    /**
     * @brief Compares two durations by their nanosecond counts.
     *
     * @return Ordering of the two durations.
     *
     * Supports ==, !=, <, <=, > and >= comparisons.
     */
    [[nodiscard]] friend constexpr auto operator<=>(Duration,
                                                    Duration) noexcept = default;

  private:
    struct raw_tag
    {
    };

    constexpr Duration(std::int64_t count, raw_tag) noexcept
        : count_(count)
    {
    }

    [[nodiscard]] static constexpr Duration from_scaled(std::int64_t count,
                                                        std::int64_t scale) noexcept
    {
      return nanoseconds(detail::require_time_value(
          detail::try_multiply_time(count, scale)));
    }

    std::int64_t count_{0};
  };
} // namespace vix

#endif // VIX_DURATION_HPP
