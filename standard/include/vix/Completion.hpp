/**
 * @file Completion.hpp
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

#ifndef VIX_COMPLETION_HPP
#define VIX_COMPLETION_HPP

#include <type_traits>
#include <utility>
#include <variant>

#include <vix/Result.hpp>

namespace vix
{
  /**
   * @brief Terminal outcome of a cancellation-aware operation.
   *
   * @details
   * An operation completes with exactly one of three statuses:
   * - Success: the operation completed successfully.
   * - Failure: the operation failed with an Error.
   * - Cancelled: the operation was cancelled.
   *
   * Cancellation is separate from operational failure.
   *
   * @see Completion
   */
  enum class CompletionStatus
  {
    Success,
    Failure,
    Cancelled,
  };

  namespace detail
  {
    struct cancelled_completion_tag
    {
    };

  } // namespace detail

  /**
   * @brief Represents success, failure, or cancellation.
   *
   * @tparam T Type of the successful result.
   *
   * @details
   * Completion<T> represents the final outcome of an operation
   * that supports cancellation.
   *
   * Unlike Result<T>, which distinguishes only success and
   * failure, Completion<T> supports three terminal states:
   *
   * - Success: contains a value of type T.
   * - Failure: contains an operational Error.
   * - Cancelled: represents cancellation without an Error.
   *
   * Use status() to determine the outcome before accessing
   * the successful value or error.
   *
   * @par Example
   * @code{.cpp}
   * vix::Completion<int> operation()
   * {
   *     return 42;
   * }
   *
   * auto result = operation();
   *
   * switch (result.status())
   * {
   * case vix::CompletionStatus::Success:
   * {
   *     int value = result.take();
   *     // Use value.
   *     break;
   * }
   *
   * case vix::CompletionStatus::Failure:
   *     // Inspect result.error().
   *     break;
   *
   * case vix::CompletionStatus::Cancelled:
   *     // Handle cancellation.
   *     break;
   * }
   * @endcode
   *
   * @par Creating a completion
   * Construct a Completion from a successful value, an Error,
   * or an existing Result<T>.
   *
   * Use cancelled() to represent cancellation explicitly.
   *
   * @par Value ownership
   * value() borrows the successful payload.
   * take() transfers ownership of the successful payload.
   *
   * A successful payload can be transferred only once.
   * After transfer, the Completion still reports Success,
   * but the payload cannot be accessed again.
   *
   * @par Cancellation
   * Cancellation is not represented by an Error.
   *
   * Completion is a terminal value, not an asynchronous
   * operation or cancellation state machine.
   *
   * The operation provider is responsible for selecting one
   * final outcome when completion, cancellation, and deadline
   * observations compete.
   *
   * @par Nested results
   * T may itself be Result<U> when the nested result is an
   * intentional successful payload, such as a batch report.
   *
   * Ordinary Vix operations should use Completion<U> directly
   * to represent success, failure, and cancellation.
   *
   * @par Invalid access
   * Accessing a value outside Success, accessing an error
   * outside Failure, or accessing a consumed successful value
   * violates the Completion contract.
   *
   * Contract violations terminate the process.
   *
   * @note
   * Completion does not represent pending work or provide
   * mechanisms to start, schedule, or cancel an operation.
   *
   * @see CompletionStatus
   * @see Result
   * @see Error
   */
  template <typename T>
  class Completion
  {
    static_assert(!std::is_void_v<T>,
                  "Use the Completion<void> specialization for no-value success.");
    static_assert(!std::is_reference_v<T>,
                  "Completion<T> does not support reference types.");
    static_assert(!std::is_const_v<T> && !std::is_volatile_v<T>,
                  "Completion<T> stores an ordinary unqualified value type.");
    static_assert(!std::is_array_v<T>,
                  "Completion<T> does not support array types.");
    static_assert(!std::is_same_v<T, Error>,
                  "Completion<Error> is not supported; Error represents Failure.");

  public:
    /**
     * @brief Default construction is not supported.
     *
     * Every Completion must have a terminal status.
     */
    Completion() = delete;

    /**
     * @brief Constructs a successful Completion by copying a value.
     *
     * @param value Successful value to store.
     *
     * @note
     * Supports returning a T value directly from a function
     * returning Completion<T>.
     */
    Completion(const T &value)
        : storage_(value)
    {
    }

    /**
     * @brief Constructs a successful Completion by moving a value.
     *
     * @param value Successful value to transfer.
     */
    Completion(T &&value)
        : storage_(std::move(value))
    {
    }

    /**
     * @brief Constructs a failed Completion by copying an error.
     *
     * @param error Operational failure to store.
     */
    Completion(const Error &error)
        : storage_(error)
    {
    }

    /**
     * @brief Constructs a failed Completion by moving an error.
     *
     * @param error Operational failure to transfer.
     */
    Completion(Error &&error)
        : storage_(std::move(error))
    {
    }

    /**
     * @brief Constructs a Completion from a Result.
     *
     * @param result The Result to convert.
     *
     * @details
     * A successful Result produces Success.
     * A failed Result produces Failure.
     *
     * This conversion never produces Cancelled.
     *
     * The successful payload or error is copied.
     *
     * @pre
     * A successful Result must have an unconsumed payload.
     */
    Completion(const Result<T> &result)
        : storage_(result ? std::variant<T, Error, detail::cancelled_completion_tag>{result.value()}
                          : std::variant<T, Error, detail::cancelled_completion_tag>{result.error()})
    {
    }

    /**
     * @brief Constructs a Completion by consuming a Result.
     *
     * @param result The Result to transfer from.
     *
     * @details
     * Preserves the success or failure outcome of the Result.
     *
     * On success, the payload is transferred using take().
     * On failure, the stored Error is moved.
     *
     * @pre
     * A successful Result must have an unconsumed payload.
     */
    Completion(Result<T> &&result)
        : storage_(result ? std::variant<T, Error, detail::cancelled_completion_tag>{result.take()}
                          : std::variant<T, Error, detail::cancelled_completion_tag>{std::move(result).error()})
    {
    }

    /**
     * @brief Creates a cancelled Completion.
     *
     * @return A Completion with status Cancelled.
     *
     * @par Example
     * @code{.cpp}
     * vix::Completion<int> result =
     *     vix::Completion<int>::cancelled();
     * @endcode
     *
     * @note
     * A cancelled Completion contains neither a successful
     * payload nor an operational Error.
     */
    [[nodiscard]] static Completion cancelled() noexcept
    {
      return Completion(detail::cancelled_completion_tag{});
    }

    /**
     * @brief Copy-constructs a Completion.
     *
     * Copies the stored outcome and payload consumption state.
     *
     * Available when the stored value type supports copying.
     */
    Completion(const Completion &) = default;

    /**
     * @brief Move-constructs a Completion.
     *
     * Transfers the stored outcome and consumption state
     * according to the stored type's move semantics.
     */
    Completion(Completion &&) noexcept(
        std::is_nothrow_move_constructible_v<
            std::variant<T, Error, detail::cancelled_completion_tag>>) = default;

    Completion &operator=(const Completion &) = delete;
    Completion &operator=(Completion &&) = delete;
    ~Completion() = default;

    /**
     * @brief Returns the terminal completion status.
     *
     * @return Success, Failure, or Cancelled.
     *
     * @note
     * The status remains Success after take() transfers
     * the successful payload.
     */
    [[nodiscard]] CompletionStatus status() const noexcept
    {
      if (std::holds_alternative<T>(storage_))
      {
        return CompletionStatus::Success;
      }
      if (std::holds_alternative<Error>(storage_))
      {
        return CompletionStatus::Failure;
      }
      return CompletionStatus::Cancelled;
    }

    /**
     * @brief Borrows the successful value.
     *
     * @return Mutable reference to the stored value.
     *
     * @pre
     * The Completion must be Success and unconsumed.
     *
     * @warning
     * Invalid access terminates the process.
     *
     * @see take()
     */
    [[nodiscard]] T &value() &
    {
      require_success();
      require_unconsumed_value();
      return std::get<T>(storage_);
    }

    /**
     * @brief Borrows the successful value as read-only.
     *
     * @return Const reference to the stored value.
     *
     * @pre
     * The Completion must be Success and unconsumed.
     *
     * @warning
     * Invalid access terminates the process.
     */
    [[nodiscard]] const T &value() const &
    {
      require_success();
      require_unconsumed_value();
      return std::get<T>(storage_);
    }

    /**
     * @brief Provides advanced rvalue access to the payload.
     *
     * @return Rvalue reference to the successful value.
     *
     * @pre
     * The Completion must be Success and unconsumed.
     *
     * @details
     * Marks the payload as consumed before returning access.
     *
     * Further access or transfer of the payload violates
     * the Completion contract.
     *
     * @note
     * Ordinary Vix code should prefer take() for ownership
     * transfer.
     */
    [[nodiscard]] T &&value() &&
    {
      require_success();
      require_unconsumed_value();
      value_taken_ = true;
      return std::get<T>(std::move(storage_));
    }

    /**
     * @brief Transfers the successful value to the caller.
     *
     * @return The successful payload, transferred by move.
     *
     * @pre
     * The Completion must be Success and unconsumed.
     *
     * @details
     * A payload can be transferred only once.
     *
     * After transfer:
     * - status() still reports Success.
     * - value() cannot access the payload.
     * - Calling take() again violates the contract.
     *
     * Consumption is recorded before moving T.
     * If moving T throws, the transfer cannot be retried.
     *
     * @par Example
     * @code{.cpp}
     * vix::Completion<int> result = 42;
     *
     * if (result.status() == vix::CompletionStatus::Success)
     * {
     *     int value = result.take();
     * }
     * @endcode
     *
     * @warning
     * Calling take() on Failure, Cancelled, or an already
     * consumed Success terminates the process.
     *
     * @see value()
     */
    [[nodiscard]] T take() noexcept(std::is_nothrow_move_constructible_v<T>)
    {
      require_success();
      require_unconsumed_value();
      value_taken_ = true;
      return std::get<T>(std::move(storage_));
    }

    /**
     * @brief Returns the operational failure.
     *
     * @return Mutable reference to the stored Error.
     *
     * @pre
     * The Completion must have status Failure.
     *
     * @warning
     * Calling error() on Success or Cancelled terminates
     * the process.
     */
    [[nodiscard]] Error &error() &
    {
      require_failure();
      return std::get<Error>(storage_);
    }

    /**
     * @brief Returns the operational failure as read-only.
     *
     * @return Const reference to the stored Error.
     *
     * @pre
     * The Completion must have status Failure.
     */
    [[nodiscard]] const Error &error() const &
    {
      require_failure();
      return std::get<Error>(storage_);
    }

    /**
     * @brief Provides rvalue access to the stored error.
     *
     * @return Rvalue reference to the stored Error.
     *
     * @pre
     * The Completion must have status Failure.
     */
    [[nodiscard]] Error &&error() &&
    {
      require_failure();
      return std::get<Error>(std::move(storage_));
    }

  private:
    explicit Completion(detail::cancelled_completion_tag tag) noexcept
        : storage_(tag)
    {
    }

    void require_success() const noexcept
    {
      if (status() != CompletionStatus::Success)
      {
        detail::result_contract_violation();
      }
    }

    void require_failure() const noexcept
    {
      if (status() != CompletionStatus::Failure)
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

    std::variant<T, Error, detail::cancelled_completion_tag> storage_;
    bool value_taken_{false};
  };

  /**
   * @brief Represents success without a value, failure, or cancellation.
   *
   * @details
   * Completion<void> is intended for cancellation-aware operations
   * that do not return a successful payload.
   *
   * It supports three terminal outcomes:
   * - Success: the operation completed successfully.
   * - Failure: an Error describes the operational failure.
   * - Cancelled: the operation was cancelled.
   *
   * Create a successful outcome using success().
   * Create a cancelled outcome using cancelled().
   * Create a failed outcome from an Error.
   *
   * @par Example
   * @code{.cpp}
   * vix::Completion<void> operation()
   * {
   *     return vix::Completion<void>::success();
   * }
   *
   * auto result = operation();
   *
   * if (result.status() == vix::CompletionStatus::Success)
   * {
   *     // Operation completed successfully.
   * }
   * @endcode
   *
   * @note
   * Error access is valid only when status() returns Failure.
   *
   * @see Completion
   * @see CompletionStatus
   */
  template <>
  class Completion<void>
  {
  public:
    /**
     * @brief Default construction is not supported.
     */
    Completion() = delete;

    /**
     * @brief Creates a successful completion without a value.
     *
     * @return A Completion with status Success.
     */
    [[nodiscard]] static Completion success() noexcept
    {
      return Completion(success_tag{});
    }

    /**
     * @brief Creates a cancelled completion.
     *
     * @return A Completion with status Cancelled.
     */
    [[nodiscard]] static Completion cancelled() noexcept
    {
      return Completion(detail::cancelled_completion_tag{});
    }

    /**
     * @brief Constructs a failed completion by copying an error.
     *
     * @param error Operational failure to store.
     */
    Completion(const Error &error)
        : storage_(error)
    {
    }

    /**
     * @brief Constructs a failed completion by moving an error.
     *
     * @param error Operational failure to transfer.
     */
    Completion(Error &&error)
        : storage_(std::move(error))
    {
    }

    /**
     * @brief Converts a Result<void> into a Completion<void>.
     *
     * @param result The Result to convert.
     *
     * @details
     * Success remains Success.
     * Failure remains Failure.
     *
     * Cancellation is never introduced by this conversion.
     */
    Completion(const Result<void> &result)
        : storage_(result ? std::variant<success_tag, Error, detail::cancelled_completion_tag>{success_tag{}}
                          : std::variant<success_tag, Error, detail::cancelled_completion_tag>{result.error()})
    {
    }

    /**
     * @brief Converts an rvalue Result<void> into a Completion<void>.
     *
     * @param result The Result to transfer from.
     *
     * @details
     * Preserves success or failure.
     * Moves the Error when the Result represents failure.
     */
    Completion(Result<void> &&result)
        : storage_(result ? std::variant<success_tag, Error, detail::cancelled_completion_tag>{success_tag{}}
                          : std::variant<success_tag, Error, detail::cancelled_completion_tag>{std::move(result).error()})
    {
    }

    /**
     * @brief Copy-constructs a Completion<void>.
     */
    Completion(const Completion &) = default;

    /**
     * @brief Move-constructs a Completion<void>.
     */
    Completion(Completion &&) noexcept(
        std::is_nothrow_move_constructible_v<
            std::variant<success_tag, Error, detail::cancelled_completion_tag>>) = default;

    Completion &operator=(const Completion &) = delete;
    Completion &operator=(Completion &&) = delete;
    ~Completion() = default;

    /**
     * @brief Returns the terminal completion status.
     *
     * @return Success, Failure, or Cancelled.
     */
    [[nodiscard]] CompletionStatus status() const noexcept
    {
      if (std::holds_alternative<success_tag>(storage_))
      {
        return CompletionStatus::Success;
      }
      if (std::holds_alternative<Error>(storage_))
      {
        return CompletionStatus::Failure;
      }
      return CompletionStatus::Cancelled;
    }

    /**
     * @brief Returns the operational failure.
     *
     * @return Mutable reference to the stored Error.
     *
     * @pre
     * The Completion must have status Failure.
     *
     * @warning
     * Invalid access terminates the process.
     */
    [[nodiscard]] Error &error() &
    {
      require_failure();
      return std::get<Error>(storage_);
    }

    /**
     * @brief Returns the operational failure as read-only.
     *
     * @return Const reference to the stored Error.
     *
     * @pre
     * The Completion must have status Failure.
     */
    [[nodiscard]] const Error &error() const &
    {
      require_failure();
      return std::get<Error>(storage_);
    }

    /**
     * @brief Provides rvalue access to the stored Error.
     *
     * @return Rvalue reference to the stored Error.
     *
     * @pre
     * The Completion must have status Failure.
     */
    [[nodiscard]] Error &&error() &&
    {
      require_failure();
      return std::get<Error>(std::move(storage_));
    }

  private:
    struct success_tag
    {
    };

    explicit Completion(success_tag tag) noexcept
        : storage_(tag)
    {
    }

    explicit Completion(detail::cancelled_completion_tag tag) noexcept
        : storage_(tag)
    {
    }

    void require_failure() const noexcept
    {
      if (status() != CompletionStatus::Failure)
      {
        detail::result_contract_violation();
      }
    }

    std::variant<success_tag, Error, detail::cancelled_completion_tag> storage_;
  };
} // namespace vix

#endif // VIX_COMPLETION_HPP
