/**
 * @file concurrency.hpp
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
 * @brief Public entry point for the Vix Concurrency domain.
 *
 * @details
 * Provides the public API for executing bounded concurrent
 * work using explicitly owned execution resources.
 *
 * Including this header makes the following capabilities
 * available:
 * - Creating execution resources with native workers.
 * - Configuring worker concurrency and pending capacity.
 * - Submitting callable tasks.
 * - Observing task completion and retrieving results.
 * - Requesting cooperative task cancellation.
 * - Configuring monotonic deadlines.
 * - Registering one-shot completion notifications.
 * - Performing orderly execution resource shutdown.
 *
 * @par Quick start
 * @code{.cpp}
 * #include <vix/concurrency.hpp>
 *
 * int main()
 * {
 *     using namespace vix::concurrency;
 *
 *     auto created = ExecutionResource::create();
 *
 *     if (!created)
 *         return 1;
 *
 *     auto resource = created.take();
 *
 *     auto submitted = resource.submit([] {
 *         return 42;
 *     });
 *
 *     if (!submitted)
 *         return 1;
 *
 *     auto work = submitted.take();
 *
 *     auto completed = work.take_completion();
 *
 *     if (!completed)
 *         return 1;
 *
 *     if (!resource.shutdown())
 *         return 1;
 *
 *     return completed.value().is_success() ? 0 : 1;
 * }
 * @endcode
 *
 * @par Execution model
 * ExecutionResource owns a bounded set of native workers
 * and controls admission of submitted work.
 *
 * The maximum accepted workload is determined by
 * max_concurrency and max_pending.
 *
 * By default, one worker is created and no pending
 * queue capacity is configured.
 *
 * @par Task results
 * Submitted callables may return:
 * - A plain value.
 * - void.
 * - vix::Result<T>.
 * - vix::Completion<T>.
 *
 * Their outcomes are normalized into vix::Completion<T>.
 *
 * WorkHandle<T> provides shared observation of one
 * accepted task.
 *
 * Completion values can be consumed only once,
 * even when multiple WorkHandle copies exist.
 *
 * @par Cancellation and deadlines
 * Cancellation is cooperative.
 *
 * Running tasks are not forcibly interrupted.
 * They can observe cancellation through WorkContext.
 *
 * Tasks cancelled before execution may complete
 * without invoking their callable.
 *
 * Deadlines use the monotonic clock provided by
 * Vix Standard.
 *
 * A deadline reached during execution requests
 * cancellation without discarding the callable's
 * eventual outcome.
 *
 * @par Completion notifications
 * WorkHandle<T>::on_ready() allows applications to
 * register a one-shot notification for task completion.
 *
 * Accepted callbacks execute on workers belonging
 * to the originating ExecutionResource.
 *
 * NotificationRegistration manages callback lifetime,
 * deactivation, and completion observation.
 *
 * @par Shutdown
 * ExecutionResource::shutdown() closes admission,
 * requests cancellation of accepted work, and waits
 * for worker and notification quiescence.
 *
 * is_shutdown() indicates that admission is closed;
 * it does not guarantee that all workers have stopped.
 *
 * @par Platform support
 * Native worker execution is provided on supported
 * Linux, macOS, Windows, and Android platforms.
 *
 * On WASI Preview 2, native worker execution is
 * unavailable and resource creation reports
 * errors::execution_unavailable.
 *
 * @note
 * This header is a convenience include.
 * It introduces no additional runtime functionality.
 *
 * Applications may include
 * <vix/concurrency/ExecutionResource.hpp> directly.
 *
 * @see vix::concurrency::ExecutionResource
 * @see vix::concurrency::WorkHandle
 * @see vix::concurrency::WorkContext
 * @see vix::concurrency::WorkOptions
 * @see vix::concurrency::ReadySubscription
 * @see vix::concurrency::NotificationRegistration
 */

#ifndef VIX_CONCURRENCY_HPP
#define VIX_CONCURRENCY_HPP

#include <vix/concurrency/ExecutionResource.hpp>

#endif // VIX_CONCURRENCY_HPP
