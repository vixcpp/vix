#include <vix/concurrency/ExecutionResource.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <deque>
#include <limits>
#include <memory>
#include <new>
#include <system_error>
#include <thread>
#include <vector>

#include <vix/detail/AllocationFailure.hpp>

namespace vix::concurrency::detail
{
  struct ResourceState
  {
    explicit ResourceState(ExecutionResource::Options value, std::size_t capacity)
        : max_concurrency(value.max_concurrency),
          notification_capacity(capacity)
    {
      active.reserve(capacity);
      shutdown_snapshot.reserve(capacity);
      deadline_snapshot.reserve(capacity);
      workers.reserve(max_concurrency);
    }

    std::size_t max_concurrency;
    std::size_t notification_capacity;
    mutable std::mutex mutex;
    std::condition_variable work_available;
    std::deque<std::shared_ptr<WorkBase>> queue;
    std::deque<std::shared_ptr<NotificationState>> notifications;
    std::vector<std::shared_ptr<WorkBase>> active;
    std::vector<std::shared_ptr<WorkBase>> shutdown_snapshot;
    std::vector<std::shared_ptr<WorkBase>> deadline_snapshot;
    std::vector<std::thread> workers;
    std::thread deadline_worker;
    bool closing{false};
    std::size_t reservations{0};
    std::atomic<std::size_t> outstanding_notifications{0};
    std::condition_variable admissions_finished;
    std::condition_variable deadline_changed;

    std::mutex shutdown_mutex;
    std::condition_variable shutdown_finished;
    bool shutdown_started{false};
    bool joined{false};
    std::thread::id shutdown_owner{};
  };

  namespace
  {
    thread_local const ResourceState *current_worker_resource = nullptr;
    thread_local const NotificationState *current_notification = nullptr;

    void dispatch_notification(const std::shared_ptr<NotificationState> &notification) noexcept
    {
      std::shared_ptr<NotificationCallback> callback;
      bool inactive = false;
      {
        std::lock_guard lock(notification->mutex);
        if (notification->phase == NotificationState::Phase::Inactive)
        {
          notification->queue_entry_retired = true;
          inactive = true;
        }
        else
        {
          if (notification->phase != NotificationState::Phase::Queued)
            std::terminate();
          notification->phase = NotificationState::Phase::Running;
          notification->queue_entry_retired = true;
          callback = notification->callback;
        }
      }
      if (inactive)
      {
        bool release = false;
        {
          std::lock_guard lock(notification->mutex);
          release = !notification->callback_cleanup_in_progress;
        }
        if (release)
          release_notification_obligation(notification);
        notification->changed.notify_all();
        return;
      }

      current_notification = notification.get();
      std::optional<Error> failure;
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
      try
      {
        auto outcome = callback->invoke();
        if (!outcome)
          failure.emplace(std::move(outcome).error());
      }
      catch (const std::exception &exception)
      {
        if (exception.what() == nullptr)
          failure.emplace(errors::notification_exception);
        else
          failure.emplace(vix::detail::allocation_or_terminate([&]
          {
            return Error(errors::notification_exception, std::string(exception.what()));
          }));
      }
      catch (...)
      {
        failure.emplace(errors::notification_exception);
      }
#else
      auto outcome = callback->invoke();
      if (!outcome)
        failure.emplace(std::move(outcome).error());
#endif

      std::shared_ptr<NotificationCallback> callback_to_destroy;
      {
        std::lock_guard lock(notification->mutex);
        if (notification->phase != NotificationState::Phase::Running)
          std::terminate();
        if (failure)
          notification->failure.emplace(std::move(*failure));
        callback_to_destroy = std::move(notification->callback);
      }
      callback.reset();
      callback_to_destroy.reset();
      {
        std::lock_guard lock(notification->mutex);
        if (notification->phase != NotificationState::Phase::Running)
          std::terminate();
        notification->phase = NotificationState::Phase::Done;
      }
      current_notification = nullptr;
      notification->changed.notify_all();
      release_notification_obligation(notification);
    }

    void worker_loop(ResourceState *resource) noexcept
    {
      current_worker_resource = resource;
      for (;;)
      {
        std::shared_ptr<WorkBase> work;
        std::shared_ptr<NotificationState> notification;
        bool should_start = false;
        {
          std::unique_lock lock(resource->mutex);
          resource->work_available.wait(lock, [resource]
          {
            return !resource->notifications.empty() || !resource->queue.empty() ||
                   (resource->closing && resource->reservations == 0 &&
                    resource->outstanding_notifications.load(std::memory_order_acquire) == 0);
          });
          if (resource->closing && resource->queue.empty() && resource->notifications.empty() &&
              resource->outstanding_notifications.load(std::memory_order_acquire) == 0)
            break;

          if (!resource->notifications.empty())
          {
            notification = std::move(resource->notifications.front());
            resource->notifications.pop_front();
          }
          else
          {
            work = std::move(resource->queue.front());
            resource->queue.pop_front();
            if (resource->closing)
            {
              should_start = false;
            }
            else
            {
              // This transition shares the admission/shutdown mutex boundary:
              // work is either Running before shutdown closes, or remains
              // Pending and shutdown cancels it without invocation.
              should_start = work->prepare_to_run();
            }
          }
        }

        if (notification)
        {
          dispatch_notification(notification);
          continue;
        }

        if (should_start)
          work->run();
        else
          work->shutdown_cancel();

        {
          std::lock_guard lock(resource->mutex);
          if (!resource->closing)
          {
            const auto found = std::find(resource->active.begin(),
                                         resource->active.end(), work);
            if (found != resource->active.end())
              resource->active.erase(found);
          }
        }
      }
      current_worker_resource = nullptr;
    }

    void deadline_loop(ResourceState *resource) noexcept
    {
      std::unique_lock lock(resource->mutex);
      for (;;)
      {
        if (resource->closing)
          break;

        std::optional<Deadline> next_deadline;
        const auto now = MonotonicClock::now();
        for (const auto &work : resource->active)
        {
          const auto deadline = work->scheduled_deadline();
          if (!deadline)
            continue;
          if (deadline->reached(now))
            resource->deadline_snapshot.push_back(work);
          else if (!next_deadline || *deadline < *next_deadline)
            next_deadline = deadline;
        }

        if (!resource->deadline_snapshot.empty())
        {
          lock.unlock();
          for (const auto &work : resource->deadline_snapshot)
            work->observe_deadline(now);
          // Releasing a WorkState can destroy its user-owned result. Do so
          // without holding the resource mutex.
          resource->deadline_snapshot.clear();
          lock.lock();
          continue;
        }

        if (!next_deadline)
        {
          resource->deadline_changed.wait(lock);
          continue;
        }

        auto remaining = next_deadline->remaining(now).to_chrono();
        // condition_variable::wait_for must not receive a duration whose
        // addition to steady_clock::now() can overflow its time_point.
        const auto maximum_wait = std::chrono::hours(24);
        if (remaining > maximum_wait)
          remaining = std::chrono::duration_cast<std::chrono::nanoseconds>(maximum_wait);
        resource->deadline_changed.wait_for(lock, remaining);
      }

      lock.unlock();
      resource->deadline_snapshot.clear();
    }

    [[nodiscard]] Result<void> shutdown_state(
        const std::shared_ptr<ResourceState> &resource) noexcept
    {
      if (is_current_notification_for_resource(resource.get()))
        return Error(errors::shutdown_from_notification_callback);
      if (is_current_resource_worker(resource.get()))
        return Error(errors::shutdown_from_worker);
      if (vix::detail::in_cancellation_callback())
        return Error(errors::shutdown_from_cancellation_callback);

      {
        std::unique_lock lock(resource->shutdown_mutex);
        if (resource->joined)
          return Result<void>::success();
        if (resource->shutdown_started)
        {
          if (resource->shutdown_owner == std::this_thread::get_id())
            return Error(errors::shutdown_reentrant);
          resource->shutdown_finished.wait(lock, [&]
          {
            return resource->joined;
          });
          return Result<void>::success();
        }
        resource->shutdown_started = true;
        resource->shutdown_owner = std::this_thread::get_id();
      }

      {
        std::lock_guard lock(resource->mutex);
        resource->closing = true;
      }
      resource->deadline_changed.notify_all();
      {
        std::unique_lock lock(resource->mutex);
        resource->admissions_finished.wait(lock, [&]
        {
          return resource->reservations == 0;
        });
        // Capacity was reserved during creation, so this snapshot performs
        // no allocation while shutdown is transitioning the resource.
        resource->shutdown_snapshot.assign(resource->active.begin(),
                                           resource->active.end());
      }

      // Cancellation callbacks are arbitrary caller code and are deliberately
      // never invoked under the resource lock.
      for (const auto &work : resource->shutdown_snapshot)
        work->shutdown_cancel();
      resource->work_available.notify_all();
      resource->deadline_changed.notify_all();

      if (resource->deadline_worker.joinable())
      {
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
        try
        {
          resource->deadline_worker.join();
        }
        catch (...)
        {
          std::terminate();
        }
#else
        resource->deadline_worker.join();
#endif
      }

      for (auto &worker : resource->workers)
      {
        if (worker.joinable())
        {
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
          try
          {
            worker.join();
          }
          catch (...)
          {
            // Once admission is closed there is no safe recovery from a
            // failed join: destroying a joinable std::thread is a Fault.
            std::terminate();
          }
#else
          worker.join();
#endif
        }
      }

      std::vector<std::shared_ptr<WorkBase>> completed_records;
      {
        std::lock_guard lock(resource->mutex);
        resource->queue.clear();
        resource->active.clear();
        // Keep the last references to user-owned completion values until
        // after the resource lock is released. Their destructors are caller
        // code and must not run while internal synchronization is held.
        completed_records.swap(resource->shutdown_snapshot);
      }
      completed_records.clear();
      {
        std::lock_guard lock(resource->shutdown_mutex);
        resource->joined = true;
      }
      resource->shutdown_finished.notify_all();
      return Result<void>::success();
    }
  } // namespace

  bool is_current_resource_worker(const ResourceState *resource) noexcept
  {
    return resource != nullptr && current_worker_resource == resource;
  }

  void notify_deadline_monitor(const std::weak_ptr<ResourceState> &owner) noexcept
  {
    if (const auto resource = owner.lock())
      resource->deadline_changed.notify_one();
  }

  bool is_current_notification(const NotificationState *notification) noexcept
  {
    return notification != nullptr && current_notification == notification;
  }

  bool is_current_notification_for_resource(const ResourceState *resource) noexcept
  {
    if (resource == nullptr || current_notification == nullptr)
      return false;
    std::shared_ptr<ResourceState> owner;
    {
      std::lock_guard lock(current_notification->mutex);
      owner = current_notification->owner.lock();
    }
    return owner.get() == resource;
  }

  bool register_notification_obligation(
      const std::shared_ptr<ResourceState> &resource,
      const std::shared_ptr<NotificationState> &notification) noexcept
  {
    {
      std::lock_guard lock(notification->mutex);
      if (notification->phase != NotificationState::Phase::Registered ||
          notification->obligation_released)
        std::terminate();
      auto current = resource->outstanding_notifications.load(std::memory_order_relaxed);
      do
      {
        if (current >= resource->notification_capacity)
          return false;
      }
      while (!resource->outstanding_notifications.compare_exchange_weak(
          current, current + 1, std::memory_order_acq_rel, std::memory_order_relaxed));
      notification->owner = resource;
    }
    return true;
  }

  bool enqueue_notification(
      const std::shared_ptr<ResourceState> &resource,
      const std::shared_ptr<NotificationState> &notification) noexcept
  {
    {
      std::lock_guard lock(notification->mutex);
      if (notification->phase == NotificationState::Phase::Inactive)
        return true;
      if (notification->phase != NotificationState::Phase::Registered ||
          notification->obligation_released)
        std::terminate();
      notification->phase = NotificationState::Phase::Queued;
    }
    {
      std::lock_guard lock(resource->mutex);
      vix::detail::allocation_or_terminate([&]
      {
        resource->notifications.emplace_back(notification);
      });
    }
    resource->work_available.notify_all();
    return true;
  }

  void release_notification_obligation(
      const std::shared_ptr<NotificationState> &notification) noexcept
  {
    std::shared_ptr<ResourceState> owner;
    {
      std::lock_guard lock(notification->mutex);
      if (notification->obligation_released)
        return;
      notification->obligation_released = true;
      owner = notification->owner.lock();
    }
    if (!owner)
      std::terminate();
    const auto previous = owner->outstanding_notifications.fetch_sub(
        1, std::memory_order_acq_rel);
    if (previous == 0)
      std::terminate();
    owner->work_available.notify_all();
  }

  void deactivate_notification(
      const std::shared_ptr<NotificationState> &notification) noexcept
  {
    std::shared_ptr<NotificationCallback> callback;
    bool release = false;
    {
      std::lock_guard lock(notification->mutex);
      if (notification->phase == NotificationState::Phase::Registered ||
          notification->phase == NotificationState::Phase::Queued)
      {
        const bool was_queued = notification->phase == NotificationState::Phase::Queued;
        notification->phase = NotificationState::Phase::Inactive;
        notification->callback_cleanup_in_progress = true;
        callback = std::move(notification->callback);
        release = !was_queued;
      }
    }
    if (!callback)
      return;
    const auto *previous_notification = current_notification;
    current_notification = notification.get();
    callback.reset();
    current_notification = previous_notification;
    {
      std::lock_guard lock(notification->mutex);
      notification->callback_cleanup_in_progress = false;
      release = release || notification->queue_entry_retired;
    }
    if (release)
      release_notification_obligation(notification);
    notification->changed.notify_all();
  }

  bool notification_wait_would_deadlock(
      const std::shared_ptr<NotificationState> &notification) noexcept
  {
    std::shared_ptr<ResourceState> owner;
    {
      std::lock_guard lock(notification->mutex);
      if (current_notification == notification.get())
        return true;
      if ((notification->phase == NotificationState::Phase::Inactive &&
           !notification->callback_cleanup_in_progress) ||
          notification->phase == NotificationState::Phase::Done)
        return false;
      owner = notification->owner.lock();
    }
    return vix::detail::in_cancellation_callback() ||
           (owner && is_current_resource_worker(owner.get()));
  }

  Result<void> wait_notification(
      const std::shared_ptr<NotificationState> &notification)
  {
    std::unique_lock lock(notification->mutex);
    notification->changed.wait(lock, [&]
    {
      return (notification->phase == NotificationState::Phase::Inactive &&
              !notification->callback_cleanup_in_progress) ||
             notification->phase == NotificationState::Phase::Done;
    });
    if (notification->phase == NotificationState::Phase::Done && notification->failure)
      return *notification->failure;
    return Result<void>::success();
  }

  Result<void> reset_notification(
      const std::shared_ptr<NotificationState> &notification)
  {
    deactivate_notification(notification);
    if (is_current_notification(notification.get()))
      return Error(errors::would_deadlock);
    if (notification_wait_would_deadlock(notification))
      return Error(errors::would_deadlock);
    return wait_notification(notification);
  }
} // namespace vix::concurrency::detail

namespace vix::concurrency
{
  Result<ExecutionResource> ExecutionResource::create(Options options)
  {
    if (options.max_concurrency == 0 ||
        options.max_pending > std::numeric_limits<std::size_t>::max() - options.max_concurrency)
      return Error(errors::invalid_configuration);

    const auto capacity = options.max_concurrency + options.max_pending;
    const std::vector<std::shared_ptr<detail::WorkBase>> size_probe;
    const std::deque<std::shared_ptr<detail::WorkBase>> queue_size_probe;
    const std::vector<std::thread> worker_size_probe;
    if (capacity > size_probe.max_size() || capacity > queue_size_probe.max_size() ||
        options.max_concurrency > worker_size_probe.max_size())
      return Error(errors::invalid_configuration);

    std::shared_ptr<detail::ResourceState> state;
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
    try
    {
      state = vix::detail::allocation_or_terminate([&]
      {
        return std::make_shared<detail::ResourceState>(options, capacity);
      });
    }
    catch (const std::bad_alloc &)
    {
      std::terminate();
    }
    catch (const std::system_error &exception)
    {
      return vix::detail::allocation_or_terminate([&]
      {
        return Error(errors::resource_creation_failed, std::string(exception.what()));
      });
    }
    catch (...)
    {
      return Error(errors::resource_creation_failed);
    }
#else
    state = vix::detail::allocation_or_terminate([&]
    {
      return std::make_shared<detail::ResourceState>(options, capacity);
    });
#endif

#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
    try
    {
      for (std::size_t index = 0; index < options.max_concurrency; ++index)
        state->workers.emplace_back(detail::worker_loop, state.get());
      state->deadline_worker = std::thread(detail::deadline_loop, state.get());
    }
    catch (const std::system_error &exception)
    {
      {
        std::lock_guard lock(state->mutex);
        state->closing = true;
      }
      state->work_available.notify_all();
      state->deadline_changed.notify_all();
      for (auto &worker : state->workers)
        if (worker.joinable())
          worker.join();
      if (state->deadline_worker.joinable())
        state->deadline_worker.join();
      return vix::detail::allocation_or_terminate([&]
      {
        return Error(errors::resource_creation_failed, std::string(exception.what()));
      });
    }
    catch (const std::bad_alloc &)
    {
      std::terminate();
    }
    catch (...)
    {
      {
        std::lock_guard lock(state->mutex);
        state->closing = true;
      }
      state->work_available.notify_all();
      state->deadline_changed.notify_all();
      for (auto &worker : state->workers)
        if (worker.joinable())
          worker.join();
      if (state->deadline_worker.joinable())
        state->deadline_worker.join();
      return Error(errors::resource_creation_failed);
    }
#else
    for (std::size_t index = 0; index < options.max_concurrency; ++index)
      state->workers.emplace_back(detail::worker_loop, state.get());
    state->deadline_worker = std::thread(detail::deadline_loop, state.get());
#endif

    return ExecutionResource(std::move(state));
  }

  ExecutionResource::ExecutionResource(ExecutionResource &&other) noexcept
      : state_(std::move(other.state_))
  {
  }

  ExecutionResource::~ExecutionResource()
  {
    if (!state_)
      return;
    if (detail::is_current_resource_worker(state_.get()) ||
        vix::detail::in_cancellation_callback())
      std::terminate();
    auto result = shutdown();
    if (!result)
      std::terminate();
  }

  Result<void> ExecutionResource::reserve_work(std::optional<Deadline> deadline)
  {
    if (!state_)
      return Error(errors::invalid_work_handle);
    std::lock_guard lock(state_->mutex);
    if (deadline && deadline->reached())
      return Error(errors::deadline_expired);
    if (state_->closing)
      return Error(errors::resource_shutting_down);
    if (state_->active.size() + state_->reservations >= state_->active.capacity())
      return Error(errors::capacity_exhausted);
    ++state_->reservations;
    return Result<void>::success();
  }

  void ExecutionResource::commit_work(
      const std::shared_ptr<detail::WorkBase> &work) noexcept
  {
    {
      std::lock_guard lock(state_->mutex);
      work->set_owner(state_);
      vix::detail::allocation_or_terminate([&]
      {
        state_->queue.emplace_back(work);
      });
      state_->active.emplace_back(work);
      --state_->reservations;
      state_->admissions_finished.notify_all();
      state_->work_available.notify_one();
    }
    state_->deadline_changed.notify_one();
  }

  Result<void> ExecutionResource::shutdown() noexcept
  {
    if (!state_)
      return Result<void>::success();
    return detail::shutdown_state(state_);
  }

  bool ExecutionResource::is_shutdown() const noexcept
  {
    if (!state_)
      return true;
    std::lock_guard lock(state_->mutex);
    return state_->closing;
  }

  NotificationRegistration::NotificationRegistration(
      NotificationRegistration &&other) noexcept
      : state_(other.state_.exchange(std::shared_ptr<detail::NotificationState>{},
                                     std::memory_order_acq_rel))
  {
  }

  NotificationRegistration &NotificationRegistration::operator=(
      NotificationRegistration &&other) noexcept
  {
    if (this != &other)
    {
      auto previous = state_.exchange(std::shared_ptr<detail::NotificationState>{},
                                      std::memory_order_acq_rel);
      if (previous)
      {
        detail::deactivate_notification(previous);
        if (!detail::is_current_notification(previous.get()) &&
            detail::notification_wait_would_deadlock(previous))
          std::terminate();
        if (!detail::is_current_notification(previous.get()))
          (void)detail::wait_notification(previous);
      }
      state_.store(other.state_.exchange(std::shared_ptr<detail::NotificationState>{},
                                         std::memory_order_acq_rel),
                   std::memory_order_release);
    }
    return *this;
  }

  NotificationRegistration::~NotificationRegistration()
  {
    auto state = state_.load(std::memory_order_acquire);
    if (!state)
      return;
    detail::deactivate_notification(state);
    if (detail::is_current_notification(state.get()))
      return;
    if (detail::notification_wait_would_deadlock(state))
      std::terminate();
    (void)detail::wait_notification(state);
  }

  void NotificationRegistration::deactivate() noexcept
  {
    auto state = state_.load(std::memory_order_acquire);
    if (state)
      detail::deactivate_notification(state);
  }

  Result<void> NotificationRegistration::reset()
  {
    auto state = state_.load(std::memory_order_acquire);
    if (!state)
      return Error(errors::invalid_notification_registration);
    auto result = detail::reset_notification(state);
    if (!result && detail::notification_wait_would_deadlock(state))
      return result;
    auto expected = std::move(state);
    (void)state_.compare_exchange_strong(expected,
                                         std::shared_ptr<detail::NotificationState>{},
                                         std::memory_order_acq_rel,
                                         std::memory_order_acquire);
    return result;
  }

  Result<void> NotificationRegistration::wait() const
  {
    auto state = state_.load(std::memory_order_acquire);
    if (!state)
      return Error(errors::invalid_notification_registration);
    if (detail::notification_wait_would_deadlock(state))
      return Error(errors::would_deadlock);
    return detail::wait_notification(state);
  }

  Result<NotificationRegistration> ReadySubscription::take_registration()
  {
    if (already_ready_ || !registration_)
      return Error(errors::invalid_notification_registration);
    NotificationRegistration registration(std::move(*registration_));
    registration_.reset();
    return registration;
  }
} // namespace vix::concurrency
