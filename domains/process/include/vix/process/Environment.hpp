/**
 * @file Environment.hpp
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

#ifndef VIX_PROCESS_ENVIRONMENT_HPP
#define VIX_PROCESS_ENVIRONMENT_HPP

#include <memory>
#include <string_view>

#include <vix/Result.hpp>

namespace vix::process
{
  class Process;
  class ProcessSpec;

  vix::Result<Process> spawn(ProcessSpec);

  /**
   * @brief Configures environment variables for a child process.
   *
   * @details
   * Environment describes the environment that a child
   * process receives when started through spawn().
   *
   * It supports:
   * - Inheriting the parent process environment.
   * - Starting with an empty environment.
   * - Adding or replacing environment variables.
   * - Removing environment variables.
   *
   * Use inherit() or empty() to create an Environment,
   * then use with() and without() to customize it.
   *
   * @par Example
   * @code{.cpp}
   * #include <vix/process/ProcessDomain.hpp>
   *
   * int main()
   * {
   *     using namespace vix::process;
   *
   *     auto result = Environment::inherit()
   *         .with("APP_MODE", "development");
   *
   *     if (!result)
   *         return 1;
   *
   *     auto environment = result.take();
   *
   *     auto cleaned = environment.without("APP_SECRET");
   *
   *     if (!cleaned)
   *         return 1;
   *
   *     auto executable = Executable::search_path("git");
   *
   *     if (!executable)
   *         return 1;
   *
   *     ProcessSpec spec(executable.take());
   *     spec.set_environment(cleaned.take());
   *
   *     // The environment will be applied by spawn().
   *     return 0;
   * }
   * @endcode
   *
   * @par Immutable configuration
   * Environment uses value semantics.
   *
   * with() and without() return a new Environment
   * rather than modifying the existing value.
   *
   * This allows different child processes to use
   * different environment configurations.
   *
   * @par Inheritance
   * inherit() starts with the parent environment.
   * empty() starts without inherited variables.
   *
   * The effective environment is assembled when
   * spawn() prepares the child process.
   *
   * @par Platform behavior
   * Variable names are case-sensitive on POSIX.
   * Windows environment variable names are compared
   * without case sensitivity.
   *
   * Environment values are supplied as UTF-8 strings
   * and converted to native representations where needed.
   *
   * @par Ownership
   * Environment is copyable and movable.
   *
   * Copies share immutable internal state.
   * Customizing an Environment creates a new
   * configuration without changing existing copies.
   *
   * @par Error handling
   * Invalid variable names or values are reported
   * through vix::Result<Environment>.
   *
   * Diagnostic allocation exhaustion during Vix-controlled
   * operations terminates the process.
   *
   * @note
   * Environment configures child processes only.
   * It does not modify the current process environment.
   *
   * @note
   * An environment created with empty() does not
   * automatically contain PATH.
   *
   * When using Executable::search_path(), ensure
   * the effective child environment contains the
   * required executable search path.
   *
   * @see ProcessSpec
   * @see Executable
   * @see spawn()
   */
  class Environment
  {
  public:
    /**
     * @brief Default construction is not supported.
     *
     * Use inherit() or empty() to create an Environment.
     */
    Environment() = delete;

    /**
     * @brief Copies an environment configuration.
     *
     * @param other Environment to copy.
     *
     * @details
     * The copy shares immutable state with the original.
     */
    Environment(const Environment &);

    /**
     * @brief Move-constructs an environment configuration.
     *
     * @param other Environment to move from.
     */
    Environment(Environment &&) noexcept;

    /**
     * @brief Copies another environment configuration.
     *
     * @param other Environment to copy.
     * @return Reference to this Environment.
     */
    Environment &operator=(const Environment &);

    /**
     * @brief Move-assigns another environment configuration.
     *
     * @param other Environment to move from.
     * @return Reference to this Environment.
     */
    Environment &operator=(Environment &&) noexcept;

    /**
     * @brief Destroys the environment configuration.
     *
     * @note
     * Destruction does not modify environment variables
     * or affect already-running child processes.
     */
    ~Environment();

    /**
     * @brief Creates an environment that inherits parent variables.
     *
     * @return An Environment configured for inheritance.
     *
     * @details
     * The child starts with the parent process environment,
     * subject to subsequent additions and removals.
     *
     * Inherited variables are collected when spawn()
     * prepares the child process.
     *
     * @par Example
     * @code{.cpp}
     * auto environment =
     *     vix::process::Environment::inherit();
     * @endcode
     *
     * @note
     * Use with() to override inherited values.
     * Use without() to exclude inherited variables.
     *
     * @see empty()
     * @see with()
     * @see without()
     */
    [[nodiscard]] static Environment inherit() noexcept;

    /**
     * @brief Creates an environment without inherited variables.
     *
     * @return An initially empty Environment.
     *
     * @details
     * The child receives only variables explicitly
     * added through with().
     *
     * This is useful when a child process requires
     * a controlled environment.
     *
     * @par Example
     * @code{.cpp}
     * auto result = vix::process::Environment::empty()
     *     .with("APP_MODE", "production");
     *
     * if (!result)
     * {
     *     // Handle result.error().
     * }
     * @endcode
     *
     * @warning
     * An empty environment has no inherited PATH.
     * Executable lookup by name may fail unless
     * a suitable PATH is explicitly provided.
     *
     * @see inherit()
     * @see with()
     */
    [[nodiscard]] static Environment empty() noexcept;

    /**
     * @brief Adds or replaces an environment variable.
     *
     * @param name UTF-8 environment variable name.
     * @param value UTF-8 value assigned to the variable.
     *
     * @return A new Environment with the variable set,
     *         or an Error if validation fails.
     *
     * @details
     * Creates a new configuration based on this Environment.
     *
     * If the variable already exists, its effective value
     * is replaced.
     *
     * Otherwise, the variable is added.
     *
     * The original Environment remains unchanged.
     *
     * @par Example
     * @code{.cpp}
     * auto result = vix::process::Environment::inherit()
     *     .with("APP_PORT", "8080");
     *
     * if (result)
     * {
     *     auto environment = result.take();
     * }
     * @endcode
     *
     * @par Validation
     * The variable name must:
     * - Be non-empty.
     * - Contain valid UTF-8.
     * - Contain no '=' character.
     * - Contain no embedded NUL characters.
     *
     * The value must contain valid UTF-8 and no
     * embedded NUL characters.
     *
     * An empty value is permitted.
     *
     * @note
     * Invalid input produces errors::invalid_environment.
     *
     * @note
     * On Windows, variable name matching is
     * case-insensitive.
     *
     * @note
     * This function does not modify the environment
     * of the current process.
     *
     * @see without()
     * @see ProcessSpec::set_environment()
     */
    [[nodiscard]] vix::Result<Environment> with(
        std::string_view name,
        std::string_view value) const;

    /**
     * @brief Removes a variable from the child environment.
     *
     * @param name UTF-8 name of the variable to remove.
     *
     * @return A new Environment excluding the variable,
     *         or an Error if the name is invalid.
     *
     * @details
     * Creates a new environment configuration in which
     * the specified variable is absent.
     *
     * This applies to inherited variables as well as
     * variables added through with().
     *
     * Removing a variable that is already absent
     * does not produce an error.
     *
     * The original Environment remains unchanged.
     *
     * @par Example
     * @code{.cpp}
     * auto environment =
     *     vix::process::Environment::inherit();
     *
     * auto result = environment.without("APP_SECRET");
     *
     * if (result)
     * {
     *     auto filtered = result.take();
     * }
     * @endcode
     *
     * @par Validation
     * The variable name must:
     * - Be non-empty.
     * - Contain valid UTF-8.
     * - Contain no '=' character.
     * - Contain no embedded NUL characters.
     *
     * @note
     * Invalid names produce errors::invalid_environment.
     *
     * @note
     * On Windows, variable name matching is
     * case-insensitive.
     *
     * @note
     * Removing a variable from a child environment
     * does not remove it from the current process.
     *
     * @see with()
     * @see ProcessSpec::set_environment()
     */
    [[nodiscard]] vix::Result<Environment> without(
        std::string_view name) const;

  private:
    struct State;

    explicit Environment(std::shared_ptr<const State>) noexcept;

    std::shared_ptr<const State> state_;

    friend class ProcessSpec;
    friend vix::Result<Process> spawn(ProcessSpec);
    friend struct detail_process_access;
  };
} // namespace vix::process

#endif // VIX_PROCESS_ENVIRONMENT_HPP
