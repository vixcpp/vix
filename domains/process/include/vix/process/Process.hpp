/**
 * @file Process.hpp
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

#ifndef VIX_PROCESS_PROCESS_HPP
#define VIX_PROCESS_PROCESS_HPP

#include <memory>
#include <optional>

#include <vix/Cancellation.hpp>
#include <vix/Completion.hpp>
#include <vix/Deadline.hpp>
#include <vix/Result.hpp>

#include <vix/process/ChildInput.hpp>
#include <vix/process/ChildOutput.hpp>
#include <vix/process/ExitStatus.hpp>
#include <vix/process/ProcessSpec.hpp>

namespace vix::process
{
  /**
   * @brief Configures cancellation and deadline behavior
   *        when waiting for a child process.
   *
   * @details
   * WaitOptions allows Process::wait() to observe
   * a cancellation request or a deadline while
   * waiting for process termination.
   *
   * Both options are optional:
   * - A CancellationToken allows cooperative cancellation.
   * - A Deadline limits how long the caller waits.
   *
   * @par Example: Wait with a deadline
   * @code{.cpp}
   * auto options = WaitOptions{
   *     {},
   *     vix::Deadline::after(
   *         vix::Duration::milliseconds(5000))
   * };
   *
   * auto completion = process.wait(options);
   * @endcode
   *
   * @par Example: Wait with cancellation
   * @code{.cpp}
   * vix::CancellationSource cancellation;
   *
   * auto completion = process.wait(
   *     WaitOptions{cancellation.token()});
   * @endcode
   *
   * @par Cancellation semantics
   * Cancellation stops the waiting operation,
   * not the child process itself.
   *
   * When cancellation is observed before termination,
   * wait() returns a Cancelled completion.
   *
   * @par Deadline semantics
   * When the deadline is reached before termination
   * is observed, wait() returns a Failure completion
   * containing errors::wait_timed_out.
   *
   * A deadline expiration is not represented
   * as cancellation.
   *
   * @note
   * Process termination is checked before cancellation
   * and deadline conditions on each polling iteration.
   *
   * A process that has already terminated can therefore
   * produce a successful completion even when cancellation
   * was requested or a deadline was reached.
   *
   * @note
   * Neither cancellation nor deadline expiration
   * automatically terminates the child process.
   *
   * @see Process::wait(WaitOptions)
   * @see vix::CancellationToken
   * @see vix::Deadline
   */
  class WaitOptions
  {
  public:
    /**
     * @brief Constructs process waiting options.
     *
     * @param cancellation Optional cancellation token.
     * @param deadline Optional waiting deadline.
     *
     * @details
     * With default arguments, waiting has no
     * cancellation request or deadline restriction.
     *
     * @par Example
     * @code{.cpp}
     * WaitOptions unlimited;
     *
     * WaitOptions timed{
     *     {},
     *     vix::Deadline::after(
     *         vix::Duration::milliseconds(1000))
     * };
     * @endcode
     *
     * @note
     * These options control waiting only.
     * They do not change the child's execution.
     */
    WaitOptions(
        vix::CancellationToken cancellation = {},
        std::optional<vix::Deadline> deadline = std::nullopt) noexcept;

  private:
    vix::CancellationToken cancellation_;
    std::optional<vix::Deadline> deadline_;

    friend class Process;
  };

  namespace detail
  {
    struct ProcessRecord;
  }

  /**
   * @brief Owns and manages a spawned child process.
   *
   * @details
   * Process represents a child process successfully
   * started through vix::process::spawn().
   *
   * It provides operations to:
   * - Retrieve the child's identity.
   * - Wait for process termination.
   * - Wait with cancellation or a deadline.
   * - Request forced termination.
   * - Access configured stdin, stdout, and stderr pipes.
   *
   * @par Example: Start and wait for a process
   * @code{.cpp}
   * #include <vix/process/ProcessDomain.hpp>
   *
   * int main()
   * {
   *     using namespace vix::process;
   *
   *     auto executable = Executable::search_path("git");
   *
   *     if (!executable)
   *         return 1;
   *
   *     ProcessSpec spec(executable.take());
   *
   *     if (!spec.add_argument("--version"))
   *         return 1;
   *
   *     auto started = spawn(spec);
   *
   *     if (!started)
   *         return 1;
   *
   *     auto process = started.take();
   *
   *     auto result = process.wait();
   *
   *     if (!result)
   *         return 1;
   *
   *     return result.value().succeeded() ? 0 : 1;
   * }
   * @endcode
   *
   * @par Ownership
   * Process exclusively owns its process-control
   * state and any untransferred pipe endpoints.
   *
   * A Process cannot be copied or move-assigned,
   * but it can be move-constructed.
   *
   * Moving transfers ownership to the destination.
   * The moved-from object must not be used for
   * process operations.
   *
   * @par Child lifetime
   * Destroying a Process does not terminate
   * the child process.
   *
   * Any untransferred pipe endpoints are closed
   * during destruction.
   *
   * On POSIX systems, Vix arranges for abandoned
   * children to be reaped after termination.
   *
   * On Windows, destruction releases the owned
   * process observation handle.
   *
   * @par Waiting
   * wait() observes the child's terminal status.
   *
   * A successfully observed exit status is cached,
   * allowing repeated wait() calls to retrieve
   * the same termination result.
   *
   * wait(WaitOptions) additionally supports
   * cancellation and deadline checks.
   *
   * @par Termination
   * terminate() requests forced termination.
   *
   * It does not automatically wait for the child
   * to reach its terminal state.
   *
   * Use wait() afterward when confirmation
   * of termination is required.
   *
   * @par Pipe ownership
   * When standard streams are configured as pipes,
   * Process initially owns their parent-side endpoints.
   *
   * Each endpoint can be transferred exactly once:
   * - take_stdin() returns ChildInput.
   * - take_stdout() returns ChildOutput.
   * - take_stderr() returns ChildOutput.
   *
   * The returned pipe object independently owns
   * its endpoint.
   *
   * @par Error handling
   * Expected operating-system failures are returned
   * through vix::Result<T> or vix::Completion<T>.
   *
   * Invalid use of a moved-from Process violates
   * its contract and terminates the application.
   *
   * @warning
   * Captured stdout or stderr may fill its pipe
   * buffer and block the child process.
   *
   * Drain piped output while the child is running
   * when substantial output is expected.
   *
   * @note
   * Process does not provide a shell or command
   * interpreter. Arguments are configured through
   * ProcessSpec and passed to the selected executable.
   *
   * @see spawn()
   * @see ProcessSpec
   * @see WaitOptions
   * @see ExitStatus
   * @see ChildInput
   * @see ChildOutput
   */
  class Process
  {
  public:
    /**
     * @brief Default construction is not supported.
     *
     * Obtain a Process through spawn().
     */
    Process() = delete;

    /**
     * @brief Copy construction is not supported.
     *
     * Process ownership is exclusive.
     */
    Process(const Process &) = delete;

    /**
     * @brief Copy assignment is not supported.
     */
    Process &operator=(const Process &) = delete;

    /**
     * @brief Transfers ownership of a child process.
     *
     * @param other Process whose ownership is transferred.
     *
     * @details
     * Move construction transfers the process record
     * and all untransferred standard-stream endpoints.
     *
     * The moved-from Process no longer owns
     * the child process state.
     *
     * @warning
     * Calling process operations on a moved-from
     * Process violates the contract.
     */
    Process(Process &&) noexcept;

    /**
     * @brief Move assignment is not supported.
     *
     * Use move construction to transfer ownership.
     */
    Process &operator=(Process &&) = delete;

    /**
     * @brief Releases resources owned by the Process.
     *
     * @details
     * The destructor closes any pipe endpoints
     * that have not been transferred to ChildInput
     * or ChildOutput objects.
     *
     * It also releases the process-control resources.
     *
     * @warning
     * Destroying Process does not terminate the child.
     *
     * The child may continue running after the
     * Process object has been destroyed.
     *
     * @note
     * Destructor cleanup cannot report errors.
     *
     * @note
     * Explicitly wait for the child when its
     * final status is important to the application.
     *
     * @see wait()
     * @see terminate()
     */
    ~Process() noexcept;

    /**
     * @brief Returns the child process identifier.
     *
     * @return An opaque ProcessId identifying the child.
     *
     * @details
     * The identifier combines Vix-managed identity
     * information with the native process identifier.
     *
     * It remains available after process termination
     * while the Process object retains valid ownership.
     *
     * @par Example
     * @code{.cpp}
     * auto identifier = process.id();
     *
     * auto text = identifier.display();
     * // Use text in logs or diagnostics.
     * @endcode
     *
     * @pre
     * The Process must not be moved-from.
     *
     * @warning
     * Calling id() on a moved-from Process
     * terminates the application.
     *
     * @note
     * ProcessId is not a native operating-system
     * process handle.
     *
     * @see ProcessId
     */
    [[nodiscard]] ProcessId id() const noexcept;

    /**
     * @brief Waits until the child process terminates.
     *
     * @return The observed ExitStatus on success,
     *         or an Error if waiting fails.
     *
     * @details
     * Blocks until the child has terminated
     * or an operational observation failure occurs.
     *
     * The result distinguishes:
     * - Successful observation of process termination.
     * - Failure to observe the process correctly.
     *
     * A child exiting with a non-zero exit code
     * still produces a successful Result<ExitStatus>.
     *
     * @par Example
     * @code{.cpp}
     * auto result = process.wait();
     *
     * if (!result)
     * {
     *     // Handle a wait operation error.
     * }
     * else if (result.value().succeeded())
     * {
     *     // Child exited with code zero.
     * }
     * else
     * {
     *     // Child exited unsuccessfully or abnormally.
     * }
     * @endcode
     *
     * @par Repeated waiting
     * Once an ExitStatus has been observed,
     * it is stored in the process record.
     *
     * Subsequent wait() calls return the
     * previously observed status.
     *
     * @pre
     * The Process must not be moved-from.
     *
     * @warning
     * Calling wait() on a moved-from Process
     * terminates the application.
     *
     * @warning
     * Waiting may block indefinitely if the child
     * never terminates.
     *
     * Use wait(WaitOptions) when a deadline
     * or cancellation is required.
     *
     * @note
     * If stdout or stderr is piped, drain the
     * streams appropriately to avoid deadlock.
     *
     * @see ExitStatus
     * @see wait(WaitOptions)
     */
    [[nodiscard]] vix::Result<ExitStatus> wait();

    /**
     * @brief Waits for termination with cancellation
     *        and optional deadline support.
     *
     * @param options Cancellation and deadline configuration.
     *
     * @return A Completion containing:
     * - Success with ExitStatus if the child terminates.
     * - Cancelled if cancellation is observed first.
     * - Failure if waiting fails or the deadline expires.
     *
     * @details
     * Observes process termination while checking
     * the supplied cancellation token and deadline.
     *
     * If the child has already terminated, its
     * ExitStatus is returned immediately.
     *
     * Cancellation produces a Cancelled completion.
     *
     * Deadline expiration produces a Failure
     * completion with errors::wait_timed_out.
     *
     * @par Example: Bounded waiting
     * @code{.cpp}
     * auto completion = process.wait(
     *     WaitOptions{
     *         {},
     *         vix::Deadline::after(
     *             vix::Duration::milliseconds(2000))
     *     });
     *
     * if (completion.status() ==
     *     vix::CompletionStatus::Success)
     * {
     *     auto status = completion.value();
     *     // Inspect the child's terminal status.
     * }
     * else if (completion.status() ==
     *          vix::CompletionStatus::Cancelled)
     * {
     *     // Waiting was cancelled.
     * }
     * else
     * {
     *     // Inspect completion.error().
     * }
     * @endcode
     *
     * @par Cancellation
     * Cancellation ends this wait operation only.
     *
     * It does not send a termination signal
     * or stop the child process.
     *
     * @par Deadline
     * A reached deadline produces
     * errors::wait_timed_out.
     *
     * The child continues running unless it
     * has independently terminated.
     *
     * @par Ordering
     * Process termination is checked before
     * cancellation and deadline conditions
     * on each waiting iteration.
     *
     * @pre
     * The Process must not be moved-from.
     *
     * @warning
     * Calling this method on a moved-from
     * Process terminates the application.
     *
     * @note
     * After cancellation or timeout, the caller
     * may wait again or call terminate().
     *
     * @note
     * A timeout is a waiting failure, not
     * an abnormal child exit status.
     *
     * @see WaitOptions
     * @see wait()
     * @see terminate()
     * @see vix::Completion
     */
    [[nodiscard]] vix::Completion<ExitStatus> wait(WaitOptions);

    /**
     * @brief Requests forced termination of the child.
     *
     * @return Success if the termination request
     *         is accepted, or an Error if it fails.
     *
     * @details
     * Requests that the operating system forcibly
     * terminate the child process.
     *
     * On POSIX systems, Vix uses SIGKILL.
     * On Windows, Vix uses TerminateProcess().
     *
     * The operation does not wait for the
     * child to reach its terminal state.
     *
     * @par Example
     * @code{.cpp}
     * auto result = process.terminate();
     *
     * if (!result)
     * {
     *     // Handle termination failure.
     * }
     * else
     * {
     *     auto finished = process.wait();
     *     // Observe the terminal status.
     * }
     * @endcode
     *
     * @par Already terminated
     * If the process has already exited,
     * the operation returns errors::already_exited.
     *
     * If process observation has been irrecoverably
     * lost, termination is rejected rather than
     * risking action on a reused native process ID.
     *
     * @pre
     * The Process must not be moved-from.
     *
     * @warning
     * Calling terminate() on a moved-from Process
     * terminates the application.
     *
     * @warning
     * Forced termination does not give the child
     * an opportunity to complete ordinary cleanup.
     *
     * @note
     * The resulting ExitStatus is platform-dependent.
     *
     * @note
     * terminate() does not automatically close
     * or drain parent-owned pipe endpoints.
     *
     * @see wait()
     * @see ExitStatus
     */
    [[nodiscard]] vix::Result<void> terminate();

    /**
     * @brief Takes ownership of the child's stdin pipe.
     *
     * @return A ChildInput on success, or an Error
     *         if the pipe cannot be transferred.
     *
     * @details
     * Transfers the parent-side writing endpoint
     * of the child's standard input.
     *
     * The process must have been configured using
     * StandardInput::pipe().
     *
     * Ownership can be transferred only once.
     *
     * @par Example
     * @code{.cpp}
     * auto result = process.take_stdin();
     *
     * if (result)
     * {
     *     auto input = result.take();
     *     // Send data through input.write_some().
     * }
     * @endcode
     *
     * @par Errors
     * Returns:
     * - errors::stdio_not_piped if stdin was not piped.
     * - errors::pipe_already_transferred if the endpoint
     *   was already taken.
     *
     * @pre
     * The Process must not be moved-from.
     *
     * @warning
     * Calling this method on a moved-from Process
     * terminates the application.
     *
     * @note
     * Close the ChildInput when input is complete
     * so the child can observe end-of-file.
     *
     * @see ChildInput
     * @see StandardInput::pipe()
     */
    [[nodiscard]] vix::Result<ChildInput> take_stdin();

    /**
     * @brief Takes ownership of the child's stdout pipe.
     *
     * @return A ChildOutput on success, or an Error
     *         if the pipe cannot be transferred.
     *
     * @details
     * Transfers the parent-side reading endpoint
     * of the child's standard output.
     *
     * The process must have been configured using
     * StandardOutput::pipe().
     *
     * Ownership can be transferred only once.
     *
     * @par Example
     * @code{.cpp}
     * auto result = process.take_stdout();
     *
     * if (result)
     * {
     *     auto output = result.take();
     *     // Read through output.read_some().
     * }
     * @endcode
     *
     * @par Errors
     * Returns:
     * - errors::stdio_not_piped if stdout was not piped.
     * - errors::pipe_already_transferred if the endpoint
     *   was already taken.
     *
     * @pre
     * The Process must not be moved-from.
     *
     * @warning
     * Calling this method on a moved-from Process
     * terminates the application.
     *
     * @warning
     * When the child produces substantial output,
     * drain the pipe before or concurrently
     * with waiting for process termination.
     *
     * @see ChildOutput
     * @see StandardOutput::pipe()
     */
    [[nodiscard]] vix::Result<ChildOutput> take_stdout();

    /**
     * @brief Takes ownership of the child's stderr pipe.
     *
     * @return A ChildOutput on success, or an Error
     *         if the pipe cannot be transferred.
     *
     * @details
     * Transfers the parent-side reading endpoint
     * of the child's standard error.
     *
     * The process must have been configured using
     * StandardError::pipe().
     *
     * Ownership can be transferred only once.
     *
     * @par Example
     * @code{.cpp}
     * auto result = process.take_stderr();
     *
     * if (result)
     * {
     *     auto errors = result.take();
     *     // Read through errors.read_some().
     * }
     * @endcode
     *
     * @par Errors
     * Returns:
     * - errors::stdio_not_piped if stderr was not piped.
     * - errors::pipe_already_transferred if the endpoint
     *   was already taken.
     *
     * @pre
     * The Process must not be moved-from.
     *
     * @warning
     * Calling this method on a moved-from Process
     * terminates the application.
     *
     * @note
     * When stderr is configured with
     * StandardError::merge_with_stdout(),
     * no separate stderr pipe is available.
     *
     * Read the combined output through
     * take_stdout() if stdout is piped.
     *
     * @see ChildOutput
     * @see StandardError::pipe()
     * @see StandardError::merge_with_stdout()
     */
    [[nodiscard]] vix::Result<ChildOutput> take_stderr();

  private:
    Process(std::shared_ptr<detail::ProcessRecord>, int stdin_descriptor,
            int stdout_descriptor, int stderr_descriptor) noexcept;

    std::shared_ptr<detail::ProcessRecord> record_;

    [[maybe_unused]] int stdin_descriptor_{-1};
    [[maybe_unused]] int stdout_descriptor_{-1};
    [[maybe_unused]] int stderr_descriptor_{-1};

    friend vix::Result<Process> spawn(ProcessSpec);
  };

  /**
   * @brief Starts a child process from a specification.
   *
   * @param spec Configuration describing the child process.
   *
   * @return An owning Process on success, or an Error
   *         if process creation fails.
   *
   * @details
   * Uses the supplied ProcessSpec to prepare:
   * - The executable and command-line arguments.
   * - The effective environment.
   * - The working directory.
   * - The configured standard streams.
   *
   * On success, returns a Process representing
   * the newly created child.
   *
   * The returned Process owns its process record
   * and any configured parent-side pipe endpoints
   * until those endpoints are transferred.
   *
   * @par Example
   * @code{.cpp}
   * auto executable = Executable::search_path("git");
   *
   * if (!executable)
   *     return;
   *
   * ProcessSpec spec(executable.take());
   *
   * if (!spec.add_argument("--version"))
   *     return;
   *
   * auto started = spawn(spec);
   *
   * if (!started)
   * {
   *     // Handle the process creation error.
   *     return;
   * }
   *
   * auto process = started.take();
   * auto status = process.wait();
   * @endcode
   *
   * @par Executable resolution
   * Executable::path() selects an explicit path.
   * Executable::search_path() selects an executable
   * name to resolve through PATH.
   *
   * @par Error handling
   * Expected failures, such as an executable not
   * being found, invalid stdio configuration,
   * or operating-system resource exhaustion,
   * are reported through Result.
   *
   * @par Platform support
   * Process creation is implemented on supported
   * POSIX and Windows platforms.
   *
   * On WASI Preview 2, spawn() returns
   * errors::unsupported_capability because native
   * child-process creation is unavailable.
   *
   * @warning
   * File-backed stdio configurations borrow File
   * objects. Those files must remain alive and
   * valid until spawn() finishes preparing the child.
   *
   * @note
   * Process creation does not wait for the child
   * to finish executing.
   *
   * @note
   * The caller is responsible for observing the
   * child's result and managing any transferred pipes.
   *
   * @see ProcessSpec
   * @see Process
   * @see Executable
   * @see Environment
   * @see StandardInput
   * @see StandardOutput
   * @see StandardError
   */
  [[nodiscard]] vix::Result<Process> spawn(ProcessSpec);
}

#endif
