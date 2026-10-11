#pragma once
#include <vix/Completion.hpp>
#include <vix/Cancellation.hpp>
#include <vix/Deadline.hpp>
#include <vix/Result.hpp>
#include <atomic>
#include <coroutine>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>
namespace vix::network::detail
{
  inline constexpr auto resource_exhausted = vix::ErrorIdentity::define<"vix.network">(17);
  inline constexpr auto provider_in_use = vix::ErrorIdentity::define<"vix.network">(20);
inline constexpr auto timed_out = vix::ErrorIdentity::define<"vix.network">(21);
  class Continuations
  {
  public:
    static vix::Result<void> ensure();
    static void enqueue(std::function<void()>);
    // Private test gate: lets abandonment tests hold a claimed queued ticket.
    static void hold_for_test(bool) noexcept;
  };
  enum class ResumeState : std::uint8_t
  {
    queued,
    resuming,
    abandoned
  };
  class ResumeTicket
  {
  public:
    explicit ResumeTicket(std::coroutine_handle<> h) noexcept : handle_(h) {}
    bool claim_resume() noexcept
    {
      auto e = ResumeState::queued;
      return state_.compare_exchange_strong(e, ResumeState::resuming);
    }
    bool abandon() noexcept
    {
      auto e = ResumeState::queued;
      return state_.compare_exchange_strong(e, ResumeState::abandoned);
    }
    void resume() noexcept
    {
      if (handle_)
        handle_.resume();
    }

  private:
    std::atomic<ResumeState> state_{ResumeState::queued};
    std::coroutine_handle<> handle_{};
  };
  // True retains this directional slot after its EPOLLONESHOT notification.
  using ReadinessHandler = std::function<bool(std::uint32_t)>;
  class ProviderCore : public std::enable_shared_from_this<ProviderCore>
  {
  public:
    static vix::Result<std::shared_ptr<ProviderCore>> create();
    ~ProviderCore();
    vix::Result<void> shutdown();
    void add_operation() noexcept;
    void finish_operation() noexcept;
    void add_resource() noexcept;
    void release_resource() noexcept;
    bool post(std::function<void()>);
    std::uint64_t register_descriptor(int, ReadinessHandler, ReadinessHandler);
    std::uint64_t register_descriptor(int, bool, bool);
    bool rearm(std::uint64_t, bool, bool);
    bool release_descriptor_direction(std::uint64_t, bool read);
    void retire(std::uint64_t);
    std::uint64_t add_deadline(vix::Deadline, std::function<void()>);
    bool invalidate_deadline(std::uint64_t);
    [[nodiscard]] bool accepts(std::uint64_t) const noexcept;
    [[nodiscard]] static std::uint64_t encode_token(std::uint32_t, std::uint32_t) noexcept;
    [[nodiscard]] static bool decode_token(std::uint64_t, std::uint32_t &, std::uint32_t &) noexcept;
    // Private-engine test hooks. Dispatch is serialized onto the reactor thread.
    bool dispatch_for_test(std::uint64_t, std::uint32_t);
    bool post_for_test(std::function<void()>);
    void empty_wake_for_test() noexcept;
    [[nodiscard]] std::uint64_t epoll_ctl_count_for_test(int) const noexcept;
    [[nodiscard]] bool epoll_mutations_owned_by_reactor_for_test() const noexcept;
    [[nodiscard]] std::uint64_t commands_executed_for_test() const noexcept;
    [[nodiscard]] std::uint64_t wake_events_for_test() const noexcept;
    [[nodiscard]] std::uint64_t wake_writes_for_test() const noexcept;
    [[nodiscard]] unsigned active_operations_for_test() const noexcept;
    [[nodiscard]] std::size_t live_deadlines_for_test() const noexcept;
    [[nodiscard]] std::size_t live_descriptor_registrations_for_test() const noexcept;
    [[nodiscard]] std::uint64_t idle_generation_for_test() const noexcept;
    [[nodiscard]] bool wait_for_idle_after_for_test(std::uint64_t) const noexcept;
    [[nodiscard]] std::uint64_t maximum_consecutive_zero_waits_for_test() const noexcept;

  private:
    enum class CommandKind : std::uint8_t
    {
      generic,
      descriptor_add,
      descriptor_mod,
      descriptor_del,
      deadline_add,
      deadline_invalidate,
      shutdown
    };
    ProviderCore(int, int) noexcept;
    void run() noexcept;
    bool wake() noexcept;
    void stop_and_join() noexcept;
    bool enqueue(CommandKind, std::function<void()>);
    bool command_and_wait(CommandKind, std::function<void()>) const;
    bool dispatch_on_reactor(std::uint64_t, std::uint32_t);
    struct Impl;
    static std::uint32_t interest(bool, bool);
    std::unique_ptr<Impl> impl_;
  };
  template <class T>
  class OperationRecord : public std::enable_shared_from_this<OperationRecord<T>>
  {
  public:
    enum class TerminalEvent : std::uint8_t { readiness_revoked, deadline_invalidated, slot_released, cancellation_released, borrow_revoked, completion_stored, provider_released, resume_queued };
    enum class Phase : std::uint8_t
    {
      created,
      suspending,
      suspended,
      completed_before_suspend,
      completed_scheduled,
      consumed
    };
    explicit OperationRecord(std::shared_ptr<ProviderCore> p) : provider_(std::move(p)) {}
    bool begin(std::coroutine_handle<> h) noexcept;
    bool commit_suspend() noexcept;
    void complete(vix::Completion<T> c);
    void observe_cancellation(vix::CancellationToken token);
    void observe_deadline(vix::Deadline deadline);
    void publish_native_for_test(vix::Completion<T> completion);
    void publish_cancellation_for_test();
    void publish_deadline_for_test();
    void hold_evaluation_for_test(bool hold) noexcept { evaluation_held_.store(hold); }
    void release_evaluation_for_test();
    void bind_descriptor_direction(std::uint64_t token, bool read) noexcept { descriptor_token_=token; descriptor_read_=read; }
    vix::Completion<T> take_completion();
    void abandon() noexcept;
    [[nodiscard]] Phase phase() const noexcept { return phase_.load(); }
    [[nodiscard]] bool native_access_allowed_for_test() const noexcept { return borrow_allowed_.load(); }
    [[nodiscard]] std::vector<TerminalEvent> terminal_trace_for_test() const { std::lock_guard l(trace_mutex_); return trace_; }

  private:
    std::atomic<Phase> phase_{Phase::created};
    mutable std::mutex mutex_;
    std::optional<vix::Completion<T>> completion_;
    std::coroutine_handle<> handle_{};
    std::shared_ptr<ProviderCore> provider_;
    std::shared_ptr<ResumeTicket> ticket_;
    std::optional<vix::CancellationRegistration> cancellation_;
    std::uint64_t deadline_token_{};
    std::uint64_t descriptor_token_{};
    bool descriptor_read_{};
    std::atomic<bool> borrow_allowed_{true};
    void record_event(TerminalEvent event) { std::lock_guard l(trace_mutex_); trace_.push_back(event); }
    mutable std::mutex trace_mutex_;
    std::vector<TerminalEvent> trace_;
    std::mutex facts_mutex_;
    std::optional<vix::Completion<T>> native_fact_;
    std::atomic<bool> cancellation_fact_{false}, deadline_fact_{false};
    std::atomic<bool> evaluation_held_{false};
    void evaluate_pending_on_reactor();
  };
  template <class T>
  bool OperationRecord<T>::begin(std::coroutine_handle<> h) noexcept
  {
    Phase e = Phase::created;
    if (!phase_.compare_exchange_strong(e, Phase::suspending))
      std::terminate();
    handle_ = h;
    provider_->add_operation();
    return true;
  }
  template <class T>
  bool OperationRecord<T>::commit_suspend() noexcept
  {
    Phase e = Phase::suspending;
    return phase_.compare_exchange_strong(e, Phase::suspended);
  }
  template <class T>
  void OperationRecord<T>::complete(vix::Completion<T> c)
  {
    Phase p = phase_.load();
    for (;;)
    {
      if (p == Phase::completed_before_suspend || p == Phase::completed_scheduled || p == Phase::consumed)
        return;
      if (p == Phase::suspending)
      {
        if (phase_.compare_exchange_weak(p, Phase::completed_before_suspend))
        {
          record_event(TerminalEvent::readiness_revoked);
          if (deadline_token_) { (void)provider_->invalidate_deadline(deadline_token_); deadline_token_=0; }
          record_event(TerminalEvent::deadline_invalidated);
          if (descriptor_token_) (void)provider_->release_descriptor_direction(descriptor_token_,descriptor_read_);
          record_event(TerminalEvent::slot_released);
          cancellation_.reset(); record_event(TerminalEvent::cancellation_released);
          borrow_allowed_=false; record_event(TerminalEvent::borrow_revoked);
          std::lock_guard l(mutex_);
          completion_.emplace(std::move(c));
          record_event(TerminalEvent::completion_stored);
          provider_->finish_operation();
          record_event(TerminalEvent::provider_released);
          provider_.reset();
          return;
        }
        continue;
      }
      if (p == Phase::suspended)
      {
        if (phase_.compare_exchange_weak(p, Phase::completed_scheduled))
        {
          record_event(TerminalEvent::readiness_revoked);
          if (deadline_token_) { (void)provider_->invalidate_deadline(deadline_token_); deadline_token_=0; }
          record_event(TerminalEvent::deadline_invalidated);
          if (descriptor_token_) (void)provider_->release_descriptor_direction(descriptor_token_,descriptor_read_);
          record_event(TerminalEvent::slot_released);
          cancellation_.reset(); record_event(TerminalEvent::cancellation_released);
          borrow_allowed_=false; record_event(TerminalEvent::borrow_revoked);
          {
            std::lock_guard l(mutex_);
            completion_.emplace(std::move(c));
            record_event(TerminalEvent::completion_stored);
            ticket_ = std::make_shared<ResumeTicket>(handle_);
          }
          provider_->finish_operation();
          record_event(TerminalEvent::provider_released);
          provider_.reset();
          auto ticket = ticket_;
          record_event(TerminalEvent::resume_queued);
          Continuations::enqueue([ticket]
                                 {if(ticket->claim_resume())ticket->resume(); });
          return;
        }
        continue;
      }
      return;
    }
  }
  template <class T>
  void OperationRecord<T>::observe_cancellation(vix::CancellationToken token)
  {
    auto weak=this->weak_from_this();
    cancellation_.emplace(token.on_cancel([weak]
    {
      if (auto record=weak.lock())
      {
        // Callback only posts; reactor owns terminalization and never resumes here.
        auto provider=record->provider_;
        if (provider) (void)provider->post([weak]
        { if (auto pending=weak.lock()) { pending->cancellation_fact_=true; pending->evaluate_pending_on_reactor(); } });
      }
    }));
  }
  template <class T>
  void OperationRecord<T>::observe_deadline(vix::Deadline deadline)
  {
    auto weak=this->weak_from_this();
    auto provider=provider_;
    if (provider) deadline_token_=provider->add_deadline(deadline,[weak]
    { if (auto pending=weak.lock()) { pending->deadline_token_=0; pending->deadline_fact_=true; pending->evaluate_pending_on_reactor(); } });
  }
  template <class T> void OperationRecord<T>::evaluate_pending_on_reactor()
  {
    if (evaluation_held_.load()) return;
    std::optional<vix::Completion<T>> native;
    { std::lock_guard l(facts_mutex_); if (native_fact_) native.emplace(std::move(*native_fact_)); }
    // Reactor observation precedence: native result, then cancellation, then reached deadline.
    if (native) { complete(std::move(*native)); return; }
    if (cancellation_fact_.load()) { complete(vix::Completion<T>::cancelled()); return; }
    if (deadline_fact_.load()) complete(vix::Completion<T>{vix::Error{timed_out,"deadline reached"}});
  }
  template <class T> void OperationRecord<T>::publish_native_for_test(vix::Completion<T> completion)
  { { std::lock_guard l(facts_mutex_); native_fact_.emplace(std::move(completion)); } auto weak=this->weak_from_this(); auto provider=provider_; if(provider)(void)provider->post([weak]{if(auto p=weak.lock())p->evaluate_pending_on_reactor();}); }
  template <class T> void OperationRecord<T>::publish_cancellation_for_test()
  { cancellation_fact_=true; auto weak=this->weak_from_this(); auto provider=provider_; if(provider)(void)provider->post([weak]{if(auto p=weak.lock())p->evaluate_pending_on_reactor();}); }
  template <class T> void OperationRecord<T>::publish_deadline_for_test()
  { deadline_fact_=true; auto weak=this->weak_from_this(); auto provider=provider_; if(provider)(void)provider->post([weak]{if(auto p=weak.lock())p->evaluate_pending_on_reactor();}); }
  template <class T> void OperationRecord<T>::release_evaluation_for_test()
  { evaluation_held_=false; auto weak=this->weak_from_this(); auto provider=provider_; if(provider)(void)provider->post([weak]{if(auto p=weak.lock())p->evaluate_pending_on_reactor();}); }
  template <class T>
  vix::Completion<T> OperationRecord<T>::take_completion()
  {
    auto p=phase_.load();
    if (p != Phase::completed_before_suspend && p != Phase::completed_scheduled)
      std::terminate();
    phase_.store(Phase::consumed);
    std::lock_guard l(mutex_);
    if (!completion_)
      std::terminate();
    return std::move(*completion_);
  }
  template <class T>
  void OperationRecord<T>::abandon() noexcept
  {
    std::lock_guard l(mutex_);
    if (ticket_)
      ticket_->abandon();
  }

  // Private Network operation façade.  It is move-only and can only be awaited as an rvalue.
  template <class T>
  class Operation
  {
  public:
    using Starter = std::function<void(std::shared_ptr<OperationRecord<T>>) >;
    Operation(std::shared_ptr<ProviderCore> provider, Starter starter) : record_(std::make_shared<OperationRecord<T>>(std::move(provider))), starter_(std::move(starter)) {}
    Operation(Operation &&) noexcept = default;
    Operation(const Operation &) = delete;
    Operation &operator=(Operation &&) = delete;
    Operation &operator=(const Operation &) = delete;
    struct Awaiter
    {
      std::shared_ptr<OperationRecord<T>> record;
      Starter starter;
      bool await_ready() const noexcept { return false; }
      bool await_suspend(std::coroutine_handle<> h)
      {
        record->begin(h);
        starter(record);
        // Once commit succeeds another thread may resume and destroy this awaiter: do not touch it.
        return record->commit_suspend();
      }
      vix::Completion<T> await_resume() { return record->take_completion(); }
      ~Awaiter()
      {
        if (!record) return;
        const auto phase=record->phase();
        if (phase==OperationRecord<T>::Phase::suspending || phase==OperationRecord<T>::Phase::suspended)
          std::terminate(); // destroying started incomplete work would leave borrowed provider state.
        if (phase==OperationRecord<T>::Phase::completed_scheduled)
          record->abandon();
      }
    };
    Awaiter operator co_await() && { return {std::move(record_),std::move(starter_)}; }
    Awaiter operator co_await() & = delete;
  private:
    std::shared_ptr<OperationRecord<T>> record_;
    Starter starter_;
  };
}
