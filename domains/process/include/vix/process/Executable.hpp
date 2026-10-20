/**
 * @file Executable.hpp
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

#ifndef VIX_PROCESS_EXECUTABLE_HPP
#define VIX_PROCESS_EXECUTABLE_HPP

#include <memory>
#include <string_view>

#include <vix/Result.hpp>
#include <vix/files/Path.hpp>

namespace vix::process
{
  class Process;
  class ProcessSpec;

  vix::Result<Process> spawn(ProcessSpec);

  /**
   * @brief Identifies a program to execute as a child process.
   *
   * @details
   * Executable describes how Vix should locate the program
   * that will be started by spawn().
   *
   * There are two ways to select a program:
   * - path(): use an explicitly provided filesystem path.
   * - search_path(): locate a program by name through PATH.
   *
   * An Executable is used to construct a ProcessSpec,
   * which configures arguments, environment variables,
   * working directory, and standard input/output.
   *
   * @par Example: Execute a program by name
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
   *     auto started = spawn(std::move(spec));
   *
   *     if (!started)
   *         return 1;
   *
   *     auto process = started.take();
   *     auto status = process.wait();
   *
   *     if (!status)
   *         return 1;
   *
   *     return status.value().succeeded() ? 0 : 1;
   * }
   * @endcode
   *
   * @par Executable resolution
   * Creating an Executable describes how the program
   * should be located. It does not start a process.
   *
   * File existence, execution permissions, and actual
   * process creation are checked when spawn() attempts
   * to launch the program.
   *
   * PATH lookup uses the process environment configured
   * for the child operation.
   *
   * @par Value semantics
   * Executable supports copying and moving.
   *
   * Copies share immutable internal state.
   * Destroying an Executable does not terminate or
   * otherwise affect a running child process.
   *
   * @note
   * Executable is a program specification, not an
   * operating-system process handle.
   *
   * @see ProcessSpec
   * @see Process
   * @see spawn()
   */
  class Executable
  {
  public:
    /**
     * @brief Default construction is not supported.
     *
     * Use path() or search_path() to select a program.
     */
    Executable() = delete;

    /**
     * @brief Copies an executable specification.
     *
     * @param other Executable to copy.
     *
     * @details
     * The copy shares immutable internal state
     * with the original specification.
     */
    Executable(const Executable &);

    /**
     * @brief Move-constructs an executable specification.
     *
     * @param other Executable to move from.
     */
    Executable(Executable &&) noexcept;

    /**
     * @brief Copies another executable specification.
     *
     * @param other Executable to copy.
     * @return Reference to this Executable.
     */
    Executable &operator=(const Executable &);

    /**
     * @brief Move-assigns another executable specification.
     *
     * @param other Executable to move from.
     * @return Reference to this Executable.
     */
    Executable &operator=(Executable &&) noexcept;

    /**
     * @brief Destroys the executable specification.
     *
     * @note
     * Destruction does not start, stop, or terminate
     * any operating-system process.
     */
    ~Executable();

    /**
     * @brief Selects an executable using a filesystem path.
     *
     * @param path Non-empty path identifying the program.
     *
     * @return An Executable on success, or an Error
     *         if the supplied path is invalid.
     *
     * @details
     * Use this factory when the location of the executable
     * is known.
     *
     * The path may be absolute or relative.
     *
     * This function does not execute the program or
     * verify that the target exists.
     *
     * @par Example
     * @code{.cpp}
     * auto path = vix::files::Path::from_utf8(
     *     "/usr/bin/git");
     *
     * if (!path)
     *     return;
     *
     * auto executable = vix::process::Executable::path(
     *     path.take());
     *
     * if (!executable)
     *     return;
     *
     * // Use executable.take() to construct ProcessSpec.
     * @endcode
     *
     * @note
     * An empty path produces errors::invalid_argument.
     *
     * @note
     * Filesystem and launch failures are reported when
     * spawn() attempts to start the process.
     *
     * @see search_path()
     * @see ProcessSpec
     */
    [[nodiscard]] static vix::Result<Executable> path(
        vix::files::Path);

    /**
     * @brief Selects an executable by searching PATH.
     *
     * @param name UTF-8 executable name without directory
     *             separators.
     *
     * @return An Executable on success, or an Error
     *         if the name is invalid.
     *
     * @details
     * Use this factory when an executable should be
     * located by name rather than by an explicit path.
     *
     * The supplied name must:
     * - Be non-empty.
     * - Contain valid UTF-8.
     * - Contain no embedded NUL characters.
     * - Contain neither '/' nor '\\' separators.
     *
     * Program resolution occurs when spawn() attempts
     * to launch the child process.
     *
     * @par Example
     * @code{.cpp}
     * auto executable =
     *     vix::process::Executable::search_path("git");
     *
     * if (!executable)
     * {
     *     // Handle invalid executable name.
     * }
     * else
     * {
     *     vix::process::ProcessSpec spec(
     *         executable.take());
     *
     *     // Configure and spawn the process.
     * }
     * @endcode
     *
     * @note
     * A valid name does not guarantee that the program
     * exists or can be executed.
     *
     * @note
     * Invalid names produce errors::invalid_argument.
     * Missing programs are reported during spawn().
     *
     * @warning
     * PATH resolution depends on the configured environment.
     * Only execute trusted programs when process integrity
     * or executable selection is security-sensitive.
     *
     * @see path()
     * @see ProcessSpec
     * @see spawn()
     */
    [[nodiscard]] static vix::Result<Executable> search_path(
        std::string_view);

  private:
    struct State;

    explicit Executable(std::shared_ptr<const State>) noexcept;

    std::shared_ptr<const State> state_;

    friend class ProcessSpec;
    friend vix::Result<Process> spawn(ProcessSpec);
    friend struct detail_process_access;
  };
} // namespace vix::process

#endif
