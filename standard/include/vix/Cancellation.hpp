/**
 * @file Cancellation.hpp
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

#ifndef VIX_CANCELLATION_HPP
#define VIX_CANCELLATION_HPP

#include <atomic>
#include <algorithm>
#include <condition_variable>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

#include <vix/detail/AllocationFailure.hpp>

namespace vix::detail
{
  [[noreturn]] inline void cancellation_contract_violation() noexcept
  {
    std::terminate();
  }

  inline thread_local unsigned int cancellation_callback_depth{0};

  class CancellationCallbackScope
  {
  public:
    CancellationCallbackScope() noexcept
    {
      ++cancellation_callback_depth;
    }

    ~CancellationCallbackScope()
    {
      --cancellation_callback_depth;
    }
  };

  [[nodiscard]] inline bool in_cancellation_callback() noexcept
  {
    return cancellation_callback_depth != 0;
  }

  struct CancellationCallback
  {
    explicit CancellationCallback(std::function<void()> callback_value)
        : callback(std::move(callback_value))
    {
    }

    [[nodiscard]] bool begin_invocation() noexcept
    {
      std::lock_guard lock(mutex);
      if (!active || running)
      {
        return false;
      }

      running = true;
      running_thread = std::this_thread::get_id();
      return true;
    }

    void finish_invocation() noexcept
    {
      {
        std::lock_guard lock(mutex);
        running = false;
        running_thread = {};
      }
      completed.notify_all();
    }

    void invoke() noexcept
    {
      if (!begin_invocation())
      {
        return;
      }

      CancellationCallbackScope scope;
      // invoke() is noexcept: an escaping callback exception is a Fault and
      // deterministically terminates rather than changing cancellation into
      // Error flow.
      callback();
      finish_invocation();
    }

    void deactivate_and_wait() noexcept
    {
      std::unique_lock lock(mutex);
      active = false;

      // Callback-context reset is nonblocking. This avoids both self-reset
      // deadlock and cycles where concurrently running callbacks reset each
      // other's registrations. The callback is inactive before return, while
      // an already-running invocation retains its entry and captures.
      if (running && (running_thread == std::this_thread::get_id() ||
                      in_cancellation_callback()))
      {
        return;
      }

      completed.wait(lock, [this]
                     { return !running; });
    }

    std::mutex mutex;
    std::condition_variable completed;
    std::function<void()> callback;
    bool active{true};
    bool running{false};
    std::thread::id running_thread{};
  };

  struct CancellationState
  {
    [[nodiscard]] bool request_cancel() noexcept
    {
      std::vector<std::shared_ptr<CancellationCallback>> callbacks_to_invoke;
      {
        std::lock_guard lock(mutex);
        if (requested)
        {
          return false;
        }

        requested = true;
        requested_atomic.store(true, std::memory_order_release);
        callbacks_to_invoke.swap(callbacks);
      }

      // Callbacks selected by the winning request run synchronously on the
      // requesting thread. Their order is intentionally unspecified.
      for (const auto &callback : callbacks_to_invoke)
      {
        callback->invoke();
      }

      return true;
    }

    [[nodiscard]] bool is_cancel_requested() const noexcept
    {
      return requested_atomic.load(std::memory_order_acquire);
    }

    std::mutex mutex;
    bool requested{false};
    std::atomic<bool> requested_atomic{false};
    std::vector<std::shared_ptr<CancellationCallback>> callbacks;
  };
} // namespace vix::detail

namespace vix
{
  class CancellationToken;

  /**
   * @brief Manages the lifetime of a cancellation callback.
   *
   * @details
   * CancellationRegistration represents an active callback
   * registered through CancellationToken::on_cancel().
   *
   * It uses RAII to automatically unregister the callback
   * when the registration is destroyed.
   *
   * Registrations can be moved but cannot be copied.
   *
   * @par Example
   * @code{.cpp}
   * vix::CancellationSource source;
   *
   * auto registration = source.token().on_cancel([] {
   *     // React to a cancellation request.
   * });
   *
   * // Unregister before leaving the scope.
   * registration.reset();
   * @endcode
   *
   * @par Callback lifetime
   * Calling reset() or destroying a registration prevents
   * new invocations of its callback.
   *
   * Outside cancellation callbacks, reset() normally
   * waits for an already-running callback to finish.
   *
   * When called from any cancellation callback, reset()
   * does not wait for running callbacks. This prevents
   * self-deadlocks and callback wait cycles.
   *
   * An already-running callback retains its captured
   * resources until that invocation finishes.
   *
   * @par Thread safety
   * Registration teardown coordinates with concurrent
   * cancellation requests.
   *
   * Do not concurrently modify or reset the same
   * CancellationRegistration object without external
   * synchronization.
   *
   * @warning
   * Do not hold a lock needed by the registered callback
   * while calling reset() outside callback execution.
   * Doing so may cause a deadlock.
   *
   * @see CancellationToken
   * @see CancellationSource
   */
  class CancellationRegistration
  {
  public:
    /**
     * @brief Constructs an empty registration.
     *
     * An empty registration owns no callback.
     * Calling reset() on it is safe.
     */
    CancellationRegistration() noexcept = default;

    CancellationRegistration(const CancellationRegistration &) = delete;
    CancellationRegistration &operator=(const CancellationRegistration &) = delete;

    /**
     * @brief Transfers ownership of a registration.
     *
     * @param other Registration to move from.
     *
     * @details
     * The destination takes ownership of the registered
     * callback and its lifetime.
     */
    CancellationRegistration(CancellationRegistration &&other) noexcept
        : state_(std::move(other.state_)),
          callback_(std::move(other.callback_))
    {
    }

    /**
     * @brief Replaces this registration by moving another.
     *
     * @param other Registration whose ownership is transferred.
     * @return Reference to this registration.
     *
     * @details
     * Any currently owned callback is unregistered before
     * the replacement registration is adopted.
     *
     * @see reset()
     */
    CancellationRegistration &operator=(CancellationRegistration &&other) noexcept
    {
      if (this != &other)
      {
        reset();
        state_ = std::move(other.state_);
        callback_ = std::move(other.callback_);
      }
      return *this;
    }

    /**
     * @brief Unregisters the callback and destroys the registration.
     *
     * @details
     * Automatically invokes reset() to release the
     * registration's callback ownership.
     *
     * @see reset()
     */
    ~CancellationRegistration()
    {
      reset();
    }

    /**
     * @brief Unregisters the cancellation callback.
     *
     * @details
     * Prevents the registered callback from beginning
     * a new invocation.
     *
     * If the callback is already running, reset() normally
     * waits for that invocation to finish.
     *
     * When reset() is called from a cancellation callback,
     * it is nonblocking. An already-running invocation
     * may continue after reset() returns.
     *
     * @par Example
     * @code{.cpp}
     * vix::CancellationSource source;
     *
     * auto registration = source.token().on_cancel([] {
     *     // Cancellation notification.
     * });
     *
     * registration.reset();
     * @endcode
     *
     * @note
     * Calling reset() multiple times is safe.
     *
     * @warning
     * Do not hold locks needed by callback execution
     * when resetting outside callback context.
     */
    void reset() noexcept
    {
      if (callback_)
      {
        if (state_)
        {
          std::lock_guard lock(state_->mutex);
          auto &callbacks = state_->callbacks;
          const auto position = std::find(callbacks.begin(),
                                          callbacks.end(),
                                          callback_);
          if (position != callbacks.end())
          {
            callbacks.erase(position);
          }
        }
        callback_->deactivate_and_wait();
      }
      callback_.reset();
      state_.reset();
    }

  private:
    friend class CancellationToken;

    CancellationRegistration(std::shared_ptr<detail::CancellationState> state,
                             std::shared_ptr<detail::CancellationCallback> callback) noexcept
        : state_(std::move(state)),
          callback_(std::move(callback))
    {
    }

    std::shared_ptr<detail::CancellationState> state_;
    std::shared_ptr<detail::CancellationCallback> callback_;
  };

  /**
   * @brief Observes cancellation requests without owning cancellation authority.
   *
   * @details
   * CancellationToken provides read access to a shared
   * cancellation state.
   *
   * Tokens are obtained from CancellationSource::token().
   *
   * A token allows callers to:
   * - Check whether cancellation has been requested.
   * - Register a callback for cancellation notification.
   *
   * A token cannot request cancellation.
   *
   * @par Example
   * @code{.cpp}
   * vix::CancellationSource source;
   * auto token = source.token();
   *
   * if (token.is_cancel_requested())
   * {
   *     // Cancellation was requested.
   * }
   * @endcode
   *
   * @par Ownership
   * CancellationToken is copyable and movable.
   *
   * Copies share the same cancellation state.
   * The shared state remains alive while tokens or
   * other owners retain it.
   *
   * Tokens remain valid after the originating
   * CancellationSource has been destroyed.
   *
   * @par Default token
   * A default-constructed token has no cancellation state.
   *
   * It always reports false from is_cancel_requested()
   * and cannot receive cancellation notifications.
   *
   * @par Thread safety
   * Cancellation requests can be observed concurrently.
   * Callbacks can be registered while cancellation is
   * being requested.
   *
   * Concurrent mutation of the same token object through
   * assignment or moving requires external synchronization.
   *
   * @note
   * Cancellation is a request, not proof that an operation
   * has stopped or completed.
   *
   * @see CancellationSource
   * @see CancellationRegistration
   */
  class CancellationToken
  {
  public:
    /**
     * @brief Constructs a token without cancellation state.
     *
     * The resulting token permanently reports no
     * cancellation request.
     */
    CancellationToken() noexcept = default;

    CancellationToken(const CancellationToken &) = default;
    CancellationToken(CancellationToken &&) noexcept = default;
    CancellationToken &operator=(const CancellationToken &) = default;
    CancellationToken &operator=(CancellationToken &&) noexcept = default;
    ~CancellationToken() = default;

    /**
     * @brief Checks whether cancellation has been requested.
     *
     * @return true if cancellation has been requested,
     *         false otherwise.
     *
     * @details
     * Returns false for a default token.
     *
     * For a token associated with a CancellationSource,
     * the value becomes true when cancellation is requested.
     *
     * @note
     * This function is thread-safe and does not wait
     * for registered callbacks to finish.
     */
    [[nodiscard]] bool is_cancel_requested() const noexcept
    {
      return state_ && state_->is_cancel_requested();
    }

    /**
     * @brief Registers a callback for cancellation notification.
     *
     * @param callback Function invoked when cancellation
     *                 is requested.
     *
     * @return A registration controlling callback lifetime,
     *         or an empty registration when no active
     *         registration is needed.
     *
     * @details
     * If cancellation has not yet been requested, the
     * callback is registered for future notification.
     *
     * If cancellation was already requested, the callback
     * executes synchronously before on_cancel() returns.
     * The returned registration is empty.
     *
     * For a default token, no callback is registered
     * and an empty registration is returned.
     *
     * Each registered callback executes at most once.
     *
     * @par Example
     * @code{.cpp}
     * vix::CancellationSource source;
     *
     * auto token = source.token();
     *
     * auto registration = token.on_cancel([] {
     *     // React to cancellation.
     * });
     *
     * source.request_cancel();
     * @endcode
     *
     * @par Execution
     * Callbacks execute synchronously.
     *
     * A callback registered before cancellation normally
     * runs on the thread requesting cancellation.
     *
     * A callback registered after cancellation runs
     * on the registering thread before this call returns.
     *
     * Callback execution order is unspecified.
     *
     * @par Lifetime
     * Keep the returned CancellationRegistration alive
     * for as long as notification is required.
     *
     * Destroying or resetting it unregisters the callback.
     *
     * @warning
     * The callback must not be empty.
     * An empty callback violates the contract and
     * terminates the process.
     *
     * @warning
     * Exceptions escaping a callback terminate the process.
     *
     * @warning
     * Callback registration may block while executing
     * a previously requested cancellation.
     *
     * Do not hold locks required by arbitrary cancellation
     * callbacks during registration.
     *
     * @see CancellationRegistration
     * @see CancellationSource::request_cancel()
     */
    [[nodiscard]] CancellationRegistration on_cancel(
        std::function<void()> callback) const
    {
      if (!callback)
      {
        detail::cancellation_contract_violation();
      }

      if (!state_)
      {
        return {};
      }

      return detail::allocation_or_terminate(
          [&]() -> CancellationRegistration
          {
            auto entry = std::make_shared<detail::CancellationCallback>(std::move(callback));
            bool invoke_now = false;
            {
              std::lock_guard lock(state_->mutex);
              if (state_->requested)
              {
                invoke_now = true;
              }
              else
              {
                state_->callbacks.push_back(entry);
              }
            }

            if (invoke_now)
            {
              entry->invoke();
              entry->deactivate_and_wait();
              return {};
            }

            return CancellationRegistration(state_, std::move(entry));
          });
    }

  private:
    friend class CancellationSource;

    explicit CancellationToken(std::shared_ptr<detail::CancellationState> state) noexcept
        : state_(std::move(state))
    {
    }

    std::shared_ptr<detail::CancellationState> state_;
  };

  /**
   * @brief Owns the authority to request cancellation.
   *
   * @details
   * CancellationSource creates and manages shared
   * cancellation-request state.
   *
   * Use token() to provide cancellation observation
   * to another part of an application.
   *
   * Use request_cancel() to signal that cancellation
   * has been requested.
   *
   * @par Example
   * @code{.cpp}
   * vix::CancellationSource source;
   *
   * auto token = source.token();
   *
   * auto registration = token.on_cancel([] {
   *     // React to a cancellation request.
   * });
   *
   * bool first_request = source.request_cancel();
   *
   * if (first_request)
   * {
   *     // This call initiated cancellation.
   * }
   * @endcode
   *
   * @par Shared authority
   * CancellationSource is copyable.
   *
   * All copies share the same cancellation state.
   * Any copy can request cancellation for that state.
   *
   * @par Request semantics
   * Cancellation is requested at most once per state.
   *
   * request_cancel() returns true only for the call
   * that first requests cancellation.
   *
   * Later requests return false.
   *
   * @par Callback execution
   * Registered callbacks run synchronously on the
   * thread that successfully requests cancellation.
   *
   * Their execution order is unspecified.
   *
   * request_cancel() may block while callbacks execute.
   *
   * @par Lifetime
   * Destroying a CancellationSource does not
   * automatically request cancellation.
   *
   * Existing tokens and other source copies may
   * continue using the shared cancellation state.
   *
   * @par Thread safety
   * Cancellation requests and state observations
   * are coordinated for concurrent use.
   *
   * Concurrent operations on the same source object
   * that modify its ownership through assignment
   * or moving require external synchronization.
   *
   * @warning
   * Do not hold a lock required by a registered callback
   * while requesting cancellation.
   *
   * @note
   * Requesting cancellation does not itself guarantee
   * that an operation has stopped.
   *
   * @see CancellationToken
   * @see CancellationRegistration
   */
  class CancellationSource
  {
  public:
    /**
     * @brief Creates an independent cancellation source.
     *
     * @details
     * Allocates a new cancellation state.
     *
     * Initially, cancellation has not been requested.
     *
     * Copies of this source share the state, while
     * separately constructed sources have independent
     * cancellation states.
     */
    CancellationSource()
        : state_(detail::allocation_or_terminate(
              []
              { return std::make_shared<detail::CancellationState>(); }))
    {
    }

    CancellationSource(const CancellationSource &) = default;
    CancellationSource(CancellationSource &&) noexcept = default;
    CancellationSource &operator=(const CancellationSource &) = default;
    CancellationSource &operator=(CancellationSource &&) noexcept = default;
    ~CancellationSource() = default;

    /**
     * @brief Returns a token observing this source.
     *
     * @return A CancellationToken sharing the current
     *         cancellation state.
     *
     * @details
     * The returned token can observe cancellation
     * without gaining the authority to request it.
     *
     * The token remains valid even if the source
     * is subsequently destroyed.
     *
     * @par Example
     * @code{.cpp}
     * vix::CancellationSource source;
     *
     * vix::CancellationToken token = source.token();
     * @endcode
     *
     * @see CancellationToken
     */
    [[nodiscard]] CancellationToken token() const noexcept
    {
      return CancellationToken(state_);
    }

    /**
     * @brief Requests cancellation of the shared state.
     *
     * @return true if this call initiated cancellation,
     *         false if cancellation was already requested.
     *
     * @details
     * Cancellation requests are idempotent.
     *
     * The first successful request marks the shared
     * state as cancellation-requested and invokes
     * registered callbacks synchronously.
     *
     * Later requests do not invoke the callbacks again.
     *
     * @par Example
     * @code{.cpp}
     * vix::CancellationSource source;
     *
     * bool first = source.request_cancel();  // true
     * bool second = source.request_cancel(); // false
     * @endcode
     *
     * @note
     * A false return does not guarantee that callbacks
     * started by another thread have finished.
     *
     * @warning
     * This function may block while callbacks execute.
     *
     * Callback exceptions escaping their invocation
     * terminate the process.
     *
     * @see is_cancel_requested()
     * @see CancellationToken::on_cancel()
     */
    [[nodiscard]] bool request_cancel() const noexcept
    {
      return state_->request_cancel();
    }

    /**
     * @brief Checks whether cancellation has been requested.
     *
     * @return true if the shared state has received
     *         a cancellation request, false otherwise.
     *
     * @details
     * This function observes the shared cancellation
     * state without initiating cancellation.
     *
     * @note
     * A true result indicates a cancellation request,
     * not necessarily completed callback execution
     * or completed operation shutdown.
     *
     * @see request_cancel()
     */
    [[nodiscard]] bool is_cancel_requested() const noexcept
    {
      return state_->is_cancel_requested();
    }

  private:
    std::shared_ptr<detail::CancellationState> state_;
  };
} // namespace vix

#endif // VIX_CANCELLATION_HPP
