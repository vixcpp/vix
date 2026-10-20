/**
 * @file TcpConnection.hpp
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
 * @brief Asynchronous TCP connection management.
 *
 * @details
 * Provides TcpConnection, the public abstraction
 * for communicating with a connected TCP peer.
 *
 * A TCP connection supports:
 * - Asynchronous byte reception.
 * - Asynchronous transmission of complete buffers.
 * - Independent read and write directions.
 * - Half-closing the local write direction.
 * - Explicit connection closure.
 *
 * Network I/O uses Operation<T> and Completion<T>
 * to represent asynchronous execution and its outcome.
 *
 * @see vix::network::TcpConnection
 * @see vix::network::Operation
 * @see vix::network::Provider
 */

#pragma once

#include <vix/Result.hpp>
#include <vix/network/Operation.hpp>
#include <cstddef>
#include <memory>
#include <span>

namespace vix::network
{
  namespace detail
  {
    class TcpConnectionState;
    class NetworkAccess;
  }

  /**
   * @brief Represents an established TCP connection.
   *
   * @details
   * TcpConnection provides asynchronous communication
   * over an established TCP socket.
   *
   * Connections are obtained through:
   * - Provider::connect() for outgoing connections.
   * - TcpListener::accept() for incoming connections.
   *
   * Applications can receive bytes, send bytes,
   * stop writing while continuing to receive,
   * or close the connection completely.
   *
   * @par Example: Receiving data
   * @code{.cpp}
   * #include <array>
   * #include <vix/network/TcpConnection.hpp>
   *
   * std::array<std::byte, 4096> buffer{};
   *
   * auto completion = co_await connection.receive(buffer);
   *
   * if (completion.status() ==
   *     vix::CompletionStatus::Success)
   * {
   *     auto count = completion.value();
   *
   *     if (count == 0)
   *     {
   *         // End-of-file or an empty buffer.
   *     }
   *     else
   *     {
   *         // Process buffer[0 .. count).
   *     }
   * }
   * @endcode
   *
   * @par Example: Sending data
   * @code{.cpp}
   * #include <array>
   *
   * std::array<std::byte, 3> message{
   *     std::byte{0x41},
   *     std::byte{0x42},
   *     std::byte{0x43}
   * };
   *
   * auto completion = co_await connection.send(message);
   *
   * if (completion.status() ==
   *     vix::CompletionStatus::Success)
   * {
   *     // All three bytes were accepted by the socket.
   * }
   * @endcode
   *
   * @par Ownership
   * TcpConnection is movable but not copyable.
   *
   * Moving transfers ownership of the public
   * connection object to its destination.
   *
   * The moved-from object no longer contains
   * a usable connection state.
   *
   * The internal implementation manages the
   * lifetime of the socket and any active operations.
   *
   * @par Asynchronous execution
   * receive() and send() create Operation<T> values.
   *
   * The underlying I/O begins when the operation
   * is consumed through co_await or an appropriate
   * operation runner.
   *
   * Operations support cancellation and deadlines
   * through OperationOptions.
   *
   * @par Independent directions
   * TCP provides independent reading and writing
   * directions.
   *
   * One receive operation and one send operation
   * may be active concurrently.
   *
   * Starting a second receive while a receive
   * is active violates the connection contract.
   *
   * Starting a second send while a send
   * is active also violates the contract.
   *
   * @par Borrowed memory
   * The buffers supplied to receive() and send()
   * are borrowed rather than copied.
   *
   * Their storage must remain valid until the
   * corresponding operation reaches terminal completion.
   *
   * The contents of a send buffer must remain
   * unchanged while the operation is outstanding.
   *
   * @par Completion behavior
   * receive() reports the number of bytes obtained.
   * A successful zero-byte read with a non-empty
   * buffer indicates end-of-file.
   *
   * send() succeeds only after the complete supplied
   * buffer has been accepted by the socket.
   *
   * An unsuccessful send may nevertheless have
   * transmitted a prefix of the supplied data.
   *
   * @par Connection closure
   * shutdown_write() closes only the local
   * sending direction.
   *
   * close() closes the entire connection.
   *
   * Neither operation should be used to interrupt
   * active asynchronous I/O.
   *
   * close() reports errors::operation_in_progress
   * while a send or receive operation is active.
   *
   * @par Cooperative providers
   * Connections associated with a cooperative Provider
   * are bound to the provider's owning thread.
   *
   * Operations performed from another thread
   * may be rejected with errors::operation_in_progress.
   *
   * @warning
   * Do not destroy a coroutine awaiter while its
   * started network operation remains incomplete.
   *
   * @warning
   * Do not release or modify borrowed buffers
   * before the associated operation terminalizes.
   *
   * @note
   * TCP transports a byte stream.
   *
   * It does not preserve application message boundaries
   * or guarantee that one send corresponds to one receive.
   *
   * @see Provider
   * @see TcpListener
   * @see Operation
   * @see OperationOptions
   */
  class TcpConnection
  {
  public:
    /**
     * @brief Transfers ownership of a TCP connection.
     *
     * @param other Connection to move from.
     *
     * @details
     * Move construction transfers the internal
     * connection state to the destination.
     *
     * The moved-from connection must not be used
     * for further network I/O.
     *
     * @note
     * Moving a connection does not establish
     * a new TCP connection.
     */
    TcpConnection(TcpConnection &&) noexcept;

    /**
     * @brief Copy construction is not supported.
     *
     * @details
     * TcpConnection provides exclusive public ownership
     * of its connection state.
     */
    TcpConnection(const TcpConnection &) = delete;

    /**
     * @brief Move assignment is not supported.
     *
     * @details
     * Use move construction to transfer ownership.
     */
    TcpConnection &operator=(TcpConnection &&) = delete;

    /**
     * @brief Copy assignment is not supported.
     */
    TcpConnection &operator=(const TcpConnection &) = delete;

    /**
     * @brief Destroys the TCP connection object.
     *
     * @details
     * Releases the object's ownership of its
     * internal connection state.
     *
     * The implementation manages the lifetime
     * of the underlying native socket.
     *
     * @note
     * Active asynchronous operations retain
     * the state they require until completion.
     *
     * @note
     * Destructor cleanup does not expose
     * native socket closing errors.
     *
     * Use close() when such errors must
     * be observed explicitly.
     *
     * @warning
     * Destruction is not a substitute for correctly
     * completing active network operations.
     *
     * @see close()
     */
    ~TcpConnection();

    /**
     * @brief Closes the TCP connection.
     *
     * @return Success if closing completes,
     *         or an Error if closing fails.
     *
     * @details
     * Releases the native socket and marks
     * the connection as closed.
     *
     * Both reading and writing become unavailable
     * after successful closure.
     *
     * @par Example
     * @code{.cpp}
     * auto result = connection.close();
     *
     * if (!result)
     * {
     *     // Handle the close error.
     * }
     * @endcode
     *
     * @par Active operations
     * If a receive or send operation is active,
     * close() returns errors::operation_in_progress.
     *
     * The connection remains available for the
     * active operation to reach terminal completion.
     *
     * @par Repeated calls
     * Closing an already closed connection is supported.
     *
     * A previously recorded native close error
     * is returned on subsequent calls.
     *
     * Otherwise, repeated closing succeeds.
     *
     * Calling close() on a moved-from TcpConnection
     * also succeeds without performing native I/O.
     *
     * @warning
     * Do not use close() to cancel an outstanding
     * send or receive operation.
     *
     * Use the operation's cancellation mechanism
     * and observe its terminal completion first.
     *
     * @note
     * Explicit closure does not guarantee that
     * previously transmitted bytes were consumed
     * by the remote application.
     *
     * @see shutdown_write()
     * @see receive()
     * @see send()
     */
    vix::Result<void> close();

    /**
     * @brief Shuts down the local TCP write direction.
     *
     * @return Success if write shutdown completes,
     *         or an Error if it cannot be performed.
     *
     * @details
     * Requests a TCP half-close of the local
     * sending direction.
     *
     * After successful shutdown, the connection
     * can still receive data from the peer.
     *
     * The peer can observe end-of-stream on its
     * receiving side after consuming previously
     * queued data.
     *
     * @par Example
     * @code{.cpp}
     * auto result = connection.shutdown_write();
     *
     * if (!result)
     * {
     *     // Handle the shutdown error.
     * }
     *
     * // Receiving may continue.
     * @endcode
     *
     * @par Repeated calls
     * Repeating shutdown_write() on an already
     * write-shutdown connection succeeds.
     *
     * @par Active send
     * If a send operation is active, shutdown_write()
     * returns errors::operation_in_progress.
     *
     * A receive operation may remain active.
     *
     * @par Closed connections
     * If the connection is already closed,
     * shutdown_write() returns errors::peer_closed.
     *
     * @warning
     * After successful write shutdown, subsequent
     * send operations cannot transmit data.
     *
     * They complete with errors::write_shutdown.
     *
     * @note
     * Half-closing the write direction does not
     * close the entire connection.
     *
     * @note
     * A successful shutdown does not guarantee
     * that the peer application has processed
     * the previously sent bytes.
     *
     * @see close()
     * @see receive()
     * @see send()
     */
    // Half-closes only the local write direction; receiving remains available.
    vix::Result<void> shutdown_write();

    /**
     * @brief Asynchronously receives bytes from the TCP peer.
     *
     * @param buffer Writable storage receiving incoming bytes.
     * @param options Optional cancellation and deadline settings.
     *
     * @return An Operation producing the number of received
     *         bytes on successful completion.
     *
     * @details
     * Creates an asynchronous receive operation.
     *
     * The operation reads up to buffer.size() bytes
     * from the TCP connection.
     *
     * A successful receive may return fewer bytes
     * than requested.
     *
     * A result of zero means:
     * - The supplied buffer is empty.
     * - Or the peer has reached end-of-stream.
     *
     * With a non-empty buffer, successful zero-byte
     * reception indicates EOF.
     *
     * @par Example
     * @code{.cpp}
     * std::array<std::byte, 4096> buffer{};
     *
     * auto completion =
     *     co_await connection.receive(buffer);
     *
     * if (completion.status() ==
     *     vix::CompletionStatus::Success)
     * {
     *     auto count = completion.value();
     *
     *     if (count == 0)
     *     {
     *         // End of stream.
     *     }
     *     else
     *     {
     *         // Process buffer[0 .. count).
     *     }
     * }
     * @endcode
     *
     * @par Buffer lifetime
     * The supplied buffer is borrowed.
     *
     * Its storage must remain valid and writable
     * until terminal completion of the operation.
     *
     * Do not access the buffer concurrently in
     * a manner that conflicts with the receive.
     *
     * @par Concurrent operations
     * One receive and one send may be active
     * concurrently on the same connection.
     *
     * Starting a second receive before the first
     * has terminalized violates the contract
     * and terminates the process.
     *
     * @par Cancellation and deadlines
     * OperationOptions allows cancellation or
     * a deadline to end the receive operation.
     *
     * Cancellation produces a Cancelled completion.
     *
     * Deadline expiration produces a Failure
     * completion containing errors::timed_out.
     *
     * @par Empty buffers
     * An empty buffer completes successfully
     * with a count of zero without receiving
     * data from the network.
     *
     * @warning
     * Do not interpret zero as EOF when the
     * supplied buffer was empty.
     *
     * @warning
     * Destroying an incomplete started operation
     * violates the Network lifetime contract.
     *
     * @note
     * TCP is a byte stream, so an individual
     * receive does not correspond to a complete
     * application-level message.
     *
     * @note
     * Receiving remains available after a
     * successful shutdown_write().
     *
     * @see OperationOptions
     * @see Operation
     * @see send()
     * @see shutdown_write()
     */
    // The supplied storage is borrowed until the operation reaches terminal completion.
    // EOF is reported as Success(0).
    Operation<std::size_t> receive(std::span<std::byte>, OperationOptions = {});

    /**
     * @brief Asynchronously sends bytes to the TCP peer.
     *
     * @param buffer Read-only storage containing bytes to send.
     * @param options Optional cancellation and deadline settings.
     *
     * @return An Operation<void> that succeeds when
     *         every supplied byte has been accepted
     *         by the local socket.
     *
     * @details
     * Creates an asynchronous send operation.
     *
     * Unlike a single native socket write, send()
     * internally continues sending until the
     * entire supplied buffer has been accepted
     * or the operation reaches another terminal outcome.
     *
     * @par Example
     * @code{.cpp}
     * std::array<std::byte, 3> bytes{
     *     std::byte{0x41},
     *     std::byte{0x42},
     *     std::byte{0x43}
     * };
     *
     * auto completion =
     *     co_await connection.send(bytes);
     *
     * if (completion.status() ==
     *     vix::CompletionStatus::Success)
     * {
     *     // All supplied bytes were accepted.
     * }
     * @endcode
     *
     * @par Complete send
     * Successful completion means that the entire
     * buffer was accepted by the socket.
     *
     * It does not guarantee delivery to or processing
     * by the remote application.
     *
     * @par Partial transmission on failure
     * A failed, timed-out, or cancelled send may
     * already have transmitted a prefix of the data.
     *
     * The operation does not roll back bytes
     * that were previously accepted by the socket.
     *
     * @par Buffer lifetime
     * The supplied bytes are borrowed.
     *
     * The buffer must remain valid and unchanged
     * until the operation reaches terminal completion.
     *
     * Modifying or releasing the data while
     * the operation is outstanding violates
     * the borrowed-buffer contract.
     *
     * @par Concurrent operations
     * One send and one receive may be active
     * concurrently on the same connection.
     *
     * Starting another send while a send
     * is active violates the contract and
     * terminates the process.
     *
     * @par Empty buffers
     * An empty send completes successfully
     * without performing native socket I/O,
     * provided that the connection is open
     * and its write direction is available.
     *
     * @par Cancellation and deadlines
     * A cancellation request may produce
     * a Cancelled completion.
     *
     * Deadline expiration produces a Failure
     * completion containing errors::timed_out.
     *
     * Neither outcome guarantees that zero
     * bytes were transmitted.
     *
     * @par Write shutdown
     * After shutdown_write() succeeds,
     * send() returns an operation that fails
     * with errors::write_shutdown.
     *
     * @warning
     * Do not retry an unsuccessful send under
     * the assumption that no data was transmitted.
     *
     * The remote peer may already have received
     * part of the original buffer.
     *
     * @warning
     * Do not destroy an incomplete started send
     * operation or release its borrowed buffer.
     *
     * @note
     * TCP does not preserve message boundaries.
     * Applications requiring framed messages
     * must implement framing at a higher layer.
     *
     * @see OperationOptions
     * @see Operation
     * @see receive()
     * @see shutdown_write()
     */
    // The supplied bytes are borrowed and must not change until terminal completion.
    // Success means the complete span was accepted by the socket; a non-success
    // may have transmitted a prefix.
    Operation<void> send(std::span<const std::byte>, OperationOptions = {});

  private:
    explicit TcpConnection(std::shared_ptr<detail::TcpConnectionState>) noexcept;
    std::shared_ptr<detail::TcpConnectionState> state_;
    friend class detail::NetworkAccess;
  };
}
