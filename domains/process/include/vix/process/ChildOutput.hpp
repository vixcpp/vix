/**
 * @file ChildOutput.hpp
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

#ifndef VIX_PROCESS_CHILD_OUTPUT_HPP
#define VIX_PROCESS_CHILD_OUTPUT_HPP

#include <cstddef>
#include <optional>
#include <span>

#include <vix/Result.hpp>

namespace vix::process
{
  /**
   * @brief Provides readable access to a child process output pipe.
   *
   * @details
   * ChildOutput represents the parent-side reading endpoint
   * of a pipe connected to a child process's standard output
   * (stdout) or standard error (stderr).
   *
   * It allows the parent process to:
   * - Read binary data produced by the child.
   * - Observe end-of-file when the pipe reaches EOF.
   * - Check whether the pipe remains open.
   * - Explicitly release the pipe endpoint.
   *
   * To obtain a ChildOutput:
   * 1. Configure StandardOutput::pipe() or
   *    StandardError::pipe().
   * 2. Start the child using spawn().
   * 3. Obtain the corresponding pipe using
   *    Process::take_stdout() or Process::take_stderr().
   *
   * @par Example: Reading stdout
   * @code{.cpp}
   * #include <array>
   * #include <vix/process/ProcessDomain.hpp>
   *
   * using namespace vix::process;
   *
   * auto executable = Executable::search_path("git");
   *
   * if (!executable)
   * {
   *     // Handle executable configuration error.
   * }
   * else
   * {
   *     ProcessSpec spec(executable.take());
   *
   *     if (!spec.add_argument("--version"))
   *     {
   *         // Handle invalid argument.
   *     }
   *     else
   *     {
   *         spec.set_stdout(StandardOutput::pipe());
   *
   *         auto started = spawn(spec);
   *
   *         if (started)
   *         {
   *             auto process = started.take();
   *             auto output = process.take_stdout();
   *
   *             if (output)
   *             {
   *                 auto reader = output.take();
   *
   *                 std::array<std::byte, 4096> buffer{};
   *
   *                 while (true)
   *                 {
   *                     auto result = reader.read_some(buffer);
   *
   *                     if (!result)
   *                     {
   *                         // Handle the read error.
   *                         break;
   *                     }
   *
   *                     auto count = result.value();
   *
   *                     if (count == 0)
   *                     {
   *                         // End of output.
   *                         break;
   *                     }
   *
   *                     // Process buffer[0 .. count).
   *                 }
   *
   *                 auto closed = reader.close();
   *
   *                 if (!closed)
   *                 {
   *                     // Handle the close error.
   *                 }
   *             }
   *         }
   *     }
   * }
   * @endcode
   *
   * @par Ownership
   * ChildOutput exclusively owns its native pipe descriptor.
   *
   * It cannot be copied or move-assigned, but it can
   * be move-constructed.
   *
   * Moving transfers ownership of the pipe endpoint
   * to the destination object.
   *
   * The moved-from object no longer owns the descriptor.
   *
   * @par Reading behavior
   * read_some() reads up to the number of bytes available
   * in the supplied buffer.
   *
   * A successful read may return fewer bytes than requested.
   *
   * A zero-byte result indicates end-of-file or an
   * empty destination buffer.
   *
   * Read operations do not automatically decode text.
   * The returned bytes may contain arbitrary binary data.
   *
   * @par End-of-file
   * Once all writing endpoints have been closed and
   * buffered data has been consumed, read_some()
   * returns zero to indicate end-of-file.
   *
   * Reaching end-of-file does not automatically
   * close the ChildOutput object.
   *
   * @par Error handling
   * Expected I/O failures are reported using
   * vix::Result<T>.
   *
   * Reading from a closed or moved-from ChildOutput
   * violates the contract and terminates the process.
   *
   * @par Resource lifetime
   * The destructor releases an open pipe automatically.
   *
   * Use close() explicitly when closing errors
   * must be observed.
   *
   * @warning
   * Reading may block while the pipe remains open
   * and no data is available.
   *
   * A child may also block if it produces enough output
   * to fill a pipe that the parent is not draining.
   *
   * When both stdout and stderr are piped separately,
   * applications may need to drain both streams
   * concurrently to avoid deadlocks.
   *
   * @note
   * ChildOutput does not own or control the child process.
   *
   * Closing the output pipe does not terminate
   * or wait for the child.
   *
   * @see StandardOutput::pipe()
   * @see StandardError::pipe()
   * @see Process::take_stdout()
   * @see Process::take_stderr()
   * @see ChildInput
   * @see Process
   */
  class ChildOutput
  {
  public:
    /**
     * @brief Default construction is not supported.
     *
     * @details
     * Obtain a ChildOutput through Process::take_stdout()
     * or Process::take_stderr() after configuring the
     * corresponding standard stream as a pipe.
     */
    ChildOutput() = delete;

    /**
     * @brief Copy construction is not supported.
     *
     * @details
     * Each ChildOutput exclusively owns its pipe endpoint.
     */
    ChildOutput(const ChildOutput &) = delete;

    /**
     * @brief Copy assignment is not supported.
     */
    ChildOutput &operator=(const ChildOutput &) = delete;

    /**
     * @brief Transfers ownership of an output pipe.
     *
     * @param other ChildOutput whose resources
     *              are transferred.
     *
     * @details
     * Move construction transfers ownership of the
     * native descriptor and its associated closing state.
     *
     * The moved-from object no longer owns the pipe.
     *
     * @note
     * Use is_open() to inspect whether an object
     * still owns an open pipe endpoint.
     *
     * @warning
     * Do not call read_some() or close() on
     * a moved-from ChildOutput.
     */
    ChildOutput(ChildOutput &&) noexcept;

    /**
     * @brief Move assignment is not supported.
     *
     * @details
     * Use move construction to transfer ownership.
     */
    ChildOutput &operator=(ChildOutput &&) = delete;

    /**
     * @brief Destroys the output pipe endpoint.
     *
     * @details
     * Releases the native descriptor if the pipe
     * remains open.
     *
     * The destructor does not wait for or terminate
     * the associated child process.
     *
     * @note
     * Errors encountered during destructor cleanup
     * cannot be reported to the caller.
     *
     * Use close() when closing errors matter.
     *
     * @see close()
     */
    ~ChildOutput() noexcept;

    /**
     * @brief Checks whether the output pipe is open.
     *
     * @return true if this object owns an open pipe
     *         descriptor, false otherwise.
     *
     * @details
     * Returns false after the pipe has been closed
     * or ownership has been moved away.
     *
     * @par Example
     * @code{.cpp}
     * if (reader.is_open())
     * {
     *     // The pipe endpoint is still open.
     * }
     * @endcode
     *
     * @note
     * An open pipe does not guarantee that the child
     * is still running or will produce more output.
     *
     * End-of-file does not necessarily mean that
     * the ChildOutput object has been closed.
     *
     * @see read_some()
     * @see close()
     */
    [[nodiscard]] bool is_open() const noexcept;

    /**
     * @brief Reads bytes from the child process output pipe.
     *
     * @param buffer Writable span receiving the data.
     *
     * @return The number of bytes read on success,
     *         or an Error if reading fails.
     *
     * @details
     * Attempts to read up to buffer.size() bytes
     * from the pipe.
     *
     * The operation may return fewer bytes
     * than requested.
     *
     * A successful result of zero indicates:
     * - End-of-file.
     * - An empty destination buffer.
     *
     * For a non-empty buffer, zero indicates that
     * the pipe has reached end-of-file.
     *
     * @par Example
     * @code{.cpp}
     * std::array<std::byte, 1024> buffer{};
     *
     * while (true)
     * {
     *     auto result = reader.read_some(buffer);
     *
     *     if (!result)
     *     {
     *         // Handle the read error.
     *         break;
     *     }
     *
     *     const auto count = result.value();
     *
     *     if (count == 0)
     *     {
     *         // End of stream.
     *         break;
     *     }
     *
     *     // Process the first count bytes.
     * }
     * @endcode
     *
     * @par Partial reads
     * A successful read does not guarantee that the
     * supplied buffer is completely filled.
     *
     * Applications requiring all output must continue
     * reading until end-of-file.
     *
     * @par Binary data
     * read_some() returns raw bytes without interpreting
     * their character encoding or content.
     *
     * Applications expecting UTF-8 text must handle
     * decoding and validation separately.
     *
     * @pre
     * The ChildOutput must be open.
     *
     * @warning
     * Calling read_some() on a closed or moved-from
     * ChildOutput terminates the process.
     *
     * @warning
     * A read may block until output becomes available
     * or the writing endpoints are closed.
     *
     * @note
     * An empty buffer returns zero immediately.
     * Use a non-empty buffer when detecting EOF.
     *
     * @note
     * The destination buffer must remain valid
     * for the duration of the call.
     *
     * @see close()
     * @see StandardOutput::pipe()
     * @see StandardError::pipe()
     */
    [[nodiscard]] vix::Result<std::size_t> read_some(std::span<std::byte>);

    /**
     * @brief Closes the child output pipe.
     *
     * @return Success if closing completes,
     *         or an Error if native closing fails.
     *
     * @details
     * Releases the parent-side reading endpoint
     * of the child's stdout or stderr pipe.
     *
     * After the close attempt, the object no longer
     * owns the descriptor.
     *
     * @par Example
     * @code{.cpp}
     * auto result = reader.close();
     *
     * if (!result)
     * {
     *     // Handle result.error().
     * }
     * @endcode
     *
     * @par Repeated calls
     * Closing is not idempotent after success.
     *
     * Calling close() a second time after a
     * successful close violates the contract.
     *
     * If native closing failed, the recorded error
     * is preserved and returned by subsequent calls.
     *
     * @pre
     * The ChildOutput must be open, unless a previous
     * close attempt produced a cached error.
     *
     * @warning
     * Calling close() on an already successfully
     * closed or moved-from ChildOutput terminates
     * the process.
     *
     * @warning
     * Unread data in the pipe is discarded when
     * the reading endpoint is closed.
     *
     * If the child continues writing without another
     * reader, its writes may fail with a broken-pipe
     * condition.
     *
     * @note
     * Closing the output pipe does not automatically
     * terminate or wait for the child process.
     *
     * @see is_open()
     * @see read_some()
     * @see Process::wait()
     */
    [[nodiscard]] vix::Result<void> close();

  private:
    explicit ChildOutput(int descriptor) noexcept;

    [[maybe_unused]] int descriptor_ = -1;
    std::optional<vix::Error> close_error_;

    friend class Process;
  };
}

#endif
