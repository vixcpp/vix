#include "TcpLoopbackFixture.hpp"
#include "CoroutineTask.hpp"
#include <vix/network/Address.hpp>
#include <vix/network/Endpoint.hpp>
#include <vix/network/Errors.hpp>
#include <vix/network/Provider.hpp>
#include <vix/network/TcpConnection.hpp>
#include "Engine.hpp"
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

using namespace vix::network;
using namespace vix::network::detail;
template<class T> struct Seen { std::mutex mutex;std::optional<vix::Completion<T>> value;std::atomic<bool> done{}; };
template<class T> struct Task { struct promise_type { std::mutex final_mutex;std::condition_variable final_cv;bool final_suspended{};Task get_return_object(){return Task{std::coroutine_handle<promise_type>::from_promise(*this)};}std::suspend_never initial_suspend()noexcept{return{};}vix::network::test_detail::FinalSuspendAwaiter<promise_type> final_suspend()noexcept{return{};}void return_void()noexcept{}void unhandled_exception()noexcept{std::terminate();} };std::coroutine_handle<promise_type> h;~Task(){vix::network::test_detail::destroy_after_final_suspend(h);} };
template<class T> Task<T> await(vix::network::Operation<T> operation,std::shared_ptr<Seen<T>> seen){auto completion=co_await std::move(operation);{std::lock_guard lock(seen->mutex);seen->value.emplace(std::move(completion));}seen->done=true;}
template<class T> static bool done_for(const std::shared_ptr<Seen<T>>& seen,std::chrono::milliseconds timeout){auto end=std::chrono::steady_clock::now()+timeout;while(!seen->done&&std::chrono::steady_clock::now()<end)std::this_thread::sleep_for(std::chrono::milliseconds(1));return seen->done;}
template<class T> static bool pending_for(const std::shared_ptr<Seen<T>>& seen,std::chrono::milliseconds duration){auto end=std::chrono::steady_clock::now()+duration;while(std::chrono::steady_clock::now()<end){if(seen->done)return false;std::this_thread::sleep_for(std::chrono::milliseconds(1));}return !seen->done;}
static TcpConnection connect_loopback(TcpLoopbackFixture& fixture,Provider& provider){auto address=Address::parse("127.0.0.1");assert(address);auto endpoint=Endpoint::create(address.take(),fixture.port);assert(endpoint);auto seen=std::make_shared<Seen<TcpConnection>>();auto task=await(provider.connect(endpoint.take()),seen);assert(done_for(seen,std::chrono::milliseconds(500)));fixture.accept_peer();std::lock_guard lock(seen->mutex);assert(seen->value->status()==vix::CompletionStatus::Success);return seen->value->take();}
static void write_all(int fd,std::span<const std::byte> bytes){std::size_t at{};while(at<bytes.size()){auto n=::write(fd,bytes.data()+at,bytes.size()-at);assert(n>0);at+=std::size_t(n);}}
static void read_exact(int fd,std::span<std::byte> bytes){std::size_t at{};while(at<bytes.size()){auto n=::read(fd,bytes.data()+at,bytes.size()-at);assert(n>0);at+=std::size_t(n);}}
static void constrain_send_buffer(TcpConnection& c){int size=4096;assert(setsockopt(NetworkAccess::native_fd_for_test(c),SOL_SOCKET,SO_SNDBUF,&size,sizeof size)==0);}
int main()
{
  // Success, idempotence, local send rejection, peer FIN/EOF, and retained read use.
  {TcpLoopbackFixture fixture;auto made=Provider::create();assert(made);auto provider=made.take();auto c=connect_loopback(fixture,provider);const std::array<std::byte,3> a{std::byte{'a'},std::byte{'b'},std::byte{'c'}};auto sent=std::make_shared<Seen<void>>();auto task=await(c.send(a),sent);assert(done_for(sent,std::chrono::milliseconds(500)));assert(c.shutdown_write());assert(c.shutdown_write());auto later=std::make_shared<Seen<void>>();auto later_task=await(c.send(a),later);assert(done_for(later,std::chrono::milliseconds(500)));{std::lock_guard lock(later->mutex);assert(later->value->status()==vix::CompletionStatus::Failure&&later->value->error().identity()==write_shutdown);}auto empty=std::make_shared<Seen<void>>();auto empty_task=await(c.send({}),empty);assert(done_for(empty,std::chrono::milliseconds(500)));{std::lock_guard lock(empty->mutex);assert(empty->value->status()==vix::CompletionStatus::Failure&&empty->value->error().identity()==write_shutdown);}std::array<std::byte,3> got{};read_exact(fixture.peer,got);assert(got==a);std::byte eof{};assert(::read(fixture.peer,&eof,1)==0);const std::array<std::byte,2> b{std::byte{'o'},std::byte{'k'}};write_all(fixture.peer,b);std::array<std::byte,2> received{};auto read=std::make_shared<Seen<std::size_t>>();auto read_task=await(c.receive(received),read);assert(done_for(read,std::chrono::milliseconds(500)));{std::lock_guard lock(read->mutex);assert(read->value->status()==vix::CompletionStatus::Success&&read->value->take()==b.size());}assert(received==b);assert(c.close());assert(provider.shutdown());auto closed=c.shutdown_write();assert(!closed&&closed.error().identity()==peer_closed);}

  // A pending read survives local write shutdown.
  {TcpLoopbackFixture fixture;auto made=Provider::create();assert(made);auto provider=made.take();auto c=connect_loopback(fixture,provider);std::array<std::byte,2> bytes{};auto read=std::make_shared<Seen<std::size_t>>();auto task=await(c.receive(bytes),read);assert(c.shutdown_write());const std::array<std::byte,2> reply{std::byte{'r'},std::byte{'x'}};write_all(fixture.peer,reply);assert(done_for(read,std::chrono::milliseconds(500)));{std::lock_guard lock(read->mutex);assert(read->value->status()==vix::CompletionStatus::Success&&read->value->take()==reply.size());}assert(bytes==reply);assert(c.close());assert(provider.shutdown());}

  // A genuine active complete-send blocks half-close, then permits it after draining.
  {TcpLoopbackFixture fixture;auto made=Provider::create();assert(made);auto provider=made.take();auto c=connect_loopback(fixture,provider);constrain_send_buffer(c);std::vector<std::byte> payload(4*1024*1024,std::byte{0x5a});auto sent=std::make_shared<Seen<void>>();auto task=await(c.send(payload),sent);assert(pending_for(sent,std::chrono::milliseconds(20)));auto busy=c.shutdown_write();assert(!busy&&busy.error().identity()==operation_in_progress);std::thread drain([&]{std::vector<std::byte> got(payload.size());read_exact(fixture.peer,got);assert(got==payload);});assert(done_for(sent,std::chrono::seconds(5)));drain.join();assert(c.shutdown_write());std::byte eof{};assert(::read(fixture.peer,&eof,1)==0);assert(c.close());assert(provider.shutdown());}
}
