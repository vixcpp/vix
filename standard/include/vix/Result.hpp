#ifndef VIX_RESULT_HPP
#define VIX_RESULT_HPP

#include <exception>
#include <type_traits>
#include <utility>
#include <variant>

#include <vix/Error.hpp>

namespace vix::detail
{
  // Invalid Result observation is a Fault, not an operational failure. This
  // private policy deliberately works without language exceptions and is not
  // a stable public Fault mechanism.
  [[noreturn]] inline void result_contract_violation() noexcept
  {
    std::terminate();
  }
} // namespace vix::detail

namespace vix
{
  /**
   * @brief Success value or expected operational failure.
   *
   * Result<T> represents exactly one of Success(T) and Failure(Error). It is
   * neither an asynchronous abstraction nor a representation of cancellation,
   * pending work, timeout, or programming Fault. Result values have no shared
   * mutable state; concurrent const observation is safe, while concurrent
   * movement or destruction of the same object is not.
   * Construction of T may throw according to T's own contract. A Vix Error's
   * controlled diagnostic allocation is process-fatal on exhaustion. A
   * throwing construction produces no Result object; no observable Result can
   * enter a third or valueless state.
   */
  template <typename T>
  class Result
  {
    static_assert(!std::is_void_v<T>,
                  "Use the Result<void> specialization for no-value success.");
    static_assert(!std::is_reference_v<T>,
                  "Result<T> does not support reference types.");
    static_assert(!std::is_const_v<T> && !std::is_volatile_v<T>,
                  "Result<T> stores an ordinary unqualified value type.");
    static_assert(!std::is_array_v<T>,
                  "Result<T> does not support array types.");
    static_assert(!std::is_same_v<T, Error>,
                  "Result<Error> is not supported; Error represents failure.");

  public:
    Result() = delete;

    // Implicit construction supports ordinary `return value;` operation code.
    Result(const T &value)
        : storage_(value)
    {
    }

    Result(T &&value)
        : storage_(std::move(value))
    {
    }

    // Implicit construction supports ordinary `return error;` propagation.
    Result(const Error &error)
        : storage_(error)
    {
    }

    Result(Error &&error)
        : storage_(std::move(error))
    {
    }

    Result(const Result &) = default;
    Result(Result &&) noexcept(
        std::is_nothrow_move_constructible_v<std::variant<T, Error>>) = default;

    // Assignment is intentionally not a fundamental operation on a one-shot
    // operation outcome. Excluding it also ensures no supported state-changing
    // operation can violate the two-state invariant for a T whose construction
    // or movement throws.
    Result &operator=(const Result &) = delete;
    Result &operator=(Result &&) = delete;

    ~Result() = default;

    [[nodiscard]] explicit operator bool() const noexcept
    {
      return std::holds_alternative<T>(storage_);
    }

    /**
     * @brief Borrow the successful value without transferring it.
     *
     * value() is observation only.  Use take() to transfer a successful
     * payload into a new owner.  Calling either operation after take() is a
     * Fault: Result remains a successful terminal outcome, but its payload
     * has already been transferred.
     */
    [[nodiscard]] T &value() &
    {
      require_value();
      require_unconsumed_value();
      return std::get<T>(storage_);
    }

    [[nodiscard]] const T &value() const &
    {
      require_value();
      require_unconsumed_value();
      return std::get<T>(storage_);
    }

    /**
     * @brief Advanced C++ interoperability transfer access.
     *
     * This preserves ordinary rvalue interoperability, including temporary
     * inspection expressions, while recording the same one-time consumption
     * state as take().  Normal Vix code should prefer take(), whose name
     * communicates ownership transfer without exposing value categories.
     */
    [[nodiscard]] T &&value() &&
    {
      require_value();
      require_unconsumed_value();
      value_taken_ = true;
      return std::get<T>(std::move(storage_));
    }

    /**
     * @brief Transfer the successful value to the caller.
     *
     * The Result remains semantically successful after transfer, so operator
     * bool() still reports Success.  The payload is no longer observable or
     * transferable.  Consumption is recorded before T's move construction;
     * if that construction throws, a caught exception cannot be used to retry
     * the transfer from a potentially moved-from payload.
     */
    [[nodiscard]] T take() noexcept(std::is_nothrow_move_constructible_v<T>)
    {
      require_value();
      require_unconsumed_value();
      value_taken_ = true;
      return std::get<T>(std::move(storage_));
    }

    [[nodiscard]] Error &error() &
    {
      require_error();
      return std::get<Error>(storage_);
    }

    [[nodiscard]] const Error &error() const &
    {
      require_error();
      return std::get<Error>(storage_);
    }

    [[nodiscard]] Error &&error() &&
    {
      require_error();
      return std::get<Error>(std::move(storage_));
    }

  private:
    void require_value() const noexcept
    {
      if (!static_cast<bool>(*this))
      {
        detail::result_contract_violation();
      }
    }

    void require_error() const noexcept
    {
      if (static_cast<bool>(*this))
      {
        detail::result_contract_violation();
      }
    }

    void require_unconsumed_value() const noexcept
    {
      if (value_taken_)
      {
        detail::result_contract_violation();
      }
    }

    std::variant<T, Error> storage_;
    bool value_taken_{false};
  };

  /**
   * @brief No-value success or expected operational failure.
   */
  template <>
  class Result<void>
  {
  public:
    Result() = delete;

    [[nodiscard]] static Result success() noexcept
    {
      return Result(success_tag{});
    }

    Result(const Error &error)
        : storage_(error)
    {
    }

    Result(Error &&error)
        : storage_(std::move(error))
    {
    }

    Result(const Result &) = default;
    Result(Result &&) noexcept(
        std::is_nothrow_move_constructible_v<std::variant<std::monostate, Error>>) = default;

    Result &operator=(const Result &) = delete;
    Result &operator=(Result &&) = delete;

    ~Result() = default;

    [[nodiscard]] explicit operator bool() const noexcept
    {
      return std::holds_alternative<std::monostate>(storage_);
    }

    [[nodiscard]] Error &error() &
    {
      require_error();
      return std::get<Error>(storage_);
    }

    [[nodiscard]] const Error &error() const &
    {
      require_error();
      return std::get<Error>(storage_);
    }

    [[nodiscard]] Error &&error() &&
    {
      require_error();
      return std::get<Error>(std::move(storage_));
    }

  private:
    struct success_tag
    {
    };

    explicit Result(success_tag) noexcept
        : storage_(std::monostate{})
    {
    }

    void require_error() const noexcept
    {
      if (static_cast<bool>(*this))
      {
        detail::result_contract_violation();
      }
    }

    std::variant<std::monostate, Error> storage_;
  };
} // namespace vix

#endif // VIX_RESULT_HPP
