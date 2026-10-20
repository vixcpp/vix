/**
 * @file Errors.hpp
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
 * @brief Error identities for the Vix Network domain.
 *
 * @details
 * Defines error identities used by asynchronous network
 * operations, TCP connections, listeners, and providers.
 *
 * All identities belong to the "vix.network" domain.
 *
 * These errors describe failures involving:
 * - Operating-system resources and network I/O.
 * - Provider lifecycle and execution state.
 * - Connection establishment and connectivity.
 * - TCP connection closure and termination.
 * - Concurrent operations and transport ownership.
 * - Cancellation deadlines and unsupported capabilities.
 *
 * Network operations report their outcomes through
 * vix::Completion<T> or vix::Result<T>, depending
 * on the operation.
 *
 * @par Example
 * @code{.cpp}
 * #include <vix/network/Errors.hpp>
 *
 * vix::Error error{
 *     vix::network::connection_refused,
 *     "the remote endpoint refused the connection"
 * };
 * @endcode
 *
 * @par Error domain
 * Error identities are defined using:
 *
 * @code{.cpp}
 * vix::ErrorIdentity::define<"vix.network">(code)
 * @endcode
 *
 * The domain and numeric code together identify
 * an error category independently of its message.
 *
 * @note
 * The identities invalid_address and invalid_port
 * are defined in Address.hpp.
 *
 * @note
 * These identities represent operational failures.
 * Violations of explicit Network lifetime or
 * concurrency contracts may instead terminate
 * the application.
 *
 * @see vix::Error
 * @see vix::ErrorIdentity
 * @see vix::network::Provider
 * @see vix::network::TcpConnection
 * @see vix::network::TcpListener
 * @see vix::network::Operation
 */

#pragma once

#include <vix/Error.hpp>

namespace vix::network
{
  /**
   * @brief A required network resource could not be allocated.
   *
   * @details
   * Indicates resource exhaustion during a Network operation.
   *
   * Possible causes include native socket limits,
   * insufficient operating-system resources, or
   * failure to allocate operation-related state.
   *
   * @note
   * This error may occur during provider initialization,
   * connection creation, or operation execution.
   *
   * Error code: 17.
   *
   * @see Provider::create()
   * @see Provider::connect()
   * @see run()
   */
  inline constexpr auto resource_exhausted = vix::ErrorIdentity::define<"vix.network">(17);

  /**
   * @brief A network input/output operation failed.
   *
   * @details
   * Represents an operating-system network I/O failure
   * that is not classified by a more specific identity.
   *
   * It may occur during socket configuration,
   * communication, or other native networking operations.
   *
   * @note
   * Applications should preserve the accompanying
   * error message for diagnostics.
   *
   * Error code: 18.
   *
   * @see TcpConnection::receive()
   * @see TcpConnection::send()
   * @see Provider::listen()
   */
  inline constexpr auto io_failure = vix::ErrorIdentity::define<"vix.network">(18);

  /**
   * @brief A native network resource could not be closed.
   *
   * @details
   * Indicates that the operating system reported
   * an error while closing a network socket
   * or another owned networking resource.
   *
   * @note
   * Explicit close() operations allow applications
   * to observe closing failures that may not be
   * reported during object destruction.
   *
   * Error code: 19.
   *
   * @see TcpConnection::close()
   * @see TcpListener::close()
   */
  inline constexpr auto close_failed = vix::ErrorIdentity::define<"vix.network">(19);

  /**
   * @brief The network provider is still in use.
   *
   * @details
   * Indicates that the Provider cannot complete
   * a lifecycle operation because resources
   * depending on its execution engine remain in use.
   *
   * Such resources may include active connections,
   * listeners, or outstanding operations.
   *
   * @par Resource cleanup
   * Applications should complete outstanding operations
   * and release dependent network resources before
   * shutting down the provider.
   *
   * Error code: 20.
   *
   * @see Provider::shutdown()
   * @see operation_in_progress
   */
  inline constexpr auto provider_in_use = vix::ErrorIdentity::define<"vix.network">(20);

  /**
   * @brief A network operation exceeded its deadline.
   *
   * @details
   * Indicates that an asynchronous network operation
   * did not reach successful completion before
   * its configured deadline.
   *
   * Deadline expiration is reported as an
   * operational failure, not as cancellation.
   *
   * @warning
   * A timed-out send operation may already have
   * transmitted some of its supplied bytes.
   *
   * @note
   * A timeout does not imply that the underlying
   * connection was automatically closed.
   *
   * Error code: 21.
   *
   * @see OperationOptions
   * @see TcpConnection::send()
   * @see TcpConnection::receive()
   * @see TcpListener::accept()
   */
  inline constexpr auto timed_out = vix::ErrorIdentity::define<"vix.network">(21);

  /**
   * @brief The network provider is no longer running.
   *
   * @details
   * Indicates that an operation was requested
   * after the Provider stopped or became unavailable
   * for further operation execution.
   *
   * @par Example
   * The synchronous run() helper reports this
   * identity when invoked on a stopped provider.
   *
   * Error code: 22.
   *
   * @see Provider::shutdown()
   * @see Provider::poll()
   * @see run()
   */
  inline constexpr auto provider_stopped = vix::ErrorIdentity::define<"vix.network">(22);

  /**
   * @brief The operating system denied a network operation.
   *
   * @details
   * Indicates insufficient permissions for a
   * requested networking capability.
   *
   * Possible examples include binding a restricted
   * local port or accessing a network resource
   * prohibited by operating-system security policies.
   *
   * @note
   * Permission requirements vary between platforms
   * and execution environments.
   *
   * Error code: 23.
   *
   * @see Provider::listen()
   * @see Provider::connect()
   */
  inline constexpr auto permission_denied = vix::ErrorIdentity::define<"vix.network">(23);

  /**
   * @brief The requested network address is unavailable.
   *
   * @details
   * Indicates that the selected address cannot
   * be used for the requested network operation.
   *
   * For example, a local bind operation may fail
   * when the selected address is not assigned
   * to an available local network interface.
   *
   * @note
   * This differs from address_in_use, which identifies
   * an address or port combination already occupied.
   *
   * Error code: 24.
   *
   * @see Provider::listen()
   * @see address_in_use
   */
  inline constexpr auto address_unavailable = vix::ErrorIdentity::define<"vix.network">(24);

  /**
   * @brief A requested networking capability is unsupported.
   *
   * @details
   * Indicates that the current platform or network
   * backend cannot provide the requested functionality.
   *
   * This can include an unavailable execution mode,
   * native socket operation, or synchronous operation
   * runner capability.
   *
   * @par Example
   * run() reports unsupported when the blocking
   * cooperative runner is unavailable on the platform.
   *
   * Error code: 25.
   *
   * @see Provider::create_cooperative()
   * @see run()
   */
  inline constexpr auto unsupported = vix::ErrorIdentity::define<"vix.network">(25);

  /**
   * @brief The remote endpoint refused a TCP connection.
   *
   * @details
   * Indicates that an outgoing TCP connection
   * was actively refused by the destination.
   *
   * This commonly occurs when no service is
   * listening on the requested destination port.
   *
   * @note
   * Connection refusal is distinct from a timeout
   * or a routing failure.
   *
   * Error code: 26.
   *
   * @see Provider::connect()
   * @see timed_out
   * @see network_unreachable
   */
  inline constexpr auto connection_refused = vix::ErrorIdentity::define<"vix.network">(26);

  /**
   * @brief An established TCP connection was reset.
   *
   * @details
   * Indicates that an existing TCP connection
   * was reset instead of completing communication
   * through an orderly stream shutdown.
   *
   * A reset may be observed while receiving
   * or transmitting data.
   *
   * @warning
   * A failed send may already have transmitted
   * a prefix of its supplied buffer.
   *
   * Error code: 27.
   *
   * @see TcpConnection::receive()
   * @see TcpConnection::send()
   */
  inline constexpr auto connection_reset = vix::ErrorIdentity::define<"vix.network">(27);

  /**
   * @brief A TCP connection was aborted.
   *
   * @details
   * Indicates that a connection could not continue
   * because the underlying networking system
   * reported an aborted connection.
   *
   * This differs from ordinary end-of-stream,
   * which is reported by receive() as a
   * successful zero-byte result for a non-empty buffer.
   *
   * Error code: 28.
   *
   * @see TcpConnection::receive()
   * @see TcpConnection::send()
   * @see connection_reset
   */
  inline constexpr auto connection_aborted = vix::ErrorIdentity::define<"vix.network">(28);

  /**
   * @brief The destination network is unreachable.
   *
   * @details
   * Indicates that the operating system cannot
   * route traffic to the requested network.
   *
   * This may occur when no suitable network route
   * or interface is available.
   *
   * @note
   * This is distinct from host_unreachable,
   * which identifies an unreachable destination host.
   *
   * Error code: 29.
   *
   * @see Provider::connect()
   * @see host_unreachable
   */
  inline constexpr auto network_unreachable = vix::ErrorIdentity::define<"vix.network">(29);

  /**
   * @brief The destination host is unreachable.
   *
   * @details
   * Indicates that the operating system cannot
   * reach the specified remote host.
   *
   * This error may arise even when a route
   * to the surrounding network exists.
   *
   * @note
   * The exact cause depends on the platform,
   * routing configuration, and network conditions.
   *
   * Error code: 30.
   *
   * @see Provider::connect()
   * @see network_unreachable
   */
  inline constexpr auto host_unreachable = vix::ErrorIdentity::define<"vix.network">(30);

  /**
   * @brief The network peer or transport is closed.
   *
   * @details
   * Indicates that a requested operation cannot
   * proceed because the relevant connection
   * or listening transport is closed.
   *
   * This identity may be reported when attempting
   * to operate on a previously closed TcpConnection
   * or TcpListener.
   *
   * @note
   * Ordinary TCP receive EOF is reported as
   * successful reception of zero bytes,
   * rather than necessarily using this identity.
   *
   * Error code: 31.
   *
   * @see TcpConnection::close()
   * @see TcpConnection::receive()
   * @see TcpListener::accept()
   */
  inline constexpr auto peer_closed = vix::ErrorIdentity::define<"vix.network">(31);

  /**
   * @brief An operation conflicts with the current network state.
   *
   * @details
   * Indicates that an operation cannot proceed
   * while another incompatible operation or
   * execution state is active.
   *
   * Examples include attempting to close a transport
   * while an asynchronous operation remains active,
   * or calling the operation runner from an
   * unsupported thread or reentrant context.
   *
   * @warning
   * Some invalid concurrency patterns are explicit
   * contract violations and may terminate the process
   * rather than return this error.
   *
   * Error code: 32.
   *
   * @see TcpConnection::close()
   * @see TcpListener::close()
   * @see Provider::poll()
   * @see run()
   */
  inline constexpr auto operation_in_progress = vix::ErrorIdentity::define<"vix.network">(32);

  /**
   * @brief The local TCP write direction has been shut down.
   *
   * @details
   * Indicates that a send operation cannot continue
   * because the connection's local write direction
   * has already been closed.
   *
   * This may occur after a successful call
   * to TcpConnection::shutdown_write().
   *
   * @note
   * A write shutdown does not necessarily prevent
   * further reception from the remote peer.
   *
   * Error code: 33.
   *
   * @see TcpConnection::shutdown_write()
   * @see TcpConnection::send()
   * @see TcpConnection::receive()
   */
  inline constexpr auto write_shutdown = vix::ErrorIdentity::define<"vix.network">(33);

  /**
   * @brief The requested local address is already in use.
   *
   * @details
   * Indicates that a requested socket bind cannot
   * proceed because the address and port combination
   * conflicts with an existing network resource.
   *
   * This commonly occurs when another service
   * is already listening on the requested port.
   *
   * @par Automatic port selection
   * Applications can use port zero when creating
   * an Endpoint for a TCP listener to request
   * an available port from the operating system.
   *
   * @note
   * Binding behavior depends on platform socket
   * options and existing socket state.
   *
   * Error code: 34.
   *
   * @see Provider::listen()
   * @see TcpListener::local_endpoint()
   * @see address_unavailable
   */
  inline constexpr auto address_in_use = vix::ErrorIdentity::define<"vix.network">(34);

  /**
   * @brief The TCP transport has been exclusively transferred.
   *
   * @details
   * Indicates that a transport is no longer available
   * through the ordinary TcpConnection interface
   * because ownership has been transferred to
   * another internal Network component.
   *
   * An operation created before the ownership
   * transfer may subsequently fail with this identity
   * if it attempts to use the transferred transport.
   *
   * @par Exclusive ownership
   * Exclusive transport transfer prevents ordinary
   * TCP operations from competing with the new owner
   * for the same underlying transport.
   *
   * This supports internal protocol integrations
   * that require exclusive control of TCP I/O.
   *
   * @warning
   * Applications must not assume that a previously
   * created operation remains usable after exclusive
   * transport ownership has changed.
   *
   * @note
   * Transport transfer is an internal Network
   * ownership mechanism, not a public
   * TcpConnection operation.
   *
   * Error code: 35.
   *
   * @see TcpConnection
   * @see operation_in_progress
   */
  // The TCP transport has been exclusively transferred to a private Network
  // owner; an already-created ordinary operation cannot use it.
  inline constexpr auto transport_owned = vix::ErrorIdentity::define<"vix.network">(35);
}
