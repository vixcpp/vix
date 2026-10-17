#include "Engine.hpp"
#include "CoroutineTask.hpp"
#include <vix/network/Address.hpp>
#include <vix/network/Provider.hpp>
#include <vix/network/TcpConnection.hpp>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>

using namespace vix::network;
using namespace vix::network::detail;

static NativeSocket native_socket_from_valid_descriptor(int descriptor)
{
  if(descriptor<0)std::abort();
  return static_cast<NativeSocket>(descriptor);
}

template<class T> struct Observation { std::mutex mutex;std::optional<vix::Completion<T>> completion;std::atomic<bool> done{}; };
template<class T> struct Task { struct promise_type { std::shared_ptr<vix::network::test_detail::FinalSuspendState> final_suspend_state=std::make_shared<vix::network::test_detail::FinalSuspendState>();Task get_return_object(){return Task{std::coroutine_handle<promise_type>::from_promise(*this)};}std::suspend_never initial_suspend()noexcept{return{};}vix::network::test_detail::FinalSuspendAwaiter<promise_type> final_suspend()noexcept{return{};}void return_void()noexcept{}void unhandled_exception()noexcept{std::terminate();} };std::coroutine_handle<promise_type> h;explicit Task(std::coroutine_handle<promise_type> value):h(value){}~Task(){vix::network::test_detail::destroy_after_final_suspend(h);}Task(Task&&o)noexcept:h(std::exchange(o.h,{})){}Task(const Task&)=delete;};
template<class T> Task<T> await_connect(vix::network::Operation<T> op,std::shared_ptr<Observation<T>> observation){auto completion=co_await std::move(op);{std::lock_guard lock(observation->mutex);observation->completion.emplace(std::move(completion));}observation->done=true;}
template<class T> static bool wait(const std::shared_ptr<Observation<T>>&o){for(int i=0;i<500&&!o->done;i++)std::this_thread::sleep_for(std::chrono::milliseconds(1));return o->done;}

int main()
{
  // Host-only real transport target.  It deliberately requires AF_INET;
  // EPERM from a restricted sandbox is a host-policy failure, not a skip.
  int listener=::socket(AF_INET,SOCK_STREAM|SOCK_CLOEXEC,0);assert(listener>=0);
  sockaddr_in native{};native.sin_family=AF_INET;native.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
  assert(::bind(listener,reinterpret_cast<sockaddr*>(&native),sizeof native)==0);assert(::listen(listener,1)==0);socklen_t size=sizeof native;assert(::getsockname(listener,reinterpret_cast<sockaddr*>(&native),&size)==0);
  auto parsed=Address::parse("127.0.0.1");assert(parsed);auto endpoint=Endpoint::create(parsed.take(),ntohs(native.sin_port));assert(endpoint);
  auto made=Provider::create();assert(made);auto provider=made.take();auto core=CandidateAccess::core_for_test(provider);auto observation=std::make_shared<Observation<TcpConnection>>();auto task=await_connect(provider.connect(endpoint.take()),observation);assert(wait(observation));int peer=::accept4(listener,nullptr,nullptr,SOCK_CLOEXEC);assert(peer>=0);
  {std::lock_guard lock(observation->mutex);assert(observation->completion->status()==vix::CompletionStatus::Success);auto connection=observation->completion->take();assert(core->active_operations_for_test()==0&&core->active_resources_for_test()==1);assert(!provider.shutdown());assert(connection.close());assert(core->active_resources_for_test()==0);}
  ::close(peer);::close(listener);assert(provider.shutdown());

  // A port reserved then released locally has no listener; Linux reports the
  // connection failure through SO_ERROR after the writable notification.
  int reserved=::socket(AF_INET,SOCK_STREAM|SOCK_CLOEXEC,0);assert(reserved>=0);sockaddr_in refused{};refused.sin_family=AF_INET;refused.sin_addr.s_addr=htonl(INADDR_LOOPBACK);assert(::bind(reserved,reinterpret_cast<sockaddr*>(&refused),sizeof refused)==0);size=sizeof refused;assert(::getsockname(reserved,reinterpret_cast<sockaddr*>(&refused),&size)==0);const auto refused_port=ntohs(refused.sin_port);::close(reserved);
  auto refused_address=Address::parse("127.0.0.1");assert(refused_address);auto refused_endpoint=Endpoint::create(refused_address.take(),refused_port);assert(refused_endpoint);auto refused_provider_result=Provider::create();assert(refused_provider_result);auto refused_provider=refused_provider_result.take();auto refused_observation=std::make_shared<Observation<TcpConnection>>();auto refused_task=await_connect(refused_provider.connect(refused_endpoint.take()),refused_observation);assert(wait(refused_observation));{std::lock_guard lock(refused_observation->mutex);assert(refused_observation->completion->status()==vix::CompletionStatus::Failure);assert(refused_observation->completion->error().identity()==connection_refused);}assert(refused_provider.shutdown());

  auto zero_address=Address::parse("127.0.0.1");assert(zero_address);auto zero_endpoint=Endpoint::create(zero_address.take(),0);assert(zero_endpoint);auto zero_provider_result=Provider::create();assert(zero_provider_result);auto zero_provider=zero_provider_result.take();auto zero_observation=std::make_shared<Observation<TcpConnection>>();auto zero_task=await_connect(zero_provider.connect(zero_endpoint.take()),zero_observation);assert(wait(zero_observation));{std::lock_guard lock(zero_observation->mutex);assert(zero_observation->completion->status()==vix::CompletionStatus::Failure&&zero_observation->completion->error().identity()==invalid_port);}assert(zero_provider.shutdown());

  // The connect write slot uses the ordinary descriptor token.  Retiring A
  // before fd-number reuse means a late writable event carrying A's token can
  // never dispatch B's completion handler.
  auto stale_made=ProviderCore::create();assert(stale_made);auto stale_provider=stale_made.take();int stale_pair[2];assert(pipe(stale_pair)==0);std::atomic<int>old_write{};auto old_connect=stale_provider->register_descriptor(native_socket_from_valid_descriptor(stale_pair[0]),{},[&](std::uint32_t){++old_write;return false;});assert(old_connect);const int old_fd=stale_pair[0];stale_provider->retire(old_connect);close(stale_pair[0]);close(stale_pair[1]);int fresh_pair[2];assert(pipe(fresh_pair)==0);const int new_fd=fresh_pair[0];std::atomic<int>new_write{};auto new_connect=stale_provider->register_descriptor(native_socket_from_valid_descriptor(new_fd),{},[&](std::uint32_t){++new_write;return false;});assert(new_connect);assert(old_fd==new_fd);assert(old_connect!=new_connect);assert(!stale_provider->dispatch_for_test(old_connect,reactor_writable));assert(new_write==0);assert(stale_provider->dispatch_for_test(new_connect,reactor_writable)&&new_write==1);stale_provider->retire(new_connect);close(fresh_pair[0]);close(fresh_pair[1]);assert(stale_provider->shutdown());

  // IPv6 is a genuine host capability check.  Only unavailable/disabled
  // loopback is skipped; any other setup or connect error remains a failure.
  int listener6=::socket(AF_INET6,SOCK_STREAM|SOCK_CLOEXEC,0);
  if(listener6<0 && (errno==EAFNOSUPPORT||errno==EPROTONOSUPPORT)) std::fprintf(stderr,"IPv6 loopback: environment skipped\\n");
  else {assert(listener6>=0);sockaddr_in6 native6{};native6.sin6_family=AF_INET6;native6.sin6_addr=in6addr_loopback;if(::bind(listener6,reinterpret_cast<sockaddr*>(&native6),sizeof native6)!=0&&errno==EADDRNOTAVAIL){std::fprintf(stderr,"IPv6 loopback: environment skipped\\n");close(listener6);}else{assert(::listen(listener6,1)==0);socklen_t size6=sizeof native6;assert(::getsockname(listener6,reinterpret_cast<sockaddr*>(&native6),&size6)==0);auto address6=Address::parse("::1");assert(address6);auto endpoint6=Endpoint::create(address6.take(),ntohs(native6.sin6_port));assert(endpoint6);auto made6=Provider::create();assert(made6);auto provider6=made6.take();auto observation6=std::make_shared<Observation<TcpConnection>>();auto task6=await_connect(provider6.connect(endpoint6.take()),observation6);assert(wait(observation6));int peer6=accept4(listener6,nullptr,nullptr,SOCK_CLOEXEC);assert(peer6>=0);{std::lock_guard lock(observation6->mutex);assert(observation6->completion->status()==vix::CompletionStatus::Success);auto connection6=observation6->completion->take();assert(connection6.close());}close(peer6);close(listener6);assert(provider6.shutdown());}}

  // Deterministic connect-completion seam.  These records use the same
  // evaluator, terminal gate, fd handoff, and Resource promotion as connect;
  // holding evaluation makes native/cancel/deadline precedence reproducible.
  auto terminal=[](const auto &record){for(int i=0;i<200;i++){if(record->terminal_trace_for_test().size()==8)return true;std::this_thread::sleep_for(std::chrono::milliseconds(1));}return false;};
  auto seam_made=ProviderCore::create();assert(seam_made);auto seam=seam_made.take();
  auto success_builder=[](NativeSocket socket,ProviderResourceLease lease){return make_tcp_connection(socket,std::move(lease));};
  auto native_success=[&](bool cancel,bool deadline){int pair[2];assert(pipe(pair)==0);auto record=std::make_shared<OperationRecord<TcpConnection>>(seam);assert(record->adopt_native_for_test(native_socket_from_valid_descriptor(pair[0])));record->begin(std::noop_coroutine());assert(record->commit_suspend());record->hold_evaluation_for_test(true);record->publish_native_success_with(success_builder);if(cancel)record->publish_cancellation_for_test();if(deadline)record->publish_deadline_for_test();record->release_evaluation_for_test();assert(terminal(record));auto completion=record->take_completion();assert(completion.status()==vix::CompletionStatus::Success);auto connection=completion.take();assert(seam->active_operations_for_test()==0&&seam->active_resources_for_test()==1);record->publish_cancellation_for_test();record->publish_deadline_for_test();record->publish_native_for_test(vix::Completion<TcpConnection>{vix::Error{connection_refused,"late"}});assert(fcntl(CandidateAccess::native_fd_for_test(connection),F_GETFD)!=-1);assert(connection.close());assert(seam->active_resources_for_test()==0);close(pair[1]);};
  native_success(true,false);native_success(false,true);
  auto native_failure=[&](bool cancel,bool deadline){int pair[2];assert(pipe(pair)==0);auto record=std::make_shared<OperationRecord<TcpConnection>>(seam);assert(record->adopt_native_for_test(native_socket_from_valid_descriptor(pair[0])));record->begin(std::noop_coroutine());assert(record->commit_suspend());record->hold_evaluation_for_test(true);record->publish_native_for_test(vix::Completion<TcpConnection>{vix::Error{connection_refused,"connect"}});if(cancel)record->publish_cancellation_for_test();if(deadline)record->publish_deadline_for_test();record->release_evaluation_for_test();assert(terminal(record));auto completion=record->take_completion();assert(completion.status()==vix::CompletionStatus::Failure&&completion.error().identity()==connection_refused);assert(record->native_close_calls_for_test()==1&&seam->active_operations_for_test()==0&&seam->active_resources_for_test()==0);record->publish_native_success_with(success_builder);assert(seam->active_resources_for_test()==0);close(pair[1]);};
  native_failure(true,false);native_failure(false,true);
  auto non_native=[&](bool cancellation){int pair[2];assert(pipe(pair)==0);auto record=std::make_shared<OperationRecord<TcpConnection>>(seam);assert(record->adopt_native_for_test(native_socket_from_valid_descriptor(pair[0])));record->begin(std::noop_coroutine());assert(record->commit_suspend());record->hold_evaluation_for_test(true);if(cancellation)record->publish_cancellation_for_test();record->publish_deadline_for_test();record->release_evaluation_for_test();assert(terminal(record));auto completion=record->take_completion();assert(completion.status()==(cancellation?vix::CompletionStatus::Cancelled:vix::CompletionStatus::Failure));if(!cancellation)assert(completion.error().identity()==timed_out);assert(record->native_close_calls_for_test()==1&&seam->active_operations_for_test()==0&&seam->active_resources_for_test()==0);record->publish_native_success_with(success_builder);assert(seam->active_resources_for_test()==0);close(pair[1]);};
  non_native(true);non_native(false);assert(seam->shutdown());
  auto stopped=Provider::create();assert(stopped);auto stopped_provider=stopped.take();assert(stopped_provider.shutdown());auto stopped_address=Address::parse("127.0.0.1");assert(stopped_address);auto stopped_endpoint=Endpoint::create(stopped_address.take(),1);assert(stopped_endpoint);auto stopped_observation=std::make_shared<Observation<TcpConnection>>();auto stopped_task=await_connect(stopped_provider.connect(stopped_endpoint.take()),stopped_observation);assert(wait(stopped_observation));{std::lock_guard lock(stopped_observation->mutex);assert(stopped_observation->completion->status()==vix::CompletionStatus::Failure&&stopped_observation->completion->error().identity()==provider_stopped);}
}
