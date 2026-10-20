/**
 * @file TcpListener.hpp
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
 * @brief Asynchronous TCP server listening and connection acceptance.
 *
 * @details
 * Provides the public API for accepting incoming TCP connections.
 *
 * A TcpListener is created through Provider::listen()
 * and allows applications to:
 * - Listen for incoming TCP connections.
 * - Accept connections asynchronously.
 * - Retrieve the bound local endpoint.
 * - Close the listening socket.
 *
 * Incoming connections are represented by TcpConnection.
 *
 * @see vix::network::TcpListener
 * @see vix::network::TcpConnection
 * @see vix::network::Provider
 * @see vix::network::Operation
 */

#pragma once

#include <vix/Result.hpp>
#include <vix/network/Address.hpp>
#include <vix/network/Operation.hpp>
#include <cstdint>
#include <memory>

namespace vix::network
{
  class TcpConnection;

  /**
   * @brief Configures the TCP listener's connection backlog.
   *
   * @details
   * ListenOptions controls the backlog requested
   * when creating a TCP listening socket.
   *
   * The backlog determines the requested capacity
   * of the operating system's pending connection queue.
   *
   * @par Example
   * @code{.cpp}
   * ListenOptions options{};
   * options.backlog = 256;
   *
   * auto result = provider.listen(endpoint, options);
   * @endcode
   *
   * @par Validation
   * The backlog must be greater than zero and
   * fit within the native operating-system range.
   *
   * A zero backlog is rejected with invalid_port.
   * A value exceeding the native integer range
   * is rejected with io_failure.
   *
   * @note
   * The operating system may adjust or limit
   * the effective backlog.
   *
   * @note
   * The backlog does not limit the total number
   * of established connections managed by an application.
   *
   * @see Provider::listen()
   */
  struct ListenOptions
  {
    /**
     * @brief Requested capacity of the pending connection queue.
     *
     * @details
     * Defaults to 128.
     *
     * The value must be nonzero and must not exceed
     * the range accepted by the native listen operation.
     */
    std::uint32_t backlog{128};
  };

  namespace detail
  {
    class TcpListenerState;
    class NetworkAccess;
  }

  /**
   * @brief Accepts incoming TCP connections asynchronously.
   *
   * @details
   * TcpListener represents a TCP socket bound to a local
   * endpoint and configured to accept incoming connections.
   *
   * Create a listener using Provider::listen().
   *
   * Each successful accept() operation produces
   * a TcpConnection representing one established
   * client connection.
   *
   * @par Example: Creating a TCP listener
   * @code{.cpp}
   * #include <vix/network/Provider.hpp>
   *
   * using namespace vix::network;
   *
   * auto address = Address::parse("127.0.0.1");
   *
   * if (!address)
   * {
   *     // Handle invalid_address.
   * }
   * else
   * {
   *     auto endpoint = Endpoint::create(
   *         address.take(),
   *         8080);
   *
   *     if (endpoint)
   *     {
   *         auto provider_result = Provider::create();
   *
   *         if (provider_result)
   *         {
   *             auto provider = provider_result.take();
   *
   *             auto listener_result =
   *                 provider.listen(endpoint.take());
   *
   *             if (listener_result)
   *             {
   *                 auto listener = listener_result.take();
   *
   *                 // The listener can now accept clients.
   *             }
   *         }
   *     }
   * }
   * @endcode
   *
   * @par Example: Accepting a connection
   * @code{.cpp}
   * auto completion = co_await listener.accept();
   *
   * if (completion.status() ==
   *     vix::CompletionStatus::Success)
   * {
   *     auto connection = completion.take();
   *
   *     // Communicate with the accepted client.
   * }
   * @endcode
   *
   * @par Asynchronous acceptance
   * accept() creates an Operation<TcpConnection>.
   *
   * The underlying accept operation starts when
   * the operation is awaited or consumed by run().
   *
   * A successful operation transfers ownership
   * of the accepted connection to the caller.
   *
   * The listening socket remains open and can
   * accept additional clients.
   *
   * @par Concurrent operations
   * Only one accept operation may be active
   * on a listener at a time.
   *
   * Starting another accept before the previous
   * operation reaches terminal completion violates
   * the listener contract and terminates the process.
   *
   * Once the previous operation has terminalized,
   * another accept operation may begin.
   *
   * @par Cancellation and deadlines
   * accept() supports OperationOptions containing
   * an optional cancellation token and deadline.
   *
   * Cancellation may produce a Cancelled completion.
   *
   * Deadline expiration produces a Failure completion
   * containing the timed_out error identity.
   *
   * Neither outcome automatically closes the listener.
   *
   * @par Ownership
   * TcpListener is move-constructible but not copyable.
   *
   * Moving transfers ownership of its internal state
   * to the destination.
   *
   * The moved-from object no longer owns
   * a usable listening socket.
   *
   * @par Provider lifetime
   * A listener retains a dependency on the Provider
   * that created it.
   *
   * A Provider cannot successfully shut down while
   * listening resources or associated operations
   * remain active.
   *
   * Close the listener and complete any outstanding
   * operations before shutting down the Provider.
   *
   * @par Cooperative providers
   * Listeners created through a cooperative Provider
   * are bound to the Provider's owning thread.
   *
   * Their operations must respect that thread affinity.
   *
   * The Provider must make progress through poll()
   * for pending asynchronous operations to complete.
   *
   * @warning
   * Do not destroy the awaiter of a started,
   * incomplete accept operation.
   *
   * @warning
   * close() cannot close the listener while an
   * accept operation remains active.
   *
   * Wait for the operation to terminalize before
   * attempting closure.
   *
   * @note
   * TCP is a byte-stream protocol.
   *
   * TcpListener accepts connections but does not
   * implement application-level protocols such
   * as HTTP or message framing.
   *
   * @see Provider::listen()
   * @see TcpConnection
   * @see Operation
   * @see OperationOptions
   * @see ListenOptions
   */
  class TcpListener
  {
  public:
    /**
     * @brief Transfers ownership of a TCP listener.
     *
     * @param other Listener whose ownership is transferred.
     *
     * @details
     * Move construction transfers the internal
     * listener state to the destination object.
     *
     * The moved-from listener no longer owns
     * the listening socket.
     *
     * @note
     * Moving a listener does not create another
     * native listening socket.
     *
     * @warning
     * Calling accept() on a moved-from listener
     * violates the contract and terminates the process.
     */
    TcpListener(TcpListener &&) noexcept;

    /**
     * @brief Copy construction is not supported.
     *
     * @details
     * A TcpListener provides exclusive public
     * ownership of its listening state.
     */
    TcpListener(const TcpListener &) = delete;

    /**
     * @brief Move assignment is not supported.
     *
     * @details
     * Use move construction to transfer ownership.
     */
    TcpListener &operator=(TcpListener &&) = delete;

    /**
     * @brief Destroys the TCP listener object.
     *
     * @details
     * Releases ownership of the listener's
     * internal state.
     *
     * When no outstanding operation retains the
     * listening state, its underlying resources
     * are released.
     *
     * @note
     * Destructor-based cleanup does not report
     * native socket closing errors.
     *
     * Use close() explicitly when closing errors
     * must be observed.
     *
     * @warning
     * Destruction is not a substitute for
     * completing an active accept operation.
     *
     * @see close()
     */
    ~TcpListener();

    /**
     * @brief Asynchronously accepts an incoming TCP connection.
     *
     * @param options Optional cancellation and deadline settings.
     *
     * @return An Operation that produces a TcpConnection
     *         when a client connection is accepted.
     *
     * @details
     * Creates an asynchronous operation that waits
     * for an incoming TCP connection.
     *
     * Upon successful completion, the accepted native
     * socket is represented by a new TcpConnection.
     *
     * The listener remains available for subsequent
     * accept operations.
     *
     * @par Example
     * @code{.cpp}
     * auto completion = co_await listener.accept();
     *
     * if (completion.status() ==
     *     vix::CompletionStatus::Success)
     * {
     *     auto client = completion.take();
     *
     *     // Receive or send data using client.
     * }
     * else if (completion.status() ==
     *          vix::CompletionStatus::Cancelled)
     * {
     *     // The accept operation was cancelled.
     * }
     * else
     * {
     *     // Handle the acceptance failure.
     * }
     * @endcode
     *
     * @par Sequential acceptance
     * Only one accept operation may be active
     * on this listener at a time.
     *
     * After an accept operation reaches terminal
     * completion, another accept may be started.
     *
     * @par Cancellation
     * Cancellation is configured through
     * OperationOptions::cancellation.
     *
     * Cancelling an accept operation does not
     * automatically close the listening socket.
     *
     * @par Deadline
     * When the configured deadline expires,
     * the operation fails with timed_out.
     *
     * Deadline expiration does not terminate
     * the Provider or close the listener.
     *
     * @par Closed listeners
     * An accept operation started on a closed
     * listener fails with peer_closed.
     *
     * @par Cooperative execution
     * When using a cooperative Provider,
     * acceptance progresses through Provider::poll().
     *
     * Starting the operation from a different thread
     * produces an operation_in_progress failure.
     *
     * @warning
     * Starting two active accept operations on the same
     * listener is a contract violation and terminates
     * the process.
     *
     * @warning
     * Destroying a started, incomplete awaiter
     * violates the Network lifetime contract.
     *
     * @note
     * Creating the Operation does not immediately
     * accept a connection.
     *
     * Execution begins when the operation is consumed.
     *
     * @see TcpConnection
     * @see OperationOptions
     * @see close()
     */
    Operation<TcpConnection> accept(OperationOptions = {});

    /**
     * @brief Returns the local endpoint of the listener.
     *
     * @return The bound Endpoint on success,
     *         or an Error if the listener is closed.
     *
     * @details
     * Returns the actual local IP address and port
     * associated with the listening socket.
     *
     * This is particularly useful when the listener
     * was created with port zero.
     *
     * In that case, the operating system assigns
     * an available port during binding.
     *
     * @par Example: Discover an assigned port
     * @code{.cpp}
     * auto endpoint = listener.local_endpoint();
     *
     * if (endpoint)
     * {
     *     auto port = endpoint.value().port();
     *     auto address = endpoint.value().address().display();
     *
     *     // Use the actual bound address and port.
     * }
     * @endcode
     *
     * @par Closed listeners
     * Calling local_endpoint() on a closed
     * listener returns peer_closed.
     *
     * The same error is returned when the
     * TcpListener has been moved-from.
     *
     * @note
     * The returned Endpoint is a value,
     * independent of the listener's lifetime.
     *
     * @note
     * A bound wildcard address does not identify
     * which network interface a client will use.
     *
     * @see Endpoint
     * @see Provider::listen()
     */
    vix::Result<Endpoint> local_endpoint() const;

    /**
     * @brief Closes the TCP listening socket.
     *
     * @return Success if closing completes,
     *         or an Error if closure fails.
     *
     * @details
     * Stops the listener from accepting new
     * incoming connections and releases its
     * native listening socket.
     *
     * Closing the listener does not close
     * previously accepted TcpConnection objects.
     *
     * @par Example
     * @code{.cpp}
     * auto result = listener.close();
     *
     * if (!result)
     * {
     *     // Handle the close error.
     * }
     * @endcode
     *
     * @par Active accept operations
     * If an accept operation is active,
     * close() returns operation_in_progress.
     *
     * The active operation must reach terminal
     * completion before closure can proceed.
     *
     * @par Repeated calls
     * Closing an already closed listener succeeds
     * when no previous close error was recorded.
     *
     * If native socket closing failed, the
     * recorded error is returned again.
     *
     * Calling close() on a moved-from listener
     * succeeds without native I/O.
     *
     * @par Cooperative providers
     * On a cooperative Provider, close() must
     * be called from the Provider's owning thread.
     *
     * An attempt from another thread returns
     * operation_in_progress.
     *
     * @warning
     * close() does not implicitly cancel or
     * abandon an outstanding accept operation.
     *
     * @note
     * Previously accepted connections remain
     * independently usable after listener closure.
     *
     * @see accept()
     * @see local_endpoint()
     * @see Provider::shutdown()
     */
    vix::Result<void> close();

  private:
    explicit TcpListener(std::shared_ptr<detail::TcpListenerState>) noexcept;
    std::shared_ptr<detail::TcpListenerState> state_;
    friend class Provider;
    friend class detail::NetworkAccess;
  };
}
