/**
 * @file Error.hpp
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
   * @brief Describes an operational failure.
   *
   * @details
   * Error is a value type representing a failure that an
   * application may need to handle.
   *
   * Every Error has a valid ErrorIdentity. It may also contain:
   * - A human-readable diagnostic message.
   * - An underlying error that caused the failure.
   * - Additional diagnostic context.
   *
   * Applications should identify errors using identity(),
   * not by comparing messages or diagnostic context.
   *
   * @par Example
   * @code{.cpp}
   * constexpr auto not_found =
   *     vix::ErrorIdentity::define<"example.files">(1);
   *
   * vix::Error error(not_found, "File not found");
   *
   * if (error.identity() == not_found)
   * {
   *     // Handle the missing file.
   * }
   *
   * auto detailed = error.with_context(
   *     {"path", "config.toml"});
   * @endcode
   *
   * @par Error propagation
   * Preserve the original identity when a lower-level error
   * remains part of the public calling contract.
   *
   * When translating an error across an abstraction boundary,
   * create an identity owned by the higher-level domain and
   * retain the original error as its cause.
   *
   * @par Value semantics
   * Error supports copying and moving.
   *
   * Copies preserve the identity, message, context and cause.
   * The underlying cause is shared through immutable ownership.
   *
   * @par Thread safety
   * Concurrent const observation of immutable Error values
   * is safe.
   *
   * Concurrent mutation, moving or assignment of the same
   * object requires external synchronization.
   *
   * @par Allocation failures
   * Allocation exhaustion during Vix-controlled diagnostic
   * operations terminates the process.
   *
   * Such failures are not converted into Error values.
   * Allocation performed by the caller before entering
   * the API follows normal C++ rules.
   *
   * @note
   * Error never represents successful execution.
   *
   * @see ErrorIdentity
   */
  class Error
  {
  public:
    /**
     * @brief Diagnostic information associated with an error.
     *
     * @details
     * Context provides additional information useful for
     * debugging, logging and diagnostics.
     *
     * Keys and values are descriptive strings.
     *
     * @par Example
     * @code{.cpp}
     * vix::Error::Context context{
     *     "path",
     *     "config.toml"
     * };
     * @endcode
     *
     * @note
     * Context does not affect error identity.
     * Standard v1 does not define universal context keys.
     */
    struct Context
    {
      std::string key;
      std::string value;
    };

    /**
     * @brief Default construction is not supported.
     *
     * Every Error must have a valid ErrorIdentity.
     */
    Error() = delete;

    /**
     * @brief Constructs an error from an identity.
     *
     * @param identity The programmatic error identity.
     *
     * @note
     * The resulting error has no message, cause or context.
     */
    explicit Error(ErrorIdentity identity) noexcept
        : identity_(identity)
    {
    }

    /**
     * @brief Constructs an error with a diagnostic message.
     *
     * @param identity The programmatic error identity.
     * @param message Human-readable error description.
     *
     * @note
     * Message text is intended for diagnostics and must
     * not be used to identify the error programmatically.
     */
    Error(ErrorIdentity identity, std::string message)
        : identity_(identity),
          message_(std::move(message))
    {
    }

    /**
     * @brief Constructs an error with an underlying cause.
     *
     * @param identity Identity of the new error.
     * @param cause The error responsible for this failure.
     *
     * @details
     * Use this constructor when translating a lower-level
     * error into an identity owned by another domain.
     *
     * The original error is retained as the cause.
     *
     * @see cause()
     */
    Error(ErrorIdentity identity, Error cause);

    /**
     * @brief Constructs an error with a message and cause.
     *
     * @param identity Identity of the new error.
     * @param message Human-readable diagnostic message.
     * @param cause The underlying error.
     *
     * @see cause()
     * @see message()
     */
    Error(ErrorIdentity identity, std::string message, Error cause);

    /**
     * @brief Copies an error.
     *
     * @param other The error to copy.
     *
     * @details
     * Copies the identity, message and diagnostic contexts.
     * The immutable cause is shared with the original.
     */
    Error(const Error &other)
        : identity_(other.identity_),
          message_(detail::allocation_or_terminate(
              [&]
              { return std::string(other.message_); })),
          cause_(other.cause_),
          contexts_(detail::allocation_or_terminate(
              [&]
              { return std::vector<Context>(other.contexts_); }))
    {
    }

    /**
     * @brief Move-constructs an error.
     */
    Error(Error &&) noexcept = default;

    /**
     * @brief Copies another error into this object.
     *
     * @param other The error to copy.
     * @return Reference to this error.
     */
    Error &operator=(const Error &other)
    {
      if (this != &other)
      {
        Error copy(other);
        swap(copy);
      }
      return *this;
    }

    /**
     * @brief Move-assigns another error.
     *
     * @return Reference to this error.
     */
    Error &operator=(Error &&) noexcept = default;

    /**
     * @brief Destroys the error and releases its resources.
     */
    ~Error() = default;

    /**
     * @brief Returns the programmatic error identity.
     *
     * @return Reference to the ErrorIdentity.
     *
     * @details
     * Use this value to determine the kind of failure.
     *
     * Unlike messages and contexts, an identity has
     * stable programmatic meaning.
     *
     * @see ErrorIdentity
     */
    [[nodiscard]] constexpr const ErrorIdentity &identity() const noexcept
    {
      return identity_;
    }

    /**
     * @brief Returns the diagnostic message.
     *
     * @return A view of the message string.
     *
     * @note
     * The returned view is valid while the underlying
     * message remains alive and unchanged.
     *
     * Use identity() for programmatic error handling.
     */
    [[nodiscard]] std::string_view message() const noexcept
    {
      return message_;
    }

    /**
     * @brief Checks whether a diagnostic message exists.
     *
     * @return true if the message is non-empty,
     *         false otherwise.
     */
    [[nodiscard]] bool has_message() const noexcept
    {
      return !message_.empty();
    }

    /**
     * @brief Returns the underlying error, if present.
     *
     * @return Pointer to the cause, or nullptr if absent.
     *
     * @details
     * A cause preserves information about the failure
     * responsible for the current error.
     *
     * The returned pointer is non-owning and must not
     * outlive the Error object providing it.
     *
     * @see has_cause()
     */
    [[nodiscard]] const Error *cause() const noexcept
    {
      return cause_.get();
    }

    /**
     * @brief Checks whether the error has a cause.
     *
     * @return true if a cause exists, false otherwise.
     */
    [[nodiscard]] bool has_cause() const noexcept
    {
      return static_cast<bool>(cause_);
    }

    /**
     * @brief Returns the diagnostic contexts.
     *
     * @return A read-only span of Context entries.
     *
     * @details
     * Contexts provide additional diagnostic information
     * without changing the error identity.
     *
     * The returned span is non-owning and is valid while
     * the underlying context storage remains unchanged.
     *
     * @see with_context()
     */
    [[nodiscard]] std::span<const Context> contexts() const noexcept
    {
      return contexts_;
    }

    /**
     * @brief Creates an enriched copy of this error.
     *
     * @param context Diagnostic context to append.
     *
     * @return A new Error containing the additional context.
     *
     * @details
     * This overload is used with an lvalue Error.
     *
     * The original error remains unchanged.
     * The returned error preserves its identity and cause.
     *
     * @par Example
     * @code{.cpp}
     * constexpr auto invalid =
     *     vix::ErrorIdentity::define<"example">(1);
     *
     * vix::Error error(invalid);
     *
     * auto detailed = error.with_context(
     *     {"operation", "load_config"});
     * @endcode
     */
    [[nodiscard]] Error with_context(Context context) const &;

    /**
     * @brief Appends context to a movable error.
     *
     * @param context Diagnostic context to append.
     *
     * @return The enriched Error value.
     *
     * @details
     * This overload operates on an rvalue Error.
     *
     * It appends context to the current value and
     * returns that value by moving it.
     *
     * The error identity and cause are preserved.
     */
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
            [&]
            { return std::make_shared<Error>(std::move(cause)); }))
  {
  }

  inline Error::Error(ErrorIdentity identity,
                      std::string message,
                      Error cause)
      : identity_(identity),
        message_(std::move(message)),
        cause_(detail::allocation_or_terminate(
            [&]
            { return std::make_shared<Error>(std::move(cause)); }))
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
