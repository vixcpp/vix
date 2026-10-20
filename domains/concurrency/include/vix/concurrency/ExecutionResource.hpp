/**
 * @file ExecutionResource.hpp
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
 * @brief Bounded native task execution and completion observation.
 *
 * @details
 * The Vix Concurrency domain provides explicitly owned resources for
 * executing callables on native worker threads. ExecutionResource controls
 * bounded admission, cooperative cancellation, monotonic deadlines,
 * one-shot completion observation, and optional completion notifications.
 *
 * Work submission returns Result<WorkHandle<T>>. Accepted work publishes
 * exactly one Completion<T>, which may represent success, failure, or
 * cancellation. A handle can be copied, but its completion can be taken
 * only once across all copies.
 *
 * @par Basic example
 * @code{.cpp}
 * #include <vix/concurrency.hpp>
 *
 * int main()
 * {
 *     auto created = vix::concurrency::ExecutionResource::create();
 *     if (!created)
 *         return 1;
 *
 *     auto resource = created.take();
 *     auto submitted = resource.submit([] { return 42; });
 *     if (!submitted)
 *         return 1;
 *
 *     auto handle = submitted.take();
 *     auto completed = handle.take_completion();
 *     if (!completed)
 *         return 1;
 *
 *     return resource.shutdown() ? 0 : 1;
 * }
 * @endcode
 *
 * @warning
 * Never destroy a live resource from one of its own workers or from a
 * cancellation callback. A noncooperative task may cause shutdown() to
 * block indefinitely; cancellation is not forced interruption.
 *
 * @see vix::concurrency::ExecutionResource
 * @see vix::concurrency::WorkHandle
 * @see vix::concurrency::WorkContext
 * @see vix::concurrency::NotificationRegistration
 */

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

  /**
   * @namespace vix::concurrency::errors
   * @brief Stable, machine-readable errors for the Concurrency domain.
   *
   * Each identity belongs to the "vix.concurrency" domain. Submission and
   * control operations report Result failures; failures of accepted work are
   * recorded separately in Completion<T>. Notification callback failures
   * are observed through NotificationRegistration::wait() or reset().
   */
  namespace errors
  {
    /**
     * @brief Invalid native execution resource configuration.
     *
     * @details Reported when max_concurrency is zero, the total requested capacity overflows, or requested sizes exceed supported container limits.
     *
     * @note Error domain: "vix.concurrency". Error code: 1.
     */
    inline constexpr auto invalid_configuration = ErrorIdentity::define<"vix.concurrency">(1);
    /**
     * @brief Native concurrent execution is unavailable.
     *
     * @details Creation is unsupported on platforms such as WASI Preview 2, where no native execution workers are provided.
     *
     * @note Error domain: "vix.concurrency". Error code: 2.
     */
    inline constexpr auto execution_unavailable = ErrorIdentity::define<"vix.concurrency">(2);
    /**
     * @brief The execution resource or submission state could not be created.
     *
     * @details For example, native worker creation or a recoverable callable preparation failure may prevent creation or admission.
     *
     * @note Error domain: "vix.concurrency". Error code: 3.
     */
    inline constexpr auto resource_creation_failed = ErrorIdentity::define<"vix.concurrency">(3);
    /**
     * @brief The execution resource has no remaining admission capacity.
     *
     * @details Total accepted work is bounded by max_concurrency + max_pending; capacity can remain occupied briefly until a completed entry is retired.
     *
     * @note Error domain: "vix.concurrency". Error code: 4.
     */
    inline constexpr auto capacity_exhausted = ErrorIdentity::define<"vix.concurrency">(4);
    /**
     * @brief The resource is closing to new submissions.
     *
     * @details Returned when admission has been closed by shutdown(); accepted work can still be finishing.
     *
     * @note Error domain: "vix.concurrency". Error code: 5.
     */
    inline constexpr auto resource_shutting_down = ErrorIdentity::define<"vix.concurrency">(5);
    /**
     * @brief A work handle or resource has no valid owning state.
     *
     * @details For example, a default-constructed or moved-from WorkHandle cannot wait for or consume a completion.
     *
     * @note Error domain: "vix.concurrency". Error code: 6.
     */
    inline constexpr auto invalid_work_handle = ErrorIdentity::define<"vix.concurrency">(6);
    /**
     * @brief The terminal completion was already consumed.
     *
     * @details Completion<T> is one-shot across all copies of the same WorkHandle<T>.
     *
     * @note Error domain: "vix.concurrency". Error code: 7.
     */
    inline constexpr auto completion_already_taken = ErrorIdentity::define<"vix.concurrency">(7);
    /**
     * @brief A requested blocking wait would deadlock.
     *
     * @details Waiting on unfinished work from a worker of the same resource or from a cancellation callback is rejected. Notification self-wait and self-reset also return this identity.
     *
     * @note Error domain: "vix.concurrency". Error code: 8.
     */
    inline constexpr auto would_deadlock = ErrorIdentity::define<"vix.concurrency">(8);
    /**
     * @brief Shutdown was requested on the resource worker thread.
     *
     * @details Joining the resource from one of its own workers is not permitted.
     *
     * @note Error domain: "vix.concurrency". Error code: 9.
     */
    inline constexpr auto shutdown_from_worker = ErrorIdentity::define<"vix.concurrency">(9);
    /**
     * @brief Shutdown was requested inside a cancellation callback.
     *
     * @details Synchronous cancellation callback execution cannot reenter shutdown safely.
     *
     * @note Error domain: "vix.concurrency". Error code: 10.
     */
    inline constexpr auto shutdown_from_cancellation_callback = ErrorIdentity::define<"vix.concurrency">(10);
    /**
     * @brief An accepted callable threw an exception.
     *
     * @details In exception-enabled builds, catchable exceptions escaping a worker callable become a failed Completion<T>; this is distinct from notification callback failures.
     *
     * @note Error domain: "vix.concurrency". Error code: 11.
     */
    inline constexpr auto worker_exception = ErrorIdentity::define<"vix.concurrency">(11);
    /**
     * @brief Shutdown was reentered by its current owner thread.
     *
     * @details The resource rejects recursive shutdown on the same shutdown thread.
     *
     * @note Error domain: "vix.concurrency". Error code: 12.
     */
    inline constexpr auto shutdown_reentrant = ErrorIdentity::define<"vix.concurrency">(12);
    /**
     * @brief A completion notification callback threw an exception.
     *
     * @details The notification registration reports this independently of the already-published work Completion<T>.
     *
     * @note Error domain: "vix.concurrency". Error code: 13.
     */
    inline constexpr auto notification_exception = ErrorIdentity::define<"vix.concurrency">(13);
    /**
     * @brief A work item already has its one notification registration slot.
     *
     * @details Only one callback may be registered for a work item, including across copies of the same WorkHandle; removing it does not reopen the slot.
     *
     * @note Error domain: "vix.concurrency". Error code: 14.
     */
    inline constexpr auto notification_already_registered = ErrorIdentity::define<"vix.concurrency">(14);
    /**
     * @brief The notification registration is empty or unavailable.
     *
     * @details For example, take_registration() fails when the work was already ready or the registration has already been taken.
     *
     * @note Error domain: "vix.concurrency". Error code: 15.
     */
    inline constexpr auto invalid_notification_registration = ErrorIdentity::define<"vix.concurrency">(15);
    /**
     * @brief The notification callback could not be prepared.
     *
     * @details Callback construction can fail before the notification is registered.
     *
     * @note Error domain: "vix.concurrency". Error code: 16.
     */
    inline constexpr auto notification_registration_failed = ErrorIdentity::define<"vix.concurrency">(16);
    /**
     * @brief The resource has no available notification obligations.
     *
     * @details Outstanding accepted notifications are bounded by the resource work capacity.
     *
     * @note Error domain: "vix.concurrency". Error code: 17.
     */
    inline constexpr auto notification_capacity_exhausted = ErrorIdentity::define<"vix.concurrency">(17);
    /**
     * @brief Shutdown was requested from a resource-owned notification callback.
     *
     * @details Shutdown cannot join its own notification execution or cleanup context.
     *
     * @note Error domain: "vix.concurrency". Error code: 18.
     */
    inline constexpr auto shutdown_from_notification_callback = ErrorIdentity::define<"vix.concurrency">(18);
    /**
     * @brief A work admission or pending work item exceeded its deadline.
     *
     * @details An already-expired admission is rejected without consuming the callable. Pending work may terminalize with this failure; running work receives only a cooperative cancellation request.
     *
     * @note Error domain: "vix.concurrency". Error code: 19.
     */
    inline constexpr auto deadline_expired = ErrorIdentity::define<"vix.concurrency">(19);
  } // namespace errors

  /// @cond VIX_CONCURRENCY_INTERNAL
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
      enum class Phase : unsigned char
      {
        Registered,
        Queued,
        Running,
        Inactive,
        Done
      };

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
  /// @endcond

  /**
   * @brief Read-only cancellation context supplied to a work callable.
   *
   * @details When a callable accepts WorkContext&, submit() passes the
   * context-taking form. A running task can periodically inspect
   * stop_requested() or pass cancellation_token() to compatible APIs.
   *
   * Cancellation is cooperative: receiving a request does not forcibly
   * interrupt the callable or replace the result it actually returns.
   *
   * @par Example
   * @code{.cpp}
   * auto submitted = resource.submit([](vix::concurrency::WorkContext &ctx) {
   *     if (ctx.stop_requested())
   *         return 0;
   *     return 42;
   * });
   * @endcode
   *
   * @note The context and its token do not grant ownership of the
   * ExecutionResource. Do not retain WorkContext& beyond the callable.
   * @see WorkHandle::request_cancel()
   * @see ExecutionResource::submit()
   */
  class WorkContext
  {
  public:
    /**
     * @brief Returns this work item's cooperative cancellation token.
     * @return A token that observes requests to cancel the accepted work.
     * @note Standard cancellation callbacks can run synchronously in the
     * thread delivering the request; do not block resource shutdown in one.
     */
    [[nodiscard]] CancellationToken cancellation_token() const noexcept
    {
      return token_;
    }
    /**
     * @brief Checks whether cancellation has been requested.
     * @return true when this work item's token reports cancellation.
     * @note A running callable decides how to respond. It may still
     * publish a successful or failed Completion<T>.
     */
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

  /**
   * @brief Optional execution constraints for a submitted work item.
   *
   * @details Deadline is an absolute vix::Deadline based on Standard's
   * process-local monotonic clock. A reached deadline rejects admission
   * without invoking or moving the supplied callable. If expiration wins
   * while work is pending, it publishes a deadline_expired failure without
   * executing the callable. If work is already running, expiration requests
   * cooperative cancellation and preserves the callable's eventual result.
   *
   * @note Deadline observation is best-effort, not a real-time guarantee.
   * @see ExecutionResource
   */
  struct WorkOptions
  {
    /**
     * @brief Optional absolute deadline for admission and execution.
     * @note std::nullopt permits execution without a deadline. Expiration
     * while running does not forcibly stop the callable.
     */
    std::optional<Deadline> deadline{};
  };

  /**
   * @brief Owns a one-shot notification attached to accepted work.
   *
   * @details A WorkHandle<T>::on_ready() registration is executed at most once
   * by a worker of the originating ExecutionResource after work terminalizes.
   * A callback may return void or Result<void>. Returned errors and catchable
   * exceptions (notification_exception) are reported by wait() and reset(),
   * never by the immutable work Completion<T>.
   *
   * A registration owns its callback lifetime. deactivate() is nonblocking;
   * an already-running callback continues. reset() deactivates and waits for
   * callback quiescence, including destruction of callback captures.
   * Destruction also deactivates and waits, except when called by that very
   * callback, where destruction is nonblocking.
   *
   * @warning Captures referencing outside objects must stay valid until
   * callback quiescence. Concurrent state methods on the same live
   * registration are supported, but moving or destroying the C++ object
   * must not race with a member call.
   *
   * @warning A blocking wait from the current callback or a resource worker
   * may return would_deadlock. Destroying a registration from a resource
   * worker while another callback is running may terminate the process.
   *
   * @see ReadySubscription
   * @see WorkHandle::on_ready()
   */
  class NotificationRegistration
  {
  public:
    /**
     * @brief Constructs an empty, invalid registration.
     * @note Obtain an active registration from ReadySubscription::take_registration().
     */
    NotificationRegistration() noexcept = default;
    /**
     * @brief Copying is disabled; the notification has one owning registration.
     */
    NotificationRegistration(const NotificationRegistration &) = delete;
    /**
     * @brief Copy assignment is disabled.
     */
    NotificationRegistration &operator=(const NotificationRegistration &) = delete;
    /**
     * @brief Transfers registration ownership to a new object.
     * @note Do not race a move with calls on the source object.
     */
    NotificationRegistration(NotificationRegistration &&) noexcept;
    /**
     * @brief Replaces this registration by moving another.
     * @return Reference to this object.
     * @warning The existing registration is first deactivated and quiesced;
     * an unsafe self-wait context may terminate rather than block.
     */
    NotificationRegistration &operator=(NotificationRegistration &&) noexcept;
    /**
     * @brief Deactivates and releases an owned notification.
     * @details Externally, destruction waits until the callback and closure
     * cleanup are quiescent. Destruction by the registered callback itself is
     * nonblocking. Callback errors cannot be reported by the destructor.
     * @warning Do not destroy a live registration concurrently with its methods.
     */
    ~NotificationRegistration();

    /**
     * @brief Checks whether a registration state is currently held.
     * @return true when this object owns a notification registration state.
     * @note Validity does not mean its callback has not already completed.
     */
    [[nodiscard]] bool valid() const noexcept
    {
      std::lock_guard lock(state_mutex_);
      return static_cast<bool>(state_);
    }
    // Nonblocking. If dispatch has already begun, that invocation continues;
    // callback captures must remain valid until wait/reset or quiescent
    // destruction completes.
    /**
     * @brief Requests nonblocking notification deactivation.
     * @details Prevents pending callback invocation when possible. A callback
     * that has already begun is allowed to finish.
     * @warning Captured referents must remain valid until wait(), reset(),
     * or quiescent destruction establishes that cleanup has finished.
     */
    void deactivate() noexcept;
    // Deactivates and waits for quiescence where doing so cannot self-deadlock.
    // Called by its own callback, it returns would_deadlock; use deactivate()
    // for nonblocking self-deactivation and retain this handle to observe the
    // terminal callback outcome later.
    /**
     * @brief Deactivates and waits for the registration to become quiescent.
     * @return Success, the callback's Result<void> failure, or an Error such
     * as would_deadlock when blocking would be unsafe.
     * @details Once finished, this object becomes invalid. If called from
     * its own callback, returns would_deadlock and retains the registration
     * so another observer can collect its outcome later.
     * @note Quiescence includes completion of callback capture destruction.
     */
    [[nodiscard]] Result<void> reset();
    // Waits for dispatch completion; callback failures are returned here.
    /**
     * @brief Waits for completion or deactivation of the notification.
     * @return Success, any callback failure, or would_deadlock when it cannot
     * safely block. An invalid registration returns invalid_notification_registration.
     * @note Unlike reset(), wait() does not deactivate the notification.
     */
    [[nodiscard]] Result<void> wait() const;

  private:
    friend class ReadySubscription;
    template <typename>
    friend class WorkHandle;
    explicit NotificationRegistration(std::shared_ptr<detail::NotificationState> state) noexcept
        : state_(std::move(state)) {}
    [[nodiscard]] std::shared_ptr<detail::NotificationState> snapshot_state() const noexcept
    {
      std::lock_guard lock(state_mutex_);
      return state_;
    }
    mutable std::mutex state_mutex_;
    std::shared_ptr<detail::NotificationState> state_;
  };

  /**
   * @brief Result of an atomic readiness check or callback registration.
   *
   * @details WorkHandle<T>::on_ready() returns either an already-ready
   * observation or a pending NotificationRegistration. If the work was ready,
   * the provided callback is not invoked. If work was not ready, extract
   * the registration with take_registration() to retain callback ownership.
   *
   * An unextracted registration is deactivated by normal RAII cleanup.
   * @see WorkHandle::on_ready()
   * @see NotificationRegistration
   */
  class ReadySubscription
  {
  public:
    /**
     * @brief Constructs a default subscription with no registration.
     */
    ReadySubscription() noexcept = default;
    /**
     * @brief Copying is disabled because registration ownership is unique.
     */
    ReadySubscription(const ReadySubscription &) = delete;
    /**
     * @brief Copy assignment is disabled.
     */
    ReadySubscription &operator=(const ReadySubscription &) = delete;
    /**
     * @brief Transfers the readiness result and any registration.
     */
    ReadySubscription(ReadySubscription &&) noexcept = default;
    /**
     * @brief Replaces this subscription with a moved subscription.
     * @return Reference to this object.
     */
    ReadySubscription &operator=(ReadySubscription &&) noexcept = default;

    /**
     * @brief Reports whether the work was terminal at registration time.
     * @return true when work was already ready and no callback was registered.
     * @note A false return indicates a registration was accepted; it does
     * not guarantee the work remains incomplete now.
     */
    [[nodiscard]] bool already_ready() const noexcept { return already_ready_; }
    /**
     * @brief Transfers the accepted notification registration to the caller.
     * @return An owning registration or invalid_notification_registration
     * when work was already ready or the registration was already consumed.
     * @note Call this before the ReadySubscription is destroyed if the
     * callback should remain registered.
     */
    [[nodiscard]] Result<NotificationRegistration> take_registration();

  private:
    template <typename>
    friend class WorkHandle;
    explicit ReadySubscription(bool ready) noexcept : already_ready_(ready) {}
    explicit ReadySubscription(NotificationRegistration registration) noexcept
        : registration_(std::move(registration)) {}
    bool already_ready_{false};
    std::optional<NotificationRegistration> registration_;
  };

  /**
   * @brief Shared control and one-shot terminal result for accepted work.
   *
   * @tparam T Successful work result type; may be void.
   *
   * @details Handles are copyable and observe the same accepted work item.
   * A handle does not own its resource's workers and destruction never
   * cancels or detaches accepted work. Completion<T> is published once,
   * and only one copy of the handle may consume it.
   *
   * @par Example
   * @code{.cpp}
   * auto accepted = resource.submit([] { return 42; });
   * if (accepted)
   * {
   *     auto handle = accepted.take();
   *     auto result = handle.take_completion();
   *     if (result)
   *     {
   *         // Inspect the terminal Completion<int>.
   *     }
   * }
   * @endcode
   *
   * @par Synchronization
   * ready(), wait(), and completion retrieval observe terminal publication
   * with synchronization. A worker of the same resource must not block on
   * unfinished work: wait() reports would_deadlock in that situation.
   *
   * @see ExecutionResource::submit()
   * @see WorkContext
   * @see NotificationRegistration
   */
  template <typename T>
  class WorkHandle
  {
  public:
    /**
     * @brief Constructs an invalid, empty work handle.
     */
    WorkHandle() noexcept = default;
    /**
     * @brief Copies the shared observation handle for the same work item.
     */
    WorkHandle(const WorkHandle &) noexcept = default;
    /**
     * @brief Transfers shared observation ownership from another handle.
     */
    WorkHandle(WorkHandle &&) noexcept = default;
    /**
     * @brief Copy-assigns a handle to the same accepted work.
     * @return Reference to this handle.
     */
    WorkHandle &operator=(const WorkHandle &) noexcept = default;
    /**
     * @brief Move-assigns a shared observation handle.
     * @return Reference to this handle.
     */
    WorkHandle &operator=(WorkHandle &&) noexcept = default;
    /**
     * @brief Releases this copy of the observation handle.
     * @note Does not cancel, interrupt, or detach the accepted work.
     */
    ~WorkHandle() = default;

    /**
     * @brief Checks whether this handle references an accepted work item.
     * @return true when its shared work state exists.
     */
    [[nodiscard]] bool valid() const noexcept { return static_cast<bool>(state_); }
    /**
     * @brief Nonblockingly checks whether a terminal completion is published.
     * @return true when the work is terminal; false for invalid or incomplete
     * handles. It does not consume the completion.
     */
    [[nodiscard]] bool ready() const noexcept;
    /**
     * @brief Requests cooperative cancellation of this work item.
     * @return true when this call newly recorded cancellation, false if
     * invalid, already requested, or already completing/terminal.
     * @details Pending work may become Cancelled without invocation. Running
     * work is not forcibly stopped and preserves the callable's actual result.
     * This call can block while synchronous cancellation callbacks execute.
     */
    [[nodiscard]] bool request_cancel() const noexcept;
    /**
     * @brief Blocks until terminal completion is published.
     * @return Success after terminal publication; invalid_work_handle if empty,
     * or would_deadlock when blocking unfinished work would be unsafe.
     * @note This does not consume the Completion<T>. A nonzero worker-level
     * failure is inspected separately by take_completion().
     */
    [[nodiscard]] Result<void> wait() const;
    /**
     * @brief Attempts to consume the terminal outcome without waiting.
     * @return An empty optional while unfinished, a populated Completion<T>
     * after terminal publication, or an Error if invalid or already consumed.
     * @warning Completion is one-shot across all copies of WorkHandle<T>.
     */
    [[nodiscard]] Result<std::optional<Completion<T>>> try_take_completion() const;
    /**
     * @brief Waits for and consumes the terminal outcome once.
     * @return Completion<T> on success, or an Error for invalid handles,
     * unsafe blocking, or completion_already_taken.
     * @details Equivalent to wait() followed by try_take_completion().
     * Completion<T> can represent success, failure, or cancellation.
     */
    [[nodiscard]] Result<Completion<T>> take_completion() const;

    /**
     * @brief Atomically observes readiness or installs a one-shot callback.
     *
     * @tparam F Type of a no-argument, nothrow-destructible callback.
     * @param callback Callable returning void or Result<void>.
     * @return ReadySubscription indicating already-ready work, or carrying
     * a NotificationRegistration, or an Error if registration fails.
     *
     * @details A work item has exactly one callback slot across all copies
     * of its WorkHandle. Once taken, even a deactivated slot cannot be reused.
     * If work is already terminal, already_ready() is true and callback is
     * not invoked. Otherwise the callback is dispatched once by a resource
     * worker after terminal publication, never inline on this calling thread.
     *
     * Callback failure or a translated notification_exception is observed
     * through NotificationRegistration::wait() or reset(), not through the
     * completed work item's Completion<T>.
     *
     * @par Example
     * @code{.cpp}
     * auto subscribed = handle.on_ready([] {});
     * if (subscribed)
     * {
     *     auto ready = subscribed.take();
     *     if (!ready.already_ready())
     *     {
     *         auto registration = ready.take_registration();
     *         if (registration)
     *         {
     *             auto owner = registration.take();
     *             (void)owner.wait();
     *         }
     *     }
     * }
     * @endcode
     *
     * @warning Keep borrowed callback captures valid until completion or
     * registration quiescence. The notification is capacity-bounded and may
     * return notification_capacity_exhausted.
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
                                                              { return std::make_shared<CallbackModel>(std::forward<F>(callback)); });
      }
      catch (...)
      {
        return Error(errors::notification_registration_failed);
      }
#else
      callback_state = vix::detail::allocation_or_terminate([&]
                                                            { return std::make_shared<CallbackModel>(std::forward<F>(callback)); });
#endif
      auto notification = vix::detail::allocation_or_terminate([&]
                                                               { return std::make_shared<detail::NotificationState>(std::move(callback_state)); });
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

  /**
   * @brief Owns a bounded native execution resource for submitted callables.
   *
   * @details Explicitly created via create(), the resource starts a fixed
   * number of native workers and accepts work only within its bounded
   * max_concurrency + max_pending capacity. There is no global executor.
   * The default creates one work worker and no pending allowance.
   *
   * A private deadline-observer thread handles monotonic expiry. Its
   * observation latency is best-effort, with no real-time guarantee.
   *
   * @par Example
   * @code{.cpp}
   * using vix::concurrency::ExecutionResource;
   * auto created = ExecutionResource::create({2, 4});
   * if (created)
   * {
   *     auto resource = created.take();
   *     auto submitted = resource.submit([] { return 7; });
   *     if (submitted)
   *     {
   *         auto handle = submitted.take();
   *         (void)handle.wait();
   *     }
   *     (void)resource.shutdown();
   * }
   * @endcode
   *
   * @par Admission
   * A successful reservation is the admission point. Rejection due to
   * shutdown, capacity, or expired deadline does not invoke or move from
   * the supplied callable. Work may still count toward capacity briefly
   * after completion until a worker retires its queue entry.
   *
   * @par Shutdown and lifetime
   * shutdown() closes admission, requests cancellation of accepted work,
   * waits for active work and notifications, and joins native threads.
   * Running callables are not forcibly interrupted; shutdown may block
   * indefinitely if they do not cooperate. is_shutdown() only reports
   * closed admission, not worker quiescence.
   *
   * @warning Destruction of a live resource from its own worker, a
   * cancellation callback, or other unsafe shutdown context is a contract
   * fault. Retain an external owner until shutdown has completed.
   *
   * @note WASI Preview 2 returns execution_unavailable rather than
   * creating native threads.
   *
   * @see WorkHandle
   * @see WorkOptions
   * @see WorkContext
   */
  class ExecutionResource
  {
  public:
    /**
     * @brief Sets worker count and pending admission allowance.
     *
     * @details max_concurrency must be positive. Total bounded capacity
     * is max_concurrency + max_pending. Overflows and unsupported container
     * sizes cause invalid_configuration.
     * @note Defaults are one work worker and zero pending allowance.
     */
    struct Options
    {
      /**
       * @brief Number of native work workers; must be at least one.
       */
      std::size_t max_concurrency{1};
      /**
       * @brief Number of additional accepted waiting slots.
       */
      std::size_t max_pending{0};
    };

    /**
     * @brief Creates an execution resource with explicit capacity.
     * @param options Worker count and pending admission allowance.
     * @return A resource or invalid_configuration, execution_unavailable,
     * or resource_creation_failed as applicable.
     * @details Creates work workers and a private deadline-observer thread
     * on supported native platforms. No global runtime is used.
     */
    [[nodiscard]] static Result<ExecutionResource> create(Options options);
    /**
     * @brief Creates an execution resource using default capacity.
     * @return A Result containing a resource with one worker and no
     * additional pending slots, or a creation error.
     * @see create(Options)
     */
    [[nodiscard]] static Result<ExecutionResource> create()
    {
      return create(Options{});
    }

    /**
     * @brief Copy construction is disabled; execution ownership is exclusive.
     */
    ExecutionResource(const ExecutionResource &) = delete;
    /**
     * @brief Copy assignment is disabled.
     */
    ExecutionResource &operator=(const ExecutionResource &) = delete;
    /**
     * @brief Transfers ownership of a resource to a new object.
     * @note The moved-from resource has no execution state.
     */
    ExecutionResource(ExecutionResource &&) noexcept;
    /**
     * @brief Move assignment is disabled.
     */
    ExecutionResource &operator=(ExecutionResource &&) = delete;
    /**
     * @brief Shuts down an owned resource and releases its workers.
     * @details Destruction performs shutdown if a resource remains owned.
     * @warning Unsafe destruction (including from a resource worker or
     * cancellation callback) is fatal. Use explicit shutdown() to observe
     * errors and ensure worker quiescence.
     */
    ~ExecutionResource();

    /**
     * @brief Submits work without a deadline.
     * @tparam F Callable type, invocable with WorkContext& or no arguments.
     * @param callable Callable to execute once.
     * @return WorkHandle<Value> on admission, or an Error if rejected.
     * @details Equivalent to submit(WorkOptions{}, callable). A callable
     * accepting both supported signatures receives WorkContext&. Lvalue
     * callables are copied; rvalues are moved only after successful admission.
     * Returned values, void, Result<T>, and Completion<T> are normalized to
     * Completion<T>. No worker is interrupted forcibly.
     */
    template <typename F>
    [[nodiscard]] auto submit(F &&callable)
        -> Result<WorkHandle<typename detail::outcome_traits<typename detail::invocation_result<
            std::decay_t<F>>::type>::value_type>>
    {
      return submit(WorkOptions{}, std::forward<F>(callable));
    }

    /**
     * @brief Submits one callable with an optional absolute deadline.
     * @tparam F Nothrow-movable, nothrow-destructible callable type.
     * @param options Optional monotonic execution deadline.
     * @param callable Callable invoked once with WorkContext& or no arguments.
     * @return An owning shared WorkHandle<T> on acceptance; otherwise an Error.
     *
     * @details Admission is bounded and atomic with shutdown closure. Rejected
     * submissions do not execute or move from the caller's callable. Lvalues
     * may be copied into staging before admission. If a deadline has already
     * expired, submission returns deadline_expired synchronously.
     *
     * A pending item whose deadline expires is not invoked and terminalizes
     * with deadline_expired. A running item receives cooperative cancellation,
     * but its eventual callable result remains authoritative.
     *
     * Callable returns may be T, void, Result<T>, or Completion<T> (including
     * void forms). Returning references, Error as successful data, or nested
     * Result/Completion payloads is unsupported. Nonvoid successful payloads
     * must be nothrow move constructible.
     *
     * @warning Borrowed captures must stay alive until the work reaches
     * terminal completion. Rejected admission does not guarantee that an
     * lvalue callable was not copied during preparation.
     *
     * @see WorkOptions
     * @see WorkHandle
     * @see WorkContext
     */
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
                                                        { return std::make_shared<detail::WorkState<Value, Callable>>(options.deadline); });
      }
      catch (...)
      {
        return Error(errors::resource_creation_failed);
      }
#else
      accepted = vix::detail::allocation_or_terminate([&]
                                                      { return std::make_shared<detail::WorkState<Value, Callable>>(options.deadline); });
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

    /**
     * @brief Closes admission and waits for all resource-owned activity.
     * @return Success after workers and accepted notifications are quiescent,
     * or a context-specific shutdown error.
     * @details Cancels still-pending work and cooperatively requests
     * cancellation of running callables. Joins workers and the private
     * deadline-observer thread, and drains registered notifications.
     * Concurrent external calls converge on the same quiescent result.
     *
     * @warning May wait indefinitely for a noncooperative callable or
     * blocking cancellation callback. Shutdown from an owned worker,
     * cancellation callback, notification callback, or reentrant shutdown
     * context returns a dedicated error rather than self-joining.
     *
     * @see is_shutdown()
     * @see WorkHandle::request_cancel()
     */
    [[nodiscard]] Result<void> shutdown() noexcept;
    // Reports that admission is closed; it does not wait for worker
    // quiescence. Use shutdown() when completion of accepted work is required.
    /**
     * @brief Reports whether admission is closed.
     * @return true when the resource has begun shutdown or is moved-from.
     * @note This does not prove all work or workers are quiescent; only
     * successful shutdown() establishes that boundary.
     */
    [[nodiscard]] bool is_shutdown() const noexcept;

  private:
    explicit ExecutionResource(std::shared_ptr<detail::ResourceState> state) noexcept
        : state_(std::move(state)) {}
    [[nodiscard]] Result<void> reserve_work(std::optional<Deadline> deadline);
    void commit_work(const std::shared_ptr<detail::WorkBase> &) noexcept;
    std::shared_ptr<detail::ResourceState> state_;
  };

  /// @cond VIX_CONCURRENCY_INTERNAL
  namespace detail
  {
    enum class WorkPhase : unsigned char
    {
      Pending,
      Running,
      Completing,
      Terminal
    };

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
          completed_.wait(lock, [this]
                          { return !cancellation_delivery_; });
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
          completed_.wait(lock, [this]
                          { return phase_ == WorkPhase::Terminal; });
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
          completed_.wait(lock, [this]
                          { return phase_ == WorkPhase::Terminal; });
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
          completed_.wait(lock, [this]
                          { return phase_ == WorkPhase::Terminal; });
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
                                                          { return Error(errors::worker_exception, std::string(message)); });
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
  /// @endcond

  /// @cond VIX_CONCURRENCY_INTERNAL_IMPLEMENTATION
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
  /// @endcond
} // namespace vix::concurrency

#endif // VIX_CONCURRENCY_EXECUTION_RESOURCE_HPP
