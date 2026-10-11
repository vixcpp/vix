#ifndef VIX_ERROR_HPP
#define VIX_ERROR_HPP

#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <vix/ErrorIdentity.hpp>
#include <vix/detail/AllocationFailure.hpp>

namespace vix
{
  /**
   * @brief Value describing an expected operational failure.
   *
   * Error always has a valid ErrorIdentity and never represents success.
   * Program logic matches identity(), never message text, context, or a
   * provider implementation detail.
   *
   * When propagation preserves a lower-level identity as part of the calling
   * contract, return that same identity. When an abstraction boundary
   * intentionally hides its lower implementation, translation creates a new
   * domain-owned identity and retains the lower Error as its cause.
   *
   * Error values have ordinary value semantics. Concurrent const observation
   * of immutable Error values is safe; concurrent mutation, move, or
   * assignment of the same C++ object remains an ordinary data race.
   * Vix-controlled diagnostic allocation is process-fatal on exhaustion. It
   * never becomes an Error or escapes as mode-dependent exception flow.
   * Allocation performed by a caller before it enters this API remains normal
   * C++ caller-owned work.
   */
  class Error
  {
  public:
    /**
     * @brief Diagnostic metadata attached without changing ErrorIdentity.
     *
     * Context is intended for logging and diagnosis, not semantic matching.
     * Keys and values have no universal vocabulary in Standard v1.
     */
    struct Context
    {
      std::string key;
      std::string value;
    };

    Error() = delete;

    explicit Error(ErrorIdentity identity) noexcept
        : identity_(identity)
    {
    }

    Error(ErrorIdentity identity, std::string message)
        : identity_(identity),
          message_(std::move(message))
    {
    }

    Error(ErrorIdentity identity, Error cause);

    Error(ErrorIdentity identity, std::string message, Error cause);

    Error(const Error &other)
        : identity_(other.identity_),
          message_(detail::allocation_or_terminate(
              [&] { return std::string(other.message_); })),
          cause_(other.cause_),
          contexts_(detail::allocation_or_terminate(
              [&] { return std::vector<Context>(other.contexts_); }))
    {
    }
    Error(Error &&) noexcept = default;
    Error &operator=(const Error &other)
    {
      if (this != &other)
      {
        Error copy(other);
        swap(copy);
      }
      return *this;
    }
    Error &operator=(Error &&) noexcept = default;
    ~Error() = default;

    [[nodiscard]] constexpr const ErrorIdentity &identity() const noexcept
    {
      return identity_;
    }

    [[nodiscard]] std::string_view message() const noexcept
    {
      return message_;
    }

    [[nodiscard]] bool has_message() const noexcept
    {
      return !message_.empty();
    }

    [[nodiscard]] const Error *cause() const noexcept
    {
      return cause_.get();
    }

    [[nodiscard]] bool has_cause() const noexcept
    {
      return static_cast<bool>(cause_);
    }

    [[nodiscard]] std::span<const Context> contexts() const noexcept
    {
      return contexts_;
    }

    /**
     * @brief Return a diagnostic enrichment retaining identity and cause.
     *
     * Context is append-only in the returned Error value. It never changes
     * the identity used by callers for machine-readable matching.
     */
    [[nodiscard]] Error with_context(Context context) const &;
    [[nodiscard]] Error with_context(Context context) &&;

  private:
    void swap(Error &other) noexcept
    {
      using std::swap;
      swap(identity_, other.identity_);
      swap(message_, other.message_);
      swap(cause_, other.cause_);
      swap(contexts_, other.contexts_);
    }

    ErrorIdentity identity_;
    std::string message_;
    std::shared_ptr<const Error> cause_;
    std::vector<Context> contexts_;
  };

  inline Error::Error(ErrorIdentity identity, Error cause)
      : identity_(identity),
        cause_(detail::allocation_or_terminate(
            [&] { return std::make_shared<Error>(std::move(cause)); }))
  {
  }

  inline Error::Error(ErrorIdentity identity,
                      std::string message,
                      Error cause)
      : identity_(identity),
        message_(std::move(message)),
        cause_(detail::allocation_or_terminate(
            [&] { return std::make_shared<Error>(std::move(cause)); }))
  {
  }

  inline Error Error::with_context(Context context) const &
  {
    return detail::allocation_or_terminate(
        [&]
        {
          Error enriched(*this);
          enriched.contexts_.push_back(std::move(context));
          return enriched;
        });
  }

  inline Error Error::with_context(Context context) &&
  {
    return detail::allocation_or_terminate(
        [&]
        {
          contexts_.push_back(std::move(context));
          return std::move(*this);
        });
  }
} // namespace vix

#endif // VIX_ERROR_HPP
