#ifndef VIX_DEADLINE_HPP
#define VIX_DEADLINE_HPP

#include <compare>
#include <optional>

#include <vix/MonotonicClock.hpp>

namespace vix
{
  /**
   * @brief Monotonic temporal boundary for an operation or bounded lifetime.
   *
   * Deadline does not encode timeout completion or cancellation. Absence of a
   * constraint is represented by std::optional<Deadline>, never by a magic
   * far-future value. Child work composes constraints with earliest().
   */
  class Deadline
  {
  public:
    Deadline() = delete;

    [[nodiscard]] static constexpr Deadline at(MonotonicInstant boundary) noexcept
    {
      return Deadline(boundary);
    }

    [[nodiscard]] static constexpr std::optional<Deadline>
    try_after(MonotonicInstant start, Duration duration) noexcept
    {
      const auto boundary = start.try_add(duration);
      return boundary ? std::optional<Deadline>{Deadline(*boundary)} : std::nullopt;
    }

    [[nodiscard]] static constexpr Deadline after(MonotonicInstant start,
                                                  Duration duration) noexcept
    {
      const auto deadline = try_after(start, duration);
      if (!deadline)
      {
        detail::time_contract_violation();
      }

      return *deadline;
    }

    [[nodiscard]] static Deadline after(Duration duration) noexcept
    {
      return after(MonotonicClock::now(), duration);
    }

    [[nodiscard]] constexpr MonotonicInstant boundary() const noexcept
    {
      return boundary_;
    }

    [[nodiscard]] constexpr bool reached(MonotonicInstant now) const noexcept
    {
      return now >= boundary_;
    }

    [[nodiscard]] bool reached() const noexcept
    {
      return reached(MonotonicClock::now());
    }

    [[nodiscard]] constexpr Duration remaining(MonotonicInstant now) const noexcept
    {
      if (reached(now))
      {
        return Duration::zero();
      }

      return boundary_ - now;
    }

    [[nodiscard]] Duration remaining() const noexcept
    {
      return remaining(MonotonicClock::now());
    }

    [[nodiscard]] static constexpr Deadline earlier(Deadline left,
                                                    Deadline right) noexcept
    {
      return left.boundary_ <= right.boundary_ ? left : right;
    }

    [[nodiscard]] static constexpr std::optional<Deadline>
    earliest(std::optional<Deadline> left,
             std::optional<Deadline> right) noexcept
    {
      if (!left)
      {
        return right;
      }
      if (!right)
      {
        return left;
      }

      return earlier(*left, *right);
    }

    [[nodiscard]] friend constexpr auto operator<=>(Deadline,
                                                    Deadline) noexcept = default;

  private:
    constexpr explicit Deadline(MonotonicInstant boundary) noexcept
        : boundary_(boundary)
    {
    }

    MonotonicInstant boundary_;
  };
} // namespace vix

#endif // VIX_DEADLINE_HPP
