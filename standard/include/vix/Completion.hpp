#ifndef VIX_COMPLETION_HPP
#define VIX_COMPLETION_HPP

#include <type_traits>
#include <utility>
#include <variant>

#include <vix/Result.hpp>

namespace vix
{
  /** @brief Terminal classification of a cancellation-aware operation. */
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
   * @brief Success value, operational Failure, or cancelled completion.
   *
   * Completion<T> is the terminal representation for cancellation-aware
   * operations. It deliberately keeps Cancelled distinct from Error. It is a
   * value type, not an operation state machine: providers must establish one
   * Completion exactly once when success, failure, cancellation, and deadline
   * observations race. T may itself be Result<U> when that Result is an
   * intentional successful payload (for example, a batch report); normal Vix
   * operation APIs still use Completion<U>, never Completion<Result<U>>, to
   * express Success, Failure, and Cancelled.
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
    Completion() = delete;

    Completion(const T &value)
        : storage_(value)
    {
    }

    Completion(T &&value)
        : storage_(std::move(value))
    {
    }

    Completion(const Error &error)
        : storage_(error)
    {
    }

    Completion(Error &&error)
        : storage_(std::move(error))
    {
    }

    Completion(const Result<T> &result)
        : storage_(result ? std::variant<T, Error, detail::cancelled_completion_tag>{result.value()}
                          : std::variant<T, Error, detail::cancelled_completion_tag>{result.error()})
    {
    }

    Completion(Result<T> &&result)
        : storage_(result ? std::variant<T, Error, detail::cancelled_completion_tag>{result.take()}
                          : std::variant<T, Error, detail::cancelled_completion_tag>{std::move(result).error()})
    {
    }

    [[nodiscard]] static Completion cancelled() noexcept
    {
      return Completion(detail::cancelled_completion_tag{});
    }

    Completion(const Completion &) = default;
    Completion(Completion &&) noexcept(
        std::is_nothrow_move_constructible_v<
            std::variant<T, Error, detail::cancelled_completion_tag>>) = default;
    Completion &operator=(const Completion &) = delete;
    Completion &operator=(Completion &&) = delete;
    ~Completion() = default;

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

    [[nodiscard]] T &value() &
    {
      require_success();
      require_unconsumed_value();
      return std::get<T>(storage_);
    }

    [[nodiscard]] const T &value() const &
    {
      require_success();
      require_unconsumed_value();
      return std::get<T>(storage_);
    }

    /**
     * @brief Advanced C++ interoperability transfer access.
     *
     * The rvalue path remains available for temporary expressions but records
     * payload consumption.  Normal Vix code should use take().
     */
    [[nodiscard]] T &&value() &&
    {
      require_success();
      require_unconsumed_value();
      value_taken_ = true;
      return std::get<T>(std::move(storage_));
    }

    /**
     * @brief Transfer a successful payload without changing terminal status.
     *
     * Completion remains Success after take(), while value() and a second
     * take() are Faults.  The consumed marker is set before T's move
     * construction so a throwing move cannot leave a retryable claim to a
     * potentially moved-from payload.
     */
    [[nodiscard]] T take() noexcept(std::is_nothrow_move_constructible_v<T>)
    {
      require_success();
      require_unconsumed_value();
      value_taken_ = true;
      return std::get<T>(std::move(storage_));
    }

    [[nodiscard]] Error &error() &
    {
      require_failure();
      return std::get<Error>(storage_);
    }

    [[nodiscard]] const Error &error() const &
    {
      require_failure();
      return std::get<Error>(storage_);
    }

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

  template <>
  class Completion<void>
  {
  public:
    Completion() = delete;

    [[nodiscard]] static Completion success() noexcept
    {
      return Completion(success_tag{});
    }

    [[nodiscard]] static Completion cancelled() noexcept
    {
      return Completion(detail::cancelled_completion_tag{});
    }

    Completion(const Error &error)
        : storage_(error)
    {
    }

    Completion(Error &&error)
        : storage_(std::move(error))
    {
    }

    Completion(const Result<void> &result)
        : storage_(result ? std::variant<success_tag, Error, detail::cancelled_completion_tag>{success_tag{}}
                          : std::variant<success_tag, Error, detail::cancelled_completion_tag>{result.error()})
    {
    }

    Completion(Result<void> &&result)
        : storage_(result ? std::variant<success_tag, Error, detail::cancelled_completion_tag>{success_tag{}}
                          : std::variant<success_tag, Error, detail::cancelled_completion_tag>{std::move(result).error()})
    {
    }

    Completion(const Completion &) = default;
    Completion(Completion &&) noexcept(
        std::is_nothrow_move_constructible_v<
            std::variant<success_tag, Error, detail::cancelled_completion_tag>>) = default;
    Completion &operator=(const Completion &) = delete;
    Completion &operator=(Completion &&) = delete;
    ~Completion() = default;

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

    [[nodiscard]] Error &error() &
    {
      require_failure();
      return std::get<Error>(storage_);
    }

    [[nodiscard]] const Error &error() const &
    {
      require_failure();
      return std::get<Error>(storage_);
    }

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
