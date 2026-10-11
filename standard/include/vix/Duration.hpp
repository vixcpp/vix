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
   * @brief Signed elapsed time measured in nanoseconds.
   *
   * Duration is a value type with one-nanosecond representation resolution and
   * a representable range of approximately +/-292 years. Negative values are
   * valid differences, but a domain API accepting a wait or timeout decides
   * whether it accepts them. Duration has no allocation and no shared state.
   */
  class Duration
  {
  public:
    constexpr Duration() noexcept = default;

    [[nodiscard]] static constexpr Duration nanoseconds(std::int64_t count) noexcept
    {
      return Duration(count, raw_tag{});
    }

    [[nodiscard]] static constexpr Duration microseconds(std::int64_t count) noexcept
    {
      return from_scaled(count, 1'000);
    }

    [[nodiscard]] static constexpr Duration milliseconds(std::int64_t count) noexcept
    {
      return from_scaled(count, 1'000'000);
    }

    [[nodiscard]] static constexpr Duration seconds(std::int64_t count) noexcept
    {
      return from_scaled(count, 1'000'000'000);
    }

    [[nodiscard]] static constexpr Duration minutes(std::int64_t count) noexcept
    {
      return from_scaled(count, 60'000'000'000LL);
    }

    [[nodiscard]] static constexpr Duration hours(std::int64_t count) noexcept
    {
      return from_scaled(count, 3'600'000'000'000LL);
    }

    [[nodiscard]] static constexpr Duration days(std::int64_t count) noexcept
    {
      return from_scaled(count, 86'400'000'000'000LL);
    }

    [[nodiscard]] static constexpr Duration zero() noexcept
    {
      return nanoseconds(0);
    }

    [[nodiscard]] static constexpr Duration min() noexcept
    {
      return nanoseconds(std::numeric_limits<std::int64_t>::min());
    }

    [[nodiscard]] static constexpr Duration max() noexcept
    {
      return nanoseconds(std::numeric_limits<std::int64_t>::max());
    }

    [[nodiscard]] static constexpr Duration from_chrono(
        std::chrono::nanoseconds value) noexcept
    {
      return nanoseconds(value.count());
    }

    [[nodiscard]] constexpr std::chrono::nanoseconds to_chrono() const noexcept
    {
      return std::chrono::nanoseconds(count_);
    }

    [[nodiscard]] constexpr std::int64_t nanosecond_count() const noexcept
    {
      return count_;
    }

    [[nodiscard]] constexpr std::optional<Duration> try_add(Duration other) const noexcept
    {
      const auto result = detail::try_add_time(count_, other.count_);
      return result ? std::optional<Duration>{nanoseconds(*result)} : std::nullopt;
    }

    [[nodiscard]] constexpr std::optional<Duration> try_subtract(Duration other) const noexcept
    {
      const auto result = detail::try_subtract_time(count_, other.count_);
      return result ? std::optional<Duration>{nanoseconds(*result)} : std::nullopt;
    }

    [[nodiscard]] constexpr std::optional<Duration> try_multiply(std::int64_t factor) const noexcept
    {
      const auto result = detail::try_multiply_time(count_, factor);
      return result ? std::optional<Duration>{nanoseconds(*result)} : std::nullopt;
    }

    [[nodiscard]] constexpr std::optional<Duration> try_divide(std::int64_t divisor) const noexcept
    {
      const auto result = detail::try_divide_time(count_, divisor);
      return result ? std::optional<Duration>{nanoseconds(*result)} : std::nullopt;
    }

    [[nodiscard]] friend constexpr Duration operator+(Duration left, Duration right) noexcept
    {
      return nanoseconds(detail::require_time_value(
          detail::try_add_time(left.count_, right.count_)));
    }

    [[nodiscard]] friend constexpr Duration operator-(Duration left, Duration right) noexcept
    {
      return nanoseconds(detail::require_time_value(
          detail::try_subtract_time(left.count_, right.count_)));
    }

    [[nodiscard]] constexpr Duration operator-() const noexcept
    {
      return nanoseconds(detail::require_time_value(
          detail::try_subtract_time(0, count_)));
    }

    [[nodiscard]] friend constexpr Duration operator*(Duration value, std::int64_t factor) noexcept
    {
      return nanoseconds(detail::require_time_value(
          detail::try_multiply_time(value.count_, factor)));
    }

    [[nodiscard]] friend constexpr Duration operator*(std::int64_t factor, Duration value) noexcept
    {
      return value * factor;
    }

    [[nodiscard]] constexpr Duration operator/(std::int64_t divisor) const noexcept
    {
      return nanoseconds(detail::require_time_value(
          detail::try_divide_time(count_, divisor)));
    }

    constexpr Duration &operator+=(Duration other) noexcept
    {
      count_ = (*this + other).count_;
      return *this;
    }

    constexpr Duration &operator-=(Duration other) noexcept
    {
      count_ = (*this - other).count_;
      return *this;
    }

    constexpr Duration &operator*=(std::int64_t factor) noexcept
    {
      count_ = (*this * factor).count_;
      return *this;
    }

    constexpr Duration &operator/=(std::int64_t divisor) noexcept
    {
      count_ = (*this / divisor).count_;
      return *this;
    }

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
