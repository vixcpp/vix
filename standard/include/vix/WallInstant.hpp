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
   * @brief Civil-time point represented as signed Unix-epoch nanoseconds.
   *
   * WallInstant is appropriate for logging, persistence, and civil-time
   * domain contracts. It is intentionally not monotonic and must not be used
   * for elapsed operational limits or Deadline enforcement.
   */
  class WallInstant
  {
  public:
    constexpr WallInstant() noexcept = default;

    [[nodiscard]] static constexpr WallInstant from_unix_nanoseconds(
        std::int64_t count) noexcept
    {
      return WallInstant(count, raw_tag{});
    }

    [[nodiscard]] static constexpr WallInstant from_chrono(
        std::chrono::sys_time<std::chrono::nanoseconds> value) noexcept
    {
      return from_unix_nanoseconds(value.time_since_epoch().count());
    }

    [[nodiscard]] constexpr std::int64_t unix_nanosecond_count() const noexcept
    {
      return count_;
    }

    [[nodiscard]] constexpr std::chrono::sys_time<std::chrono::nanoseconds>
    to_chrono() const noexcept
    {
      return std::chrono::sys_time<std::chrono::nanoseconds>{
          std::chrono::nanoseconds{count_}};
    }

    [[nodiscard]] constexpr std::optional<WallInstant> try_add(Duration duration) const noexcept
    {
      const auto result = detail::try_add_time(count_, duration.nanosecond_count());
      return result ? std::optional<WallInstant>{from_unix_nanoseconds(*result)} : std::nullopt;
    }

    [[nodiscard]] constexpr std::optional<WallInstant> try_subtract(Duration duration) const noexcept
    {
      const auto result = detail::try_subtract_time(count_, duration.nanosecond_count());
      return result ? std::optional<WallInstant>{from_unix_nanoseconds(*result)} : std::nullopt;
    }

    [[nodiscard]] friend constexpr WallInstant operator+(WallInstant instant,
                                                           Duration duration) noexcept
    {
      return from_unix_nanoseconds(detail::require_time_value(
          detail::try_add_time(instant.count_, duration.nanosecond_count())));
    }

    [[nodiscard]] friend constexpr WallInstant operator-(WallInstant instant,
                                                           Duration duration) noexcept
    {
      return from_unix_nanoseconds(detail::require_time_value(
          detail::try_subtract_time(instant.count_, duration.nanosecond_count())));
    }

    [[nodiscard]] friend constexpr Duration operator-(WallInstant left,
                                                        WallInstant right) noexcept
    {
      return Duration::nanoseconds(detail::require_time_value(
          detail::try_subtract_time(left.count_, right.count_)));
    }

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
