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
   * @brief Move-only RAII lifetime for a cancellation callback.
   *
   * reset() and destruction prevent a callback from beginning afterwards and
   * normally wait for an already-running callback. When invoked from any
   * cancellation callback, they are nonblocking to prevent callback wait
   * cycles; an already-running callback retains its own capture until it
   * returns. Do not hold a lock required by a callback while resetting from
   * non-callback code.
   */
  class CancellationRegistration
  {
  public:
    CancellationRegistration() noexcept = default;
    CancellationRegistration(const CancellationRegistration &) = delete;
    CancellationRegistration &operator=(const CancellationRegistration &) = delete;

    CancellationRegistration(CancellationRegistration &&other) noexcept
        : state_(std::move(other.state_)),
          callback_(std::move(other.callback_))
    {
    }

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

    ~CancellationRegistration()
    {
      reset();
    }

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
   * @brief Copyable borrowed observation of one cancellation-request state.
   *
   * A token grants neither cancellation authority nor resource lifecycle
   * authority. A default token is permanently non-cancelled. Tokens keep a
   * non-default shared state alive and remain valid after all Sources vanish.
   */
  class CancellationToken
  {
  public:
    CancellationToken() noexcept = default;
    CancellationToken(const CancellationToken &) = default;
    CancellationToken(CancellationToken &&) noexcept = default;
    CancellationToken &operator=(const CancellationToken &) = default;
    CancellationToken &operator=(CancellationToken &&) noexcept = default;
    ~CancellationToken() = default;

    [[nodiscard]] bool is_cancel_requested() const noexcept
    {
      return state_ && state_->is_cancel_requested();
    }

    /**
     * @brief Register a synchronous callback for a future or prior request.
     *
     * If cancellation has already been requested, callback executes before
     * this call returns and the returned registration is empty. Otherwise its
     * registration controls callback lifetime. A callback runs at most once;
     * callback order is unspecified. Registration may synchronously run its
     * callback and therefore may block. Escaping callback exceptions are
     * Faults. Providers must not hold a lock needed by arbitrary cancellation
     * work while requesting cancellation or registering a callback.
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
   * @brief Copyable authority to request cancellation of one shared state.
   *
   * Copying a Source deliberately shares request authority. Destruction never
   * requests cancellation; an Operation must not be cancelled merely because
   * one owner object went out of scope. request_cancel() is idempotent and
   * returns true only for the request that won the race. It is noexcept but
   * may synchronously execute arbitrary registered callback work and block.
   */
  class CancellationSource
  {
  public:
    CancellationSource()
        : state_(detail::allocation_or_terminate(
              [] { return std::make_shared<detail::CancellationState>(); }))
    {
    }

    CancellationSource(const CancellationSource &) = default;
    CancellationSource(CancellationSource &&) noexcept = default;
    CancellationSource &operator=(const CancellationSource &) = default;
    CancellationSource &operator=(CancellationSource &&) noexcept = default;
    ~CancellationSource() = default;

    [[nodiscard]] CancellationToken token() const noexcept
    {
      return CancellationToken(state_);
    }

    [[nodiscard]] bool request_cancel() const noexcept
    {
      return state_->request_cancel();
    }

    [[nodiscard]] bool is_cancel_requested() const noexcept
    {
      return state_->is_cancel_requested();
    }

  private:
    std::shared_ptr<detail::CancellationState> state_;
  };
} // namespace vix

#endif // VIX_CANCELLATION_HPP
