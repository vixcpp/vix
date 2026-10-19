#include "Engine.hpp"
#if !defined(__wasi__)
#include <condition_variable>
#include <chrono>
#endif
#include <deque>
#include <limits>
#include <queue>
#include <stdexcept>
#if !defined(__wasi__)
#include <thread>
#endif
#include <unordered_map>
namespace vix::network::detail
{
  namespace
  {
    thread_local bool on_continuation_worker = false;
#if !defined(__wasi__)
    struct Pool
    {
      std::mutex m;
      std::condition_variable cv;
      std::deque<std::function<void()>> q;
      std::vector<std::thread> workers;
      bool started{};
      bool stopping{};
      bool held{};
      unsigned fail_after{std::numeric_limits<unsigned>::max()};

      ~Pool()
      {
        {
          std::lock_guard lock(m);
          stopping = true;
          held = false;
        }
        cv.notify_all();
        for (auto &thread : workers)
          if (thread.joinable())
            thread.join();
      }
    };
    Pool &pool()
    {
      static Pool p;
      return p;
    }
    void worker()
    {
      on_continuation_worker = true;
      auto &p = pool();
      for (;;)
      {
        std::function<void()> f;
        {
          std::unique_lock l(p.m);
          p.cv.wait(l, [&] { return p.stopping || !p.q.empty(); });
          if (p.q.empty() && p.stopping)
            return;
          f = std::move(p.q.front());
          p.q.pop_front();
          p.cv.wait(l, [&] { return p.stopping || !p.held; });
        }
        f();
      }
    }
#endif
  }
  bool Continuations::on_worker_thread() noexcept { return on_continuation_worker; }
  vix::Result<void> Continuations::ensure()
  {
#if defined(__wasi__)
    return vix::Error{unsupported, "background continuation workers are unavailable on WASI; use a cooperative provider"};
#else
    auto &p = pool();
    std::unique_lock lock(p.m);
    if (p.started)
      return vix::Result<void>::success();
    if (p.stopping)
      return vix::Error{resource_exhausted, "continuation workers are stopping"};

    const auto fail_after = std::exchange(p.fail_after, std::numeric_limits<unsigned>::max());
    try
    {
      p.workers.reserve(2);
      for (unsigned started = 0; started < 2; ++started)
      {
        if (started == fail_after)
          throw std::runtime_error("injected continuation worker startup failure");
        p.workers.emplace_back(worker);
      }
    }
    catch (...)
    {
      p.stopping = true;
      p.held = false;
      lock.unlock();
      p.cv.notify_all();
      for (auto &thread : p.workers)
        if (thread.joinable())
          thread.join();
      lock.lock();
      p.workers.clear();
      p.stopping = false;
      return vix::Error{resource_exhausted, "cannot start continuation workers"};
    }
    p.started = true;
    return vix::Result<void>::success();
#endif
  }
  void Continuations::enqueue(std::function<void()> f)
  {
#if defined(__wasi__)
    (void)f;
    std::terminate();
#else
    auto &p = pool();
    {
      std::lock_guard l(p.m);
      if (!p.started || p.stopping)
        std::terminate();
      p.q.push_back(std::move(f));
    }
    p.cv.notify_one();
#endif
  }
  void Continuations::hold_for_test(bool hold) noexcept
  {
#if !defined(__wasi__)
    auto &p = pool();
    {
      std::lock_guard l(p.m);
      p.held = hold;
    }
    if (!hold)
      p.cv.notify_all();
#else
    (void)hold;
#endif
  }
  void Continuations::fail_worker_creation_after_for_test(unsigned count) noexcept
  {
#if !defined(__wasi__)
    auto &p = pool();
    std::lock_guard lock(p.m);
    if (!p.started && !p.stopping)
      p.fail_after = count;
#else
    (void)count;
#endif
  }
  struct ProviderCore::Impl
  {
    struct DeadlineEntry
    {
      vix::Deadline boundary;
      std::uint32_t timer_id, generation;
      std::function<void()> callback;
    };
    struct DeadlineLater
    {
      bool operator()(const DeadlineEntry &a, const DeadlineEntry &b) const noexcept
      {
        if (a.boundary.boundary() != b.boundary.boundary())
          return a.boundary.boundary() > b.boundary.boundary();
        return a.timer_id > b.timer_id;
      }
    };
    struct DeadlineControl
    {
      std::uint32_t generation;
      bool active;
    };
    struct DescriptorEntry
    {
      std::uint32_t registration_id, generation;
      NativeSocket native_socket;
      ReadinessHandler read_slot, write_slot;
      std::uint64_t read_revision{1}, write_revision{1};
      bool active{true};
    };
    struct Command
    {
      CommandKind kind;
      std::uint64_t sequence;
      std::function<void()> action;
    };
    std::unique_ptr<Reactor> reactor;
#if !defined(__wasi__)
    std::thread thread;
#endif
    std::atomic<bool> running{true}, accepting{true};
    std::mutex queue_m;
    std::deque<Command> commands;
    std::mutex resume_m;
    std::vector<std::shared_ptr<ResumeTicket>> cooperative_resumes;
    std::atomic<bool> hold_cooperative_resumes_for_test{};
    std::atomic<std::uint64_t> next_sequence{1}, executed{};
#if !defined(__wasi__)
    std::mutex startup_m;
    std::condition_variable startup_cv;
    bool startup_finished{}, startup_success{};
#endif
    std::atomic<unsigned> ops{}, resources{};
    std::uint32_t next_registration{1}, next_generation{1};
    std::unordered_map<std::uint32_t, DescriptorEntry> entries;
    std::uint32_t next_timer{1}, next_timer_generation{1};
    std::priority_queue<DeadlineEntry, std::vector<DeadlineEntry>, DeadlineLater> deadlines;
    std::unordered_map<std::uint32_t, DeadlineControl> deadline_controls;
    std::atomic<std::uint64_t> idle_generation{}, consecutive_zero_waits{}, maximum_consecutive_zero_waits{};
#if !defined(__wasi__)
    mutable std::mutex idle_m;
    std::condition_variable idle_cv;
#endif
    std::atomic<bool> owner_only{true};
    bool cooperative{};
    bool driving{};
#if !defined(__wasi__)
    std::thread::id owner{};
#endif
  };
  ProviderCore::ProviderCore(std::unique_ptr<Reactor> reactor, bool cooperative) : impl_(std::make_unique<Impl>())
  {
    impl_->reactor = std::move(reactor);
    impl_->cooperative = cooperative;
  }
  std::uint64_t ProviderCore::encode_token(std::uint32_t i, std::uint32_t g) noexcept { return (std::uint64_t{i} << 32) | g; }
  bool ProviderCore::decode_token(std::uint64_t t, std::uint32_t &i, std::uint32_t &g) noexcept
  {
    constexpr auto component_mask = (std::uint64_t{1} << 32) - 1;
    i = static_cast<std::uint32_t>((t >> 32) & component_mask);
    g = static_cast<std::uint32_t>(t & component_mask);
    return i && g;
  }
  vix::Result<std::shared_ptr<ProviderCore>> ProviderCore::create(bool cooperative)
  {
#if defined(__wasi__)
    if (!cooperative)
      return vix::Error{unsupported, "background-driven Network providers are unavailable on WASI; use create_cooperative()"};
#else
    if (!cooperative)
    {
      auto infra = Continuations::ensure();
      if (!infra)
        return infra.error();
    }
#endif
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
    auto native_reactor = [&]() -> vix::Result<std::unique_ptr<Reactor>>
    {
      try { return Reactor::create(); }
      catch (...) { return vix::Error{resource_exhausted, "cannot allocate network reactor state"}; }
    }();
#else
    auto native_reactor = Reactor::create();
#endif
    if (!native_reactor)
      return native_reactor.error();
    std::shared_ptr<ProviderCore> p;
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
    try { p = std::shared_ptr<ProviderCore>(new ProviderCore(native_reactor.take(), cooperative)); }
    catch (...) { return vix::Error{resource_exhausted, "cannot allocate network provider state"}; }
#else
    p = std::shared_ptr<ProviderCore>(new ProviderCore(native_reactor.take(), cooperative));
#endif
    if (cooperative)
    {
#if !defined(__wasi__)
      p->impl_->owner = std::this_thread::get_id();
#endif
      return p;
    }
#if !defined(__wasi__)
    try
    {
      p->impl_->thread = std::thread([core = p.get()] { core->run(); });
    }
    catch (...)
    {
      return vix::Error{resource_exhausted, "cannot start reactor"};
    }
    {
      std::unique_lock l(p->impl_->startup_m);
      p->impl_->startup_cv.wait(l, [&]
                                { return p->impl_->startup_finished; });
    }
    if (!p->impl_->startup_success)
    {
      if (p->impl_->thread.joinable())
        p->impl_->thread.join();
      return vix::Error{resource_exhausted, "cannot register reactor wake"};
    }
    return p;
#else
    return vix::Error{unsupported, "background-driven Network providers are unavailable on WASI; use create_cooperative()"};
#endif
  }
  ProviderCore::~ProviderCore()
  {
    if (impl_ && impl_->running)
    {
      if (impl_->ops || impl_->resources)
        std::terminate();
      stop_and_join();
    }
  }
  // MPSC publication is protected by queue_m. Background providers require
  // native notification; a cooperative WASI reactor intentionally has no
  // consumer thread and drains this queue on the caller's next poll().
  bool ProviderCore::wake() noexcept
  {
    if (!impl_->reactor)
      return false;
    // Commands may contain stack-bound completion rendezvous. Publishing one
    // without guaranteeing reactor notification would let its caller return
    // while that queued closure still references dead stack state.
    if (!impl_->reactor->wake())
      std::terminate();
    return true;
  }
  bool ProviderCore::enqueue(CommandKind kind, std::function<void()> f)
  {
    if (!f)
      return false;
    {
      std::lock_guard l(impl_->queue_m);
      if (!impl_->accepting.load())
        return false;
      impl_->commands.push_back({kind, impl_->next_sequence++, std::move(f)});
    }
    return wake();
  }
  bool ProviderCore::post(std::function<void()> f) { return enqueue(CommandKind::generic, std::move(f)); }
  bool ProviderCore::cooperative() const noexcept { return impl_->cooperative; }
  bool ProviderCore::on_owner_thread() const noexcept
  {
#if defined(__wasi__)
    return true; // WASI provider execution is single-threaded and caller-driven.
#else
    return std::this_thread::get_id() == impl_->owner;
#endif
  }
  void ProviderCore::schedule_resume(std::shared_ptr<ResumeTicket> ticket)
  {
    if (!ticket)
      return;
    if (!impl_->cooperative)
    {
      Continuations::enqueue([ticket = std::move(ticket)]
                             { if (ticket->claim_resume()) ticket->resume(); });
      return;
    }
    std::lock_guard lock(impl_->resume_m);
    impl_->cooperative_resumes.push_back(std::move(ticket));
  }
  bool ProviderCore::post_for_test(std::function<void()> f) { return post(std::move(f)); }
  void ProviderCore::empty_wake_for_test() noexcept { (void)wake(); }
  void ProviderCore::hold_cooperative_resumes_for_test(bool hold) noexcept { impl_->hold_cooperative_resumes_for_test = hold; }
  bool ProviderCore::command_and_wait(CommandKind kind, std::function<void()> f) const
  {
    if (on_owner_thread())
    {
      f();
      return true;
    }
    // A cooperative provider has no consumer thread to service a synchronous
    // rendezvous. Its mutable reactor state is confined to the creating thread.
    if (impl_->cooperative)
      return false;
#if defined(__wasi__)
    (void)kind;
    (void)f;
    return false;
#else
    std::mutex m;
    std::condition_variable cv;
    bool done = false;
    bool accepted = const_cast<ProviderCore *>(this)->enqueue(kind, [&]
                                                              {f();{std::lock_guard l(m);done=true;}cv.notify_one(); });
    if (!accepted)
      return false;
    std::unique_lock l(m);
    cv.wait(l, [&]
            { return done; });
    return true;
#endif
  }
  vix::Result<std::size_t> ProviderCore::poll()
  {
    return poll_impl(false);
  }
  vix::Result<std::size_t> ProviderCore::run_one()
  {
    if (!impl_->cooperative)
      return vix::Error{unsupported, "blocking progress turns require a cooperative provider"};
    return poll_impl(true);
  }
  bool ProviderCore::may_run_here() const noexcept
  {
    if (!impl_->running)
      return false;
    if (impl_->cooperative)
      return on_owner_thread() && !impl_->driving;
    return !on_owner_thread() && !Continuations::on_worker_thread();
  }
  vix::Result<std::size_t> ProviderCore::poll_impl(bool wait_for_event)
  {
    if (!impl_->cooperative)
      return vix::Error{unsupported, "poll is available only for cooperative providers"};
    if (!on_owner_thread())
      return vix::Error{operation_in_progress, "cooperative provider is confined to its creating thread"};
    if (!impl_->running)
      return vix::Error{provider_stopped, "provider is stopped"};
    if (impl_->driving)
      return vix::Error{operation_in_progress, "cooperative provider progress is not reentrant"};
    impl_->driving = true;
    struct DriveGuard
    {
      bool &active;
      ~DriveGuard() { active = false; }
    } guard{impl_->driving};

    std::size_t progress{};
    auto drain_commands = [&]
    {
      for (;;)
      {
        std::deque<Impl::Command> batch;
        {
          std::lock_guard lock(impl_->queue_m);
          batch.swap(impl_->commands);
        }
        if (batch.empty())
          return;
        for (auto &command : batch)
        {
          command.action();
          ++impl_->executed;
          ++progress;
          if (command.kind == CommandKind::shutdown)
          {
            impl_->running = false;
            return;
          }
        }
      }
    };
    auto live_head = [&]()
    {
      while (!impl_->deadlines.empty())
      {
        const auto &entry = impl_->deadlines.top();
        auto it = impl_->deadline_controls.find(entry.timer_id);
        if (it != impl_->deadline_controls.end() && it->second.active && it->second.generation == entry.generation)
          return true;
        impl_->deadlines.pop();
      }
      return false;
    };
    auto expire = [&]()
    {
      const auto now = vix::MonotonicClock::now();
      while (live_head() && impl_->deadlines.top().boundary.reached(now))
      {
        auto entry = std::move(const_cast<Impl::DeadlineEntry &>(impl_->deadlines.top()));
        impl_->deadlines.pop();
        auto it = impl_->deadline_controls.find(entry.timer_id);
        if (it == impl_->deadline_controls.end() || !it->second.active || it->second.generation != entry.generation)
          continue;
        it->second.active = false;
        impl_->deadline_controls.erase(it);
        entry.callback();
        ++progress;
      }
    };

    // Drain queued work before polling so a same-thread operation started just
    // before poll() can arm its socket in this progress turn.
    drain_commands();
    expire();
    if (impl_->running)
    {
      ReactorEvent events[16];
      int timeout = 0;
      if (wait_for_event && progress == 0)
      {
        bool commands_pending{};
        {
          std::lock_guard lock(impl_->queue_m);
          commands_pending = !impl_->commands.empty();
        }
        bool resumes_pending{};
        {
          std::lock_guard lock(impl_->resume_m);
          resumes_pending = !impl_->cooperative_resumes.empty();
        }
        if (!commands_pending && !resumes_pending)
        {
          if (!live_head())
            timeout = -1;
          else
          {
            const auto remaining = impl_->deadlines.top().boundary
                                       .remaining(vix::MonotonicClock::now())
                                       .nanosecond_count();
            if (remaining <= 0)
              timeout = 0;
            else
            {
              const auto milliseconds = remaining / 1000000LL +
                                        (remaining % 1000000LL != 0);
              timeout = milliseconds > 2147483647LL
                            ? 2147483647
                            : static_cast<int>(milliseconds);
            }
          }
        }
      }
      const int count = impl_->reactor->wait(events, 16, timeout);
      if (count < 0)
        return vix::Error{io_failure, "cooperative reactor poll failed"};
      bool woke = false;
      for (int i = 0; i < count; ++i)
        if (events[i].token == 0)
          woke = true;
      if (woke)
        drain_commands();
      expire();
      for (int i = 0; i < count && impl_->running; ++i)
        if (events[i].token != 0 && dispatch_on_reactor(events[i].token, events[i].events))
          ++progress;
      drain_commands();
      expire();
    }

    // Resumption is deliberately last: readiness handlers and terminal
    // cleanup must finish before user coroutine code can re-enter the provider.
    if (impl_->hold_cooperative_resumes_for_test.load())
      return progress;
    std::vector<std::shared_ptr<ResumeTicket>> resumes;
    {
      std::lock_guard lock(impl_->resume_m);
      resumes.swap(impl_->cooperative_resumes);
    }
    for (auto &ticket : resumes)
    {
      if (ticket && ticket->claim_resume())
      {
        ++progress;
        ticket->resume();
      }
    }
    return progress;
  }
  void ProviderCore::run() noexcept
  {
#if !defined(__wasi__)
    impl_->owner = std::this_thread::get_id();
    {
      std::lock_guard l(impl_->startup_m);
      impl_->startup_success = true;
      impl_->startup_finished = true;
    }
    impl_->startup_cv.notify_one();
    auto drain_commands = [&]
    {for(;;){std::deque<Impl::Command>batch;{std::lock_guard l(impl_->queue_m);batch.swap(impl_->commands);}if(batch.empty())return;for(auto&command:batch){command.action();++impl_->executed;if(command.kind==CommandKind::shutdown){impl_->running=false;return;}}} };
    auto live_head = [&]()
    { while (!impl_->deadlines.empty()) { const auto &entry=impl_->deadlines.top(); auto it=impl_->deadline_controls.find(entry.timer_id); if (it!=impl_->deadline_controls.end() && it->second.active && it->second.generation==entry.generation) return true; impl_->deadlines.pop(); } return false; };
    auto expire = [&]()
    { const auto now=vix::MonotonicClock::now(); while (live_head() && impl_->deadlines.top().boundary.reached(now)) { auto entry=std::move(const_cast<Impl::DeadlineEntry &>(impl_->deadlines.top())); impl_->deadlines.pop(); auto it=impl_->deadline_controls.find(entry.timer_id); if (it==impl_->deadline_controls.end() || !it->second.active || it->second.generation!=entry.generation) continue; it->second.active=false; impl_->deadline_controls.erase(it); entry.callback(); } };
    auto wait_budget = [&]()
    { if (!live_head()) return -1; const auto remaining=impl_->deadlines.top().boundary.remaining(vix::MonotonicClock::now()).nanosecond_count(); if (remaining<=0) return 0; const auto ms=remaining/1000000LL+(remaining%1000000LL!=0); return ms>2147483647LL ? 2147483647 : static_cast<int>(ms); };
    ReactorEvent events[16];
    while (impl_->running)
    {
      expire();
      const int budget = wait_budget();
      if (budget == 0)
      {
        // A boundary may be crossed after `expire()` but before budgeting.
        // Process it in the next loop iteration instead of turning a stale
        // head into a repeated zero-timeout reactor poll busy-spin.
        expire();
        continue;
      }
      impl_->consecutive_zero_waits = 0;
      ++impl_->idle_generation;
      impl_->idle_cv.notify_all();
      const int n = impl_->reactor->wait(events, 16, budget);
      if (n < 0)
        continue;
      bool woke = false;
      for (int i = 0; i < n; i++)
        if (events[i].token == 0)
          woke = true;
      if (woke)
        drain_commands();
      expire();
      for (int i = 0; i < n && impl_->running; i++)
        if (events[i].token != 0)
          dispatch_on_reactor(events[i].token, events[i].events);
    }
#endif
  }
  // Generation is monotonic and retirement erases the identity: stale events cannot target a
  // new registration even when the operating system reuses its native socket value.
  bool ProviderCore::dispatch_on_reactor(std::uint64_t token, std::uint32_t events)
  {
    std::uint32_t id, g;
    if (!decode_token(token, id, g))
      return false;
    auto it = impl_->entries.find(id);
    if (it == impl_->entries.end() || !it->second.active || it->second.generation != g)
      return false;
    bool out = false;
    ReadinessHandler read_slot, write_slot;
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
    try
    {
      read_slot = it->second.read_slot;
      write_slot = it->second.write_slot;
    }
    catch (...)
    {
      return false;
    }
#else
    read_slot = it->second.read_slot;
    write_slot = it->second.write_slot;
#endif
    const auto read_revision = it->second.read_revision;
    const auto write_revision = it->second.write_revision;
    const auto terminal_events = reactor_error | reactor_hangup;
    const bool rr = events & (reactor_readable | terminal_events);
    const bool wr = events & (reactor_writable | terminal_events);
    bool retain_read = true, retain_write = true;
    if (rr && read_slot)
    {
      out = true;
      retain_read = read_slot(events);
    }
    if (wr && write_slot)
    {
      out = true;
      retain_write = write_slot(events);
    }
    // Readiness callbacks may terminalize operations and synchronously retire
    // this registration. Never retain a map reference across user-independent
    // transport callbacks that can mutate the descriptor table.
    it = impl_->entries.find(id);
    if (it == impl_->entries.end() || !it->second.active || it->second.generation != g)
      return out;
    auto &e = it->second;
    if (rr && read_slot && !retain_read && e.read_revision == read_revision)
    {
      e.read_slot = {};
      ++e.read_revision;
    }
    if (wr && write_slot && !retain_write && e.write_revision == write_revision)
    {
      e.write_slot = {};
      ++e.write_revision;
    }
    if (e.read_slot || e.write_slot)
    {
      if (!on_owner_thread())
        impl_->owner_only = false;
      (void)impl_->reactor->modify(e.native_socket, token, bool(e.read_slot), bool(e.write_slot));
    }
    else
    {
      // A handler that declines rearm has relinquished the registration.
      // Retire it before a successful operation hands the same native fd to a
      // Resource, otherwise the replacement registration can conflict.
      if (!on_owner_thread())
        impl_->owner_only = false;
      (void)impl_->reactor->remove(e.native_socket);
      e.active = false;
      impl_->entries.erase(it);
    }
    return out;
  }
  bool ProviderCore::dispatch_for_test(std::uint64_t t, std::uint32_t e)
  {
    bool out = false;
    return command_and_wait(CommandKind::generic, [&]
                            { out = dispatch_on_reactor(t, e); }) &&
           out;
  }
  std::uint64_t ProviderCore::register_descriptor(NativeSocket socket, ReadinessHandler r, ReadinessHandler w)
  {
    std::uint64_t out{};
    if (!command_and_wait(CommandKind::descriptor_add, [&]
      {if(socket==invalid_socket||!impl_->next_registration||!impl_->next_generation)return;auto id=impl_->next_registration++;auto g=impl_->next_generation++;auto t=encode_token(id,g);Impl::DescriptorEntry e{id,g,socket,std::move(r),std::move(w)};if(!on_owner_thread())impl_->owner_only=false;if(impl_->reactor->add(socket,t,bool(e.read_slot),bool(e.write_slot))){impl_->entries.emplace(id,std::move(e));out=t;} }))
      return 0;
    return out;
  }
  bool ProviderCore::set_descriptor_handlers(std::uint64_t t, ReadinessHandler r, ReadinessHandler w)
  {
    bool out = false;
    return command_and_wait(CommandKind::descriptor_mod, [&]
                            { std::uint32_t id,g; if(!decode_token(t,id,g)) return; auto it=impl_->entries.find(id);
      if(it==impl_->entries.end()||it->second.generation!=g) return;
      auto &e=it->second; e.read_slot=std::move(r); e.write_slot=std::move(w);
      ++e.read_revision; ++e.write_revision;
      if(!e.read_slot&&!e.write_slot) return;
      out=impl_->reactor->modify(e.native_socket,t,bool(e.read_slot),bool(e.write_slot)); }) &&
           out;
  }
  std::uint64_t ProviderCore::register_descriptor(NativeSocket socket, bool r, bool w)
  {
    return register_descriptor(socket, r ? ReadinessHandler{[](std::uint32_t)
                                                        { return true; }}
                                     : ReadinessHandler{},
                               w ? ReadinessHandler{[](std::uint32_t)
                                                    { return true; }}
                                 : ReadinessHandler{});
  }
  bool ProviderCore::rearm(std::uint64_t t, bool r, bool w)
  {
    bool out = false;
    return command_and_wait(CommandKind::descriptor_mod, [&]
                            {std::uint32_t id,g;if(!decode_token(t,id,g))return;auto it=impl_->entries.find(id);if(it==impl_->entries.end()||it->second.generation!=g)return;auto&e=it->second;e.read_slot=r?ReadinessHandler{[](std::uint32_t){return true;}}:ReadinessHandler{};e.write_slot=w?ReadinessHandler{[](std::uint32_t){return true;}}:ReadinessHandler{};++e.read_revision;++e.write_revision;if(!r&&!w)return;out=impl_->reactor->modify(e.native_socket,t,r,w); }) &&
           out;
  }
  bool ProviderCore::release_descriptor_direction(std::uint64_t t, bool read)
  {
    bool out = false;
    return command_and_wait(CommandKind::descriptor_mod, [&]
                            {std::uint32_t id,g;if(!decode_token(t,id,g))return;auto it=impl_->entries.find(id);if(it==impl_->entries.end()||it->second.generation!=g)return;auto&e=it->second;if(read){e.read_slot={};++e.read_revision;}else{e.write_slot={};++e.write_revision;}if(!e.read_slot&&!e.write_slot){(void)impl_->reactor->remove(e.native_socket);e.active=false;impl_->entries.erase(it);out=true;return;}out=impl_->reactor->modify(e.native_socket,t,bool(e.read_slot),bool(e.write_slot)); }) &&
           out;
  }
  void ProviderCore::retire(std::uint64_t t)
  {
    (void)command_and_wait(CommandKind::descriptor_del, [&]
                           {std::uint32_t id,g;if(!decode_token(t,id,g))return;auto it=impl_->entries.find(id);if(it==impl_->entries.end()||it->second.generation!=g)return;if(!on_owner_thread())impl_->owner_only=false;(void)impl_->reactor->remove(it->second.native_socket);it->second.active=false;it->second.read_slot={};it->second.write_slot={};impl_->entries.erase(it); });
  }
  std::uint64_t ProviderCore::add_deadline(vix::Deadline deadline, std::function<void()> callback)
  {
    struct Request
    {
      vix::Deadline deadline;
      std::function<void()> callback;
      std::uint64_t token{};
      Request(vix::Deadline d, std::function<void()> c) : deadline(d), callback(std::move(c)) {}
    };
    auto request = std::make_shared<Request>(deadline, std::move(callback));
    if (!command_and_wait(CommandKind::deadline_add, [this, request]
                          {if(!impl_->next_timer||!impl_->next_timer_generation)return;auto id=impl_->next_timer++;auto generation=impl_->next_timer_generation++;impl_->deadline_controls.emplace(id,Impl::DeadlineControl{generation,true});impl_->deadlines.push({request->deadline,id,generation,std::move(request->callback)});request->token=encode_token(id,generation); }))
      return 0;
    return request->token;
  }
  bool ProviderCore::invalidate_deadline(std::uint64_t token)
  {
    bool invalidated = false;
    return command_and_wait(CommandKind::deadline_invalidate, [this, token, &invalidated]
                            {std::uint32_t id,generation;if(!decode_token(token,id,generation))return;auto it=impl_->deadline_controls.find(id);if(it==impl_->deadline_controls.end()||it->second.generation!=generation)return;it->second.active=false;impl_->deadline_controls.erase(it);invalidated=true; }) &&
           invalidated;
  }
  bool ProviderCore::accepts(std::uint64_t t) const noexcept
  {
    bool out = false;
    return command_and_wait(CommandKind::generic, [&]
                            {std::uint32_t id,g;if(!decode_token(t,id,g))return;auto it=impl_->entries.find(id);out=it!=impl_->entries.end()&&it->second.active&&it->second.generation==g; }) &&
           out;
  }
  std::uint64_t ProviderCore::reactor_control_count_for_test(ReactorControl operation) const noexcept
  { return impl_->reactor ? impl_->reactor->control_count(operation) : 0; }
  bool ProviderCore::reactor_mutations_owned_by_reactor_for_test() const noexcept { return impl_->owner_only; }
  std::uint64_t ProviderCore::commands_executed_for_test() const noexcept { return impl_->executed; }
  std::uint64_t ProviderCore::wake_events_for_test() const noexcept { return impl_->reactor ? impl_->reactor->wake_events() : 0; }
  std::uint64_t ProviderCore::wake_writes_for_test() const noexcept { return impl_->reactor ? impl_->reactor->wake_writes() : 0; }
  unsigned ProviderCore::active_operations_for_test() const noexcept { return impl_->ops.load(); }
  unsigned ProviderCore::active_resources_for_test() const noexcept { return impl_->resources.load(); }
  bool ProviderCore::has_dependencies() const noexcept { return impl_->ops.load() || impl_->resources.load(); }
  bool ProviderCore::running() const noexcept { return impl_->running.load(); }
  std::size_t ProviderCore::live_deadlines_for_test() const noexcept
  {
    std::size_t count{};
    (void)command_and_wait(CommandKind::generic, [this, &count]
                           { count = impl_->deadline_controls.size(); });
    return count;
  }
  std::size_t ProviderCore::live_descriptor_registrations_for_test() const noexcept
  {
    std::size_t count{};
    (void)command_and_wait(CommandKind::generic, [this, &count]
                           { count = impl_->entries.size(); });
    return count;
  }
  std::uint64_t ProviderCore::idle_generation_for_test() const noexcept { return impl_->idle_generation.load(); }
  bool ProviderCore::wait_for_idle_after_for_test(std::uint64_t generation) const noexcept
  {
#if defined(__wasi__)
    (void)generation;
    return false;
#else
    std::unique_lock lock(impl_->idle_m);
    return impl_->idle_cv.wait_for(lock, std::chrono::seconds(1), [this, generation]
                                   { return impl_->idle_generation.load() > generation || !impl_->running.load(); });
#endif
  }
  std::uint64_t ProviderCore::maximum_consecutive_zero_waits_for_test() const noexcept { return impl_->maximum_consecutive_zero_waits.load(); }
  void ProviderCore::add_operation() noexcept { ++impl_->ops; }
  void ProviderCore::finish_operation() noexcept { --impl_->ops; }
  void ProviderCore::add_resource() noexcept { ++impl_->resources; }
  void ProviderCore::release_resource() noexcept { --impl_->resources; }
  vix::Result<void> ProviderCore::shutdown()
  {
    if (!impl_->running)
      return vix::Result<void>::success();
    if (impl_->ops || impl_->resources)
      return vix::Error{provider_in_use, "provider dependencies remain"};
    if (impl_->cooperative)
    {
      if (!on_owner_thread())
        return vix::Error{operation_in_progress, "cooperative provider must be shut down by its creating thread"};
      if (impl_->driving)
        return vix::Error{operation_in_progress, "cannot shut down during a progress turn"};
      for (;;)
      {
        auto progressed = poll();
        if (!progressed)
          return progressed.error();
        std::lock_guard lock(impl_->queue_m);
        if (!impl_->commands.empty())
          continue;
        impl_->accepting = false;
        impl_->running = false;
        break;
      }
      impl_->reactor.reset();
      return vix::Result<void>::success();
    }
#if defined(__wasi__)
    return vix::Error{unsupported, "background-driven Network providers are unavailable on WASI"};
#else
    {
      std::lock_guard l(impl_->queue_m);
      if (!impl_->accepting.exchange(false))
        return vix::Result<void>::success();
      impl_->commands.push_back({CommandKind::shutdown, impl_->next_sequence++, [] {}});
    }
    if (!wake())
      return vix::Error{resource_exhausted, "cannot wake reactor shutdown"};
    if (impl_->thread.joinable())
      impl_->thread.join();
    impl_->reactor.reset();
    return vix::Result<void>::success();
#endif
  }
  void ProviderCore::stop_and_join() noexcept
  {
#if defined(__wasi__)
    impl_->accepting = false;
    impl_->running = false;
    impl_->reactor.reset();
#else
    impl_->accepting = false;
    impl_->running = false;
    (void)wake();
    if (impl_->thread.joinable())
      impl_->thread.join();
    impl_->reactor.reset();
#endif
  }
}
