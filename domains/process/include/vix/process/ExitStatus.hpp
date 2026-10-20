/**
 * @file ExitStatus.hpp
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

#ifndef VIX_PROCESS_EXIT_STATUS_HPP
#define VIX_PROCESS_EXIT_STATUS_HPP

#include <cstdint>
#include <optional>
#include <string>

namespace vix::process
{
  /**
   * @brief Identifies a child process managed by Vix.
   *
   * @details
   * ProcessId is an opaque identifier associated with
   * a child process started through the Vix Process domain.
   *
   * It combines an internal generation identifier with
   * a native process identifier.
   *
   * Use Process::id() to obtain the identifier of a
   * running or completed child process.
   *
   * @par Example
   * @code{.cpp}
   * auto id = process.id();
   *
   * auto text = id.display();
   *
   * // Use text in logs or diagnostics.
   * @endcode
   *
   * @par Identity
   * Two ProcessId values can be compared for equality.
   *
   * The generation component helps distinguish
   * process records independently of native process
   * identifier reuse.
   *
   * @par Value semantics
   * ProcessId is copyable and movable.
   *
   * It does not own an operating-system process
   * or control its lifetime.
   *
   * @note
   * ProcessId is not necessarily equivalent to a
   * native operating-system PID.
   *
   * Its display representation is intended for
   * diagnostics, not for native process management
   * or cross-process identity exchange.
   *
   * @see Process
   * @see Process::id()
   */
  class ProcessId
  {
  public:
    /**
     * @brief Default construction is not supported.
     *
     * Obtain a ProcessId from Process::id().
     */
    ProcessId() = delete;

    /**
     * @brief Copies a process identifier.
     */
    ProcessId(const ProcessId &) noexcept = default;

    /**
     * @brief Move-constructs a process identifier.
     */
    ProcessId(ProcessId &&) noexcept = default;

    /**
     * @brief Copies another process identifier.
     *
     * @return Reference to this ProcessId.
     */
    ProcessId &operator=(const ProcessId &) noexcept = default;

    /**
     * @brief Move-assigns another process identifier.
     *
     * @return Reference to this ProcessId.
     */
    ProcessId &operator=(ProcessId &&) noexcept = default;

    /**
     * @brief Returns a diagnostic representation of the identifier.
     *
     * @return A string containing the process identifier.
     *
     * @details
     * The current representation combines the internal
     * generation and native identifier, separated by ':'.
     *
     * @par Example
     * @code{.cpp}
     * auto id = process.id();
     *
     * std::string text = id.display();
     *
     * // Log or display the process identity.
     * @endcode
     *
     * @note
     * The returned text should not be parsed to recover
     * a native PID or used as a persistent identity.
     *
     * @note
     * Diagnostic allocation exhaustion terminates
     * the process under Vix's allocation policy.
     *
     * @see Process::id()
     */
    [[nodiscard]] std::string display() const;

    /**
     * @brief Compares two process identifiers.
     *
     * @param left First process identifier.
     * @param right Second process identifier.
     *
     * @return true if both generation and native
     *         identifier components are equal,
     *         false otherwise.
     *
     * @note
     * Equality compares identity values. It does
     * not check whether a process is still running.
     */
    friend bool operator==(ProcessId, ProcessId) noexcept = default;

  private:
    ProcessId(std::uint64_t generation,
              std::uint64_t native) noexcept
        : generation_(generation),
          native_(native)
    {
    }

    std::uint64_t generation_;
    std::uint64_t native_;

    friend struct detail_process_record_access;
  };

  /**
   * @brief Classifies how a child process terminated.
   *
   * @details
   * ExitKind distinguishes ordinary process exit
   * from abnormal termination.
   *
   * @see ExitStatus
   */
  enum class ExitKind
  {
    exited,  ///< The process exited with an exit code.
    abnormal ///< The process terminated abnormally.
  };

  /**
   * @brief Describes the terminal status of a child process.
   *
   * @details
   * ExitStatus represents the observed outcome of a
   * child process after it has terminated.
   *
   * It provides:
   * - The termination classification.
   * - An optional numeric exit code.
   * - A convenience check for successful termination.
   *
   * Obtain an ExitStatus using Process::wait().
   *
   * @par Example
   * @code{.cpp}
   * auto result = process.wait();
   *
   * if (!result)
   * {
   *     // Handle the wait operation failure.
   * }
   * else
   * {
   *     const auto &status = result.value();
   *
   *     if (status.succeeded())
   *     {
   *         // Child exited successfully.
   *     }
   *     else if (auto code = status.exit_code())
   *     {
   *         // Child exited with a non-zero code.
   *     }
   *     else
   *     {
   *         // Child terminated abnormally.
   *     }
   * }
   * @endcode
   *
   * @par Successful termination
   * A process is considered successful only when:
   * - Its termination kind is ExitKind::exited.
   * - An exit code is available.
   * - The exit code equals zero.
   *
   * A non-zero exit code does not necessarily mean
   * that waiting for the process failed.
   *
   * It indicates that the child completed with
   * a non-success exit status.
   *
   * @par Abnormal termination
   * An abnormal termination is represented by
   * ExitKind::abnormal.
   *
   * The current process implementations do not
   * provide an exit code for abnormal termination.
   *
   * @par Value semantics
   * ExitStatus is a lightweight value type.
   *
   * It can be copied and moved without owning
   * or modifying the underlying process.
   *
   * @note
   * ExitStatus describes process termination.
   * It does not represent failures of spawn()
   * or wait(), which are reported separately.
   *
   * @see ExitKind
   * @see Process::wait()
   * @see Process
   */
  class ExitStatus
  {
  public:
    /**
     * @brief Default construction is not supported.
     *
     * ExitStatus values are produced by the
     * process implementation.
     */
    ExitStatus() = delete;

    /**
     * @brief Copies an exit status.
     */
    ExitStatus(const ExitStatus &) noexcept = default;

    /**
     * @brief Move-constructs an exit status.
     */
    ExitStatus(ExitStatus &&) noexcept = default;

    /**
     * @brief Copies another exit status.
     *
     * @return Reference to this ExitStatus.
     */
    ExitStatus &operator=(const ExitStatus &) noexcept = default;

    /**
     * @brief Move-assigns another exit status.
     *
     * @return Reference to this ExitStatus.
     */
    ExitStatus &operator=(ExitStatus &&) noexcept = default;

    /**
     * @brief Returns the process termination classification.
     *
     * @return ExitKind::exited for normal exit,
     *         or ExitKind::abnormal otherwise.
     *
     * @par Example
     * @code{.cpp}
     * if (status.kind() == ExitKind::abnormal)
     * {
     *     // Handle abnormal termination.
     * }
     * @endcode
     *
     * @see ExitKind
     * @see exit_code()
     */
    [[nodiscard]] ExitKind kind() const noexcept
    {
      return kind_;
    }

    /**
     * @brief Returns the child's exit code when available.
     *
     * @return The numeric exit code, or std::nullopt
     *         if no exit code is available.
     *
     * @details
     * A normal exit provides the code returned by
     * the child process.
     *
     * A zero exit code conventionally indicates success.
     * Non-zero values indicate other child-defined outcomes.
     *
     * Abnormal termination may have no exit code.
     *
     * @par Example
     * @code{.cpp}
     * if (auto code = status.exit_code())
     * {
     *     if (*code != 0)
     *     {
     *         // Handle the child's non-zero exit code.
     *     }
     * }
     * @endcode
     *
     * @note
     * Always check the optional value before accessing it.
     *
     * @see succeeded()
     * @see kind()
     */
    [[nodiscard]] std::optional<std::uint32_t> exit_code() const noexcept
    {
      return code_;
    }

    /**
     * @brief Checks whether the child exited successfully.
     *
     * @return true if the process exited normally
     *         with code zero, false otherwise.
     *
     * @details
     * Success requires all three conditions:
     * - kind() is ExitKind::exited.
     * - exit_code() contains a value.
     * - The exit code is zero.
     *
     * @par Example
     * @code{.cpp}
     * if (status.succeeded())
     * {
     *     // The child reported successful execution.
     * }
     * else
     * {
     *     // Inspect kind() and exit_code().
     * }
     * @endcode
     *
     * @note
     * A false result does not mean Process::wait()
     * itself failed. The child may simply have returned
     * a non-zero exit code.
     *
     * @see exit_code()
     * @see kind()
     */
    [[nodiscard]] bool succeeded() const noexcept
    {
      return kind_ == ExitKind::exited && code_ && *code_ == 0;
    }

  private:
    ExitStatus(ExitKind kind,
               std::optional<std::uint32_t> code) noexcept
        : kind_(kind),
          code_(code)
    {
    }

    ExitKind kind_;
    std::optional<std::uint32_t> code_;

    friend struct detail_process_record_access;
  };
} // namespace vix::process

#endif // VIX_PROCESS_EXIT_STATUS_HPP
