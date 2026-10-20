/**
 * @file standard.hpp
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
 * @brief Convenience header for the Vix Standard primitives.
 *
 * @details
 * This header provides a single entry point for the currently
 * approved Vix Standard types.
 *
 * Include <vix/standard.hpp> when an application needs several
 * Standard primitives without including each header separately.
 *
 * The available primitives cover:
 *
 * - Errors: ErrorIdentity and Error.
 * - Results: Result and Completion.
 * - Time: Duration, WallInstant, and MonotonicInstant.
 * - Clocks: WallClock and MonotonicClock.
 * - Deadlines: Deadline.
 * - Cancellation: CancellationSource, CancellationToken,
 *   and CancellationRegistration.
 *
 * @par Example
 * @code{.cpp}
 * #include <vix/standard.hpp>
 *
 * int main()
 * {
 *     auto timeout = vix::Duration::seconds(5);
 *     auto deadline = vix::Deadline::after(timeout);
 *
 *     if (deadline.reached())
 *     {
 *         // The deadline has been reached.
 *     }
 *
 *     return 0;
 * }
 * @endcode
 *
 * @par Header usage
 * Applications may include this convenience header or
 * include individual Standard headers directly.
 *
 * @note
 * This header exposes only the currently approved
 * Standard primitives. It does not establish a commitment
 * to a broader umbrella API or automatic inclusion of
 * future Vix domains.
 *
 * @see ErrorIdentity
 * @see Error
 * @see Duration
 * @see Deadline
 * @see CancellationSource
 * @see CompletionStatus
 */

#ifndef VIX_STANDARD_HPP
#define VIX_STANDARD_HPP

// Convenience boundary header for the currently approved Vix Standard
// primitives. It is intentionally not a commitment to a broad umbrella API.

#include <vix/Error.hpp>
#include <vix/ErrorIdentity.hpp>
#include <vix/Result.hpp>
#include <vix/Duration.hpp>
#include <vix/WallInstant.hpp>
#include <vix/MonotonicInstant.hpp>
#include <vix/WallClock.hpp>
#include <vix/MonotonicClock.hpp>
#include <vix/Deadline.hpp>
#include <vix/Cancellation.hpp>
#include <vix/Completion.hpp>

#endif // VIX_STANDARD_HPP
