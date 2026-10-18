#ifndef VIX_CONCURRENCY_EXECUTION_RESOURCE_HPP
#define VIX_CONCURRENCY_EXECUTION_RESOURCE_HPP

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

#include <vix/Cancellation.hpp>
#include <vix/Completion.hpp>
#include <vix/Deadline.hpp>
#include <vix/Error.hpp>
#include <vix/Result.hpp>
#include <vix/detail/AllocationFailure.hpp>

namespace vix::concurrency
{
  class WorkContext;

  namespace errors
  {
    inline constexpr auto invalid_configuration = ErrorIdentity::define<"vix.concurrency">(1);
    inline constexpr auto execution_unavailable = ErrorIdentity::define<"vix.concurrency">(2);
    inline constexpr auto resource_creation_failed = ErrorIdentity::define<"vix.concurrency">(3);
    inline constexpr auto capacity_exhausted = ErrorIdentity::define<"vix.concurrency">(4);
    inline constexpr auto resource_shutting_down = ErrorIdentity::define<"vix.concurrency">(5);
    inline constexpr auto invalid_work_handle = ErrorIdentity::define<"vix.concurrency">(6);
    inline constexpr auto completion_already_taken = ErrorIdentity::define<"vix.concurrency">(7);
    inline constexpr auto would_deadlock = ErrorIdentity::define<"vix.concurrency">(8);
    inline constexpr auto shutdown_from_worker = ErrorIdentity::define<"vix.concurrency">(9);
    inline constexpr auto shutdown_from_cancellation_callback = ErrorIdentity::define<"vix.concurrency">(10);
    inline constexpr auto worker_exception = ErrorIdentity::define<"vix.concurrency">(11);
    inline constexpr auto shutdown_reentrant = ErrorIdentity::define<"vix.concurrency">(12);
    inline constexpr auto notification_exception = ErrorIdentity::define<"vix.concurrency">(13);
    inline constexpr auto notification_already_registered = ErrorIdentity::define<"vix.concurrency">(14);
    inline constexpr auto invalid_notification_registration = ErrorIdentity::define<"vix.concurrency">(15);
    inline constexpr auto notification_registration_failed = ErrorIdentity::define<"vix.concurrency">(16);
    inline constexpr auto notification_capacity_exhausted = ErrorIdentity::define<"vix.concurrency">(17);
    inline constexpr auto shutdown_from_notification_callback = ErrorIdentity::define<"vix.concurrency">(18);
    inline constexpr auto deadline_expired = ErrorIdentity::define<"vix.concurrency">(19);
  } // namespace errors

  namespace detail
  {
    struct ResourceState;
    struct NotificationCallback
    {
      virtual ~NotificationCallback() = default;
      virtual Result<void> invoke() = 0;
    };
    struct NotificationState
    {
      enum class Phase : unsigned char { Registered, Queued, Running, Inactive, Done };

      explicit NotificationState(std::shared_ptr<NotificationCallback> value)
          : callback(std::move(value)) {}

      mutable std::mutex mutex;
      std::condition_variable changed;
      std::weak_ptr<ResourceState> owner;
      std::shared_ptr<NotificationCallback> callback;
      std::optional<Error> failure;
      Phase phase{Phase::Registered};
      bool obligation_released{false};
      bool callback_cleanup_in_progress{false};
      bool queue_entry_retired{false};
    };

    [[nodiscard]] bool is_current_resource_worker(const ResourceState *) noexcept;
    [[nodiscard]] bool is_current_notification(const NotificationState *) noexcept;
    [[nodiscard]] bool is_current_notification_for_resource(const ResourceState *) noexcept;
    [[nodiscard]] bool enqueue_notification(
        const std::shared_ptr<ResourceState> &,
        const std::shared_ptr<NotificationState> &) noexcept;
    [[nodiscard]] bool register_notification_obligation(
        const std::shared_ptr<ResourceState> &,
        const std::shared_ptr<NotificationState> &) noexcept;
    void release_notification_obligation(
        const std::shared_ptr<NotificationState> &) noexcept;
    void notify_deadline_monitor(const std::weak_ptr<ResourceState> &) noexcept;
    [[nodiscard]] Result<void> wait_notification(
        const std::shared_ptr<NotificationState> &);
    [[nodiscard]] bool notification_wait_would_deadlock(
        const std::shared_ptr<NotificationState> &) noexcept;
    void deactivate_notification(const std::shared_ptr<NotificationState> &) noexcept;
    [[nodiscard]] Result<void> reset_notification(
        const std::shared_ptr<NotificationState> &);

    struct WorkBase
    {
      virtual ~WorkBase() = default;
      virtual bool prepare_to_run() noexcept = 0;
      virtual void run() noexcept = 0;
      virtual void shutdown_cancel() noexcept = 0;
      virtual void set_owner(const std::shared_ptr<ResourceState> &) noexcept = 0;
      [[nodiscard]] virtual std::optional<Deadline> scheduled_deadline() const noexcept = 0;
      virtual void observe_deadline(MonotonicInstant) noexcept = 0;
    };

    template <typename F, bool = std::is_invocable_v<F &, WorkContext &>>
    struct invocation_result;
    template <typename F>
    struct invocation_result<F, true>
    {
      using type = std::invoke_result_t<F &, WorkContext &>;
    };
    template <typename F>
    struct invocation_result<F, false>
    {
      using type = std::invoke_result_t<F &>;
    };

    template <typename T>
    struct is_result : std::false_type
    {
    };
    template <typename T>
    struct is_result<Result<T>> : std::true_type
    {
      using value_type = T;
    };
    template <typename T>
    struct is_completion : std::false_type
    {
    };
    template <typename T>
    struct is_completion<Completion<T>> : std::true_type
    {
      using value_type = T;
    };

    template <typename R, bool = std::is_void_v<R>>
    struct outcome_traits
    {
      using value_type = std::remove_cv_t<R>;
      static constexpr int kind = 0; // plain value
    };
    template <typename R>
    struct outcome_traits<R, true>
    {
      using value_type = void;
      static constexpr int kind = 1;
    };
    template <typename T>
    struct outcome_traits<Result<T>, false>
    {
      using value_type = T;
      static constexpr int kind = 2;
    };
    template <typename T>
    struct outcome_traits<Completion<T>, false>
    {
      using value_type = T;
      static constexpr int kind = 3;
    };

    template <typename T>
    struct WorkStateBase : WorkBase
    {
      virtual bool ready() const noexcept = 0;
      virtual bool request_cancel() noexcept = 0;
      virtual Result<void> wait() const = 0;
      virtual Result<std::optional<Completion<T>>> try_take() = 0;
      virtual Result<bool> subscribe(
          const std::shared_ptr<NotificationState> &) = 0;
      std::weak_ptr<ResourceState> owner_;
    };
    template <typename T, typename F>
    struct WorkState;
  } // namespace detail

  /** @brief Read-only cooperative cancellation context for one callable. */
  class WorkContext
  {
  public:
    [[nodiscard]] CancellationToken cancellation_token() const noexcept
    {
      return token_;
    }
    [[nodiscard]] bool stop_requested() const noexcept
    {
      return token_.is_cancel_requested();
    }

  private:
    template <typename, typename>
    friend struct detail::WorkState;
    explicit WorkContext(CancellationToken token) noexcept : token_(std::move(token)) {}
    CancellationToken token_;
  };

  /** @brief Per-work execution constraints. Deadlines use Standard's monotonic clock. */
  struct WorkOptions
  {
    std::optional<Deadline> deadline{};
  };

  /** @brief Owns one accepted one-shot completion notification.
   *
   * Destruction deactivates and waits for callback quiescence, except when
   * performed by that callback itself (where it is nonblocking). Call
   * deactivate() for nonblocking deactivation; an already-running callback
   * continues. wait() and reset() report the callback's Result<void> failure
   * or a translated notification_exception.
   * State operations may be called concurrently on one live registration;
   * moving or destroying that C++ object must not race with a member call.
   */
  class NotificationRegistration
  {
  public:
    NotificationRegistration() noexcept = default;
    NotificationRegistration(const NotificationRegistration &) = delete;
    NotificationRegistration &operator=(const NotificationRegistration &) = delete;
    NotificationRegistration(NotificationRegistration &&) noexcept;
    NotificationRegistration &operator=(NotificationRegistration &&) noexcept;
    ~NotificationRegistration();

    [[nodiscard]] bool valid() const noexcept
    {
      return static_cast<bool>(state_.load(std::memory_order_acquire));
    }
    // Nonblocking. If dispatch has already begun, that invocation continues;
    // callback captures must remain valid until wait/reset or quiescent
    // destruction completes.
    void deactivate() noexcept;
    // Deactivates and waits for quiescence where doing so cannot self-deadlock.
    // Called by its own callback, it returns would_deadlock; use deactivate()
    // for nonblocking self-deactivation and retain this handle to observe the
    // terminal callback outcome later.
    [[nodiscard]] Result<void> reset();
    // Waits for dispatch completion; callback failures are returned here.
    [[nodiscard]] Result<void> wait() const;

  private:
    friend class ReadySubscription;
    template <typename> friend class WorkHandle;
    explicit NotificationRegistration(std::shared_ptr<detail::NotificationState> state) noexcept
        : state_(std::move(state)) {}
    std::atomic<std::shared_ptr<detail::NotificationState>> state_;
  };

  /** @brief Result of atomically observing readiness or accepting a callback.
   *
   * If a registration is not taken, its RAII cleanup deactivates it.
   */
  class ReadySubscription
  {
  public:
    ReadySubscription() noexcept = default;
    ReadySubscription(const ReadySubscription &) = delete;
    ReadySubscription &operator=(const ReadySubscription &) = delete;
    ReadySubscription(ReadySubscription &&) noexcept = default;
    ReadySubscription &operator=(ReadySubscription &&) noexcept = default;

    [[nodiscard]] bool already_ready() const noexcept { return already_ready_; }
    [[nodiscard]] Result<NotificationRegistration> take_registration();

  private:
    template <typename> friend class WorkHandle;
    explicit ReadySubscription(bool ready) noexcept : already_ready_(ready) {}
    explicit ReadySubscription(NotificationRegistration registration) noexcept
        : registration_(std::move(registration)) {}
    bool already_ready_{false};
    std::optional<NotificationRegistration> registration_;
  };

  /** @brief Shared control and one-shot result observation for accepted work. */
  template <typename T>
  class WorkHandle
  {
  public:
    WorkHandle() noexcept = default;
    WorkHandle(const WorkHandle &) noexcept = default;
    WorkHandle(WorkHandle &&) noexcept = default;
    WorkHandle &operator=(const WorkHandle &) noexcept = default;
    WorkHandle &operator=(WorkHandle &&) noexcept = default;
    ~WorkHandle() = default;

    [[nodiscard]] bool valid() const noexcept { return static_cast<bool>(state_); }
    [[nodiscard]] bool ready() const noexcept;
    [[nodiscard]] bool request_cancel() const noexcept;
    [[nodiscard]] Result<void> wait() const;
    [[nodiscard]] Result<std::optional<Completion<T>>> try_take_completion() const;
    [[nodiscard]] Result<Completion<T>> take_completion() const;

    /**
     * Atomically observe terminal readiness or accept the work item's single
     * notification callback. Accepted callbacks run once on a worker owned by
     * the originating ExecutionResource, never inline on this calling thread.
     * The callback may return void or Result<void>; escaping exceptions are
     * reported by the resulting NotificationRegistration, not by Completion.
     */
    template <typename F>
    [[nodiscard]] Result<ReadySubscription> on_ready(F &&callback) const
    {
      using Callback = std::decay_t<F>;
      static_assert(std::is_invocable_v<Callback &>,
                    "notification callback must be invocable without arguments");
      using Return = std::invoke_result_t<Callback &>;
      static_assert(std::is_void_v<Return> || std::is_same_v<Return, Result<void>>,
                    "notification callback must return void or vix::Result<void>");
      static_assert(std::is_nothrow_destructible_v<Callback>,
                    "notification callback must be nothrow destructible");

      if (!state_)
        return Error(errors::invalid_work_handle);

      struct CallbackModel final : detail::NotificationCallback
      {
        explicit CallbackModel(F &&value) : callback(std::forward<F>(value)) {}
        Result<void> invoke() override
        {
          if constexpr (std::is_void_v<Return>)
          {
            std::invoke(callback);
            return Result<void>::success();
          }
          else
          {
            return std::invoke(callback);
          }
        }
        Callback callback;
      };

      std::shared_ptr<detail::NotificationCallback> callback_state;
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
      try
      {
        callback_state = vix::detail::allocation_or_terminate([&]
        {
          return std::make_shared<CallbackModel>(std::forward<F>(callback));
        });
      }
      catch (...)
      {
        return Error(errors::notification_registration_failed);
      }
#else
      callback_state = vix::detail::allocation_or_terminate([&]
      {
        return std::make_shared<CallbackModel>(std::forward<F>(callback));
      });
#endif
      auto notification = vix::detail::allocation_or_terminate([&]
      {
        return std::make_shared<detail::NotificationState>(std::move(callback_state));
      });
      auto subscribed = state_->subscribe(notification);
      if (!subscribed)
        return std::move(subscribed).error();
      if (subscribed.value())
        return ReadySubscription(true);
      return ReadySubscription(NotificationRegistration(std::move(notification)));
    }

  private:
    friend class ExecutionResource;
    explicit WorkHandle(std::shared_ptr<detail::WorkStateBase<T>> state) noexcept
        : state_(std::move(state)) {}
    std::shared_ptr<detail::WorkStateBase<T>> state_;
  };

  /** @brief Explicit owner of bounded concurrent work and native workers. */
  class ExecutionResource
  {
  public:
    struct Options
    {
      std::size_t max_concurrency{1};
      std::size_t max_pending{0};
    };

    [[nodiscard]] static Result<ExecutionResource> create(Options options);
    [[nodiscard]] static Result<ExecutionResource> create()
    {
      return create(Options{});
    }

    ExecutionResource(const ExecutionResource &) = delete;
    ExecutionResource &operator=(const ExecutionResource &) = delete;
    ExecutionResource(ExecutionResource &&) noexcept;
    ExecutionResource &operator=(ExecutionResource &&) = delete;
    ~ExecutionResource();

    template <typename F>
    [[nodiscard]] auto submit(F &&callable)
        -> Result<WorkHandle<typename detail::outcome_traits<typename detail::invocation_result<
            std::decay_t<F>>::type>::value_type>>
    {
      return submit(WorkOptions{}, std::forward<F>(callable));
    }

    template <typename F>
    [[nodiscard]] auto submit(WorkOptions options, F &&callable)
        -> Result<WorkHandle<typename detail::outcome_traits<typename detail::invocation_result<
            std::decay_t<F>>::type>::value_type>>
    {
      using Callable = std::decay_t<F>;
      constexpr bool takes_context = std::is_invocable_v<Callable &, WorkContext &>;
      static_assert(takes_context || std::is_invocable_v<Callable &>,
                    "work must be invocable with WorkContext& or with no arguments");
      using Return = typename detail::invocation_result<Callable>::type;
      using Traits = detail::outcome_traits<Return>;
      using Value = typename Traits::value_type;
      static_assert(!std::is_reference_v<Return>, "work may not return a reference");
      static_assert(!std::is_same_v<Value, Error>, "work may not use Error as a successful payload");
      static_assert(!detail::is_result<Value>::value && !detail::is_completion<Value>::value,
                    "nested Result/Completion work outcomes are ambiguous and unsupported");
      static_assert(std::is_void_v<Value> || std::is_nothrow_move_constructible_v<Value>,
                    "work result must be nothrow move constructible for one-shot observation");
      static_assert(std::is_nothrow_move_constructible_v<Callable>,
                    "work callable must be nothrow move constructible");
      static_assert(std::is_nothrow_destructible_v<Callable>,
                    "work callable must be nothrow destructible");

      if (!state_)
        return Error(errors::invalid_work_handle);

      std::shared_ptr<detail::WorkState<Value, Callable>> accepted;
      if (options.deadline && options.deadline->reached())
        return Error(errors::deadline_expired);
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
      try
      {
        accepted = vix::detail::allocation_or_terminate([&]
        {
          return std::make_shared<detail::WorkState<Value, Callable>>(options.deadline);
        });
      }
      catch (...)
      {
        return Error(errors::resource_creation_failed);
      }
#else
      accepted = vix::detail::allocation_or_terminate([&]
      {
          return std::make_shared<detail::WorkState<Value, Callable>>(options.deadline);
      });
#endif

      std::optional<Callable> staged_lvalue;
      (void)staged_lvalue;
      if constexpr (std::is_lvalue_reference_v<F>)
      {
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
        try
        {
          staged_lvalue.emplace(callable);
        }
        catch (...)
        {
          return Error(errors::resource_creation_failed);
        }
#else
        staged_lvalue.emplace(callable);
#endif
      }

      // Admission is reserved under the resource mutex. Every callable
      // transfer that follows is nothrow, so a successful reservation cannot
      // later be reported as rejected after shutdown observes it.
      auto reservation = reserve_work(options.deadline);
      if (!reservation)
        return std::move(reservation).error();
      if constexpr (std::is_lvalue_reference_v<F>)
        accepted->set_callable(std::move(*staged_lvalue));
      else
        accepted->set_callable(std::forward<F>(callable));
      commit_work(accepted);
      return WorkHandle<Value>(std::move(accepted));
    }

    [[nodiscard]] Result<void> shutdown() noexcept;
    // Reports that admission is closed; it does not wait for worker
    // quiescence. Use shutdown() when completion of accepted work is required.
    [[nodiscard]] bool is_shutdown() const noexcept;

  private:
    explicit ExecutionResource(std::shared_ptr<detail::ResourceState> state) noexcept
        : state_(std::move(state)) {}
    [[nodiscard]] Result<void> reserve_work(std::optional<Deadline> deadline);
    void commit_work(const std::shared_ptr<detail::WorkBase> &) noexcept;
    std::shared_ptr<detail::ResourceState> state_;
  };

  namespace detail
  {
    enum class WorkPhase : unsigned char { Pending, Running, Completing, Terminal };

    template <typename T, typename F>
  struct WorkState final : WorkStateBase<T>
    {
      explicit WorkState(std::optional<Deadline> deadline)
          : deadline_(deadline), cancellation_(), context_(cancellation_.token()) {}

      template <typename G>
      void set_callable(G &&callable) noexcept
      {
        callable_.emplace(std::forward<G>(callable));
      }

      void set_owner(const std::shared_ptr<ResourceState> &owner) noexcept override
      {
        this->owner_ = owner;
      }

      bool prepare_to_run() noexcept override
      {
        bool timeout_pending = false;
        {
          std::lock_guard lock(mutex_);
          if (phase_ != WorkPhase::Pending)
            return false;
          if (cancel_requested_)
          {
            phase_ = WorkPhase::Completing;
            pending_cancel_completion_ = true;
            return false;
          }
          if (deadline_ && deadline_->reached())
          {
            deadline_observed_ = true;
            phase_ = WorkPhase::Completing;
            pending_timeout_completion_ = true;
            timeout_pending = true;
          }
          else
          {
            phase_ = WorkPhase::Running;
          }
        }
        if (timeout_pending)
          finish_pending_timeout();
        return !timeout_pending;
      }

      [[nodiscard]] std::optional<Deadline> scheduled_deadline() const noexcept override
      {
        std::lock_guard lock(mutex_);
        if (!deadline_ || deadline_observed_ || cancel_requested_ ||
            (phase_ != WorkPhase::Pending && phase_ != WorkPhase::Running))
          return std::nullopt;
        return deadline_;
      }

      void observe_deadline(MonotonicInstant now) noexcept override
      {
        bool publish_timeout = false;
        bool deliver_cancellation = false;
        {
          std::lock_guard lock(mutex_);
          if (!deadline_ || deadline_observed_ || !deadline_->reached(now) ||
              (phase_ != WorkPhase::Pending && phase_ != WorkPhase::Running))
            return;
          deadline_observed_ = true;
          if (phase_ == WorkPhase::Pending)
          {
            phase_ = WorkPhase::Completing;
            pending_timeout_completion_ = true;
            publish_timeout = true;
          }
          else if (!cancel_requested_)
          {
            cancel_requested_ = true;
            cancellation_delivery_ = true;
            deliver_cancellation = true;
          }
        }
        notify_deadline_monitor(this->owner_);
        if (publish_timeout)
          finish_pending_timeout();
        if (deliver_cancellation)
          deliver_cancellation_request();
      }

      void run() noexcept override
      {

        std::optional<Completion<T>> result;
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
        try
        {
          result.emplace(invoke_and_normalize());
        }
        catch (const std::exception &exception)
        {
          result.emplace(exception_completion(exception.what()));
        }
        catch (...)
        {
          result.emplace(Completion<T>(Error(errors::worker_exception)));
        }
#else
        result.emplace(invoke_and_normalize());
#endif
        {
          std::lock_guard lock(mutex_);
          phase_ = WorkPhase::Completing;
        }
        callable_.reset();
        publish(std::move(*result));
      }

      void shutdown_cancel() noexcept override
      {
        (void)request_cancel();
        {
          std::unique_lock lock(mutex_);
          completed_.wait(lock, [this] { return !cancellation_delivery_; });
          if (phase_ == WorkPhase::Pending)
          {
            phase_ = WorkPhase::Completing;
            pending_cancel_completion_ = true;
          }
        }
        finish_pending_cancellation();
        finish_pending_timeout();
        std::unique_lock lock(mutex_);
        if (phase_ == WorkPhase::Completing &&
            (pending_cancel_completion_ || pending_timeout_completion_))
          completed_.wait(lock, [this] { return phase_ == WorkPhase::Terminal; });
      }

      [[nodiscard]] bool ready() const noexcept override
      {
        std::lock_guard lock(mutex_);
        return phase_ == WorkPhase::Terminal;
      }

      [[nodiscard]] bool request_cancel() noexcept override
      {
        bool cancel_pending = false;
        {
          std::lock_guard lock(mutex_);
          if (phase_ == WorkPhase::Completing || phase_ == WorkPhase::Terminal)
            return false;
          if (cancel_requested_)
            return false;
          cancel_requested_ = true;
          cancellation_delivery_ = true;
          deadline_observed_ = deadline_.has_value();
          if (phase_ == WorkPhase::Pending)
          {
            phase_ = WorkPhase::Completing;
            pending_cancel_completion_ = true;
            cancel_pending = true;
          }
        }
        notify_deadline_monitor(this->owner_);
        deliver_cancellation_request();
        finish_pending_cancellation();
        if (cancel_pending)
        {
          std::unique_lock lock(mutex_);
          completed_.wait(lock, [this] { return phase_ == WorkPhase::Terminal; });
        }
        return true;
      }

      [[nodiscard]] Result<void> wait() const override
      {
        std::unique_lock lock(mutex_);
        if (phase_ != WorkPhase::Terminal)
        {
          if (vix::detail::in_cancellation_callback())
            return Error(errors::would_deadlock);
          const auto owner = this->owner_.lock();
          if (owner && is_current_resource_worker(owner.get()))
            return Error(errors::would_deadlock);
          completed_.wait(lock, [this] { return phase_ == WorkPhase::Terminal; });
        }
        return Result<void>::success();
      }

      [[nodiscard]] Result<std::optional<Completion<T>>> try_take() override
      {
        std::lock_guard lock(mutex_);
        if (phase_ != WorkPhase::Terminal)
          return std::optional<Completion<T>>{};
        if (consumed_)
          return Error(errors::completion_already_taken);
        consumed_ = true;
        return std::optional<Completion<T>>(std::move(completion_));
      }

      Result<bool> subscribe(
          const std::shared_ptr<NotificationState> &notification) override
      {
        std::lock_guard lock(mutex_);
        if (phase_ == WorkPhase::Terminal)
          return true;
        if (notification_)
          return Error(errors::notification_already_registered);
        auto owner = this->owner_.lock();
        if (!owner)
          return Error(errors::resource_shutting_down);
        if (!register_notification_obligation(owner, notification))
          return Error(errors::notification_capacity_exhausted);
        notification_ = notification;
        return false;
      }

    private:
      void finish_pending_cancellation() noexcept
      {
        bool should_publish = false;
        {
          std::lock_guard lock(mutex_);
          if (phase_ == WorkPhase::Completing && pending_cancel_completion_ &&
              !pending_cancel_finalizing_)
          {
            pending_cancel_finalizing_ = true;
            should_publish = true;
          }
        }
        if (should_publish)
        {
          callable_.reset();
          publish(Completion<T>::cancelled());
        }
      }

      void finish_pending_timeout() noexcept
      {
        bool should_publish = false;
        {
          std::lock_guard lock(mutex_);
          if (phase_ == WorkPhase::Completing && pending_timeout_completion_ &&
              !pending_timeout_finalizing_)
          {
            pending_timeout_finalizing_ = true;
            should_publish = true;
          }
        }
        if (should_publish)
        {
          callable_.reset();
          publish(Completion<T>(Error(errors::deadline_expired)));
        }
      }

      void deliver_cancellation_request() noexcept
      {
        (void)cancellation_.request_cancel();
        {
          std::lock_guard lock(mutex_);
          cancellation_delivery_ = false;
        }
        completed_.notify_all();
      }

      [[nodiscard]] Completion<T> invoke_and_normalize()
      {
        constexpr bool takes_context = std::is_invocable_v<F &, WorkContext &>;
        using ReturnType = typename invocation_result<F>::type;
        constexpr int kind = outcome_traits<ReturnType>::kind;
        if constexpr (kind == 1)
        {
          if constexpr (takes_context)
            std::invoke(*callable_, context_);
          else
            std::invoke(*callable_);
          return Completion<void>::success();
        }
        else
        {
          if constexpr (takes_context)
          {
            auto value = std::invoke(*callable_, context_);
            return Completion<T>(std::move(value));
          }
          else
          {
            auto value = std::invoke(*callable_);
            return Completion<T>(std::move(value));
          }
        }
      }

#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
      [[nodiscard]] static Completion<T> exception_completion(const char *message)
      {
        if (message == nullptr)
          return Completion<T>(Error(errors::worker_exception));
        auto error = vix::detail::allocation_or_terminate([&]
        {
          return Error(errors::worker_exception, std::string(message));
        });
        return Completion<T>(std::move(error));
      }
#endif

      void publish(Completion<T> completion) noexcept
      {
        std::shared_ptr<NotificationState> notification;
        std::shared_ptr<ResourceState> owner;
        {
          std::lock_guard lock(mutex_);
          if (phase_ != WorkPhase::Completing)
            std::terminate();
          completion_.emplace(std::move(completion));
          phase_ = WorkPhase::Terminal;
          pending_cancel_completion_ = false;
          pending_cancel_finalizing_ = false;
          pending_timeout_completion_ = false;
          pending_timeout_finalizing_ = false;
          notification = std::move(notification_);
          if (notification)
            owner = this->owner_.lock();
        }
        completed_.notify_all();
        notify_deadline_monitor(this->owner_);
        if (notification)
        {
          if (!owner)
            std::terminate();
          (void)enqueue_notification(owner, notification);
        }
      }

      mutable std::mutex mutex_;
      mutable std::condition_variable completed_;
      std::optional<Deadline> deadline_;
      CancellationSource cancellation_;
      WorkContext context_;
      std::optional<F> callable_;
      std::optional<Completion<T>> completion_;
      std::shared_ptr<NotificationState> notification_;
      WorkPhase phase_{WorkPhase::Pending};
      bool cancel_requested_{false};
      bool cancellation_delivery_{false};
      bool pending_cancel_completion_{false};
      bool pending_cancel_finalizing_{false};
      bool pending_timeout_completion_{false};
      bool pending_timeout_finalizing_{false};
      bool deadline_observed_{false};
      bool consumed_{false};
    };
  } // namespace detail

  template <typename T>
  bool WorkHandle<T>::ready() const noexcept
  {
    return state_ && state_->ready();
  }

  template <typename T>
  bool WorkHandle<T>::request_cancel() const noexcept
  {
    return state_ && state_->request_cancel();
  }

  template <typename T>
  Result<void> WorkHandle<T>::wait() const
  {
    if (!state_)
      return Error(errors::invalid_work_handle);
    return state_->wait();
  }

  template <typename T>
  Result<std::optional<Completion<T>>> WorkHandle<T>::try_take_completion() const
  {
    if (!state_)
      return Error(errors::invalid_work_handle);
    return state_->try_take();
  }

  template <typename T>
  Result<Completion<T>> WorkHandle<T>::take_completion() const
  {
    auto waited = wait();
    if (!waited)
      return std::move(waited).error();
    auto completion = try_take_completion();
    if (!completion)
      return std::move(completion).error();
    return std::move(*completion.value());
  }
} // namespace vix::concurrency

#endif // VIX_CONCURRENCY_EXECUTION_RESOURCE_HPP
