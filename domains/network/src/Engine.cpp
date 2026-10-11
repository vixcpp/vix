#include "Engine.hpp"
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <pthread.h>
#include <unistd.h>
#include <cerrno>
#include <condition_variable>
#include <deque>
#include <queue>
#include <thread>
#include <unordered_map>
namespace vix::network::detail
{
  namespace
  {
    struct Pool
    {
      std::mutex m;
      std::condition_variable cv;
      std::deque<std::function<void()>> q;
    };
    Pool &pool()
    {
      static auto *p = new Pool;
      return *p;
    }
    void *worker(void *)
    {
      auto &p = pool();
      for (;;)
      {
        std::function<void()> f;
        {
          std::unique_lock l(p.m);
          p.cv.wait(l, [&]
                    { return !p.q.empty(); });
          f = std::move(p.q.front());
          p.q.pop_front();
        }
        f();
      }
    }
    std::atomic<bool> pool_ready{};
    constexpr std::uint32_t terminal_events = EPOLLERR | EPOLLHUP
#ifdef EPOLLRDHUP
                                              | EPOLLRDHUP
#endif
        ;
  }
  vix::Result<void> Continuations::ensure()
  {
    bool e = false;
    if (pool_ready.load())
      return vix::Result<void>::success();
    if (pool_ready.compare_exchange_strong(e, true))
    {
      pthread_t a{}, b{};
      if (pthread_create(&a, nullptr, worker, nullptr) || pthread_create(&b, nullptr, worker, nullptr))
      {
        pool_ready = false;
        return vix::Error{resource_exhausted, "cannot start continuation workers"};
      }
      pthread_detach(a);
      pthread_detach(b);
    }
    return vix::Result<void>::success();
  }
  void Continuations::enqueue(std::function<void()> f)
  {
    auto &p = pool();
    {
      std::lock_guard l(p.m);
      p.q.push_back(std::move(f));
    }
    p.cv.notify_one();
  }
  struct ProviderCore::Impl
  {
    struct DeadlineEntry { vix::Deadline boundary; std::uint32_t timer_id, generation; std::function<void()> callback; };
    struct DeadlineLater { bool operator()(const DeadlineEntry &a, const DeadlineEntry &b) const noexcept { if (a.boundary.boundary() != b.boundary.boundary()) return a.boundary.boundary() > b.boundary.boundary(); return a.timer_id > b.timer_id; } };
    struct DeadlineControl { std::uint32_t generation; bool active; };
    struct DescriptorEntry
    {
      std::uint32_t registration_id, generation;
      int native_fd;
      ReadinessHandler read_slot, write_slot;
      std::uint32_t current_interest_mask{};
      bool active{true};
    };
    struct Command
    {
      CommandKind kind;
      std::uint64_t sequence;
      std::function<void()> action;
    };
    int ep{}, wake{};
    pthread_t thread{};
    std::atomic<bool> running{true}, accepting{true}, thread_started{false};
    std::mutex queue_m;
    std::deque<Command> commands;
    std::atomic<std::uint64_t> next_sequence{1}, executed{}, wake_writes{}, wake_events{};
    std::mutex startup_m;
    std::condition_variable startup_cv;
    bool startup_finished{}, startup_success{};
    std::atomic<unsigned> ops{}, resources{};
    std::uint32_t next_registration{1}, next_generation{1};
    std::unordered_map<std::uint32_t, DescriptorEntry> entries;
    std::uint32_t next_timer{1}, next_timer_generation{1};
    std::priority_queue<DeadlineEntry, std::vector<DeadlineEntry>, DeadlineLater> deadlines;
    std::unordered_map<std::uint32_t, DeadlineControl> deadline_controls;
    std::atomic<std::uint64_t> adds{}, mods{}, dels{};
    std::atomic<bool> owner_only{true};
    std::thread::id owner{};
  };
  std::uint32_t ProviderCore::interest(bool read, bool write)
  {
    std::uint32_t r = EPOLLONESHOT | EPOLLERR | EPOLLHUP;
    if (read)
      r |= EPOLLIN;
    if (write)
      r |= EPOLLOUT;
#ifdef EPOLLRDHUP
    r |= EPOLLRDHUP;
#endif
    return r;
  }
  ProviderCore::ProviderCore(int e, int w) noexcept : impl_(new Impl)
  {
    impl_->ep = e;
    impl_->wake = w;
  }
  std::uint64_t ProviderCore::encode_token(std::uint32_t i, std::uint32_t g) noexcept { return (std::uint64_t{i} << 32) | g; }
  bool ProviderCore::decode_token(std::uint64_t t, std::uint32_t &i, std::uint32_t &g) noexcept
  {
    i = t >> 32;
    g = t;
    return i && g;
  }
  vix::Result<std::shared_ptr<ProviderCore>> ProviderCore::create()
  {
    auto infra = Continuations::ensure();
    if (!infra)
      return infra.error();
    int e = epoll_create1(EPOLL_CLOEXEC), w = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (e < 0 || w < 0)
    {
      if (e >= 0)
        close(e);
      if (w >= 0)
        close(w);
      return vix::Error{resource_exhausted, "cannot create reactor"};
    }
    auto p = std::shared_ptr<ProviderCore>(new ProviderCore(e, w));
    if (pthread_create(&p->impl_->thread, nullptr, [](void *x) -> void *
                       {static_cast<ProviderCore*>(x)->run();return nullptr; }, p.get()))
    {
      close(w);
      close(e);
      return vix::Error{resource_exhausted, "cannot start reactor"};
    }
    p->impl_->thread_started = true;
    {
      std::unique_lock l(p->impl_->startup_m);
      p->impl_->startup_cv.wait(l, [&]
                                { return p->impl_->startup_finished; });
    }
    if (!p->impl_->startup_success)
    {
      pthread_join(p->impl_->thread, nullptr);
      close(w);
      close(e);
      return vix::Error{resource_exhausted, "cannot register reactor wake"};
    }
    return p;
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
  // MPSC publication is protected by queue_m. Unlocking after command insertion publishes the
  // fully-owned Command to the sole reactor consumer; eventfd is notification, not memory sync.
  bool ProviderCore::wake() noexcept
  {
    std::uint64_t one = 1;
    for (;;)
    {
      auto n = write(impl_->wake, &one, sizeof one);
      if (n == sizeof one)
      {
        ++impl_->wake_writes;
        return true;
      }
      if (n < 0 && errno == EINTR)
        continue;
      if (n < 0 && errno == EAGAIN)
        return true;
      std::terminate();
    }
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
  bool ProviderCore::post_for_test(std::function<void()> f) { return post(std::move(f)); }
  void ProviderCore::empty_wake_for_test() noexcept { (void)wake(); }
  bool ProviderCore::command_and_wait(CommandKind kind, std::function<void()> f) const
  {
    if (std::this_thread::get_id()==impl_->owner) { f(); return true; }
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
  }
  void ProviderCore::run() noexcept
  {
    impl_->owner = std::this_thread::get_id();
    epoll_event wake_event{};
    wake_event.events = EPOLLIN;
    wake_event.data.u64 = 0;
    bool ready = epoll_ctl(impl_->ep, EPOLL_CTL_ADD, impl_->wake, &wake_event) == 0;
    {
      std::lock_guard l(impl_->startup_m);
      impl_->startup_success = ready;
      impl_->startup_finished = true;
    }
    impl_->startup_cv.notify_one();
    if (!ready)
    {
      impl_->running = false;
      return;
    }
    auto drain_wake = [&]
    {std::uint64_t value;for(;;){auto n=read(impl_->wake,&value,sizeof value);if(n==sizeof value)continue;if(n<0&&errno==EINTR)continue;if(n<0&&errno==EAGAIN)break;std::terminate();}++impl_->wake_events; };
    auto drain_commands = [&]
    {for(;;){std::deque<Impl::Command>batch;{std::lock_guard l(impl_->queue_m);batch.swap(impl_->commands);}if(batch.empty())return;for(auto&command:batch){command.action();++impl_->executed;if(command.kind==CommandKind::shutdown){impl_->running=false;return;}}} };
    auto live_head = [&]()
    { while (!impl_->deadlines.empty()) { const auto &entry=impl_->deadlines.top(); auto it=impl_->deadline_controls.find(entry.timer_id); if (it!=impl_->deadline_controls.end() && it->second.active && it->second.generation==entry.generation) return true; impl_->deadlines.pop(); } return false; };
    auto expire = [&]()
    { const auto now=vix::MonotonicClock::now(); while (live_head() && impl_->deadlines.top().boundary.reached(now)) { auto entry=std::move(const_cast<Impl::DeadlineEntry &>(impl_->deadlines.top())); impl_->deadlines.pop(); auto it=impl_->deadline_controls.find(entry.timer_id); if (it==impl_->deadline_controls.end() || !it->second.active || it->second.generation!=entry.generation) continue; it->second.active=false; impl_->deadline_controls.erase(it); entry.callback(); } };
    auto wait_budget = [&]()
    { if (!live_head()) return -1; const auto remaining=impl_->deadlines.top().boundary.remaining(vix::MonotonicClock::now()).nanosecond_count(); if (remaining<=0) return 0; const auto ms=remaining/1000000LL+(remaining%1000000LL!=0); return ms>2147483647LL ? 2147483647 : static_cast<int>(ms); };
    epoll_event es[16];
    while (impl_->running)
    {
      expire();
      int n;
      do
      {
        n = epoll_wait(impl_->ep, es, 16, wait_budget());
      } while (n < 0 && errno == EINTR);
      if (n < 0)
        continue;
      bool woke = false;
      for (int i = 0; i < n; i++)
        if (es[i].data.u64 == 0)
        {
          drain_wake();
          woke = true;
        }
      if (woke)
        drain_commands();
      expire();
      for (int i = 0; i < n && impl_->running; i++)
        if (es[i].data.u64 != 0)
          dispatch_on_reactor(es[i].data.u64, es[i].events);
    }
  }
  // Generation is monotonic and retirement erases the identity: stale events cannot target a
  // new registration even when Linux reuses the same numeric native fd.
  bool ProviderCore::dispatch_on_reactor(std::uint64_t token, std::uint32_t events)
  {
    std::uint32_t id, g;
    if (!decode_token(token, id, g))
      return false;
    auto it = impl_->entries.find(id);
    if (it == impl_->entries.end() || !it->second.active || it->second.generation != g)
      return false;
    bool out = false;
    auto &e = it->second;
    bool rr = events & (EPOLLIN | terminal_events), wr = events & (EPOLLOUT | terminal_events);
    if (rr && e.read_slot)
    {
      out = true;
      if (!e.read_slot(events))
        e.read_slot = {};
    }
    if (wr && e.write_slot)
    {
      out = true;
      if (!e.write_slot(events))
        e.write_slot = {};
    }
    if (e.read_slot || e.write_slot)
    {
      e.current_interest_mask = interest(bool(e.read_slot), bool(e.write_slot));
      epoll_event ev{};
      ev.events = e.current_interest_mask;
      ev.data.u64 = token;
      if (std::this_thread::get_id() != impl_->owner)
        impl_->owner_only = false;
      if (epoll_ctl(impl_->ep, EPOLL_CTL_MOD, e.native_fd, &ev) == 0)
        ++impl_->mods;
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
  std::uint64_t ProviderCore::register_descriptor(int fd, ReadinessHandler r, ReadinessHandler w)
  {
    std::uint64_t out{};
    if (!command_and_wait(CommandKind::descriptor_add, [&]
                          {if(fd<0||!impl_->next_registration||!impl_->next_generation)return;auto id=impl_->next_registration++;auto g=impl_->next_generation++;auto t=encode_token(id,g);Impl::DescriptorEntry e{id,g,fd,std::move(r),std::move(w)};e.current_interest_mask=interest(bool(e.read_slot),bool(e.write_slot));epoll_event ev{};ev.events=e.current_interest_mask;ev.data.u64=t;if(std::this_thread::get_id()!=impl_->owner)impl_->owner_only=false;if(epoll_ctl(impl_->ep,EPOLL_CTL_ADD,fd,&ev)==0){++impl_->adds;impl_->entries.emplace(id,std::move(e));out=t;} }))
      return 0;
    return out;
  }
  std::uint64_t ProviderCore::register_descriptor(int fd, bool r, bool w)
  {
    return register_descriptor(fd, r ? ReadinessHandler{[](std::uint32_t)
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
                            {std::uint32_t id,g;if(!decode_token(t,id,g))return;auto it=impl_->entries.find(id);if(it==impl_->entries.end()||it->second.generation!=g)return;auto&e=it->second;e.read_slot=r?ReadinessHandler{[](std::uint32_t){return true;}}:ReadinessHandler{};e.write_slot=w?ReadinessHandler{[](std::uint32_t){return true;}}:ReadinessHandler{};if(!r&&!w)return;e.current_interest_mask=interest(r,w);epoll_event ev{};ev.events=e.current_interest_mask;ev.data.u64=t;if(epoll_ctl(impl_->ep,EPOLL_CTL_MOD,e.native_fd,&ev)==0){++impl_->mods;out=true;} }) &&
           out;
  }
  bool ProviderCore::release_descriptor_direction(std::uint64_t t, bool read)
  {
    bool out=false;
    return command_and_wait(CommandKind::descriptor_mod,[&]
                            {std::uint32_t id,g;if(!decode_token(t,id,g))return;auto it=impl_->entries.find(id);if(it==impl_->entries.end()||it->second.generation!=g)return;auto&e=it->second;if(read)e.read_slot={};else e.write_slot={};if(!e.read_slot&&!e.write_slot)return;e.current_interest_mask=interest(bool(e.read_slot),bool(e.write_slot));epoll_event ev{};ev.events=e.current_interest_mask;ev.data.u64=t;if(epoll_ctl(impl_->ep,EPOLL_CTL_MOD,e.native_fd,&ev)==0){++impl_->mods;out=true;}})&&out;
  }
  void ProviderCore::retire(std::uint64_t t)
  {
    (void)command_and_wait(CommandKind::descriptor_del, [&]
                           {std::uint32_t id,g;if(!decode_token(t,id,g))return;auto it=impl_->entries.find(id);if(it==impl_->entries.end()||it->second.generation!=g)return;if(std::this_thread::get_id()!=impl_->owner)impl_->owner_only=false;if(epoll_ctl(impl_->ep,EPOLL_CTL_DEL,it->second.native_fd,nullptr)==0)++impl_->dels;it->second.active=false;it->second.read_slot={};it->second.write_slot={};impl_->entries.erase(it); });
  }
  std::uint64_t ProviderCore::add_deadline(vix::Deadline deadline, std::function<void()> callback)
  {
    struct Request { vix::Deadline deadline; std::function<void()> callback; std::uint64_t token{}; Request(vix::Deadline d,std::function<void()> c):deadline(d),callback(std::move(c)){} };
    auto request=std::make_shared<Request>(deadline,std::move(callback));
    if (!command_and_wait(CommandKind::deadline_add,[this,request]
                          {if(!impl_->next_timer||!impl_->next_timer_generation)return;auto id=impl_->next_timer++;auto generation=impl_->next_timer_generation++;impl_->deadline_controls.emplace(id,Impl::DeadlineControl{generation,true});impl_->deadlines.push({request->deadline,id,generation,std::move(request->callback)});request->token=encode_token(id,generation);})) return 0;
    return request->token;
  }
  bool ProviderCore::invalidate_deadline(std::uint64_t token)
  {
    bool invalidated=false;
    return command_and_wait(CommandKind::deadline_invalidate,[this,token,&invalidated]
                            {std::uint32_t id,generation;if(!decode_token(token,id,generation))return;auto it=impl_->deadline_controls.find(id);if(it==impl_->deadline_controls.end()||it->second.generation!=generation)return;it->second.active=false;impl_->deadline_controls.erase(it);invalidated=true;}) && invalidated;
  }
  bool ProviderCore::accepts(std::uint64_t t) const noexcept
  {
    bool out = false;
    return command_and_wait(CommandKind::generic, [&]
                            {std::uint32_t id,g;if(!decode_token(t,id,g))return;auto it=impl_->entries.find(id);out=it!=impl_->entries.end()&&it->second.active&&it->second.generation==g; }) &&
           out;
  }
  std::uint64_t ProviderCore::epoll_ctl_count_for_test(int o) const noexcept
  {
    if (o == EPOLL_CTL_ADD)
      return impl_->adds;
    if (o == EPOLL_CTL_MOD)
      return impl_->mods;
    if (o == EPOLL_CTL_DEL)
      return impl_->dels;
    return 0;
  }
  bool ProviderCore::epoll_mutations_owned_by_reactor_for_test() const noexcept { return impl_->owner_only; }
  std::uint64_t ProviderCore::commands_executed_for_test() const noexcept { return impl_->executed; }
  std::uint64_t ProviderCore::wake_events_for_test() const noexcept { return impl_->wake_events; }
  std::uint64_t ProviderCore::wake_writes_for_test() const noexcept { return impl_->wake_writes; }
  std::size_t ProviderCore::live_deadlines_for_test() const noexcept { std::size_t count{}; (void)command_and_wait(CommandKind::generic,[this,&count]{count=impl_->deadline_controls.size();}); return count; }
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
    {
      std::lock_guard l(impl_->queue_m);
      if (!impl_->accepting.exchange(false))
        return vix::Result<void>::success();
      impl_->commands.push_back({CommandKind::shutdown, impl_->next_sequence++, [] {}});
    }
    if (!wake())
      return vix::Error{resource_exhausted, "cannot wake reactor shutdown"};
    if (impl_->thread_started)
      pthread_join(impl_->thread, nullptr);
    close(impl_->wake);
    close(impl_->ep);
    return vix::Result<void>::success();
  }
  void ProviderCore::stop_and_join() noexcept
  {
    impl_->accepting = false;
    impl_->running = false;
    (void)wake();
    if (impl_->thread_started)
      pthread_join(impl_->thread, nullptr);
    close(impl_->wake);
    close(impl_->ep);
  }
}
