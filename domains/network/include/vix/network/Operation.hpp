/**
 * @file Operation.hpp
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
 *
 * @brief Asynchronous operations for the Vix Network domain.
 *
 * @details
 * Provides the asynchronous operation abstraction used
 * by TCP networking APIs.
 *
 * An Operation<T> represents work that can be started
 * and awaited by a C++ coroutine.
 *
 * Its eventual outcome is represented by
 * vix::Completion<T>, which distinguishes successful
 * completion, operational failure, and cancellation.
 *
 * Operations support optional cancellation tokens
 * and deadlines through OperationOptions.
 *
 * @par Asynchronous execution
 * Network operations are created by APIs such as:
 * - Provider::connect().
 * - TcpListener::accept().
 * - TcpConnection::receive().
 * - TcpConnection::send().
 *
 * An operation begins when its awaiter is suspended
 * through co_await.
 *
 * @par Synchronous entry points
 * Applications that do not use coroutines directly
 * can execute a Network operation through run(),
 * declared in Provider.hpp.
 *
 * @code{.cpp}
 * auto result = vix::network::run(
 *     provider,
 *     provider.connect(endpoint));
 *
 * if (!result)
 * {
 *     // The operation runner could not start.
 * }
 * else
 * {
 *     auto completion = result.take();
 *     // Inspect the operation's terminal outcome.
 * }
 * @endcode
 *
 * @par Operation lifetime
 * An operation that has started must remain owned
 * by its awaiter until terminal completion.
 *
 * Destroying an awaiter while its started operation
 * remains incomplete violates the Network contract.
 *
 * This restriction protects the lifetime of borrowed
 * buffers, transport resources, and coroutine state.
 *
 * @see vix::network::Operation
 * @see vix::network::OperationOptions
 * @see vix::Completion
 * @see vix::network::Provider
 */

#pragma once

#include <vix/Cancellation.hpp>
#include <vix/Completion.hpp>
#include <vix/Deadline.hpp>
#include <coroutine>
#include <functional>
#include <memory>
#include <optional>
#include <utility>

namespace vix::network
{
  /**
   * @brief Configures cancellation and deadline behavior
   *        for an asynchronous network operation.
   *
   * @details
   * OperationOptions is supplied when creating
   * asynchronous TCP operations.
   *
   * It contains:
   * - An optional cancellation token.
   * - An optional deadline.
   *
   * @par Default behavior
   * Default construction provides no cancellation
   * request and no deadline.
   *
   * @par Example: Default options
   * @code{.cpp}
   * OperationOptions options{};
   *
   * auto operation = provider.connect(endpoint, options);
   * @endcode
   *
   * @par Example: Deadline
   * @code{.cpp}
   * OperationOptions options{
   *     {},
   *     vix::Deadline::after(
   *         vix::Duration::milliseconds(5000))
   * };
   *
   * auto operation = provider.connect(endpoint, options);
   * @endcode
   *
   * @par Example: Cancellation
   * @code{.cpp}
   * vix::CancellationSource source;
   *
   * OperationOptions options{
   *     source.token(),
   *     std::nullopt
   * };
   *
   * auto operation = provider.connect(endpoint, options);
   * @endcode
   *
   * @par Cancellation semantics
   * Cancellation requests that the outstanding
   * operation stop making progress.
   *
   * An operation whose cancellation is observed
   * before successful completion may produce
   * a Cancelled completion.
   *
   * Cancellation does not imply that externally
   * visible I/O has been rolled back.
   *
   * @par Deadline semantics
   * A deadline limits the period during which
   * an operation may remain outstanding.
   *
   * Deadline expiration is reported as an
   * operational failure with the Network
   * timed_out error identity.
   *
   * It is distinct from cancellation.
   *
   * @note
   * On cooperative providers, cancellation and
   * deadlines are processed when the provider
   * makes progress through poll().
   *
   * @note
   * Successful completion, cancellation, and
   * timeout are arbitrated by the Network
   * operation state machine.
   *
   * @see vix::CancellationToken
   * @see vix::Deadline
   * @see vix::Completion
   * @see Operation
   */
  struct OperationOptions
  {
    /**
     * @brief Token used to request operation cancellation.
     *
     * @details
     * The default token does not request cancellation.
     *
     * Cancellation is cooperative and may race
     * with successful operation completion.
     */
    vix::CancellationToken cancellation{};

    /**
     * @brief Optional operation deadline.
     *
     * @details
     * std::nullopt means that no deadline
     * restriction has been configured.
     *
     * An expired deadline produces an operation
     * failure rather than a Cancelled completion.
     */
    std::optional<vix::Deadline> deadline{};
  };

  namespace detail
  {
    /**
     * @brief Internal state interface for an asynchronous operation.
     *
     * @internal
     *
     * @details
     * OperationState separates the public coroutine
     * interface from the Network engine's operation
     * scheduling and completion machinery.
     *
     * Concrete implementations manage:
     * - Coroutine continuation registration.
     * - Operation submission and suspension.
     * - Terminal completion publication.
     * - Lifetime and abandonment validation.
     *
     * Applications should use Operation<T> rather
     * than implementing this interface.
     *
     * @tparam T Type of the successful operation result.
     */
    template <class T>
    class OperationState
    {
    public:
      /**
       * @brief Destroys the internal operation state.
       */
      virtual ~OperationState() = default;

      /**
       * @brief Registers the awaiting coroutine.
       *
       * @param handle Coroutine waiting for completion.
       *
       * @return Internal operation-start state.
       *
       * @note
       * This method is part of the Network engine
       * contract and is not a public extension point.
       */
      virtual bool begin(std::coroutine_handle<>) noexcept = 0;

      /**
       * @brief Commits the coroutine suspension decision.
       *
       * @return true when the coroutine should remain
       *         suspended, false otherwise.
       *
       * @details
       * Coordinates suspension with completion that
       * may occur while the operation is starting.
       *
       * @note
       * The return value follows the C++ coroutine
       * await_suspend() contract.
       */
      virtual bool commit_suspend() noexcept = 0;

      /**
       * @brief Retrieves the terminal operation outcome.
       *
       * @return Completion containing the operation result.
       *
       * @pre
       * The operation must have reached terminal completion.
       */
      virtual vix::Completion<T> take_completion() = 0;

      /**
       * @brief Releases or validates an abandoned awaiter.
       *
       * @details
       * Called when the owning awaiter is destroyed.
       *
       * Abandoning a started, incomplete operation
       * violates the Network lifetime contract.
       *
       * An operation that has already terminalized
       * may safely retire a pending continuation.
       */
      virtual void abandon() noexcept = 0;

      /**
       * @brief Reports whether the operation remains incomplete.
       *
       * @return true if terminal completion has
       *         not yet been reached.
       */
      virtual bool incomplete() const noexcept = 0;

      /**
       * @brief Reports whether the operation has been scheduled.
       *
       * @return Internal scheduling state.
       *
       * @note
       * Intended for Network implementation use.
       */
      virtual bool scheduled() const noexcept = 0;
    };

    /**
     * @brief Internal factory for typed network operations.
     *
     * @internal
     *
     * @tparam T Successful operation result type.
     */
    template <class T>
    class OperationFactory;
  }

  /**
   * @brief Represents a coroutine-awaitable network operation.
   *
   * @tparam T Type returned by a successful operation.
   *
   * @details
   * Operation<T> is the public asynchronous operation
   * type of the Vix Network domain.
   *
   * It represents an operation that can eventually
   * produce a vix::Completion<T>.
   *
   * Operations are returned by networking APIs,
   * including TCP connect, accept, receive, and send.
   *
   * @par Coroutine usage
   * An Operation is consumed through co_await.
   *
   * Inside a coroutine:
   * @code{.cpp}
   * auto completion = co_await provider.connect(endpoint);
   *
   * if (completion.status() ==
   *     vix::CompletionStatus::Success)
   * {
   *     auto connection = completion.take();
   *     // Use the connection.
   * }
   * else if (completion.status() ==
   *          vix::CompletionStatus::Cancelled)
   * {
   *     // The operation was cancelled.
   * }
   * else
   * {
   *     // Handle the operation failure.
   * }
   * @endcode
   *
   * @par Synchronous usage
   * Applications without a coroutine entry point
   * can use the blocking run() helper.
   *
   * @code{.cpp}
   * auto result = run(
   *     provider,
   *     provider.connect(endpoint));
   *
   * if (!result)
   * {
   *     // Runner setup or admission failed.
   * }
   * else
   * {
   *     auto completion = result.take();
   *     // Inspect the operation outcome.
   * }
   * @endcode
   *
   * @par Completion model
   * Completion<T> distinguishes three outcomes:
   * - Success: the operation produced a value.
   * - Failure: the operation reported an Error.
   * - Cancelled: cancellation was observed.
   *
   * Deadline expiration is represented as a
   * Failure containing the Network timed_out error.
   *
   * @par Ownership
   * Operation<T> is movable but not copyable.
   *
   * Each operation has one consuming awaiter.
   * It must be awaited as an rvalue.
   *
   * The operation's state is transferred into
   * the awaiter by operator co_await() &&.
   *
   * @par Execution
   * Constructing an Operation does not submit
   * its underlying work to the Network engine.
   *
   * Work is started when the coroutine awaits
   * the operation or when run() consumes it.
   *
   * @par Lifetime contract
   * Once started, an operation must reach terminal
   * completion before its owning awaiter is destroyed.
   *
   * Destroying an incomplete started awaiter is
   * a contract violation and currently terminates
   * the process.
   *
   * If an operation has already reached terminal
   * completion but its continuation is still queued,
   * the continuation may be safely retired.
   *
   * @par Borrowed buffers
   * Some operations, including receive() and send(),
   * borrow memory provided by the caller.
   *
   * Such buffers must remain valid until the operation
   * reaches terminal completion.
   *
   * A send buffer must not be modified while
   * the operation is outstanding.
   *
   * @warning
   * Cancellation does not automatically make it safe
   * to destroy an incomplete awaiter or its buffers.
   *
   * Wait for terminal completion before releasing
   * resources associated with a started operation.
   *
   * @note
   * Operation<T> is an awaitable result of an operation,
   * not a general-purpose coroutine task or scheduler.
   *
   * @see OperationOptions
   * @see vix::Completion
   * @see Provider
   * @see TcpConnection
   * @see TcpListener
   */
  template <class T>
  class Operation
  {
  public:
    /**
     * @brief Move-constructs an asynchronous operation.
     *
     * @details
     * Transfers the operation state and starter
     * to the destination object.
     *
     * @note
     * An Operation is typically consumed directly
     * as the temporary returned by a Network API.
     */
    Operation(Operation &&) noexcept = default;

    /**
     * @brief Copy construction is not supported.
     *
     * @details
     * An operation has a single consuming owner.
     */
    Operation(const Operation &) = delete;

    /**
     * @brief Move assignment is not supported.
     */
    Operation &operator=(Operation &&) = delete;

    /**
     * @brief Copy assignment is not supported.
     */
    Operation &operator=(const Operation &) = delete;

    /**
     * @brief Coroutine awaiter for a Network operation.
     *
     * @details
     * Awaiter owns the shared operation state and
     * the function used to start the operation.
     *
     * It integrates Network operations with the
     * C++ coroutine suspension protocol.
     *
     * @par Suspension protocol
     * The awaiter:
     * 1. Registers the awaiting coroutine.
     * 2. Starts the operation through its starter.
     * 3. Commits the suspension decision.
     * 4. Retrieves Completion<T> upon resumption.
     *
     * @par Lifetime
     * The awaiter must not be destroyed while
     * a started operation remains incomplete.
     *
     * @note
     * This is an implementation-facing type.
     * Applications should normally use co_await
     * rather than constructing an Awaiter directly.
     */
    struct Awaiter
    {
      /**
       * @brief Shared state of the awaited operation.
       */
      std::shared_ptr<detail::OperationState<T>> state;

      /**
       * @brief Function responsible for starting the operation.
       */
      std::function<void(std::shared_ptr<detail::OperationState<T>>)> starter;

      /**
       * @brief Indicates whether the coroutine can skip suspension.
       *
       * @return false.
       *
       * @details
       * Network operations always enter the
       * await_suspend() protocol.
       *
       * An operation that completes immediately
       * may still avoid suspension when
       * commit_suspend() returns false.
       */
      bool await_ready() const noexcept { return false; }

      /**
       * @brief Registers, starts, and suspends the operation.
       *
       * @param h Handle of the awaiting coroutine.
       *
       * @return true if the coroutine remains suspended,
       *         false if it should continue immediately.
       *
       * @details
       * First registers the coroutine with the
       * operation state.
       *
       * Then invokes the operation starter.
       *
       * Finally commits the suspension decision,
       * accounting for completion that may occur
       * during operation startup.
       *
       * @note
       * Continuation scheduling is managed by
       * the Network operation state machine.
       */
      bool await_suspend(std::coroutine_handle<> h)
      {
        state->begin(h);
        starter(state);
        return state->commit_suspend();
      }

      /**
       * @brief Retrieves the completed operation outcome.
       *
       * @return The terminal Completion<T>.
       *
       * @details
       * Called when the awaiting coroutine resumes
       * after the operation has completed.
       *
       * @par Example
       * @code{.cpp}
       * auto completion = co_await connection.receive(buffer);
       * @endcode
       *
       * @pre
       * The operation must have reached terminal completion.
       *
       * @see vix::Completion
       */
      vix::Completion<T> await_resume() { return state->take_completion(); }

      /**
       * @brief Releases the awaiter's operation state.
       *
       * @details
       * Invokes the operation's abandonment handler
       * when the awaiter still owns a state.
       *
       * Abandonment after terminal completion is
       * supported, including retirement of a queued
       * but no longer needed continuation.
       *
       * @warning
       * Destroying the awaiter while its started
       * operation remains incomplete violates
       * the Network lifetime contract.
       *
       * The current implementation terminates
       * the process in that case.
       */
      ~Awaiter()
      {
        if (state)
          state->abandon();
      }
    };

    /**
     * @brief Transfers the operation into its coroutine awaiter.
     *
     * @return An Awaiter owning the operation state
     *         and starter.
     *
     * @details
     * Consumes an rvalue Operation.
     *
     * This enables natural use of Network APIs:
     *
     * @code{.cpp}
     * auto completion = co_await provider.connect(endpoint);
     * @endcode
     *
     * The operation's state and starter are moved
     * into the returned awaiter.
     *
     * @note
     * The operation is single-consumer.
     * Do not attempt to await it more than once.
     */
    Awaiter operator co_await() && { return {std::move(state_), std::move(starter_)}; }

    /**
     * @brief Prevents awaiting an Operation as an lvalue.
     *
     * @details
     * The consuming awaiter requires ownership
     * of the operation state.
     *
     * Temporaries returned from Network methods
     * can be awaited directly.
     *
     * @par Valid
     * @code{.cpp}
     * auto completion = co_await provider.connect(endpoint);
     * @endcode
     *
     * @par Invalid
     * @code{.cpp}
     * auto operation = provider.connect(endpoint);
     * // co_await operation; // Lvalue awaiting is deleted.
     * @endcode
     *
     * @note
     * An explicitly stored Operation can be consumed
     * as an rvalue when required.
     */
    Awaiter operator co_await() & = delete;

  private:
    using Starter = std::function<void(std::shared_ptr<detail::OperationState<T>>)>;

    Operation(std::shared_ptr<detail::OperationState<T>> state, Starter starter) : state_(std::move(state)), starter_(std::move(starter)) {}

    std::shared_ptr<detail::OperationState<T>> state_;
    Starter starter_;

    friend class detail::OperationFactory<T>;
  };
}
