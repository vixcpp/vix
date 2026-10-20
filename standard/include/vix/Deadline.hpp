/**
 * @file Deadline.hpp
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

#ifndef VIX_DEADLINE_HPP
#define VIX_DEADLINE_HPP

#include <compare>
#include <optional>

#include <vix/MonotonicClock.hpp>

namespace vix
{
  /**
   * @brief Represents a deadline based on monotonic time.
   *
   * @details
   * Deadline defines a point in monotonic time by which
   * an operation should complete.
   *
   * It is useful for:
   * - Limiting operation execution time.
   * - Defining time boundaries for network operations.
   * - Managing bounded resource lifetimes.
   * - Propagating time constraints to child operations.
   *
   * A Deadline contains a MonotonicInstant representing
   * its temporal boundary.
   *
   * @par Example
   * @code{.cpp}
   * auto deadline = vix::Deadline::after(
   *     vix::Duration::seconds(5));
   *
   * if (deadline.reached())
   * {
   *     // The deadline has been reached.
   * }
   *
   * auto remaining = deadline.remaining();
   * @endcode
   *
   * @par Monotonic time
   * Deadline uses MonotonicClock rather than WallClock.
   *
   * Changes to the system's civil time do not affect
   * its monotonic boundary.
   *
   * @par Optional deadlines
   * An operation without a deadline should use
   * std::optional<Deadline> containing std::nullopt.
   *
   * Do not represent an unlimited deadline using
   * an artificially distant timestamp.
   *
   * @par Combining deadlines
   * Use earliest() to combine optional constraints.
   *
   * When both deadlines exist, the earlier boundary
   * takes precedence.
   *
   * @par Cancellation and completion
   * Deadline only represents a temporal constraint.
   *
   * It does not automatically stop an operation,
   * trigger cancellation, or produce a Completion.
   *
   * Operation providers are responsible for enforcing
   * deadlines and resolving their terminal outcomes.
   *
   * @note
   * Negative durations are accepted by the arithmetic.
   * They can produce deadlines before the starting instant.
   *
   * @see MonotonicInstant
   * @see MonotonicClock
   * @see Duration
   * @see Completion
   */
  class Deadline
  {
  public:
    /**
     * @brief Default construction is not supported.
     *
     * A Deadline must have an explicit temporal boundary.
     *
     * Use at() or after() to create one.
     */
    Deadline() = delete;

    /**
     * @brief Creates a deadline at a specific monotonic instant.
     *
     * @param boundary The monotonic time boundary.
     *
     * @return A Deadline at the specified instant.
     *
     * @par Example
     * @code{.cpp}
     * auto now = vix::MonotonicClock::now();
     *
     * auto deadline = vix::Deadline::at(now);
     * @endcode
     *
     * @note
     * The boundary may already have been reached.
     *
     * @see boundary()
     */
    [[nodiscard]] static constexpr Deadline at(MonotonicInstant boundary) noexcept
    {
      return Deadline(boundary);
    }

    /**
     * @brief Safely creates a deadline relative to an instant.
     *
     * @param start Starting monotonic instant.
     * @param duration Duration added to the starting instant.
     *
     * @return The resulting Deadline, or std::nullopt
     *         if the boundary is unrepresentable.
     *
     * @details
     * This function allows applications to handle
     * arithmetic overflow without terminating.
     *
     * @par Example
     * @code{.cpp}
     * auto start = vix::MonotonicClock::now();
     *
     * auto deadline = vix::Deadline::try_after(
     *     start,
     *     vix::Duration::seconds(10));
     *
     * if (deadline)
     * {
     *     // A valid deadline was created.
     * }
     * @endcode
     *
     * @note
     * Negative durations are permitted.
     *
     * @see after()
     */
    [[nodiscard]] static constexpr std::optional<Deadline>
    try_after(MonotonicInstant start, Duration duration) noexcept
    {
      const auto boundary = start.try_add(duration);
      return boundary ? std::optional<Deadline>{Deadline(*boundary)} : std::nullopt;
    }

    /**
     * @brief Creates a deadline relative to a monotonic instant.
     *
     * @param start Starting monotonic instant.
     * @param duration Duration to add.
     *
     * @return The resulting Deadline.
     *
     * @par Example
     * @code{.cpp}
     * auto start = vix::MonotonicClock::now();
     *
     * auto deadline = vix::Deadline::after(
     *     start,
     *     vix::Duration::seconds(5));
     * @endcode
     *
     * @warning
     * An unrepresentable boundary violates the time
     * contract and terminates the process.
     *
     * Use try_after() when overflow must be handled.
     *
     * @see try_after()
     */
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

    /**
     * @brief Creates a deadline relative to the current time.
     *
     * @param duration Duration from the current monotonic time.
     *
     * @return A Deadline with the calculated boundary.
     *
     * @details
     * Captures the current time using MonotonicClock::now()
     * and adds the specified duration.
     *
     * @par Example
     * @code{.cpp}
     * auto deadline = vix::Deadline::after(
     *     vix::Duration::milliseconds(500));
     * @endcode
     *
     * @note
     * A zero or negative duration creates a boundary
     * at or before the captured starting instant.
     *
     * @warning
     * An unrepresentable boundary terminates the process.
     *
     * @see MonotonicClock::now()
     */
    [[nodiscard]] static Deadline after(Duration duration) noexcept
    {
      return after(MonotonicClock::now(), duration);
    }

    /**
     * @brief Returns the deadline's monotonic boundary.
     *
     * @return The MonotonicInstant defining the deadline.
     *
     * @note
     * This function does not read the current clock.
     */
    [[nodiscard]] constexpr MonotonicInstant boundary() const noexcept
    {
      return boundary_;
    }

    /**
     * @brief Checks whether a deadline has been reached.
     *
     * @param now Monotonic instant used for evaluation.
     *
     * @return true if now is at or beyond the boundary,
     *         false otherwise.
     *
     * @details
     * The caller supplies the observation time.
     * This is useful when several deadlines must be
     * evaluated against the same clock observation.
     *
     * @par Example
     * @code{.cpp}
     * auto now = vix::MonotonicClock::now();
     *
     * if (deadline.reached(now))
     * {
     *     // The deadline has expired.
     * }
     * @endcode
     */
    [[nodiscard]] constexpr bool reached(MonotonicInstant now) const noexcept
    {
      return now >= boundary_;
    }

    /**
     * @brief Checks whether the deadline has been reached now.
     *
     * @return true if the current monotonic time is at
     *         or beyond the boundary, false otherwise.
     *
     * @details
     * Uses MonotonicClock::now() to obtain the
     * current observation time.
     *
     * @par Example
     * @code{.cpp}
     * if (deadline.reached())
     * {
     *     // The deadline has expired.
     * }
     * @endcode
     */
    [[nodiscard]] bool reached() const noexcept
    {
      return reached(MonotonicClock::now());
    }

    /**
     * @brief Calculates the time remaining until the deadline.
     *
     * @param now Monotonic instant used for evaluation.
     *
     * @return The remaining Duration, or zero if the
     *         deadline has already been reached.
     *
     * @details
     * If now is before the boundary, the remaining
     * duration is calculated as boundary - now.
     *
     * Once the deadline is reached, this function
     * returns Duration::zero() instead of a
     * negative duration.
     *
     * @par Example
     * @code{.cpp}
     * auto now = vix::MonotonicClock::now();
     *
     * auto remaining = deadline.remaining(now);
     * @endcode
     *
     * @warning
     * An unrepresentable duration violates the time
     * contract and terminates the process.
     */
    [[nodiscard]] constexpr Duration remaining(MonotonicInstant now) const noexcept
    {
      if (reached(now))
      {
        return Duration::zero();
      }

      return boundary_ - now;
    }

    /**
     * @brief Returns the time remaining from the current instant.
     *
     * @return A non-negative Duration until the boundary,
     *         or zero when the deadline has been reached.
     *
     * @details
     * Reads MonotonicClock::now() and calculates the
     * remaining time.
     *
     * @par Example
     * @code{.cpp}
     * auto deadline = vix::Deadline::after(
     *     vix::Duration::seconds(10));
     *
     * auto remaining = deadline.remaining();
     * @endcode
     *
     * @warning
     * An unrepresentable remaining duration terminates
     * the process.
     */
    [[nodiscard]] Duration remaining() const noexcept
    {
      return remaining(MonotonicClock::now());
    }

    /**
     * @brief Returns the earlier of two deadlines.
     *
     * @param left First deadline.
     * @param right Second deadline.
     *
     * @return The deadline with the earlier boundary.
     *
     * @par Example
     * @code{.cpp}
     * auto first = vix::Deadline::after(
     *     vix::Duration::seconds(5));
     *
     * auto second = vix::Deadline::after(
     *     vix::Duration::seconds(10));
     *
     * auto effective = vix::Deadline::earlier(
     *     first, second);
     * @endcode
     *
     * @note
     * When both boundaries are equal, left is returned.
     *
     * @see earliest()
     */
    [[nodiscard]] static constexpr Deadline earlier(Deadline left,
                                                    Deadline right) noexcept
    {
      return left.boundary_ <= right.boundary_ ? left : right;
    }

    /**
     * @brief Combines two optional deadline constraints.
     *
     * @param left First optional deadline.
     * @param right Second optional deadline.
     *
     * @return The earliest available deadline, or
     *         std::nullopt if neither constraint exists.
     *
     * @details
     * An absent deadline means that no temporal
     * constraint is imposed by that value.
     *
     * Combination follows these rules:
     * - Both present: return the earlier deadline.
     * - One present: return the existing deadline.
     * - Neither present: return std::nullopt.
     *
     * @par Example
     * @code{.cpp}
     * std::optional<vix::Deadline> parent =
     *     vix::Deadline::after(
     *         vix::Duration::seconds(10));
     *
     * std::optional<vix::Deadline> child =
     *     vix::Deadline::after(
     *         vix::Duration::seconds(3));
     *
     * auto effective = vix::Deadline::earliest(
     *     parent, child);
     * @endcode
     *
     * @par Child operations
     * Use earliest() to combine a parent deadline
     * with a more specific child-operation deadline.
     *
     * This prevents the child from extending the
     * parent's temporal constraint.
     *
     * @see earlier()
     */
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

    /**
     * @brief Compares two deadline boundaries.
     *
     * @return Ordering of their monotonic boundaries.
     *
     * @details
     * Supports ==, !=, <, <=, > and >= comparisons.
     *
     * An earlier deadline compares less than a later one.
     */
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
