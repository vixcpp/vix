/**
 * @file ProcessDomain.hpp
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
 * @brief Public entry point for the Vix Process domain.
 *
 * @details
 * Provides the complete public API for creating,
 * configuring, and managing child processes.
 *
 * Including this header makes the following
 * capabilities available:
 * - Selecting executables by path or through PATH.
 * - Configuring command-line arguments.
 * - Managing child process environments.
 * - Setting the working directory.
 * - Configuring standard input, output, and error.
 * - Reading from and writing to child process pipes.
 * - Waiting for process termination.
 * - Supporting cancellation and waiting deadlines.
 * - Requesting child process termination.
 * - Inspecting process identities and exit statuses.
 * - Handling process-related errors.
 *
 * @par Quick start
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
 *     auto child = started.take();
 *
 *     auto result = child.wait();
 *
 *     if (!result)
 *         return 1;
 *
 *     return result.value().succeeded() ? 0 : 1;
 * }
 * @endcode
 *
 * @par Configuration model
 * Process execution consists of three main steps:
 *
 * 1. Select an executable using Executable.
 * 2. Configure execution using ProcessSpec.
 * 3. Start and manage the child using spawn() and Process.
 *
 * ProcessSpec separates process configuration from
 * execution, allowing arguments, environment variables,
 * working directories, and standard streams to be
 * configured before launching the child.
 *
 * @par Process ownership
 * A successful spawn() returns an owning Process.
 *
 * Process objects are non-copyable and move-constructible.
 * Destroying a Process does not automatically terminate
 * the child process.
 *
 * Use wait() to observe completion and terminate()
 * when forced termination is required.
 *
 * @par Standard streams
 * StandardInput, StandardOutput, and StandardError
 * configure how the child communicates with its parent.
 *
 * When pipes are requested, the corresponding endpoints
 * can be transferred from Process:
 * - take_stdin() provides ChildInput.
 * - take_stdout() provides ChildOutput.
 * - take_stderr() provides ChildOutput.
 *
 * Each pipe endpoint can be transferred only once.
 *
 * @par Waiting and cancellation
 * Process::wait() observes the child's final status.
 *
 * WaitOptions adds support for cancellation tokens
 * and optional deadlines.
 *
 * Cancellation and deadline expiration affect only
 * the waiting operation. They do not automatically
 * terminate the child process.
 *
 * @par Error handling
 * Expected operational failures are reported through
 * vix::Result<T> or vix::Completion<T>.
 *
 * Process-specific error identities are defined in
 * the vix::process::errors namespace.
 *
 * A non-zero child exit code is represented by
 * ExitStatus, not by a failed wait operation.
 *
 * @par Platform support
 * The Process domain provides native implementations
 * for supported POSIX and Windows platforms.
 *
 * On WASI Preview 2, native child-process creation
 * is unavailable and spawn() reports
 * errors::unsupported_capability.
 *
 * @note
 * This header is a convenience include.
 * It introduces no additional runtime functionality
 * beyond the APIs declared in its included headers.
 *
 * Applications may include individual headers
 * when only a subset of the Process API is needed.
 *
 * @see vix::process::Executable
 * @see vix::process::ProcessSpec
 * @see vix::process::Environment
 * @see vix::process::StandardInput
 * @see vix::process::StandardOutput
 * @see vix::process::StandardError
 * @see vix::process::Process
 * @see vix::process::ChildInput
 * @see vix::process::ChildOutput
 * @see vix::process::ExitStatus
 * @see vix::process::WaitOptions
 * @see vix::process::spawn()
 */

#ifndef VIX_PROCESS_DOMAIN_HPP
#define VIX_PROCESS_DOMAIN_HPP

#include <vix/process/ChildInput.hpp>
#include <vix/process/ChildOutput.hpp>
#include <vix/process/Environment.hpp>
#include <vix/process/Errors.hpp>
#include <vix/process/Executable.hpp>
#include <vix/process/ExitStatus.hpp>
#include <vix/process/Process.hpp>
#include <vix/process/ProcessSpec.hpp>
#include <vix/process/Stdio.hpp>

#endif
