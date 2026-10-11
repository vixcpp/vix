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
   * @brief Point on Vix's process-local monotonic clock domain.
   *
   * Its numeric origin is intentionally opaque. MonotonicInstant values are
   * suitable for elapsed-time calculation and Deadline boundaries, but not
   * for persistence, logging timestamps, or comparison across processes.
   */
  class MonotonicInstant
  {
  public:
    constexpr MonotonicInstant() noexcept = default;

    [[nodiscard]] static constexpr MonotonicInstant min() noexcept
    {
      return MonotonicInstant(std::numeric_limits<std::int64_t>::min(), raw_tag{});
    }

    [[nodiscard]] static constexpr MonotonicInstant max() noexcept
    {
      return MonotonicInstant(std::numeric_limits<std::int64_t>::max(), raw_tag{});
    }

    [[nodiscard]] constexpr std::optional<MonotonicInstant>
    try_add(Duration duration) const noexcept
    {
      const auto result = detail::try_add_time(count_, duration.nanosecond_count());
      return result ? std::optional<MonotonicInstant>{MonotonicInstant(*result, raw_tag{})}
                    : std::nullopt;
    }

    [[nodiscard]] constexpr std::optional<MonotonicInstant>
    try_subtract(Duration duration) const noexcept
    {
      const auto result = detail::try_subtract_time(count_, duration.nanosecond_count());
      return result ? std::optional<MonotonicInstant>{MonotonicInstant(*result, raw_tag{})}
                    : std::nullopt;
    }

    [[nodiscard]] friend constexpr MonotonicInstant operator+(
        MonotonicInstant instant,
        Duration duration) noexcept
    {
      return MonotonicInstant(detail::require_time_value(
                                  detail::try_add_time(instant.count_, duration.nanosecond_count())),
                              raw_tag{});
    }

    [[nodiscard]] friend constexpr MonotonicInstant operator-(
        MonotonicInstant instant,
        Duration duration) noexcept
    {
      return MonotonicInstant(detail::require_time_value(
                                  detail::try_subtract_time(instant.count_, duration.nanosecond_count())),
                              raw_tag{});
    }

    [[nodiscard]] friend constexpr Duration operator-(MonotonicInstant left,
                                                        MonotonicInstant right) noexcept
    {
      return Duration::nanoseconds(detail::require_time_value(
          detail::try_subtract_time(left.count_, right.count_)));
    }

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
