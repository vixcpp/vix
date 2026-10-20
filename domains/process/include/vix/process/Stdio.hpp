/**
 * @file Stdio.hpp
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

#ifndef VIX_PROCESS_STDIO_HPP
#define VIX_PROCESS_STDIO_HPP

#include <vix/files/File.hpp>

namespace vix::process
{
  class Process;
  class ProcessSpec;

  vix::Result<Process> spawn(ProcessSpec);

  /**
   * @brief Configures the standard input of a child process.
   *
   * @details
   * StandardInput determines where a child process receives
   * its standard input (stdin).
   *
   * Four input modes are available:
   * - inherit(): use the parent's standard input.
   * - null(): provide an empty input stream.
   * - pipe(): create a pipe writable by the parent.
   * - from_file(): read input from an existing File.
   *
   * Pass the configuration to ProcessSpec::set_stdin()
   * before starting the process with spawn().
   *
   * @par Example
   * @code{.cpp}
   * spec.set_stdin(vix::process::StandardInput::pipe());
   *
   * auto started = vix::process::spawn(std::move(spec));
   *
   * if (started)
   * {
   *     auto child = started.take();
   *     auto input = child.take_stdin();
   *
   *     if (input)
   *     {
   *         auto writer = input.take();
   *         // Write data using writer.write_some().
   *     }
   * }
   * @endcode
   *
   * @par File ownership
   * from_file() borrows an existing vix::files::File.
   * It does not transfer ownership or close that file.
   *
   * The borrowed File must remain alive, open, and
   * readable until spawn() has finished preparing
   * the child process.
   *
   * The native handle is duplicated during spawning.
   *
   * @par Value semantics
   * StandardInput is a lightweight, copyable configuration
   * value. Copying it does not duplicate a native handle.
   *
   * @note
   * Creating a StandardInput does not start a process
   * or create a pipe. Resources are prepared by spawn().
   *
   * @see ProcessSpec
   * @see ChildInput
   * @see vix::files::File
   */
  class StandardInput
  {
  public:
    /**
     * @brief Default construction is not supported.
     *
     * Select an input mode using a named factory.
     */
    StandardInput() = delete;

    StandardInput(const StandardInput &) noexcept = default;
    StandardInput(StandardInput &&) noexcept = default;
    StandardInput &operator=(const StandardInput &) noexcept = default;
    StandardInput &operator=(StandardInput &&) noexcept = default;

    /**
     * @brief Inherits the parent's standard input.
     *
     * @return A configuration selecting inherited stdin.
     *
     * @details
     * The child receives its standard input from
     * the parent's corresponding standard stream,
     * subject to platform support.
     *
     * @par Example
     * @code{.cpp}
     * spec.set_stdin(StandardInput::inherit());
     * @endcode
     */
    [[nodiscard]] static StandardInput inherit() noexcept;

    /**
     * @brief Provides an empty standard input stream.
     *
     * @return A configuration selecting null input.
     *
     * @details
     * The child's stdin is connected to a null source.
     * Reading from this source reaches end-of-file.
     *
     * @par Example
     * @code{.cpp}
     * spec.set_stdin(StandardInput::null());
     * @endcode
     *
     * @note
     * Useful when the child must not read interactive
     * input from the parent process.
     */
    [[nodiscard]] static StandardInput null() noexcept;

    /**
     * @brief Creates a pipe for writing to the child's stdin.
     *
     * @return A configuration selecting piped stdin.
     *
     * @details
     * spawn() creates the pipe and connects its reading
     * end to the child process.
     *
     * The parent obtains the writing end through
     * Process::take_stdin().
     *
     * @par Example
     * @code{.cpp}
     * spec.set_stdin(StandardInput::pipe());
     * @endcode
     *
     * @note
     * Close the parent's pipe when no more input will
     * be sent, so the child can observe end-of-file.
     *
     * @see ChildInput
     * @see Process::take_stdin()
     */
    [[nodiscard]] static StandardInput pipe() noexcept;

    /**
     * @brief Redirects the child's stdin from an existing file.
     *
     * @param file Open File providing the input.
     *
     * @return A configuration referencing the supplied file.
     *
     * @pre
     * The File must remain alive and open with read
     * or read_write access until spawn() uses it.
     *
     * @details
     * This function borrows the supplied File.
     * It does not transfer file ownership.
     *
     * During spawning, the native handle is duplicated
     * for use by the child process.
     *
     * @par Example
     * @code{.cpp}
     * spec.set_stdin(StandardInput::from_file(input_file));
     * @endcode
     *
     * @warning
     * A closed, moved-from, or incorrectly accessed File
     * violates the spawning contract.
     *
     * Do not destroy or move the borrowed File before
     * spawn() finishes using the configuration.
     *
     * @see vix::files::File
     */
    [[nodiscard]] static StandardInput from_file(
        vix::files::File &) noexcept;

  private:
    enum class Kind
    {
      inherit,
      null,
      pipe,
      file
    };

    explicit StandardInput(Kind kind, vix::files::File *file = nullptr) noexcept
        : kind_(kind), file_(file) {}

    Kind kind_;
    vix::files::File *file_;

    friend struct detail_process_access;
    friend vix::Result<Process> spawn(ProcessSpec);
  };

  /**
   * @brief Configures the standard output of a child process.
   *
   * @details
   * StandardOutput determines where a child process sends
   * its standard output (stdout).
   *
   * Four output modes are available:
   * - inherit(): use the parent's standard output.
   * - null(): discard standard output.
   * - pipe(): make output readable by the parent.
   * - to_file(): redirect output to an existing File.
   *
   * Pass the configuration to ProcessSpec::set_stdout()
   * before calling spawn().
   *
   * @par Example
   * @code{.cpp}
   * spec.set_stdout(StandardOutput::pipe());
   *
   * auto started = vix::process::spawn(std::move(spec));
   *
   * if (started)
   * {
   *     auto child = started.take();
   *     auto output = child.take_stdout();
   *
   *     if (output)
   *     {
   *         auto reader = output.take();
   *         // Read using reader.read_some().
   *     }
   * }
   * @endcode
   *
   * @par File ownership
   * to_file() borrows an existing writable File.
   *
   * The File must remain alive and open until spawn()
   * finishes preparing the child process.
   *
   * Its native handle is duplicated during spawning.
   * The original File remains owned by the caller.
   *
   * @par Value semantics
   * StandardOutput supports copying and moving.
   * Configuration copies do not duplicate native handles.
   *
   * @note
   * Output redirection does not guarantee that application
   * output is immediately flushed or persisted to storage.
   *
   * @see ProcessSpec
   * @see ChildOutput
   * @see StandardError
   */
  class StandardOutput
  {
  public:
    /**
     * @brief Default construction is not supported.
     */
    StandardOutput() = delete;

    StandardOutput(const StandardOutput &) noexcept = default;
    StandardOutput(StandardOutput &&) noexcept = default;
    StandardOutput &operator=(const StandardOutput &) noexcept = default;
    StandardOutput &operator=(StandardOutput &&) noexcept = default;

    /**
     * @brief Inherits the parent's standard output.
     *
     * @return A configuration selecting inherited stdout.
     *
     * @par Example
     * @code{.cpp}
     * spec.set_stdout(StandardOutput::inherit());
     * @endcode
     *
     * @note
     * Output is directed to the parent's corresponding
     * standard stream, subject to platform support.
     */
    [[nodiscard]] static StandardOutput inherit() noexcept;

    /**
     * @brief Discards the child's standard output.
     *
     * @return A configuration selecting null output.
     *
     * @details
     * Output written to stdout is sent to a null sink.
     *
     * @par Example
     * @code{.cpp}
     * spec.set_stdout(StandardOutput::null());
     * @endcode
     */
    [[nodiscard]] static StandardOutput null() noexcept;

    /**
     * @brief Captures the child's stdout through a pipe.
     *
     * @return A configuration selecting piped stdout.
     *
     * @details
     * spawn() connects the child's stdout to a pipe.
     *
     * The parent obtains the reading end through
     * Process::take_stdout().
     *
     * @par Example
     * @code{.cpp}
     * spec.set_stdout(StandardOutput::pipe());
     * @endcode
     *
     * @note
     * Pipe reads may be partial. Read until end-of-file
     * when complete output is required.
     *
     * @warning
     * If the child produces more output than the pipe can
     * buffer, waiting without draining the pipe may block
     * both the parent and child.
     *
     * @see ChildOutput
     * @see Process::take_stdout()
     */
    [[nodiscard]] static StandardOutput pipe() noexcept;

    /**
     * @brief Redirects stdout to an existing writable file.
     *
     * @param file File receiving the child's output.
     *
     * @return A configuration referencing the supplied file.
     *
     * @pre
     * The File must remain alive and open with write
     * or read_write access until spawn() uses it.
     *
     * @details
     * The supplied File remains owned by the caller.
     * Its native handle is duplicated during spawning.
     *
     * @par Example
     * @code{.cpp}
     * spec.set_stdout(StandardOutput::to_file(output_file));
     * @endcode
     *
     * @note
     * Whether existing file contents are preserved,
     * truncated, or appended depends on how the File
     * was originally opened.
     *
     * @warning
     * A closed or incorrectly accessed File violates
     * the spawning contract.
     *
     * @see vix::files::OpenOptions
     */
    [[nodiscard]] static StandardOutput to_file(
        vix::files::File &) noexcept;

  private:
    enum class Kind
    {
      inherit,
      null,
      pipe,
      file
    };

    explicit StandardOutput(Kind kind, vix::files::File *file = nullptr) noexcept
        : kind_(kind), file_(file) {}

    Kind kind_;
    vix::files::File *file_;

    friend struct detail_process_access;
    friend vix::Result<Process> spawn(ProcessSpec);
  };

  /**
   * @brief Configures the standard error of a child process.
   *
   * @details
   * StandardError determines where a child process sends
   * its diagnostic and error output (stderr).
   *
   * Five modes are available:
   * - inherit(): use the parent's standard error.
   * - null(): discard standard error.
   * - pipe(): capture standard error through a pipe.
   * - to_file(): redirect standard error to a File.
   * - merge_with_stdout(): send stderr to stdout.
   *
   * Configure stderr using ProcessSpec::set_stderr().
   *
   * @par Example
   * @code{.cpp}
   * spec.set_stdout(StandardOutput::pipe());
   * spec.set_stderr(StandardError::merge_with_stdout());
   *
   * // Both output streams use the selected stdout
   * // destination after spawn().
   * @endcode
   *
   * @par Merged output
   * merge_with_stdout() routes the child's standard
   * error to the same destination as its standard output.
   *
   * If stdout is piped, the parent can read both streams
   * through Process::take_stdout().
   *
   * If stdout is redirected to a file, both streams
   * use that output destination.
   *
   * @par Ownership
   * A configuration created by to_file() borrows its
   * File without transferring ownership.
   *
   * The borrowed File must remain alive and writable
   * until spawn() finishes preparing the child.
   *
   * @note
   * Merging output does not add message boundaries
   * or guarantee a particular ordering of writes
   * performed by the child.
   *
   * @see StandardOutput
   * @see ChildOutput
   * @see ProcessSpec
   */
  class StandardError
  {
  public:
    /**
     * @brief Default construction is not supported.
     */
    StandardError() = delete;

    StandardError(const StandardError &) noexcept = default;
    StandardError(StandardError &&) noexcept = default;
    StandardError &operator=(const StandardError &) noexcept = default;
    StandardError &operator=(StandardError &&) noexcept = default;

    /**
     * @brief Inherits the parent's standard error.
     *
     * @return A configuration selecting inherited stderr.
     *
     * @par Example
     * @code{.cpp}
     * spec.set_stderr(StandardError::inherit());
     * @endcode
     */
    [[nodiscard]] static StandardError inherit() noexcept;

    /**
     * @brief Discards the child's standard error.
     *
     * @return A configuration selecting null stderr.
     *
     * @par Example
     * @code{.cpp}
     * spec.set_stderr(StandardError::null());
     * @endcode
     */
    [[nodiscard]] static StandardError null() noexcept;

    /**
     * @brief Captures stderr through a separate pipe.
     *
     * @return A configuration selecting piped stderr.
     *
     * @details
     * The parent obtains the pipe's reading end through
     * Process::take_stderr().
     *
     * @par Example
     * @code{.cpp}
     * spec.set_stderr(StandardError::pipe());
     * @endcode
     *
     * @warning
     * A child can block when its stderr pipe becomes full.
     * Drain captured output while the child is running.
     *
     * @see ChildOutput
     * @see Process::take_stderr()
     */
    [[nodiscard]] static StandardError pipe() noexcept;

    /**
     * @brief Redirects stderr to an existing writable file.
     *
     * @param file File receiving diagnostic output.
     *
     * @return A configuration referencing the supplied file.
     *
     * @pre
     * The File must remain alive and open with write
     * or read_write access until spawn() uses it.
     *
     * @details
     * The File remains owned by the caller.
     * Its native handle is duplicated during spawning.
     *
     * @par Example
     * @code{.cpp}
     * spec.set_stderr(StandardError::to_file(error_file));
     * @endcode
     *
     * @warning
     * A closed or incorrectly accessed File violates
     * the spawning contract.
     *
     * @see vix::files::File
     */
    [[nodiscard]] static StandardError to_file(
        vix::files::File &) noexcept;

    /**
     * @brief Redirects stderr to the configured stdout destination.
     *
     * @return A configuration merging stderr with stdout.
     *
     * @details
     * The child writes both standard output and standard
     * error to the destination selected for stdout.
     *
     * This is useful when an application needs to capture
     * combined output without managing two separate pipes.
     *
     * @par Example
     * @code{.cpp}
     * spec.set_stdout(StandardOutput::pipe());
     * spec.set_stderr(StandardError::merge_with_stdout());
     * @endcode
     *
     * @note
     * This does not create an independent stderr pipe.
     * When stdout is piped, capture the combined output
     * through Process::take_stdout().
     *
     * @note
     * The relative ordering of output from different
     * child streams is not guaranteed.
     *
     * @see StandardOutput::pipe()
     * @see Process::take_stdout()
     */
    [[nodiscard]] static StandardError merge_with_stdout() noexcept;

  private:
    enum class Kind
    {
      inherit,
      null,
      pipe,
      file,
      merge_stdout
    };

    explicit StandardError(Kind kind, vix::files::File *file = nullptr) noexcept
        : kind_(kind), file_(file) {}

    Kind kind_;
    vix::files::File *file_;

    friend struct detail_process_access;
    friend vix::Result<Process> spawn(ProcessSpec);
  };
} // namespace vix::process

#endif // VIX_PROCESS_STDIO_HPP
