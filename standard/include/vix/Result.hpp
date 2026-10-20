/**
 * @file Result.hpp
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
   * @brief Represents either a successful value or an operational error.
   *
   * @tparam T Type of the value returned on success.
   *
   * @details
   * Result<T> provides a consistent way to report the outcome
   * of an operation without using exceptions for expected failures.
   *
   * A Result contains exactly one of:
   * - A successful value of type T.
   * - An Error describing an operational failure.
   *
   * Use operator bool() to check the outcome before accessing
   * its value or error.
   *
   * @par Example
   * @code{.cpp}
   * vix::Result<int> get_count()
   * {
   *     return 42;
   * }
   *
   * auto result = get_count();
   *
   * if (!result)
   * {
   *     // Handle result.error().
   * }
   * else
   * {
   *     int count = result.take();
   * }
   * @endcode
   *
   * @par Value access
   * Use value() to inspect or borrow a successful value.
   * Use take() to transfer the value to the caller.
   *
   * A successful value can be transferred only once.
   * After transfer, the Result remains successful, but its
   * payload cannot be accessed again.
   *
   * @par Invalid access
   * Accessing a value from a failed Result, accessing an
   * error from a successful Result, or accessing a consumed
   * successful value violates the Result contract.
   *
   * Contract violations terminate the process.
   *
   * @par Exception behavior
   * Construction of T follows the exception guarantees of T.
   *
   * If construction throws, no Result object is produced.
   * An observable Result never enters a third or valueless state.
   *
   * @par Thread safety
   * Concurrent const observation is safe when the stored
   * value is also safely observable.
   *
   * Concurrent movement or destruction of the same Result
   * object requires appropriate synchronization.
   *
   * @note
   * Result does not represent cancellation, pending work,
   * asynchronous execution, timeouts or programming faults.
   *
   * @see Error
   * @see Result<void>
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
    /**
     * @brief Default construction is not supported.
     *
     * A Result must contain either a value or an error.
     */
    Result() = delete;

    /**
     * @brief Constructs a successful Result by copying a value.
     *
     * @param value The successful value to store.
     *
     * @note
     * Implicit construction allows functions returning Result<T>
     * to return a T value directly.
     */
    Result(const T &value)
        : storage_(value)
    {
    }

    /**
     * @brief Constructs a successful Result by moving a value.
     *
     * @param value The successful value to transfer.
     */
    Result(T &&value)
        : storage_(std::move(value))
    {
    }

    /**
     * @brief Constructs a failed Result by copying an error.
     *
     * @param error The operational error to store.
     *
     * @note
     * Implicit construction allows functions returning Result<T>
     * to return an Error directly.
     */
    Result(const Error &error)
        : storage_(error)
    {
    }

    /**
     * @brief Constructs a failed Result by moving an error.
     *
     * @param error The operational error to transfer.
     */
    Result(Error &&error)
        : storage_(std::move(error))
    {
    }

    /**
     * @brief Copy-constructs a Result.
     *
     * Copies the stored outcome and consumption state.
     *
     * Available when the stored value type supports copying.
     */
    Result(const Result &) = default;

    /**
     * @brief Move-constructs a Result.
     *
     * Transfers the stored outcome and consumption state
     * according to the stored type's move semantics.
     */
    Result(Result &&) noexcept(
        std::is_nothrow_move_constructible_v<std::variant<T, Error>>) = default;

    // Assignment is intentionally not a fundamental operation on a one-shot
    // operation outcome. Excluding it also ensures no supported state-changing
    // operation can violate the two-state invariant for a T whose construction
    // or movement throws.
    Result &operator=(const Result &) = delete;
    Result &operator=(Result &&) = delete;

    ~Result() = default;

    /**
     * @brief Checks whether the operation succeeded.
     *
     * @return true for success, false for failure.
     *
     * @note
     * A successful Result remains successful even after
     * its value has been transferred with take().
     */
    [[nodiscard]] explicit operator bool() const noexcept
    {
      return std::holds_alternative<T>(storage_);
    }

    /**
     * @brief Borrows the successful value.
     *
     * @return Mutable reference to the stored value.
     *
     * @pre The Result must be successful and unconsumed.
     *
     * @warning
     * Calling this function on a failed or consumed Result
     * terminates the process.
     *
     * @note
     * Borrowing does not transfer ownership.
     * The reference must not outlive its stored value.
     *
     * @see take()
     */
    [[nodiscard]] T &value() &
    {
      require_value();
      require_unconsumed_value();
      return std::get<T>(storage_);
    }

    /**
     * @brief Borrows the successful value as read-only.
     *
     * @return Const reference to the stored value.
     *
     * @pre The Result must be successful and unconsumed.
     *
     * @warning
     * Invalid access terminates the process.
     */
    [[nodiscard]] const T &value() const &
    {
      require_value();
      require_unconsumed_value();
      return std::get<T>(storage_);
    }

    /**
     * @brief Provides advanced rvalue access to the stored value.
     *
     * @return Rvalue reference to the successful value.
     *
     * @pre The Result must be successful and unconsumed.
     *
     * @details
     * This overload supports advanced C++ interoperability.
     * Access marks the successful payload as consumed.
     *
     * Subsequent attempts to access or transfer that payload
     * violate the Result contract.
     *
     * @note
     * For ordinary Vix code, prefer take() when transferring
     * ownership.
     */
    [[nodiscard]] T &&value() &&
    {
      require_value();
      require_unconsumed_value();
      value_taken_ = true;
      return std::get<T>(std::move(storage_));
    }

    /**
     * @brief Transfers the successful value to the caller.
     *
     * @return The stored value, transferred by move.
     *
     * @pre The Result must be successful and unconsumed.
     *
     * @details
     * take() is the preferred way to obtain ownership of a
     * successful value.
     *
     * The payload can be transferred only once.
     *
     * After transfer:
     * - The Result still reports success.
     * - Its successful value is no longer accessible.
     * - Calling take() again terminates the process.
     *
     * Consumption is recorded before moving T.
     * If moving T throws, the transfer cannot be retried.
     *
     * @par Example
     * @code{.cpp}
     * vix::Result<int> result = 42;
     *
     * if (result)
     * {
     *     int value = result.take();
     * }
     * @endcode
     *
     * @warning
     * Calling take() on a failed or consumed Result
     * terminates the process.
     *
     * @see value()
     */
    [[nodiscard]] T take() noexcept(std::is_nothrow_move_constructible_v<T>)
    {
      require_value();
      require_unconsumed_value();
      value_taken_ = true;
      return std::get<T>(std::move(storage_));
    }

    /**
     * @brief Returns the operational error.
     *
     * @return Mutable reference to the stored Error.
     *
     * @pre The Result must represent failure.
     *
     * @warning
     * Calling error() on a successful Result
     * terminates the process.
     */
    [[nodiscard]] Error &error() &
    {
      require_error();
      return std::get<Error>(storage_);
    }

    /**
     * @brief Returns the operational error as read-only.
     *
     * @return Const reference to the stored Error.
     *
     * @pre The Result must represent failure.
     */
    [[nodiscard]] const Error &error() const &
    {
      require_error();
      return std::get<Error>(storage_);
    }

    /**
     * @brief Provides rvalue access to the stored error.
     *
     * @return Rvalue reference to the stored Error.
     *
     * @pre The Result must represent failure.
     *
     * @note
     * This overload supports transferring the error
     * through ordinary C++ move semantics.
     */
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
   * @brief Represents success without a value, or an operational error.
   *
   * @details
   * Result<void> is used for operations that only need to
   * communicate whether they succeeded or failed.
   *
   * Unlike Result<T>, successful execution does not
   * produce a value.
   *
   * Create a successful result using success().
   * Create a failed result using an Error.
   *
   * @par Example
   * @code{.cpp}
   * vix::Result<void> perform_operation()
   * {
   *     return vix::Result<void>::success();
   * }
   *
   * auto result = perform_operation();
   *
   * if (!result)
   * {
   *     // Handle result.error().
   * }
   * @endcode
   *
   * @note
   * Error access on a successful Result<void> is
   * a contract violation and terminates the process.
   *
   * @see Result
   * @see Error
   */
  template <>
  class Result<void>
  {
  public:
    /**
     * @brief Default construction is not supported.
     *
     * Use success() or construct from an Error.
     */
    Result() = delete;

    /**
     * @brief Creates a successful Result without a value.
     *
     * @return A successful Result<void>.
     */
    [[nodiscard]] static Result success() noexcept
    {
      return Result(success_tag{});
    }

    /**
     * @brief Constructs a failed Result by copying an error.
     *
     * @param error The operational error to store.
     */
    Result(const Error &error)
        : storage_(error)
    {
    }

    /**
     * @brief Constructs a failed Result by moving an error.
     *
     * @param error The operational error to transfer.
     */
    Result(Error &&error)
        : storage_(std::move(error))
    {
    }

    /**
     * @brief Copy-constructs a Result<void>.
     */
    Result(const Result &) = default;

    /**
     * @brief Move-constructs a Result<void>.
     */
    Result(Result &&) noexcept(
        std::is_nothrow_move_constructible_v<std::variant<std::monostate, Error>>) = default;

    Result &operator=(const Result &) = delete;
    Result &operator=(Result &&) = delete;

    ~Result() = default;

    /**
     * @brief Checks whether the operation succeeded.
     *
     * @return true on success, false on failure.
     */
    [[nodiscard]] explicit operator bool() const noexcept
    {
      return std::holds_alternative<std::monostate>(storage_);
    }

    /**
     * @brief Returns the operational error.
     *
     * @return Mutable reference to the stored Error.
     *
     * @pre The Result must represent failure.
     *
     * @warning
     * Calling error() on success terminates the process.
     */
    [[nodiscard]] Error &error() &
    {
      require_error();
      return std::get<Error>(storage_);
    }

    /**
     * @brief Returns the operational error as read-only.
     *
     * @return Const reference to the stored Error.
     *
     * @pre The Result must represent failure.
     */
    [[nodiscard]] const Error &error() const &
    {
      require_error();
      return std::get<Error>(storage_);
    }

    /**
     * @brief Provides rvalue access to the stored error.
     *
     * @return Rvalue reference to the stored Error.
     *
     * @pre The Result must represent failure.
     */
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
