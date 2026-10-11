#ifndef VIX_DETAIL_TIME_CONTRACT_HPP
#define VIX_DETAIL_TIME_CONTRACT_HPP

#include <cstdint>
#include <exception>
#include <limits>
#include <optional>

namespace vix::detail
{
  // Time arithmetic contract violations are Faults, never operational Errors.
  // This private policy works in exception-disabled builds and is not a public
  // Fault mechanism.
  [[noreturn]] inline void time_contract_violation() noexcept
  {
    std::terminate();
  }

  [[nodiscard]] constexpr std::optional<std::int64_t>
  try_add_time(std::int64_t left, std::int64_t right) noexcept
  {
    if ((right > 0 && left > std::numeric_limits<std::int64_t>::max() - right) ||
        (right < 0 && left < std::numeric_limits<std::int64_t>::min() - right))
    {
      return std::nullopt;
    }

    return left + right;
  }

  [[nodiscard]] constexpr std::optional<std::int64_t>
  try_subtract_time(std::int64_t left, std::int64_t right) noexcept
  {
    if ((right < 0 && left > std::numeric_limits<std::int64_t>::max() + right) ||
        (right > 0 && left < std::numeric_limits<std::int64_t>::min() + right))
    {
      return std::nullopt;
    }

    return left - right;
  }

  [[nodiscard]] constexpr std::optional<std::int64_t>
  try_multiply_time(std::int64_t left, std::int64_t right) noexcept
  {
    if (left == 0 || right == 0)
    {
      return std::int64_t{0};
    }

    if ((left > 0 && right > 0 &&
         left > std::numeric_limits<std::int64_t>::max() / right) ||
        (left > 0 && right < 0 &&
         right < std::numeric_limits<std::int64_t>::min() / left) ||
        (left < 0 && right > 0 &&
         left < std::numeric_limits<std::int64_t>::min() / right) ||
        (left < 0 && right < 0 &&
         left < std::numeric_limits<std::int64_t>::max() / right))
    {
      return std::nullopt;
    }

    return left * right;
  }

  [[nodiscard]] constexpr std::optional<std::int64_t>
  try_divide_time(std::int64_t left, std::int64_t right) noexcept
  {
    if (right == 0 ||
        (left == std::numeric_limits<std::int64_t>::min() && right == -1))
    {
      return std::nullopt;
    }

    return left / right;
  }

  [[nodiscard]] constexpr std::int64_t
  require_time_value(std::optional<std::int64_t> value) noexcept
  {
    if (!value)
    {
      time_contract_violation();
    }

    return *value;
  }
} // namespace vix::detail

#endif // VIX_DETAIL_TIME_CONTRACT_HPP
