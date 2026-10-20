/**
 * @file ChildInput.hpp
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

#ifndef VIX_PROCESS_CHILD_INPUT_HPP
#define VIX_PROCESS_CHILD_INPUT_HPP

#include <cstddef>
#include <optional>
#include <span>

#include <vix/Result.hpp>

namespace vix::process
{
  /**
   * @brief Provides writable access to a child process's stdin pipe.
   *
   * @details
   * ChildInput represents the parent-side writing endpoint
   * of a pipe connected to a child process's standard input.
   *
   * It allows the parent process to:
   * - Send binary data to the child.
   * - Check whether the pipe remains open.
   * - Close the pipe to signal end-of-input.
   *
   * To obtain a ChildInput:
   * 1. Configure stdin using StandardInput::pipe().
   * 2. Start the child with spawn().
   * 3. Take the writing endpoint using Process::take_stdin().
   *
   * @par Example
   * @code{.cpp}
   * #include <string_view>
   * #include <span>
   * #include <vix/process/ProcessDomain.hpp>
   *
   * // Configure stdin before spawning the child.
   * spec.set_stdin(StandardInput::pipe());
   *
   * auto started = spawn(spec);
   *
   * if (!started)
   * {
   *     // Handle process creation failure.
   * }
   * else
   * {
   *     auto process = started.take();
   *
   *     auto input = process.take_stdin();
   *
   *     if (input)
   *     {
   *         auto writer = input.take();
   *
   *         std::string_view text = "Hello from Vix!\n";
   *         auto bytes = std::as_bytes(
   *             std::span{text.data(), text.size()});
   *
   *         std::size_t written = 0;
   *
   *         while (written < bytes.size())
   *         {
   *             auto result = writer.write_some(
   *                 bytes.subspan(written));
   *
   *             if (!result)
   *             {
   *                 // Handle the write error.
   *                 break;
   *             }
   *
   *             written += result.value();
   *         }
   *
   *         auto closed = writer.close();
   *
   *         if (!closed)
   *         {
   *             // Handle the close error.
   *         }
   *     }
   * }
   * @endcode
   *
   * @par Ownership
   * ChildInput exclusively owns its native pipe descriptor.
   *
   * It cannot be copied or move-assigned, but it can
   * be move-constructed.
   *
   * Moving transfers ownership of the pipe endpoint
   * to the destination object.
   *
   * The moved-from object no longer owns the descriptor.
   *
   * @par Writing behavior
   * write_some() attempts to write bytes into the pipe.
   *
   * A successful write may transfer fewer bytes
   * than requested.
   *
   * Applications that need to send all bytes must
   * repeat write_some() until the complete buffer
   * has been transferred.
   *
   * @par End-of-input
   * Closing ChildInput releases the parent's writing
   * endpoint.
   *
   * Once all writing endpoints have been closed,
   * the child can observe end-of-file on stdin
   * after consuming any buffered data.
   *
   * @par Broken pipes
   * If the child closes its reading endpoint,
   * subsequent writes may fail with
   * errors::broken_pipe.
   *
   * On supported POSIX platforms, Vix handles
   * SIGPIPE for this write operation so that a
   * broken pipe can be reported as an error.
   *
   * @par Error handling
   * Operational failures are reported through
   * vix::Result<T>.
   *
   * Writing to a closed or moved-from ChildInput
   * violates the contract and terminates the process.
   *
   * @par Resource lifetime
   * The destructor closes an open pipe automatically.
   *
   * Use close() explicitly when closing errors
   * must be observed.
   *
   * @warning
   * Pipe writes may block when the pipe buffer is full
   * and the child is not consuming input.
   *
   * Avoid waiting for a child that requires more input
   * while leaving its input pipe unwritten or unclosed.
   *
   * @note
   * ChildInput does not own or manage the child process.
   *
   * Closing the pipe does not terminate the child.
   *
   * @see StandardInput::pipe()
   * @see Process::take_stdin()
   * @see Process
   * @see ChildOutput
   */
  class ChildInput
  {
  public:
    /**
     * @brief Default construction is not supported.
     *
     * @details
     * ChildInput instances are obtained from
     * Process::take_stdin() after spawning a child
     * with piped standard input.
     *
     * @see Process::take_stdin()
     */
    ChildInput() = delete;

    /**
     * @brief Copy construction is not supported.
     *
     * @details
     * Each ChildInput exclusively owns its pipe endpoint.
     * Copying would create conflicting ownership.
     */
    ChildInput(const ChildInput &) = delete;

    /**
     * @brief Copy assignment is not supported.
     */
    ChildInput &operator=(const ChildInput &) = delete;

    /**
     * @brief Transfers ownership of a child input pipe.
     *
     * @param other ChildInput whose pipe ownership
     *              is transferred.
     *
     * @details
     * Move construction transfers the native descriptor
     * and any recorded closing error to the destination.
     *
     * After the move, the source object no longer owns
     * the pipe endpoint.
     *
     * @note
     * Use is_open() to check whether the destination
     * or source currently owns an open pipe.
     *
     * @warning
     * Do not call write_some() or close() on a
     * moved-from ChildInput.
     */
    ChildInput(ChildInput &&) noexcept;

    /**
     * @brief Move assignment is not supported.
     *
     * @details
     * Ownership can be transferred through
     * move construction.
     */
    ChildInput &operator=(ChildInput &&) = delete;

    /**
     * @brief Destroys the pipe endpoint.
     *
     * @details
     * If the pipe is still open, its descriptor
     * is released during destruction.
     *
     * The destructor does not terminate or wait
     * for the child process.
     *
     * @note
     * Destructor-based cleanup cannot report
     * closing errors.
     *
     * Use close() explicitly when the result
     * of closing matters.
     *
     * @see close()
     */
    ~ChildInput() noexcept;

    /**
     * @brief Checks whether the input pipe is open.
     *
     * @return true if this object owns an open pipe
     *         descriptor, false otherwise.
     *
     * @details
     * Returns false after the pipe has been closed
     * or its ownership has been moved away.
     *
     * @par Example
     * @code{.cpp}
     * if (writer.is_open())
     * {
     *     // The pipe is available for writing.
     * }
     * @endcode
     *
     * @note
     * An open pipe does not guarantee that the
     * child is still running or accepting input.
     *
     * @see close()
     * @see write_some()
     */
    [[nodiscard]] bool is_open() const noexcept;

    /**
     * @brief Writes bytes to the child's standard input.
     *
     * @param buffer Read-only span containing bytes to send.
     *
     * @return The number of bytes successfully written,
     *         or an Error if the write operation fails.
     *
     * @details
     * Attempts to transfer up to buffer.size() bytes
     * into the child's stdin pipe.
     *
     * A successful operation may write fewer bytes
     * than requested.
     *
     * An empty buffer succeeds and returns zero
     * without performing a native write.
     *
     * For a non-empty buffer, a native write that
     * makes no progress is reported as an error.
     *
     * @par Example
     * @code{.cpp}
     * std::string_view message = "hello\n";
     *
     * auto bytes = std::as_bytes(
     *     std::span{message.data(), message.size()});
     *
     * std::size_t offset = 0;
     *
     * while (offset < bytes.size())
     * {
     *     auto result = writer.write_some(
     *         bytes.subspan(offset));
     *
     *     if (!result)
     *     {
     *         // Handle the write failure.
     *         break;
     *     }
     *
     *     offset += result.value();
     * }
     * @endcode
     *
     * @par Partial writes
     * The caller must use the returned byte count
     * to determine how much data was accepted.
     *
     * A successful partial write is not an error.
     *
     * @par Broken pipe
     * When the child closes its reading endpoint,
     * a write may return errors::broken_pipe.
     *
     * @pre
     * The ChildInput must be open.
     *
     * @warning
     * Calling write_some() on a closed or
     * moved-from ChildInput terminates the process.
     *
     * @warning
     * This operation may block until the child reads
     * data or the pipe becomes writable.
     *
     * @note
     * A successful write confirms that bytes were
     * accepted by the pipe, not that the child
     * has processed them.
     *
     * @note
     * The supplied buffer must remain valid
     * for the duration of the call.
     *
     * @see close()
     * @see StandardInput::pipe()
     */
    [[nodiscard]] vix::Result<std::size_t> write_some(std::span<const std::byte>);

    /**
     * @brief Closes the child's standard input pipe.
     *
     * @return Success if native closing completes,
     *         or an Error if closing fails.
     *
     * @details
     * Releases ownership of the parent's writing
     * endpoint of the child's stdin pipe.
     *
     * Closing allows the child to observe end-of-file
     * once all writers have closed and any buffered
     * input has been consumed.
     *
     * The descriptor is released from this object
     * even if the native close operation reports
     * an error.
     *
     * @par Example
     * @code{.cpp}
     * auto result = writer.close();
     *
     * if (!result)
     * {
     *     // Handle result.error().
     * }
     * @endcode
     *
     * @par Repeated calls
     * Unlike File::close(), successful closing
     * is not idempotent.
     *
     * Calling close() again after a successful
     * close violates the contract.
     *
     * If a previous close attempt failed, the
     * recorded error is returned on subsequent calls.
     *
     * @pre
     * The ChildInput must be open, unless a previous
     * close attempt produced a cached error.
     *
     * @warning
     * Calling close() on an already successfully
     * closed or moved-from ChildInput terminates
     * the process.
     *
     * @warning
     * Do not call write_some() after closing.
     *
     * @note
     * Closing the input pipe does not wait for or
     * terminate the child process.
     *
     * @see is_open()
     * @see write_some()
     * @see Process::wait()
     */
    [[nodiscard]] vix::Result<void> close();

  private:
    explicit ChildInput(int descriptor) noexcept;

    [[maybe_unused]] int descriptor_ = -1;
    std::optional<vix::Error> close_error_;

    friend class Process;
  };
}

#endif
