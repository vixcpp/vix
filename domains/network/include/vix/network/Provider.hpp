/**
 * @file Provider.hpp
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
 * @brief Network execution, TCP connections, and operation progression.
 *
 * @details
 * Provides the main execution interface for the Vix Network domain.
 *
 * Provider manages the networking engine responsible for:
 * - Creating outgoing TCP connections.
 * - Creating TCP listeners.
 * - Progressing asynchronous network operations.
 * - Processing cancellation and deadlines.
 * - Resuming eligible coroutine continuations.
 * - Managing network engine shutdown.
 *
 * Two execution modes are available:
 * - Background execution with Provider::create().
 * - Cooperative execution with Provider::create_cooperative().
 *
 * The run() helper provides a synchronous entry point
 * for applications that do not use coroutines directly.
 *
 * @see vix::network::Provider
 * @see vix::network::run()
 * @see vix::network::Operation
 * @see vix::network::TcpConnection
 * @see vix::network::TcpListener
 */

#pragma once

#include <vix/network/Operation.hpp>
#include <vix/network/TcpConnection.hpp>
#include <vix/network/TcpListener.hpp>
#include <vix/network/Errors.hpp>
#include <vix/Result.hpp>
#include <cstddef>
#include <memory>
#include <condition_variable>
#include <coroutine>
#include <exception>
#include <mutex>
#include <optional>
#include <utility>

namespace vix::network
{
  class Endpoint;
  class TcpConnection;
  class TcpListener;
  class Provider;

  namespace detail
  {
    class NetworkAccess;
    class RunnerAccess;
  }

  /**
   * @brief Runs one asynchronous Network operation synchronously.
   *
   * @tparam T Successful result type of the operation.
   *
   * @param provider Provider responsible for the operation.
   * @param operation Operation to execute and observe.
   *
   * @return A Result containing the terminal Completion<T>,
   *         or an Error if the runner cannot execute.
   *
   * @details
   * This function bridges the asynchronous Network API
   * with synchronous application code.
   *
   * It starts one operation and waits for its terminal
   * completion.
   *
   * On background providers, the existing network reactor
   * continues progressing the operation.
   *
   * On cooperative providers, run() drives the provider
   * on its owning thread until the operation completes.
   *
   * @par Error handling
   * There are two distinct levels of failure:
   *
   * - Result failure: the runner could not be admitted
   *   or started.
   * - Completion failure: the Network operation ran
   *   and reported an operational error.
   *
   * @note
   * run() is a synchronous entry point, not a general
   * coroutine scheduler or replacement for co_await.
   *
   * @warning
   * The call may block until the operation terminalizes.
   * Configure OperationOptions when cancellation or
   * a deadline is required.
   *
   * @see Provider
   * @see Operation
   * @see OperationOptions
   * @see vix::Completion
   */
  template <class T>
  vix::Result<vix::Completion<T>> run(Provider &, Operation<T> &&);

  /**
   * @brief Owns and manages a Network execution engine.
   *
   * @details
   * Provider is the central entry point for TCP networking
   * in the Vix Network domain.
   *
   * It creates TCP connections and listeners, manages
   * asynchronous I/O, and controls the progression
   * and lifetime of the underlying networking engine.
   *
   * @par Background execution
   * Provider::create() creates a provider whose networking
   * operations are progressed by a background reactor.
   *
   * Applications can start operations and await their
   * completion without calling poll().
   *
   * @par Cooperative execution
   * Provider::create_cooperative() creates a provider
   * without background progression threads.
   *
   * Operations progress only when the provider is driven
   * through poll() or the cooperative run() helper.
   *
   * The provider and its resources are bound to the
   * thread that created the cooperative provider.
   *
   * @par Example: Creating a provider
   * @code{.cpp}
   * #include <vix/network/Provider.hpp>
   *
   * int main()
   * {
   *     auto result = vix::network::Provider::create();
   *
   *     if (!result)
   *         return 1;
   *
   *     auto provider = result.take();
   *
   *     // Use provider.connect() or provider.listen().
   *
   *     if (!provider.shutdown())
   *         return 1;
   *
   *     return 0;
   * }
   * @endcode
   *
   * @par Example: Cooperative progression
   * @code{.cpp}
   * auto result = Provider::create_cooperative();
   *
   * if (result)
   * {
   *     auto provider = result.take();
   *
   *     // Start operations before polling.
   *
   *     auto progressed = provider.poll();
   *
   *     if (!progressed)
   *     {
   *         // Handle the progression error.
   *     }
   * }
   * @endcode
   *
   * @par Operation model
   * connect() creates an Operation<TcpConnection>.
   *
   * The operation is submitted when consumed through
   * co_await or run().
   *
   * listen() creates a listening socket synchronously
   * and returns a TcpListener.
   *
   * Incoming connections are accepted asynchronously
   * through TcpListener::accept().
   *
   * @par Ownership
   * Provider is move-constructible but not copyable.
   *
   * Moving transfers ownership of the networking
   * engine handle to the destination object.
   *
   * Connections, listeners, and active operations
   * maintain their required relationships with
   * the provider's internal execution state.
   *
   * @par Shutdown
   * shutdown() requests orderly termination
   * of the networking engine.
   *
   * The provider cannot shut down successfully
   * while dependent network resources or operations
   * remain active.
   *
   * Applications should complete operations,
   * close connections and listeners, and then
   * shut down the provider.
   *
   * @par Cancellation and deadlines
   * Network operations accept OperationOptions.
   *
   * Cancellation and deadline conditions are
   * processed as the networking engine progresses.
   *
   * On cooperative providers, a lack of polling
   * also prevents those conditions from progressing.
   *
   * @warning
   * Do not call poll() recursively or from a thread
   * other than the cooperative provider's owner.
   *
   * @warning
   * Do not abandon a started, incomplete asynchronous
   * operation or release its borrowed buffers.
   *
   * @note
   * The provider manages transport execution.
   * It does not implement application-level protocols
   * such as HTTP or TLS.
   *
   * @see Operation
   * @see OperationOptions
   * @see TcpConnection
   * @see TcpListener
   * @see run()
   */
  class Provider
  {
  public:
    /**
     * @brief Creates a background-driven Network provider.
     *
     * @return An owning Provider on success,
     *         or an Error if initialization fails.
     *
     * @details
     * Initializes the networking engine with
     * background operation progression.
     *
     * The reactor processes:
     * - Pending network commands.
     * - Socket readiness events.
     * - Cancellation and deadline conditions.
     * - Coroutine resumptions.
     *
     * Applications using this mode do not need
     * to call poll() manually.
     *
     * @par Example
     * @code{.cpp}
     * auto result = Provider::create();
     *
     * if (!result)
     * {
     *     // Handle provider creation failure.
     * }
     * else
     * {
     *     auto provider = result.take();
     * }
     * @endcode
     *
     * @note
     * The provider uses the platform's supported
     * native network backend.
     *
     * @see create_cooperative()
     * @see shutdown()
     */
    static vix::Result<Provider> create();

    /**
     * @brief Creates a thread-free cooperative Network provider.
     *
     * @return An owning Provider on success,
     *         or an Error if initialization fails.
     *
     * @details
     * Creates a networking engine without background
     * threads responsible for operation progression.
     *
     * All network operations advance when the
     * application explicitly invokes poll()
     * or uses the cooperative run() helper.
     *
     * @par Thread affinity
     * A cooperative provider is bound to the
     * thread that created it.
     *
     * Its associated resources and operations
     * must respect that thread affinity.
     *
     * Moving the Provider does not transfer
     * its owning-thread identity.
     *
     * @par Example
     * @code{.cpp}
     * auto result = Provider::create_cooperative();
     *
     * if (!result)
     * {
     *     // Handle initialization failure.
     * }
     * else
     * {
     *     auto provider = result.take();
     *
     *     auto progress = provider.poll();
     *
     *     if (!progress)
     *     {
     *         // Handle progression failure.
     *     }
     * }
     * @endcode
     *
     * @warning
     * Operations will not progress without
     * polling or another supported progression
     * mechanism on the owning thread.
     *
     * @note
     * Cooperative execution is useful for
     * applications that want explicit control
     * over network progression.
     *
     * @see create()
     * @see poll()
     * @see run()
     */
    // Creates a thread-free provider. Operations advance only when poll() is
    // called; the provider and all of its resources are thread-affine to the
    // creating thread. poll() is nonblocking and may not be called recursively.
    static vix::Result<Provider> create_cooperative();

    /**
     * @brief Transfers ownership of a Network provider.
     *
     * @param other Provider to move from.
     *
     * @details
     * Transfers the internal engine handle
     * to the destination Provider.
     *
     * @note
     * Moving a cooperative provider does not
     * change its originating thread affinity.
     */
    Provider(Provider &&) noexcept = default;

    /**
     * @brief Copy construction is not supported.
     *
     * @details
     * Provider represents a single owning handle
     * to its Network execution engine.
     */
    Provider(const Provider &) = delete;

    /**
     * @brief Move assignment is not supported.
     *
     * @details
     * Use move construction to transfer ownership.
     */
    Provider &operator=(Provider &&) = delete;

    /**
     * @brief Copy assignment is not supported.
     */
    Provider &operator=(const Provider &) = delete;

    /**
     * @brief Destroys the Network provider handle.
     *
     * @details
     * Releases the Provider's ownership of
     * its internal networking engine state.
     *
     * @note
     * Use shutdown() explicitly when the application
     * needs to observe shutdown failures.
     *
     * @warning
     * Complete outstanding operations and close
     * dependent resources before destroying
     * the owning Provider whenever possible.
     *
     * @see shutdown()
     */
    ~Provider();

    /**
     * @brief Shuts down the Network execution engine.
     *
     * @return Success when shutdown completes,
     *         or an Error if shutdown is unavailable.
     *
     * @details
     * Requests shutdown of the provider's networking
     * engine and its associated progression machinery.
     *
     * A successful shutdown prevents further
     * network operations from being started
     * through the provider.
     *
     * @par Active dependencies
     * Shutdown requires the provider to have
     * no active operations or dependent resources
     * that still require its execution engine.
     *
     * If such dependencies remain, shutdown
     * fails with operation_in_progress.
     *
     * @par Example
     * @code{.cpp}
     * // Complete outstanding operations first.
     * // Close all listeners and connections.
     *
     * auto result = provider.shutdown();
     *
     * if (!result)
     * {
     *     // Handle shutdown failure.
     * }
     * @endcode
     *
     * @par Cooperative providers
     * A cooperative provider must be shut down
     * from its owning thread.
     *
     * @warning
     * Shutdown is not a cancellation mechanism
     * for outstanding asynchronous operations.
     *
     * Ensure operations have terminalized before
     * releasing their required execution engine.
     *
     * @see poll()
     * @see TcpConnection::close()
     * @see TcpListener::close()
     */
    vix::Result<void> shutdown();

    /**
     * @brief Performs one nonblocking Network progression turn.
     *
     * @return The number of progress actions observed,
     *         or an Error if polling is not permitted.
     *
     * @details
     * Advances a cooperative provider by processing:
     * 1. Queued network commands.
     * 2. Expired operation deadlines.
     * 3. Ready native socket events.
     * 4. Eligible coroutine resumptions.
     *
     * The call is nonblocking.
     *
     * A successful return value of zero indicates
     * that no progress action was observed during
     * the current turn.
     *
     * The returned count is not a byte count,
     * connection count, or completion count.
     *
     * @par Example
     * @code{.cpp}
     * auto result = provider.poll();
     *
     * if (!result)
     * {
     *     // Handle progression failure.
     * }
     * else
     * {
     *     auto actions = result.value();
     *     // Inspect the number of progress actions.
     * }
     * @endcode
     *
     * @par Thread affinity
     * poll() must be called from the thread
     * that created the cooperative provider.
     *
     * Recursive polling is not supported.
     *
     * @par Background providers
     * Background providers already progress
     * their operations independently.
     *
     * Calling poll() on such a provider is invalid.
     *
     * @par Errors
     * Polling may fail when:
     * - The provider has stopped.
     * - The provider is not cooperative.
     * - The call violates thread affinity.
     * - Another progression turn is already active.
     *
     * @warning
     * Do not call poll() recursively, including
     * from a coroutine resumed by the same poll turn.
     *
     * @note
     * One poll() call does not guarantee that
     * a pending operation will complete.
     *
     * Applications should continue making progress
     * as long as outstanding operations require it.
     *
     * @see create_cooperative()
     * @see run()
     * @see shutdown()
     */
    // Performs one nonblocking progress turn: queued commands, expired
    // deadlines, ready native events, then eligible coroutine resumptions.
    // Returns the number of progress actions observed. Valid only for a
    // cooperative provider; a background provider is already self-driven.
    vix::Result<std::size_t> poll();

    /**
     * @brief Creates an asynchronous outgoing TCP connection operation.
     *
     * @param endpoint Remote IP address and TCP port.
     * @param options Optional cancellation and deadline settings.
     *
     * @return An Operation<TcpConnection> that produces
     *         an established connection on success.
     *
     * @details
     * Creates an operation for establishing
     * an outgoing TCP connection.
     *
     * The operation is not started immediately.
     *
     * It begins when consumed through co_await
     * or the run() helper.
     *
     * @par Example: Connect synchronously
     * @code{.cpp}
     * auto result = run(
     *     provider,
     *     provider.connect(endpoint));
     *
     * if (!result)
     * {
     *     // Handle runner admission failure.
     * }
     * else
     * {
     *     auto completion = result.take();
     *
     *     if (completion.status() ==
     *         vix::CompletionStatus::Success)
     *     {
     *         auto connection = completion.take();
     *         // Communicate with the TCP peer.
     *     }
     * }
     * @endcode
     *
     * @par Endpoint requirements
     * The endpoint must contain a valid
     * numeric IPv4 or IPv6 address.
     *
     * Its destination port must be nonzero.
     *
     * Port zero is reserved for operations
     * such as automatic listener port allocation.
     *
     * @par Cancellation and deadlines
     * The operation may be configured with
     * a cancellation token or deadline.
     *
     * A cancelled operation does not produce
     * a connected TcpConnection.
     *
     * Deadline expiration is reported as
     * an operational failure.
     *
     * @par Provider lifetime
     * The provider's execution engine must remain
     * available while the connection operation
     * is outstanding.
     *
     * @warning
     * Do not destroy a started, incomplete
     * connection operation.
     *
     * @note
     * TCP connection establishment does not
     * perform DNS resolution.
     *
     * Use Address::parse() to construct
     * a numeric endpoint.
     *
     * @see Endpoint
     * @see TcpConnection
     * @see OperationOptions
     * @see run()
     */
    Operation<TcpConnection> connect(const Endpoint &, OperationOptions = {});

    /**
     * @brief Creates a TCP listener on a local endpoint.
     *
     * @param endpoint Local address and port to bind.
     * @param options Listener configuration.
     *
     * @return A TcpListener on success,
     *         or an Error if listening fails.
     *
     * @details
     * Creates and binds a native TCP socket,
     * then configures it to accept incoming connections.
     *
     * Unlike connect(), listen() is a synchronous
     * operation returning vix::Result<TcpListener>.
     *
     * @par Example
     * @code{.cpp}
     * auto result = provider.listen(endpoint);
     *
     * if (!result)
     * {
     *     // Handle the listening failure.
     * }
     * else
     * {
     *     auto listener = result.take();
     *
     *     // Accept clients through listener.accept().
     * }
     * @endcode
     *
     * @par Automatic port allocation
     * A local endpoint with port zero requests
     * an available port from the operating system.
     *
     * After successful binding, obtain the actual
     * endpoint using TcpListener::local_endpoint().
     *
     * @par Backlog
     * ListenOptions configures the requested size
     * of the pending connection queue.
     *
     * The default backlog is 128.
     *
     * The operating system may impose its own
     * effective backlog limits.
     *
     * @par Provider lifetime
     * The returned listener depends on the
     * provider's execution engine.
     *
     * Close the listener before shutting down
     * the provider.
     *
     * @note
     * Binding to a wildcard address may expose
     * the listener on multiple network interfaces.
     *
     * Choose the local address according to the
     * intended accessibility of the application.
     *
     * @see TcpListener
     * @see ListenOptions
     * @see Endpoint
     */
    vix::Result<TcpListener> listen(const Endpoint &, ListenOptions = {});

  private:
    explicit Provider(std::shared_ptr<void> core) noexcept : core_(std::move(core)) {}
    std::shared_ptr<void> core_;
    friend class detail::NetworkAccess;
    friend class detail::RunnerAccess;
    template <class T>
    friend vix::Result<vix::Completion<T>> run(Provider &, Operation<T> &&);
  };

  namespace detail
  {
    /**
     * @brief Internal access bridge for synchronous operation execution.
     *
     * @internal
     *
     * @details
     * RunnerAccess provides the run() helper with
     * controlled access to Provider execution state.
     *
     * It checks whether synchronous execution
     * is supported and whether a cooperative
     * progression turn can be performed.
     *
     * This class is not a public extension point.
     */
    class RunnerAccess
    {
    public:
      /**
       * @brief Describes whether run() may start an operation.
       *
       * @internal
       */
      enum class Admission
      {
        ready,      ///< Runner execution is permitted.
        stopped,    ///< The provider is stopped.
        reentrant,  ///< Current execution state rejects reentry.
        unsupported ///< The runner is unavailable on this platform.
      };

      /**
       * @brief Performs one internal provider progression turn.
       *
       * @param provider Provider to progress.
       *
       * @return Progress action count or an Error.
       *
       * @internal
       */
      static vix::Result<std::size_t> run_one(Provider &);

      /**
       * @brief Tests whether a Provider uses cooperative execution.
       *
       * @param provider Provider to inspect.
       *
       * @return true for a cooperative provider,
       *         false otherwise.
       *
       * @internal
       */
      static bool cooperative(Provider &) noexcept;

      /**
       * @brief Checks whether synchronous execution is permitted.
       *
       * @param provider Provider to inspect.
       *
       * @return The current runner admission state.
       *
       * @internal
       */
      static Admission admission(Provider &) noexcept;
    };

    /**
     * @brief Shared completion state for a synchronous operation runner.
     *
     * @internal
     *
     * @tparam T Successful result type of the operation.
     *
     * @details
     * RunState coordinates completion publication between
     * the asynchronous observer and the synchronous caller.
     *
     * Its mutex and condition variable protect access
     * to the published Completion<T>.
     *
     * Applications should use run() rather than
     * constructing RunState directly.
     */
    template <class T>
    struct RunState
    {
      std::mutex mutex;
      std::condition_variable changed;
      std::optional<vix::Completion<T>> completion;
      bool ready{};

      /**
       * @brief Publishes the terminal operation completion.
       *
       * @param value Completed operation outcome.
       *
       * @details
       * Stores the completion under the state mutex,
       * marks the result as ready, and wakes
       * any waiting synchronous caller.
       *
       * @internal
       */
      void publish(vix::Completion<T> value)
      {
        {
          std::lock_guard lock(mutex);
          completion.emplace(std::move(value));
          ready = true;
        }
        changed.notify_all();
      }
    };

    /**
     * @brief Internal detached coroutine used by run().
     *
     * @internal
     *
     * @details
     * Provides a minimal coroutine return type
     * for observing one asynchronous Network operation.
     *
     * The coroutine starts immediately and publishes
     * its terminal outcome through RunState.
     *
     * This type is an implementation detail,
     * not an application-facing task abstraction.
     */
    struct DetachedCoroutine
    {
      /**
       * @brief Promise type for a detached operation observer.
       *
       * @internal
       */
      struct promise_type
      {
        /**
         * @brief Returns the detached coroutine object.
         */
        DetachedCoroutine get_return_object() const noexcept { return {}; }

        /**
         * @brief Starts the coroutine without initial suspension.
         */
        std::suspend_never initial_suspend() const noexcept { return {}; }

        /**
         * @brief Completes without a final suspension.
         */
        std::suspend_never final_suspend() const noexcept { return {}; }

        /**
         * @brief Completes the coroutine normally.
         */
        void return_void() const noexcept {}

        /**
         * @brief Terminates when an exception escapes the coroutine.
         *
         * @warning
         * Unhandled exceptions are fatal for this
         * internal detached coroutine.
         */
        void unhandled_exception() const noexcept { std::terminate(); }
      };
    };

    /**
     * @brief Awaits and publishes one Network operation.
     *
     * @internal
     *
     * @tparam T Successful operation result type.
     *
     * @param state Shared completion publication state.
     * @param operation Operation to observe.
     *
     * @details
     * Consumes the operation through co_await
     * and publishes its terminal Completion<T>.
     *
     * The synchronous run() caller observes the
     * published completion through RunState.
     */
    template <class T>
    DetachedCoroutine observe_operation(std::shared_ptr<RunState<T>> state,
                                        Operation<T> operation)
    {
      auto completion = co_await std::move(operation);
      state->publish(std::move(completion));
    }
  }

  /**
   * @brief Executes and synchronously observes one Network operation.
   *
   * @tparam T Successful result type of the operation.
   *
   * @param provider Provider responsible for execution.
   * @param operation Asynchronous operation to consume.
   *
   * @return A Result containing the terminal Completion<T>,
   *         or an Error if runner setup or admission fails.
   *
   * @details
   * This function provides a synchronous entry point
   * to the asynchronous Network operation model.
   *
   * It creates an internal coroutine that awaits
   * the supplied operation and publishes its outcome.
   *
   * The calling thread then waits for that outcome.
   *
   * @par Background providers
   * For a background provider, the underlying reactor
   * progresses the operation independently.
   *
   * The caller waits on a condition variable
   * until the terminal completion is published.
   *
   * @par Cooperative providers
   * For a cooperative provider, run() performs
   * progression turns on the provider's owning thread.
   *
   * This allows one asynchronous operation to be
   * completed without an external polling loop.
   *
   * @par Example
   * @code{.cpp}
   * auto result = run(
   *     provider,
   *     provider.connect(endpoint));
   *
   * if (!result)
   * {
   *     // The runner could not start.
   * }
   * else
   * {
   *     auto completion = result.take();
   *
   *     if (completion.status() ==
   *         vix::CompletionStatus::Success)
   *     {
   *         auto connection = completion.take();
   *         // Use the connection.
   *     }
   *     else if (completion.status() ==
   *              vix::CompletionStatus::Cancelled)
   *     {
   *         // The operation was cancelled.
   *     }
   *     else
   *     {
   *         // Handle the operation failure.
   *     }
   * }
   * @endcode
   *
   * @par Admission errors
   * Before starting the operation, run() checks
   * whether synchronous execution is allowed.
   *
   * It returns:
   * - provider_stopped if the provider is stopped.
   * - unsupported if blocking cooperative execution
   *   is unavailable on the current platform.
   * - operation_in_progress if execution is rejected
   *   because of thread affinity or reentrancy.
   *
   * @par Allocation failures
   * When exceptions are enabled, exceptions caught
   * during runner setup are reported as
   * resource_exhausted.
   *
   * @par Completion errors
   * A successfully started operation returns
   * its Completion<T> even when the operation
   * itself fails or is cancelled.
   *
   * Thus:
   * - Result failure describes runner failure.
   * - Completion failure describes operation failure.
   *
   * @par Lifetime
   * The operation is moved into its observer coroutine
   * and remains owned until terminal completion.
   *
   * Borrowed buffers and resources associated with
   * the operation must remain valid for that duration.
   *
   * @warning
   * run() may block until the operation completes.
   *
   * @warning
   * run() is not a general-purpose scheduler and
   * must not be used to recursively drive an active
   * cooperative progression turn.
   *
   * @note
   * Applications using coroutines directly should
   * ordinarily consume operations through co_await.
   *
   * @see Provider
   * @see Operation
   * @see OperationOptions
   * @see vix::Completion
   */
  // Starts one Network operation and synchronously observes its terminal
  // Completion. Cooperative providers are progressed on their owner thread;
  // background providers continue on their existing reactor. This is an
  // entry-point bridge, not a scheduler or a replacement for co_await.
  template <class T>
  vix::Result<vix::Completion<T>> run(Provider &provider, Operation<T> &&operation)
  {
    const auto admission = detail::RunnerAccess::admission(provider);
    if (admission == detail::RunnerAccess::Admission::stopped)
      return vix::Error{provider_stopped, "cannot run an operation on a stopped provider"};
    if (admission == detail::RunnerAccess::Admission::unsupported)
      return vix::Error{unsupported,
                        "blocking cooperative operation runner is unavailable on this platform"};
    if (admission != detail::RunnerAccess::Admission::ready)
      return vix::Error{operation_in_progress,
                        "Network operation runner is unavailable on this provider thread or state"};

    std::shared_ptr<detail::RunState<T>> state;
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
    try
    {
      state = std::make_shared<detail::RunState<T>>();
      (void)detail::observe_operation<T>(state, std::move(operation));
    }
    catch (...)
    {
      return vix::Error{resource_exhausted,
                        "cannot allocate Network operation runner state"};
    }
#else
    state = std::make_shared<detail::RunState<T>>();
    (void)detail::observe_operation<T>(state, std::move(operation));
#endif

    if (detail::RunnerAccess::cooperative(provider))
    {
      for (;;)
      {
        {
          std::lock_guard lock(state->mutex);
          if (state->ready)
            return vix::Result<vix::Completion<T>>{std::move(*state->completion)};
        }
        auto progressed = detail::RunnerAccess::run_one(provider);
        if (!progressed)
        {
          std::unique_lock lock(state->mutex);
          state->changed.wait(lock, [&]
                              { return state->ready; });
          return vix::Result<vix::Completion<T>>{std::move(*state->completion)};
        }
      }
    }

    std::unique_lock lock(state->mutex);
    state->changed.wait(lock, [&]
                        { return state->ready; });
    return vix::Result<vix::Completion<T>>{std::move(*state->completion)};
  }
}
