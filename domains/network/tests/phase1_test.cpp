#include "Engine.hpp"
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cassert>
#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>
#include <type_traits>
using namespace vix::network::detail;
static bool wait_for(const std::atomic<int>&v,int n){for(int i=0;i<100;i++){if(v.load()>=n)return true;std::this_thread::sleep_for(std::chrono::milliseconds(10));}return false;}
int main(){
  static_assert(!std::is_copy_constructible_v<Operation<int>>);static_assert(std::is_move_constructible_v<Operation<int>>);
  std::uint32_t id{},gen{};auto packed=ProviderCore::encode_token(77,19);assert(ProviderCore::decode_token(packed,id,gen)&&id==77&&gen==19);assert(!ProviderCore::decode_token(0,id,gen));
  auto made=ProviderCore::create();assert(made);auto p=made.take();
  // Independent read-only and write-only registrations use real epoll readiness.
  int read_pipe[2];assert(pipe(read_pipe)==0);std::atomic<int> only_read{};
  auto read_only=p->register_descriptor(read_pipe[0],[&](std::uint32_t){++only_read;return false;},{});assert(read_only);assert(write(read_pipe[1],"r",1)==1);assert(wait_for(only_read,1));p->retire(read_only);close(read_pipe[0]);close(read_pipe[1]);
  int write_pair[2];assert(socketpair(AF_UNIX,SOCK_STREAM,0,write_pair)==0);std::atomic<int> only_write{};
  auto write_only=p->register_descriptor(write_pair[0],{},[&](std::uint32_t){++only_write;return false;});assert(write_only);assert(wait_for(only_write,1));p->retire(write_only);close(write_pair[0]);close(write_pair[1]);
  int fds[2];assert(socketpair(AF_UNIX,SOCK_STREAM,0,fds)==0);std::atomic<int> reads{},writes{};
  auto both=p->register_descriptor(fds[0],[&](std::uint32_t){++reads;return true;},[&](std::uint32_t){++writes;return true;});assert(both&&p->accepts(both));
  assert(wait_for(writes,1)); // kernel EPOLLOUT through epoll_wait
  assert(write(fds[1],"x",1)==1);assert(wait_for(reads,1)); // kernel EPOLLIN through epoll_wait
  auto mods=p->epoll_ctl_count_for_test(EPOLL_CTL_MOD);assert(mods>=2); // ONESHOT rearmed after each readiness event
  // Deterministic directional completion fixture shares the reactor dispatcher.
  std::atomic<int> r{},w{};int second[2];assert(pipe(second)==0);auto directional=p->register_descriptor(second[0],[&](std::uint32_t){++r;return false;},[&](std::uint32_t){++w;return true;});assert(directional);
  assert(p->dispatch_for_test(directional,EPOLLIN|EPOLLOUT));assert(r==1&&w==1);
  assert(p->dispatch_for_test(directional,EPOLLOUT));assert(r==1&&w==2); // read removed, write preserved
  p->retire(directional);close(second[0]);close(second[1]);
  std::atomic<int> r2{},w2{};int third[2];assert(pipe(third)==0);auto inverse=p->register_descriptor(third[0],[&](std::uint32_t){++r2;return true;},[&](std::uint32_t){++w2;return false;});assert(inverse);
  assert(p->dispatch_for_test(inverse,EPOLLIN|EPOLLOUT));assert(r2==1&&w2==1);assert(p->dispatch_for_test(inverse,EPOLLIN));assert(r2==2&&w2==1); // write removed, read preserved
  // Retired A is rejected after a real remove/close and a fresh B registration.
  p->retire(inverse);close(third[0]);close(third[1]);auto old=both;p->retire(old);close(fds[0]);close(fds[1]);
  int reused[2];assert(pipe(reused)==0);std::atomic<int> b{};auto fresh=p->register_descriptor(reused[0],[&](std::uint32_t){++b;return true;},{});assert(fresh);assert(!p->dispatch_for_test(old,EPOLLIN));assert(b==0);assert(!p->accepts(old)&&p->accepts(fresh));
  assert(p->epoll_ctl_count_for_test(EPOLL_CTL_ADD)>=6);assert(p->epoll_ctl_count_for_test(EPOLL_CTL_DEL)>=5);assert(p->epoll_mutations_owned_by_reactor_for_test());
  p->retire(fresh);close(reused[0]);close(reused[1]);p->add_operation();assert(!p->shutdown());p->finish_operation();assert(p->shutdown());

  // Phase 1B: retain every command payload until the one reactor consumer executes it.
  auto made2=ProviderCore::create();assert(made2);auto q=made2.take();
  constexpr int producers=8,per_producer=1500,total=producers*per_producer;
  struct StressState{std::unique_ptr<std::atomic<unsigned>[]>seen{new std::atomic<unsigned>[total]};std::atomic<int>executed{},entered{},release{},chained{};};auto state=std::make_shared<StressState>();for(int i=0;i<total;i++)state->seen[i]=0;
  assert(q->post_for_test([state]{state->entered=true;while(!state->release.load())std::this_thread::yield();}));for(int i=0;i<100&&!state->entered;i++)std::this_thread::sleep_for(std::chrono::milliseconds(10));assert(state->entered);
  std::vector<std::thread> workers;for(int pidx=0;pidx<producers;pidx++)workers.emplace_back([state,q,pidx]{for(int n=0;n<per_producer;n++){int sequence=pidx*per_producer+n;assert(q->post_for_test([state,sequence]{++state->seen[sequence];++state->executed;}));}});
  for(auto&worker:workers)worker.join();state->release=true;assert(wait_for(state->executed,total));for(int i=0;i<total;i++)assert(state->seen[i]==1);
  assert(q->wake_writes_for_test()>q->wake_events_for_test()); // blocked consumer forces eventfd coalescing
  auto wakes=q->wake_events_for_test();q->empty_wake_for_test();std::this_thread::sleep_for(std::chrono::milliseconds(20));assert(q->wake_events_for_test()>wakes); // empty wake is harmless
  // A command posted by the reactor is queued safely and receives a later wake.
  assert(q->post_for_test([state,q]{++state->chained;assert(q->post_for_test([state]{++state->chained;}));}));assert(wait_for(state->chained,2));
  // Concurrent descriptor requests remain serialized through the same queue.
  std::vector<int> descriptor_fds;std::vector<std::thread> descriptor_workers;for(int n=0;n<8;n++){int pair[2];assert(pipe(pair)==0);descriptor_fds.push_back(pair[0]);descriptor_fds.push_back(pair[1]);}
  for(int n=0;n<8;n++)descriptor_workers.emplace_back([&,n]{auto token=q->register_descriptor(descriptor_fds[n*2],true,false);assert(token);assert(q->rearm(token,true,false));q->retire(token);});for(auto&worker:descriptor_workers)worker.join();for(int fd:descriptor_fds)close(fd);
  assert(q->epoll_ctl_count_for_test(EPOLL_CTL_ADD)>=8);assert(q->epoll_ctl_count_for_test(EPOLL_CTL_MOD)>=8);assert(q->epoll_ctl_count_for_test(EPOLL_CTL_DEL)>=8);assert(q->epoll_mutations_owned_by_reactor_for_test());
  // Phase 1C: absolute deadlines share the monotonic Standard clock and expire once.
  std::atomic<int> expired{};auto stale=q->add_deadline(vix::Deadline::after(vix::Duration::milliseconds(20)),[&]{expired+=100;});assert(stale&&q->invalidate_deadline(stale));
  auto first=q->add_deadline(vix::Deadline::after(vix::Duration::milliseconds(80)),[&]{expired+=1;});assert(first);std::this_thread::sleep_for(std::chrono::milliseconds(10));auto earlier=q->add_deadline(vix::Deadline::after(vix::Duration::milliseconds(15)),[&]{expired+=10;});assert(earlier);assert(wait_for(expired,10));assert(expired==10);assert(wait_for(expired,11));assert(expired==11);
  auto reached=q->add_deadline(vix::Deadline::at(vix::MonotonicClock::now()-vix::Duration::milliseconds(1)),[&]{expired+=1;});assert(reached);assert(wait_for(expired,12));
  std::atomic<int> simultaneous{};auto equal_boundary=vix::Deadline::after(vix::Duration::milliseconds(25));for(int n=0;n<64;n++)assert(q->add_deadline(equal_boundary,[&]{++simultaneous;}));assert(wait_for(simultaneous,64));assert(simultaneous==64);
  std::atomic<int> concurrent_expiry{};std::vector<std::thread> timer_producers;for(int producer=0;producer<8;producer++)timer_producers.emplace_back([q,&concurrent_expiry]{for(int n=0;n<100;n++)assert(q->add_deadline(vix::Deadline::after(vix::Duration::milliseconds(10)),[&concurrent_expiry]{++concurrent_expiry;}));});for(auto&producer:timer_producers)producer.join();assert(wait_for(concurrent_expiry,800));assert(concurrent_expiry==800);
  for(int n=0;n<200;n++){auto token=q->add_deadline(vix::Deadline::after(vix::Duration::milliseconds(5)),[&]{expired+=1000;});assert(token);assert(q->invalidate_deadline(token));}assert(q->live_deadlines_for_test()==0);assert(q->shutdown());

  // The shutdown boundary rejects new work, while work accepted before it drains exactly once.
  struct StopState{std::atomic<bool>hold{},go{},shutdown_ok{};std::atomic<int>before_stop{};};auto stop_state=std::make_shared<StopState>();auto made3=ProviderCore::create();assert(made3);auto stopping=made3.take();assert(stopping->post_for_test([stop_state]{stop_state->hold=true;while(!stop_state->go.load())std::this_thread::yield();}));for(int i=0;i<100&&!stop_state->hold;i++)std::this_thread::sleep_for(std::chrono::milliseconds(10));assert(stop_state->hold);assert(stopping->post_for_test([stop_state]{++stop_state->before_stop;}));std::thread shutdown_thread([stopping,stop_state]{stop_state->shutdown_ok=bool(stopping->shutdown());});bool rejected=false;for(int i=0;i<100&&!rejected;i++){rejected=!stopping->post_for_test([]{});std::this_thread::sleep_for(std::chrono::milliseconds(1));}assert(rejected);stop_state->go=true;shutdown_thread.join();assert(stop_state->shutdown_ok&&stop_state->before_stop==1);assert(!stopping->post_for_test([]{}));
  auto made4=ProviderCore::create();assert(made4);auto deadline_shutdown=made4.take();std::atomic<int>after_shutdown{};assert(deadline_shutdown->add_deadline(vix::Deadline::after(vix::Duration::seconds(1)),[&]{++after_shutdown;}));assert(deadline_shutdown->shutdown());assert(after_shutdown==0);assert(!deadline_shutdown->add_deadline(vix::Deadline::after(vix::Duration::milliseconds(1)),[]{}));
  // Phase 1D deterministic handshake probes. Completion before commit cannot queue a resume.
  auto made5=ProviderCore::create();assert(made5);auto operations=made5.take();auto before=std::make_shared<OperationRecord<int>>(operations);before->begin(std::noop_coroutine());before->complete(vix::Completion<int>{7});assert(!before->commit_suspend());auto early=before->take_completion();assert(early.status()==vix::CompletionStatus::Success&&early.take()==7);
  auto after=std::make_shared<OperationRecord<void>>(operations);after->begin(std::noop_coroutine());assert(after->commit_suspend());after->complete(vix::Completion<void>::success());for(int i=0;i<100&&after->phase()!=OperationRecord<void>::Phase::completed_scheduled;i++)std::this_thread::sleep_for(std::chrono::milliseconds(1));assert(after->phase()==OperationRecord<void>::Phase::completed_scheduled);assert(after->take_completion().status()==vix::CompletionStatus::Success);
  auto move_only=std::make_shared<OperationRecord<std::unique_ptr<int>>>(operations);move_only->begin(std::noop_coroutine());move_only->complete(vix::Completion<std::unique_ptr<int>>{std::make_unique<int>(9)});assert(!move_only->commit_suspend());assert(*move_only->take_completion().take()==9);
  // Phase 1E: Standard cancellation callback only posts work; reactor owns completion.
  int directions[2];assert(socketpair(AF_UNIX,SOCK_STREAM,0,directions)==0);auto direction_token=operations->register_descriptor(directions[0],true,true);assert(direction_token);auto cancelled=std::make_shared<OperationRecord<void>>(operations);cancelled->bind_descriptor_direction(direction_token,true);cancelled->begin(std::noop_coroutine());assert(cancelled->commit_suspend());vix::CancellationSource source;cancelled->observe_cancellation(source.token());assert(source.request_cancel());for(int i=0;i<100&&(cancelled->phase()!=OperationRecord<void>::Phase::completed_scheduled||cancelled->native_access_allowed_for_test());i++)std::this_thread::sleep_for(std::chrono::milliseconds(1));assert(!cancelled->native_access_allowed_for_test());assert(cancelled->take_completion().status()==vix::CompletionStatus::Cancelled);assert(operations->accepts(direction_token));assert(operations->rearm(direction_token,false,true));auto trace=cancelled->terminal_trace_for_test();assert(trace.size()==8&&trace[0]==OperationRecord<void>::TerminalEvent::readiness_revoked&&trace[4]==OperationRecord<void>::TerminalEvent::borrow_revoked&&trace[5]==OperationRecord<void>::TerminalEvent::completion_stored&&trace[6]==OperationRecord<void>::TerminalEvent::provider_released&&trace[7]==OperationRecord<void>::TerminalEvent::resume_queued);operations->retire(direction_token);close(directions[0]);close(directions[1]);
  auto timed=std::make_shared<OperationRecord<void>>(operations);timed->begin(std::noop_coroutine());assert(timed->commit_suspend());timed->observe_deadline(vix::Deadline::after(vix::Duration::milliseconds(2)));for(int i=0;i<100&&timed->phase()!=OperationRecord<void>::Phase::completed_scheduled;i++)std::this_thread::sleep_for(std::chrono::milliseconds(1));assert(timed->take_completion().status()==vix::CompletionStatus::Failure);
  // Every terminal source releases only its owning direction; the opposite uses the real reactor dispatcher.
  auto directional_case=[&](bool remove_read,int source_kind){int pair[2];assert(socketpair(AF_UNIX,SOCK_STREAM,0,pair)==0);std::atomic<int>survived{};auto token=operations->register_descriptor(pair[0],[&](std::uint32_t){++survived;return true;},[&](std::uint32_t){++survived;return true;});assert(token);auto record=std::make_shared<OperationRecord<void>>(operations);record->bind_descriptor_direction(token,remove_read);record->begin(std::noop_coroutine());assert(record->commit_suspend());vix::CancellationSource cancel;if(source_kind==0)assert(operations->post([record]{record->complete(vix::Completion<void>::success());}));else if(source_kind==1)assert(operations->post([record]{record->complete(vix::Completion<void>{vix::Error{timed_out,"mock failure"}});}));else if(source_kind==2){record->observe_cancellation(cancel.token());assert(cancel.request_cancel());}else record->observe_deadline(vix::Deadline::after(vix::Duration::milliseconds(2)));for(int i=0;i<100&&record->phase()!=OperationRecord<void>::Phase::completed_scheduled;i++)std::this_thread::sleep_for(std::chrono::milliseconds(1));auto completion=record->take_completion();if(source_kind==0)assert(completion.status()==vix::CompletionStatus::Success);else if(source_kind==2)assert(completion.status()==vix::CompletionStatus::Cancelled);else assert(completion.status()==vix::CompletionStatus::Failure);assert(operations->accepts(token));assert(operations->dispatch_for_test(token,remove_read?EPOLLOUT:EPOLLIN));assert(survived>0);operations->retire(token);close(pair[0]);close(pair[1]);};
  for(int source_kind=0;source_kind<4;source_kind++){directional_case(true,source_kind);directional_case(false,source_kind);}assert(operations->shutdown());return 0;
}
