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
 * @brief Error identities for the Vix Process domain.
 *
 * @details
 * Defines the stable error identities used by operations
 * in the vix::process namespace.
 *
 * All identities belong to the "vix.process" domain
 * and use distinct numeric codes.
 *
 * These identities distinguish failures involving:
 * - Executable selection and process creation.
 * - Arguments and environment configuration.
 * - Working directories and standard streams.
 * - Process waiting, observation, and termination.
 * - Pipe I/O and resource cleanup.
 * - Unsupported platform capabilities.
 *
 * Errors are reported through vix::Result<T> or
 * vix::Completion<T>, depending on the operation.
 *
 * @note
 * ErrorIdentity values identify error categories.
 * They are not exceptions and do not themselves
 * contain operating-system diagnostic details.
 *
 * @see vix::ErrorIdentity
 * @see vix::process::spawn()
 * @see vix::process::Process
 */

/**
 * @namespace vix::process::errors
 * @brief Error identities for process operations.
 *
 * @details
 * Each identity uses the "vix.process" domain
 * and a unique numeric code.
 *
 * Applications can use these identities to
 * distinguish expected operational failures
 * without relying on platform-specific error messages.
 */

#ifndef VIX_PROCESS_ERRORS_HPP
#define VIX_PROCESS_ERRORS_HPP

#include <vix/ErrorIdentity.hpp>

namespace vix::process::errors
{
  /**
   * @brief The requested executable could not be found.
   *
   * @details
   * Returned when process creation cannot locate
   * the selected executable.
   *
   * This may occur when resolving an explicit
   * executable path or searching for a program
   * through the configured PATH environment.
   *
   * @note
   * Successfully creating an Executable does not
   * guarantee that the program exists at spawn time.
   *
   * @see Executable::path()
   * @see Executable::search_path()
   * @see spawn()
   *
   * Error code: 1.
   */
  inline constexpr auto executable_not_found = ErrorIdentity::define<"vix.process">(1);

  /**
   * @brief The selected filesystem object cannot be executed.
   *
   * @details
   * Indicates that an executable candidate is not
   * suitable for execution.
   *
   * For example, the selected path may identify
   * an object that is not an executable program.
   *
   * @see executable_not_found
   * @see access_denied
   * @see spawn()
   *
   * Error code: 2.
   */
  inline constexpr auto not_executable = ErrorIdentity::define<"vix.process">(2);

  /**
   * @brief Access to a required process resource was denied.
   *
   * @details
   * Indicates that the operating system rejected
   * an operation because of access restrictions.
   *
   * This may involve executable permissions or
   * another protected resource needed during
   * process creation.
   *
   * @note
   * Permission behavior depends on the operating
   * system and its security policies.
   *
   * @see not_executable
   * @see spawn()
   *
   * Error code: 3.
   */
  inline constexpr auto access_denied = ErrorIdentity::define<"vix.process">(3);

  /**
   * @brief An executable name or process argument is invalid.
   *
   * @details
   * Indicates that a supplied argument violates
   * the Process domain's input requirements.
   *
   * Examples include:
   * - Invalid UTF-8 in an argument.
   * - Embedded NUL characters.
   * - Invalid executable search names.
   *
   * Executable search names must not contain
   * directory separators.
   *
   * @see Executable::search_path()
   * @see ProcessSpec::add_argument()
   *
   * Error code: 4.
   */
  inline constexpr auto invalid_argument = ErrorIdentity::define<"vix.process">(4);

  /**
   * @brief An environment variable name or value is invalid.
   *
   * @details
   * Returned when an environment configuration
   * contains invalid input.
   *
   * Environment variable names must be non-empty,
   * valid UTF-8 strings without '=' or embedded NUL.
   *
   * Values must be valid UTF-8 without embedded NUL.
   * Empty values are permitted.
   *
   * @see Environment::with()
   * @see Environment::without()
   *
   * Error code: 5.
   */
  inline constexpr auto invalid_environment = ErrorIdentity::define<"vix.process">(5);

  /**
   * @brief The requested working directory is invalid.
   *
   * @details
   * Indicates that a child process cannot use
   * the configured working directory.
   *
   * An empty directory path is rejected during
   * ProcessSpec configuration.
   *
   * Other directory-related failures may be
   * discovered when spawn() prepares the child.
   *
   * @see ProcessSpec::set_working_directory()
   * @see spawn()
   *
   * Error code: 6.
   */
  inline constexpr auto invalid_working_directory = ErrorIdentity::define<"vix.process">(6);

  /**
   * @brief A standard stream configuration is invalid.
   *
   * @details
   * Indicates that stdin, stdout, or stderr cannot
   * be configured as requested.
   *
   * This may involve an unsuitable native handle
   * or an unsupported stream configuration during
   * process creation.
   *
   * @see StandardInput
   * @see StandardOutput
   * @see StandardError
   * @see spawn()
   *
   * Error code: 7.
   */
  inline constexpr auto invalid_stdio = ErrorIdentity::define<"vix.process">(7);

  /**
   * @brief The requested standard stream was not configured as a pipe.
   *
   * @details
   * Returned when an application attempts to
   * retrieve a pipe endpoint that was not created.
   *
   * For example, calling Process::take_stdout()
   * when stdout was configured with inherit()
   * instead of pipe().
   *
   * @see Process::take_stdin()
   * @see Process::take_stdout()
   * @see Process::take_stderr()
   *
   * Error code: 8.
   */
  inline constexpr auto stdio_not_piped = ErrorIdentity::define<"vix.process">(8);

  /**
   * @brief A standard stream pipe has already been transferred.
   *
   * @details
   * Returned when an application attempts to
   * take ownership of a pipe endpoint more than once.
   *
   * Each configured parent-side pipe endpoint
   * can be transferred from Process only once.
   *
   * @note
   * This differs from stdio_not_piped, which
   * indicates that no pipe was configured.
   *
   * @see Process::take_stdin()
   * @see Process::take_stdout()
   * @see Process::take_stderr()
   *
   * Error code: 9.
   */
  inline constexpr auto pipe_already_transferred = ErrorIdentity::define<"vix.process">(9);

  /**
   * @brief The receiving end of a pipe is unavailable.
   *
   * @details
   * Indicates that a write cannot continue because
   * the corresponding pipe reader has closed its endpoint
   * or is otherwise no longer available.
   *
   * This commonly occurs when writing to the stdin
   * of a child process that has exited or closed stdin.
   *
   * @note
   * Successfully starting a child process does not
   * guarantee that it will continue accepting input.
   *
   * @see ChildInput::write_some()
   *
   * Error code: 10.
   */
  inline constexpr auto broken_pipe = ErrorIdentity::define<"vix.process">(10);

  /**
   * @brief An operating-system resource could not be allocated.
   *
   * @details
   * Indicates that the operating system could not
   * provide a resource required by a process operation.
   *
   * Examples may include:
   * - Process creation resources.
   * - Native handles or descriptors.
   * - Pipe resources.
   *
   * @note
   * Resource availability depends on system limits,
   * permissions, and current system load.
   *
   * @see spawn()
   *
   * Error code: 11.
   */
  inline constexpr auto resource_exhausted = ErrorIdentity::define<"vix.process">(11);

  /**
   * @brief The operating system failed to create a child process.
   *
   * @details
   * Represents a process creation failure that
   * cannot be classified more specifically.
   *
   * The failure occurs while preparing or launching
   * a child from its ProcessSpec.
   *
   * @see spawn()
   * @see ProcessSpec
   * @see resource_exhausted
   *
   * Error code: 12.
   */
  inline constexpr auto spawn_failed = ErrorIdentity::define<"vix.process">(12);

  /**
   * @brief Waiting for a child process failed.
   *
   * @details
   * Indicates that the operating system could
   * not complete a process waiting operation.
   *
   * This is distinct from a child process exiting
   * with a non-zero code.
   *
   * A non-zero child exit code is represented
   * by a successfully obtained ExitStatus.
   *
   * @see Process::wait()
   * @see ExitStatus
   * @see observation_lost
   *
   * Error code: 13.
   */
  inline constexpr auto wait_failed = ErrorIdentity::define<"vix.process">(13);

  /**
   * @brief Reliable observation of the child process was lost.
   *
   * @details
   * Indicates that Vix can no longer reliably
   * determine or observe the child's process state.
   *
   * In this condition, operations requiring a valid
   * process identity may be rejected.
   *
   * This avoids acting on an unreliable native
   * process identifier that could have been reused.
   *
   * @see Process::wait()
   * @see Process::terminate()
   *
   * Error code: 14.
   */
  inline constexpr auto observation_lost = ErrorIdentity::define<"vix.process">(14);

  /**
   * @brief The deadline for waiting was reached.
   *
   * @details
   * Returned when Process::wait(WaitOptions)
   * reaches its configured deadline before
   * process termination is observed.
   *
   * This produces a Failure completion rather
   * than a Cancelled completion.
   *
   * @warning
   * A waiting timeout does not terminate
   * the child process.
   *
   * The caller may wait again or explicitly
   * request termination.
   *
   * @see Process::wait()
   * @see WaitOptions
   * @see Process::terminate()
   *
   * Error code: 15.
   */
  inline constexpr auto wait_timed_out = ErrorIdentity::define<"vix.process">(15);

  /**
   * @brief A request to terminate the child process failed.
   *
   * @details
   * Indicates that the operating system rejected
   * or failed to complete a termination request.
   *
   * Forced termination is requested through
   * Process::terminate().
   *
   * @note
   * A successful termination request does not
   * itself provide the child's final ExitStatus.
   *
   * Use wait() to observe termination.
   *
   * @see Process::terminate()
   * @see Process::wait()
   *
   * Error code: 16.
   */
  inline constexpr auto termination_failed = ErrorIdentity::define<"vix.process">(16);

  /**
   * @brief The child process has already terminated.
   *
   * @details
   * Returned when an operation requires a running
   * child but the process has already exited.
   *
   * For example, terminate() cannot terminate
   * a child that has already finished.
   *
   * @see Process::terminate()
   * @see ExitStatus
   *
   * Error code: 17.
   */
  inline constexpr auto already_exited = ErrorIdentity::define<"vix.process">(17);

  /**
   * @brief Reading from a child output pipe failed.
   *
   * @details
   * Indicates that a native read operation failed
   * while consuming data from a child's stdout
   * or stderr pipe.
   *
   * This is distinct from end-of-file, which
   * is represented by a successful zero-byte read
   * when a non-empty buffer is supplied.
   *
   * @see ChildOutput::read_some()
   *
   * Error code: 18.
   */
  inline constexpr auto pipe_read_failed = ErrorIdentity::define<"vix.process">(18);

  /**
   * @brief Closing a process-related pipe failed.
   *
   * @details
   * Indicates that the operating system reported
   * a failure while closing an owned pipe endpoint.
   *
   * ChildInput and ChildOutput retain a recorded
   * closing error after a failed close attempt.
   *
   * Explicit close() calls allow applications
   * to observe these failures.
   *
   * @note
   * Destructor-based cleanup cannot report
   * closing errors to the caller.
   *
   * @see ChildInput::close()
   * @see ChildOutput::close()
   *
   * Error code: 19.
   */
  inline constexpr auto close_failed = ErrorIdentity::define<"vix.process">(19);

  /**
   * @brief The requested process capability is unsupported.
   *
   * @details
   * Indicates that the current platform cannot
   * provide a requested Process domain operation.
   *
   * For example, native child-process creation
   * is unavailable under WASI Preview 2, where
   * spawn() reports this error.
   *
   * @note
   * This identity distinguishes unavailable
   * platform functionality from an ordinary
   * process creation failure.
   *
   * @see spawn()
   *
   * Error code: 20.
   */
  inline constexpr auto unsupported_capability = ErrorIdentity::define<"vix.process">(20);
}

#endif
