/**
 * @file ProcessSpec.hpp
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

#ifndef VIX_PROCESS_PROCESS_SPEC_HPP
#define VIX_PROCESS_PROCESS_SPEC_HPP

#include <memory>
#include <optional>
#include <string_view>

#include <vix/Result.hpp>
#include <vix/files/Path.hpp>

#include <vix/process/Environment.hpp>
#include <vix/process/Executable.hpp>
#include <vix/process/Stdio.hpp>

namespace vix::process
{
  class Process;
  class ProcessSpec;

  vix::Result<Process> spawn(ProcessSpec);

  /**
   * @brief Configures a child process before execution.
   *
   * @details
   * ProcessSpec describes how a child process should
   * be started.
   *
   * It allows applications to configure:
   * - The executable to launch.
   * - Command-line arguments.
   * - Environment variables.
   * - The working directory.
   * - Standard input, output, and error streams.
   *
   * A ProcessSpec is a configuration value, not a
   * running process. Use spawn() to start the child.
   *
   * @par Example
   * @code{.cpp}
   * #include <vix/process/ProcessDomain.hpp>
   *
   * int main()
   * {
   *     using namespace vix::process;
   *
   *     auto executable = Executable::search_path("git");
   *     if (!executable)
   *         return 1;
   *
   *     ProcessSpec spec(executable.take());
   *
   *     if (!spec.add_argument("--version"))
   *         return 1;
   *
   *     spec.set_stdout(StandardOutput::inherit());
   *     spec.set_stderr(StandardError::inherit());
   *
   *     auto started = spawn(spec);
   *     if (!started)
   *         return 1;
   *
   *     auto child = started.take();
   *     auto status = child.wait();
   *
   *     if (!status)
   *         return 1;
   *
   *     return status.value().succeeded() ? 0 : 1;
   * }
   * @endcode
   *
   * @par Default configuration
   * A newly constructed ProcessSpec:
   * - Has no additional command-line arguments.
   * - Inherits the parent process environment.
   * - Has no explicitly configured working directory.
   * - Inherits stdin, stdout, and stderr.
   *
   * @par Configuration and execution
   * Configuration methods do not launch a process.
   *
   * Some input validation occurs when configuring
   * the specification. Filesystem checks, executable
   * resolution, and process creation occur during spawn().
   *
   * @par Ownership
   * ProcessSpec owns its configuration state.
   *
   * It supports copying and moving.
   *
   * Copying creates an independent configuration,
   * allowing different child processes to use
   * different arguments, environments, and streams.
   *
   * File-backed standard stream configurations borrow
   * their File objects. Copies of ProcessSpec do not
   * extend the lifetime of those borrowed files.
   *
   * @par Error handling
   * Operations that validate user input return
   * vix::Result<void>.
   *
   * Expected launch failures are reported by spawn().
   *
   * @note
   * ProcessSpec does not control a running child's
   * lifetime. After spawning, use Process to observe
   * or manage the child.
   *
   * @see Executable
   * @see Environment
   * @see StandardInput
   * @see StandardOutput
   * @see StandardError
   * @see Process
   * @see spawn()
   */
  class ProcessSpec
  {
  public:
    /**
     * @brief Default construction is not supported.
     *
     * A ProcessSpec requires an Executable.
     */
    ProcessSpec() = delete;

    /**
     * @brief Creates a process specification.
     *
     * @param executable Program to execute.
     *
     * @details
     * Initializes the specification with inherited
     * environment and standard streams.
     *
     * No additional arguments or explicit working
     * directory are configured initially.
     *
     * @par Example
     * @code{.cpp}
     * auto executable = Executable::search_path("git");
     *
     * if (executable)
     * {
     *     ProcessSpec spec(executable.take());
     * }
     * @endcode
     *
     * @note
     * Construction does not start the program.
     *
     * @see Executable
     * @see spawn()
     */
    explicit ProcessSpec(Executable);

    /**
     * @brief Copies a process specification.
     *
     * @param other Specification to copy.
     *
     * @details
     * Creates an independent copy of the configuration.
     *
     * Changes to the copy do not change the original.
     *
     * File-backed stream configurations continue
     * borrowing their original File objects.
     */
    ProcessSpec(const ProcessSpec &);

    /**
     * @brief Move-constructs a process specification.
     *
     * @param other Specification to move from.
     *
     * @details
     * Transfers ownership of the configuration state
     * to the new object.
     */
    ProcessSpec(ProcessSpec &&) noexcept;

    /**
     * @brief Copies another process specification.
     *
     * @param other Specification to copy.
     * @return Reference to this ProcessSpec.
     *
     * @details
     * Replaces the current configuration with
     * an independent copy of the supplied one.
     */
    ProcessSpec &operator=(const ProcessSpec &);

    /**
     * @brief Move-assigns another process specification.
     *
     * @param other Specification to move from.
     * @return Reference to this ProcessSpec.
     */
    ProcessSpec &operator=(ProcessSpec &&) noexcept;

    /**
     * @brief Destroys the process specification.
     *
     * @note
     * Destruction releases configuration resources
     * but does not terminate a spawned child process.
     */
    ~ProcessSpec();

    /**
     * @brief Appends a command-line argument.
     *
     * @param value UTF-8 argument to append.
     *
     * @return Success if the argument is accepted,
     *         or an Error if validation fails.
     *
     * @details
     * Each call appends one argument in the order supplied.
     *
     * Arguments are stored as independent strings.
     * The caller does not need to keep the original
     * string_view alive after the call returns.
     *
     * The argument must contain valid UTF-8 and
     * must not contain embedded NUL characters.
     *
     * An empty argument is permitted.
     *
     * @par Example
     * @code{.cpp}
     * if (!spec.add_argument("--version"))
     * {
     *     // Handle invalid argument.
     * }
     * @endcode
     *
     * @note
     * Invalid arguments produce errors::invalid_argument.
     *
     * @note
     * Arguments are passed as individual values.
     * Shell quoting and command-line interpretation
     * are not performed by this function.
     *
     * @see Executable
     */
    [[nodiscard]] vix::Result<void> add_argument(
        std::string_view);

    /**
     * @brief Sets the environment for the child process.
     *
     * @param environment Environment configuration to use.
     *
     * @details
     * Replaces the previously configured environment.
     *
     * Use Environment::inherit() to inherit parent
     * variables or Environment::empty() to begin
     * with an empty environment.
     *
     * The effective environment is prepared during spawn().
     *
     * @par Example
     * @code{.cpp}
     * auto environment = Environment::inherit()
     *     .with("APP_MODE", "production");
     *
     * if (environment)
     * {
     *     spec.set_environment(environment.take());
     * }
     * @endcode
     *
     * @note
     * This function does not modify the current
     * process environment.
     *
     * @see Environment
     * @see spawn()
     */
    void set_environment(Environment);

    /**
     * @brief Sets the child's working directory.
     *
     * @param path Directory to use as the child's
     *             working directory.
     *
     * @return Success if the path is accepted,
     *         or an Error if validation fails.
     *
     * @details
     * The supplied path must not be empty.
     *
     * The path may be absolute or relative.
     *
     * This function stores the configuration without
     * checking whether the directory exists.
     *
     * Directory availability and accessibility are
     * checked when spawn() starts the child.
     *
     * @par Example
     * @code{.cpp}
     * auto directory = vix::files::Path::from_utf8(
     *     "workspace");
     *
     * if (directory)
     * {
     *     auto result = spec.set_working_directory(
     *         directory.take());
     *
     *     if (!result)
     *     {
     *         // Handle invalid working directory.
     *     }
     * }
     * @endcode
     *
     * @note
     * An empty path produces
     * errors::invalid_working_directory.
     *
     * @note
     * This function does not change the working
     * directory of the current process.
     *
     * @see vix::files::Path
     * @see spawn()
     */
    [[nodiscard]] vix::Result<void> set_working_directory(
        vix::files::Path);

    /**
     * @brief Configures the child's standard input.
     *
     * @param input Standard input configuration.
     *
     * @details
     * Replaces the current stdin configuration.
     *
     * Available modes include:
     * - Inheriting the parent's stdin.
     * - Reading from a null input source.
     * - Creating a parent-writable pipe.
     * - Reading from an existing File.
     *
     * @par Example
     * @code{.cpp}
     * spec.set_stdin(StandardInput::pipe());
     * @endcode
     *
     * @note
     * File-backed configurations borrow their File.
     * The borrowed file must remain alive and open
     * until spawn() finishes using it.
     *
     * @see StandardInput
     * @see Process::take_stdin()
     */
    void set_stdin(StandardInput);

    /**
     * @brief Configures the child's standard output.
     *
     * @param output Standard output configuration.
     *
     * @details
     * Replaces the current stdout configuration.
     *
     * Available modes include:
     * - Inheriting the parent's stdout.
     * - Discarding output.
     * - Capturing output through a pipe.
     * - Writing output to an existing File.
     *
     * @par Example
     * @code{.cpp}
     * spec.set_stdout(StandardOutput::pipe());
     * @endcode
     *
     * @warning
     * When stdout is piped, the parent must drain
     * the pipe while the child produces output.
     * Otherwise, a full pipe may block the child.
     *
     * @note
     * File-backed configurations borrow their File
     * until spawn() finishes preparing the child.
     *
     * @see StandardOutput
     * @see Process::take_stdout()
     */
    void set_stdout(StandardOutput);

    /**
     * @brief Configures the child's standard error.
     *
     * @param error Standard error configuration.
     *
     * @details
     * Replaces the current stderr configuration.
     *
     * Available modes include:
     * - Inheriting the parent's stderr.
     * - Discarding diagnostic output.
     * - Capturing stderr through a pipe.
     * - Writing stderr to an existing File.
     * - Merging stderr with stdout.
     *
     * @par Example
     * @code{.cpp}
     * spec.set_stdout(StandardOutput::pipe());
     * spec.set_stderr(
     *     StandardError::merge_with_stdout());
     * @endcode
     *
     * @note
     * When stderr is merged with stdout, both
     * streams share the configured stdout destination.
     *
     * File-backed configurations borrow their File
     * until spawn() finishes preparing the child.
     *
     * @warning
     * Separately piped stdout and stderr may both
     * require draining to prevent child-process blocking.
     *
     * @see StandardError
     * @see StandardOutput
     * @see Process::take_stderr()
     */
    void set_stderr(StandardError);

  private:
    struct State;

    std::unique_ptr<State> state_;

    friend struct detail_process_access;
    friend class Process;
    friend vix::Result<Process> spawn(ProcessSpec);
  };
} // namespace vix::process

#endif // VIX_PROCESS_PROCESS_SPEC_HPP
