#pragma once
#include <vix/Completion.hpp>
#include <vix/Cancellation.hpp>
#include <vix/Deadline.hpp>
#include <vix/Result.hpp>
#include <vix/network/Errors.hpp>
#include <vix/network/Operation.hpp>
#include "Reactor.hpp"
#include <atomic>
#include <coroutine>
#include <cstdint>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <vector>
#include <variant>
#include <utility>
namespace vix::network
{
  class Provider;
  class TcpConnection;
}
namespace vix::network::detail
{
  using vix::network::io_failure;
  using vix::network::provider_in_use;
  using vix::network::resource_exhausted;
  using vix::network::timed_out;
  class Continuations
  {
  public:
    static vix::Result<void> ensure();
    static void enqueue(std::function<void()>);
    // Private test gate: lets abandonment tests hold a claimed queued ticket.
    static void hold_for_test(bool) noexcept;
    // Private test hook: fail after starting this many workers during the next
    // initialization attempt. The hook is consumed by that attempt.
    static void fail_worker_creation_after_for_test(unsigned) noexcept;
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
  // True retains this directional slot after its one-shot readiness notification.
  using ReadinessHandler = std::function<bool(std::uint32_t)>;
  class ProviderCore : public std::enable_shared_from_this<ProviderCore>
  {
  public:
    static vix::Result<std::shared_ptr<ProviderCore>> create(bool cooperative = false);
    ~ProviderCore();
    vix::Result<void> shutdown();
    void add_operation() noexcept;
    void finish_operation() noexcept;
    void add_resource() noexcept;
    void release_resource() noexcept;
    bool post(std::function<void()>);
    [[nodiscard]] bool cooperative() const noexcept;
    [[nodiscard]] bool on_owner_thread() const noexcept;
    vix::Result<std::size_t> poll();
    void schedule_resume(std::shared_ptr<ResumeTicket>);
    std::uint64_t register_descriptor(NativeSocket, ReadinessHandler, ReadinessHandler);
    bool set_descriptor_handlers(std::uint64_t, ReadinessHandler, ReadinessHandler);
    std::uint64_t register_descriptor(NativeSocket, bool, bool);
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
    void hold_cooperative_resumes_for_test(bool) noexcept;
    [[nodiscard]] std::uint64_t reactor_control_count_for_test(ReactorControl) const noexcept;
    [[nodiscard]] bool reactor_mutations_owned_by_reactor_for_test() const noexcept;
    [[nodiscard]] std::uint64_t commands_executed_for_test() const noexcept;
    [[nodiscard]] std::uint64_t wake_events_for_test() const noexcept;
    [[nodiscard]] std::uint64_t wake_writes_for_test() const noexcept;
    [[nodiscard]] unsigned active_operations_for_test() const noexcept;
    [[nodiscard]] unsigned active_resources_for_test() const noexcept;
    [[nodiscard]] bool has_dependencies() const noexcept;
    [[nodiscard]] bool running() const noexcept;
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
    explicit ProviderCore(std::unique_ptr<Reactor>, bool cooperative);
    void run() noexcept;
    bool wake() noexcept;
    void stop_and_join() noexcept;
    bool enqueue(CommandKind, std::function<void()>);
    bool command_and_wait(CommandKind, std::function<void()>) const;
    bool dispatch_on_reactor(std::uint64_t, std::uint32_t);
    struct Impl;
    std::unique_ptr<Impl> impl_;
  };
  // Engine-private native ownership.  A descriptor has exactly one terminal
  // disposition: it is transferred to a Resource or released by the losing
  // terminal path.
  class NativeFd
  {
  public:
    enum class State : std::uint8_t
    {
      none,
      owned,
      transferred,
      released
    };
    NativeFd() = default;
    NativeFd(const NativeFd &) = delete;
    NativeFd &operator=(const NativeFd &) = delete;
    NativeFd(NativeFd &&) = delete;
    NativeFd &operator=(NativeFd &&) = delete;
    ~NativeFd()
    {
      if (state_ == State::owned)
        release();
    }
    bool adopt(NativeSocket fd) noexcept
    {
      if (state_ != State::none || fd == invalid_socket)
        return false;
      fd_ = fd;
      state_ = State::owned;
      return true;
    }
    NativeSocket transfer() noexcept
    {
      if (state_ != State::owned)
        std::terminate();
      state_ = State::transferred;
      return std::exchange(fd_, invalid_socket);
    }
    void release() noexcept
    {
      if (state_ != State::owned)
        std::terminate();
      const NativeSocket fd = std::exchange(fd_, invalid_socket);
      state_ = State::released;
      ++close_calls_;
      (void)close_native_socket(fd);
    }
    [[nodiscard]] State state() const noexcept { return state_; }
    [[nodiscard]] unsigned close_calls() const noexcept { return close_calls_; }

  private:
    NativeSocket fd_{invalid_socket};
    State state_{State::none};
    unsigned close_calls_{};
  };
  class ProviderResourceLease
  {
  public:
    explicit ProviderResourceLease(std::shared_ptr<ProviderCore> provider) noexcept : provider_(std::move(provider))
    {
      if (provider_)
        provider_->add_resource();
    }
    ProviderResourceLease(ProviderResourceLease &&other) noexcept : provider_(std::move(other.provider_)) {}
    ProviderResourceLease &operator=(ProviderResourceLease &&) = delete;
    ProviderResourceLease(const ProviderResourceLease &) = delete;
    ProviderResourceLease &operator=(const ProviderResourceLease &) = delete;
    ~ProviderResourceLease()
    {
      if (provider_)
        provider_->release_resource();
    }
    [[nodiscard]] std::shared_ptr<ProviderCore> provider() const noexcept { return provider_; }

  private:
    std::shared_ptr<ProviderCore> provider_;
  };
  class TcpConnectionState;
  template <class T>
  class OperationRecord;
  class TcpTransportLease
  {
  public:
    TcpTransportLease() noexcept = default;
    TcpTransportLease(TcpTransportLease &&) noexcept;
    TcpTransportLease(const TcpTransportLease &) = delete;
    TcpTransportLease &operator=(TcpTransportLease &&) = delete;
    TcpTransportLease &operator=(const TcpTransportLease &) = delete;
    ~TcpTransportLease();

    // Private Network operations authorized by this lease. This class lives
    // only in the non-installed engine header, not in the public API.
    vix::network::Operation<std::size_t> receive(std::span<std::byte>, OperationOptions = {});
    vix::network::Operation<void> send(std::span<const std::byte>, OperationOptions = {});
    vix::Result<void> shutdown_write() noexcept;
    // Permanently close a claimed transport. Network defers descriptor close
    // until active child operations have retired.
    void close() noexcept;

  private:
    explicit TcpTransportLease(std::shared_ptr<TcpConnectionState>) noexcept;
    std::shared_ptr<TcpConnectionState> state_;
    friend class NetworkAccess;
  };
  class NetworkAccess
  {
  public:
    static TcpConnection adopt_tcp_for_test(Provider &, int);
    static std::shared_ptr<ProviderCore> core_for_test(const Provider &);
    static int native_fd_for_test(const TcpConnection &) noexcept;
    struct TransportCountersForTest
    {
      std::uint64_t read_progressions{};
      std::uint64_t write_progressions{};
      std::uint64_t native_recv_attempts{};
      std::uint64_t native_send_attempts{};
      std::uint64_t read_buffer_accesses{};
      std::uint64_t write_buffer_accesses{};
      std::uint64_t read_slot_releases{};
      std::uint64_t write_slot_releases{};
      std::size_t write_offset{};
      std::size_t write_offset_at_terminal_release{};
    };
    static void hold_transport_progression_for_test(const TcpConnection &, bool) noexcept;
    static void fail_descriptor_arm_for_test(const TcpConnection &, bool) noexcept;
    static bool post_read_progression_for_test(const TcpConnection &);
    static bool post_write_progression_for_test(const TcpConnection &);
    static void publish_receive_for_test(const TcpConnection &, vix::Completion<std::size_t>);
    static void publish_send_for_test(const TcpConnection &, vix::Completion<void>);
    static void capture_next_receive_record_for_test(const TcpConnection &, std::shared_ptr<OperationRecord<std::size_t>> &);
    static void capture_next_send_record_for_test(const TcpConnection &, std::shared_ptr<OperationRecord<void>> &);
    static TransportCountersForTest transport_counters_for_test(const TcpConnection &);
    static TransportCountersForTest transport_counters_for_test(const TcpTransportLease &);
    // Private TLS integration: returns the provider retained by an exclusive
    // transport lease without exposing its native descriptor.
    static std::shared_ptr<ProviderCore> provider_for_tls_lease(const TcpTransportLease &) noexcept;
    static TcpConnection from_handoff(NativeSocket, ProviderResourceLease);
    // Experimental, engine-private ownership transfer. Engine.hpp is not an
    // installed header; ordinary SDK consumers cannot access this entrypoint.
    static vix::Result<TcpTransportLease> claim_tcp_for_tls(TcpConnection &&);
#ifdef VIX_NETWORK_TEST_CAPTURE
    static void set_before_native_receive_hook_for_test(const TcpConnection &,
                                                         void (*)(void *) noexcept, void *) noexcept;
#endif
  };
  // Compatibility spelling for private focused tests only.  It is not in any
  // installed public header.
  using CandidateAccess = NetworkAccess;
  TcpConnection make_tcp_connection(NativeSocket, ProviderResourceLease);
  // Private focused-test bridge; never installed.
  vix::Error transport_error_for_test(int) noexcept;
  // Temporary private test Resource; this is deliberately not a public TCP
  // type.  It owns the transferred descriptor and the promoted Provider lease.
  class SyntheticResource
  {
  public:
    SyntheticResource(NativeSocket fd, ProviderResourceLease lease) noexcept : fd_(fd), lease_(std::move(lease)) {}
    SyntheticResource(SyntheticResource &&other) noexcept : fd_(std::exchange(other.fd_, invalid_socket)), lease_(std::move(other.lease_)) {}
    SyntheticResource &operator=(SyntheticResource &&) = delete;
    SyntheticResource(const SyntheticResource &) = delete;
    SyntheticResource &operator=(const SyntheticResource &) = delete;
    ~SyntheticResource()
    {
      if (fd_ != invalid_socket)
      {
        ++close_calls_;
        (void)close_native_socket(fd_);
      }
    }
    [[nodiscard]] int native_fd_for_test() const noexcept { return static_cast<int>(fd_); }
    [[nodiscard]] unsigned close_calls_for_test() const noexcept { return close_calls_; }

  private:
    NativeSocket fd_{invalid_socket};
    ProviderResourceLease lease_{std::shared_ptr<ProviderCore>{}};
    unsigned close_calls_{};
  };
  template <class T>
  class OperationRecord : public OperationState<T>, public std::enable_shared_from_this<OperationRecord<T>>
  {
  public:
    enum class TerminalEvent : std::uint8_t
    {
      readiness_revoked,
      deadline_invalidated,
      slot_released,
      cancellation_released,
      borrow_revoked,
      completion_stored,
      provider_released,
      resume_queued
    };
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
    bool begin(std::coroutine_handle<> h) noexcept override;
    bool commit_suspend() noexcept override;
    void complete(vix::Completion<T> c);
    void observe_cancellation(vix::CancellationToken token);
    void observe_deadline(vix::Deadline deadline);
    void publish_native_for_test(vix::Completion<T> completion);
    void publish_cancellation_for_test();
    void publish_deadline_for_test();
    bool adopt_native_for_test(NativeSocket fd) noexcept { return native_.adopt(fd); }
    bool adopt_native_for_connect(NativeSocket fd) noexcept { return native_.adopt(fd); }
    [[nodiscard]] NativeFd::State native_state_for_test() const noexcept { return native_.state(); }
    [[nodiscard]] unsigned native_close_calls_for_test() const noexcept { return native_.close_calls(); }
    template <class Producer>
    bool try_complete_success_with(Producer &&producer);
    template <class Producer>
    void publish_native_success_with(Producer &&producer);
    void hold_evaluation_for_test(bool hold) noexcept { evaluation_held_.store(hold); }
    void release_evaluation_for_test();
    void bind_descriptor_direction(std::uint64_t token, bool read) noexcept
    {
      descriptor_token_ = token;
      descriptor_read_ = read;
    }
    void set_terminal_cleanup(std::function<void()> cleanup) { terminal_cleanup_ = std::move(cleanup); }
    vix::Completion<T> take_completion() override;
    void abandon() noexcept override;
    [[nodiscard]] bool incomplete() const noexcept override
    {
      const auto p = phase_.load();
      return p == Phase::suspending || p == Phase::suspended;
    }
    [[nodiscard]] bool scheduled() const noexcept override { return phase_.load() == Phase::completed_scheduled; }
    [[nodiscard]] Phase phase() const noexcept { return phase_.load(); }
    [[nodiscard]] bool native_access_allowed_for_test() const noexcept { return borrow_allowed_.load(); }
    [[nodiscard]] std::vector<TerminalEvent> terminal_trace_for_test() const
    {
      std::lock_guard l(trace_mutex_);
      return trace_;
    }

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
    NativeFd native_;
    std::function<void()> terminal_cleanup_;
    void record_event(TerminalEvent event)
    {
      std::lock_guard l(trace_mutex_);
      trace_.push_back(event);
    }
    mutable std::mutex trace_mutex_;
    std::vector<TerminalEvent> trace_;
    std::mutex facts_mutex_;
    std::optional<vix::Completion<T>> native_fact_;
    std::conditional_t<std::is_void_v<T>, std::monostate, std::optional<std::function<T(NativeSocket, ProviderResourceLease)>>> native_success_builder_;
    std::atomic<bool> cancellation_fact_{false}, deadline_fact_{false};
    std::atomic<bool> evaluation_held_{false};
    bool claim_terminal(bool &) noexcept;
    template <class Build>
    void finish_terminal_winner(bool, Build &&);
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
  bool OperationRecord<T>::claim_terminal(bool &queue) noexcept
  {
    Phase p = phase_.load();
    for (;;)
    {
      if (p == Phase::completed_before_suspend || p == Phase::completed_scheduled || p == Phase::consumed)
        return false;
      if (p == Phase::suspending)
      {
        if (phase_.compare_exchange_weak(p, Phase::completed_before_suspend))
        {
          queue = false;
          return true;
        }
        continue;
      }
      if (p == Phase::suspended)
      {
        if (phase_.compare_exchange_weak(p, Phase::completed_scheduled))
        {
          queue = true;
          return true;
        }
        continue;
      }
      return false;
    }
  }
  template <class T>
  template <class Build>
  void OperationRecord<T>::finish_terminal_winner(bool queue, Build &&build)
  {
    // The phase CAS in claim_terminal() has already selected the only winner.
    // All ordinary terminal paths share this cleanup/storage sequence.
    record_event(TerminalEvent::readiness_revoked);
    if (deadline_token_)
    {
      (void)provider_->invalidate_deadline(deadline_token_);
      deadline_token_ = 0;
    }
    record_event(TerminalEvent::deadline_invalidated);
    if (descriptor_token_)
      (void)provider_->release_descriptor_direction(descriptor_token_, descriptor_read_);
    record_event(TerminalEvent::slot_released);
    if (terminal_cleanup_)
    {
      auto cleanup = std::move(terminal_cleanup_);
      cleanup();
    }
    cancellation_.reset();
    record_event(TerminalEvent::cancellation_released);
    borrow_allowed_ = false;
    record_event(TerminalEvent::borrow_revoked);
    std::shared_ptr<ResumeTicket> ticket;
    {
      std::lock_guard l(mutex_);
      completion_.emplace(build());
      record_event(TerminalEvent::completion_stored);
      if (queue)
        ticket = ticket_ = std::make_shared<ResumeTicket>(handle_);
    }
    provider_->finish_operation();
    record_event(TerminalEvent::provider_released);
    auto provider = provider_;
    provider_.reset();
    if (queue)
    {
      record_event(TerminalEvent::resume_queued);
      provider->schedule_resume(std::move(ticket));
    }
  }
  template <class T>
  void OperationRecord<T>::complete(vix::Completion<T> c)
  {
    bool queue{};
    if (!claim_terminal(queue))
      return;
    finish_terminal_winner(queue, [this, completion = std::move(c)]() mutable
                           { if (native_.state()==NativeFd::State::owned) native_.release(); return std::move(completion); });
  }
  template <class T>
  template <class Producer>
  bool OperationRecord<T>::try_complete_success_with(Producer &&producer)
  {
    bool queue{};
    if (!claim_terminal(queue))
      return false;
    finish_terminal_winner(queue, [this, producer = std::forward<Producer>(producer)]() mutable
                           {
      // Promotion precedes operation release, so there is no Provider
      // dependency gap while constructing the successful Resource.
      ProviderResourceLease lease{provider_};
      return producer(native_.transfer(), std::move(lease)); });
    return true;
  }
  template <class T>
  template <class Producer>
  void OperationRecord<T>::publish_native_success_with(Producer &&producer)
  {
    {
      std::lock_guard l(facts_mutex_);
      native_success_builder_.emplace(std::forward<Producer>(producer));
    }
    auto weak = this->weak_from_this();
    auto provider = provider_;
    if (provider)
      (void)provider->post([weak]
                           {if(auto p=weak.lock())p->evaluate_pending_on_reactor(); });
  }
  template <class T>
  void OperationRecord<T>::observe_cancellation(vix::CancellationToken token)
  {
    auto weak = this->weak_from_this();
    auto provider = provider_;
    cancellation_.emplace(token.on_cancel([weak, provider = std::move(provider)]
                                          {
      if (auto record=weak.lock())
      {
        // Callback only posts; reactor owns terminalization and never resumes here.
        if (provider) (void)provider->post([weak]
        { if (auto pending=weak.lock()) { pending->cancellation_fact_=true; pending->evaluate_pending_on_reactor(); } });
      } }));
  }
  template <class T>
  void OperationRecord<T>::observe_deadline(vix::Deadline deadline)
  {
    auto weak = this->weak_from_this();
    auto provider = provider_;
    if (provider)
      deadline_token_ = provider->add_deadline(deadline, [weak]
                                               { if (auto pending=weak.lock()) { pending->deadline_token_=0; pending->deadline_fact_=true; pending->evaluate_pending_on_reactor(); } });
  }
  template <class T>
  void OperationRecord<T>::evaluate_pending_on_reactor()
  {
    if (evaluation_held_.load())
      return;
    std::optional<vix::Completion<T>> native;
    if constexpr (!std::is_void_v<T>)
    {
      std::optional<std::function<T(NativeSocket, ProviderResourceLease)>> native_success;
      {
        std::lock_guard l(facts_mutex_);
        if (native_fact_)
          native.emplace(std::move(*native_fact_));
        else if (native_success_builder_)
          native_success.emplace(std::move(*native_success_builder_));
      }
      if (native)
      {
        complete(std::move(*native));
        return;
      }
      if (native_success)
      {
        (void)try_complete_success_with(std::move(*native_success));
        return;
      }
    }
    else
    {
      std::lock_guard l(facts_mutex_);
      if (native_fact_)
        native.emplace(std::move(*native_fact_));
    }
    // Reactor observation precedence: native result, then cancellation, then reached deadline.
    if (native)
    {
      complete(std::move(*native));
      return;
    }
    if (cancellation_fact_.load())
    {
      complete(vix::Completion<T>::cancelled());
      return;
    }
    if (deadline_fact_.load())
      complete(vix::Completion<T>{vix::Error{timed_out, "deadline reached"}});
  }
  template <class T>
  void OperationRecord<T>::publish_native_for_test(vix::Completion<T> completion)
  {
    {
      std::lock_guard l(facts_mutex_);
      native_fact_.emplace(std::move(completion));
    }
    auto weak = this->weak_from_this();
    auto provider = provider_;
    if (provider)
      (void)provider->post([weak]
                           {if(auto p=weak.lock())p->evaluate_pending_on_reactor(); });
  }
  template <class T>
  void OperationRecord<T>::publish_cancellation_for_test()
  {
    cancellation_fact_ = true;
    auto weak = this->weak_from_this();
    auto provider = provider_;
    if (provider)
      (void)provider->post([weak]
                           {if(auto p=weak.lock())p->evaluate_pending_on_reactor(); });
  }
  template <class T>
  void OperationRecord<T>::publish_deadline_for_test()
  {
    deadline_fact_ = true;
    auto weak = this->weak_from_this();
    auto provider = provider_;
    if (provider)
      (void)provider->post([weak]
                           {if(auto p=weak.lock())p->evaluate_pending_on_reactor(); });
  }
  template <class T>
  void OperationRecord<T>::release_evaluation_for_test()
  {
    evaluation_held_ = false;
    auto weak = this->weak_from_this();
    auto provider = provider_;
    if (provider)
      (void)provider->post([weak]
                           {if(auto p=weak.lock())p->evaluate_pending_on_reactor(); });
  }
  template <class T>
  vix::Completion<T> OperationRecord<T>::take_completion()
  {
    auto p = phase_.load();
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

  template <class T>
  class OperationFactory
  {
  public:
    using RecordStarter = std::function<void(std::shared_ptr<OperationRecord<T>>)>;
    static vix::network::Operation<T> make(std::shared_ptr<ProviderCore> provider, RecordStarter starter)
    {
      auto record = std::make_shared<OperationRecord<T>>(std::move(provider));
      auto state = std::static_pointer_cast<OperationState<T>>(record);
      return vix::network::Operation<T>{std::move(state), [record = std::move(record), starter = std::move(starter)](std::shared_ptr<OperationState<T>>)
                                        { starter(record); }};
    }
    static vix::network::Operation<T> make_with_capture(std::shared_ptr<ProviderCore> provider, RecordStarter starter, std::function<void(std::shared_ptr<OperationRecord<T>>)> capture)
    {
      auto record = std::make_shared<OperationRecord<T>>(std::move(provider));
      if (capture)
        capture(record);
      auto state = std::static_pointer_cast<OperationState<T>>(record);
      return vix::network::Operation<T>{std::move(state), [record = std::move(record), starter = std::move(starter)](std::shared_ptr<OperationState<T>>)
                                        { starter(record); }};
    }
  };
  // Private Network operation façade.  It is move-only and can only be awaited as an rvalue.
  template <class T>
  class Operation
  {
  public:
    using Starter = std::function<void(std::shared_ptr<OperationRecord<T>>)>;
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
        if (!record)
          return;
        const auto phase = record->phase();
        if (phase == OperationRecord<T>::Phase::suspending || phase == OperationRecord<T>::Phase::suspended)
          std::terminate(); // destroying started incomplete work would leave borrowed provider state.
        if (phase == OperationRecord<T>::Phase::completed_scheduled)
          record->abandon();
      }
    };
    Awaiter operator co_await() && { return {std::move(record_), std::move(starter_)}; }
    Awaiter operator co_await() & = delete;

  private:
    std::shared_ptr<OperationRecord<T>> record_;
    Starter starter_;
  };
}
