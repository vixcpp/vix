#include <vix/network/Address.hpp>
#include <vix/network/Endpoint.hpp>
#include <vix/network/Errors.hpp>
#include <vix/network/Provider.hpp>
#include <vix/network/TcpConnection.hpp>
#include <vix/network/TcpListener.hpp>
#include "CoroutineTask.hpp"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <spawn.h>
#include <fcntl.h>
#include <unistd.h>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstring>
#include <cstdio>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>

using namespace vix::network;
extern char **environ;

template<class T> struct Seen { std::mutex mutex; std::optional<vix::Completion<T>> value; std::atomic<bool> done{}; };
template<class T> struct Task
{
  struct promise_type
  {
    std::mutex final_mutex;
    std::condition_variable final_cv;
    bool final_suspended{};
    Task get_return_object() { return Task{std::coroutine_handle<promise_type>::from_promise(*this)}; }
    std::suspend_never initial_suspend() noexcept { return {}; }
    vix::network::test_detail::FinalSuspendAwaiter<promise_type> final_suspend() noexcept { return {}; }
    void return_void() noexcept {}
    void unhandled_exception() noexcept { std::terminate(); }
  };
  std::coroutine_handle<promise_type> h;
  ~Task() { vix::network::test_detail::destroy_after_final_suspend(h); }
};
template<class T> Task<T> await(Operation<T> operation, std::shared_ptr<Seen<T>> seen)
{
  auto completion = co_await std::move(operation);
  { std::lock_guard lock(seen->mutex); seen->value.emplace(std::move(completion)); }
  seen->done = true;
}
template<class T> bool done(const std::shared_ptr<Seen<T>> &seen, std::chrono::milliseconds timeout = std::chrono::milliseconds(500))
{
  const auto end = std::chrono::steady_clock::now() + timeout;
  while (!seen->done && std::chrono::steady_clock::now() < end) std::this_thread::sleep_for(std::chrono::milliseconds(1));
  return seen->done;
}
static Endpoint loopback(std::uint16_t port = 0)
{
  auto address = Address::parse("127.0.0.1"); assert(address);
  auto endpoint = Endpoint::create(address.take(), port); assert(endpoint);
  return endpoint.take();
}
static int connect_peer(const Endpoint &endpoint)
{
  int fd = ::socket(AF_INET, SOCK_STREAM, 0); assert(fd >= 0);
  assert(::fcntl(fd, F_SETFD, FD_CLOEXEC) == 0);
  sockaddr_in address{}; address.sin_family = AF_INET; address.sin_port = htons(endpoint.port());
  address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  assert(::connect(fd, reinterpret_cast<sockaddr *>(&address), sizeof address) == 0);
  return fd;
}
static int connect_peer_v6(const Endpoint &endpoint)
{
  int fd = ::socket(AF_INET6, SOCK_STREAM, 0); assert(fd >= 0);
  sockaddr_in6 address{}; address.sin6_family = AF_INET6; address.sin6_port = htons(endpoint.port());
  address.sin6_addr = in6addr_loopback;
  assert(::connect(fd, reinterpret_cast<sockaddr *>(&address), sizeof address) == 0);
  return fd;
}
static TcpConnection take_connection(const std::shared_ptr<Seen<TcpConnection>> &seen)
{
  std::lock_guard lock(seen->mutex);
  assert(seen->value->status() == vix::CompletionStatus::Success);
  return seen->value->take();
}
static int overlap_child()
{
  std::set_terminate([] { _Exit(42); });
  auto made = Provider::create(); if (!made) return 43;
  auto provider = made.take(); auto listener = provider.listen(loopback()); if (!listener) return 44;
  auto value = listener.take(); auto first = std::make_shared<Seen<TcpConnection>>();
  auto first_task = await(value.accept(), first);
  auto second = value.accept(); auto second_task = await(std::move(second), std::make_shared<Seen<TcpConnection>>());
  return 45;
}
static int spawn_overlap(const char *self)
{
  pid_t child{}; char *const args[] = {const_cast<char *>(self), const_cast<char *>("--overlap"), nullptr};
  assert(posix_spawn(&child, self, nullptr, nullptr, args, environ) == 0);
  int status{}; assert(waitpid(child, &status, 0) == child); return status;
}

int main(int argc, char **argv)
{
  if (argc == 2 && std::strcmp(argv[1], "--overlap") == 0) return overlap_child();

  auto made = Provider::create(); assert(made); auto provider = made.take();
  auto listener_result = provider.listen(loopback());
  if (!listener_result) return 77; // loopback networking is unavailable in this sandbox.
  auto listener = listener_result.take();
  auto local = listener.local_endpoint(); assert(local && local.value().port() != 0);
  assert(!provider.shutdown() && provider.shutdown().error().identity() == provider_in_use);

  // A native peer reaches the listener through reactor READ readiness.
  auto accepted = std::make_shared<Seen<TcpConnection>>();
  auto accepted_task = await(listener.accept(), accepted);
  assert(!done(accepted, std::chrono::milliseconds(20)));
  int peer = connect_peer(local.value());
  assert(done(accepted));
  auto connection = take_connection(accepted);
  const std::array<std::byte,3> inbound{std::byte{'i'},std::byte{'n'},std::byte{'!'}};
  assert(::write(peer, inbound.data(), inbound.size()) == static_cast<ssize_t>(inbound.size()));
  std::array<std::byte,3> received{};
  auto read = std::make_shared<Seen<std::size_t>>(); auto read_task = await(connection.receive(received), read);
  assert(done(read)); { std::lock_guard lock(read->mutex); assert(read->value->status() == vix::CompletionStatus::Success && read->value->take() == received.size()); }
  assert(received == inbound);
  const std::array<std::byte,3> outbound{std::byte{'o'},std::byte{'u'},std::byte{'t'}};
  auto sent = std::make_shared<Seen<void>>(); auto sent_task = await(connection.send(outbound), sent); assert(done(sent));
  std::array<std::byte,3> peer_received{}; assert(::read(peer, peer_received.data(), peer_received.size()) == static_cast<ssize_t>(peer_received.size())); assert(peer_received == outbound);
  assert(connection.shutdown_write()); std::byte eof{}; assert(::read(peer, &eof, 1) == 0);
  auto peer_closed_accept = std::make_shared<Seen<TcpConnection>>();
  auto peer_closed_task = await(listener.accept(), peer_closed_accept);
  int peer_closed_fd = connect_peer(local.value()); assert(done(peer_closed_accept));
  auto peer_closed_connection = take_connection(peer_closed_accept);
  assert(::shutdown(peer_closed_fd, SHUT_WR) == 0);
  std::byte peer_closed_byte{};
  auto peer_closed_read = std::make_shared<Seen<std::size_t>>();
  auto peer_closed_read_task = await(peer_closed_connection.receive(std::span<std::byte>{&peer_closed_byte, 1}), peer_closed_read);
  assert(done(peer_closed_read));
  { std::lock_guard lock(peer_closed_read->mutex); assert(peer_closed_read->value->status() == vix::CompletionStatus::Success && peer_closed_read->value->take() == 0); }
  assert(peer_closed_connection.close()); ::close(peer_closed_fd);
  assert(listener.close()); // accepted resource is independent of the listener.
  auto held_by_connection = provider.shutdown();
  assert(!held_by_connection && held_by_connection.error().identity() == provider_in_use);
  assert(connection.close()); ::close(peer);
  assert(provider.shutdown());

  // Cancellation, timeout, and stale queued progression all leave the listener reusable.
  auto made_reuse = Provider::create(); assert(made_reuse); auto reuse_provider = made_reuse.take();
  auto reusable_result = reuse_provider.listen(loopback()); assert(reusable_result); auto reusable = reusable_result.take();
  auto reusable_endpoint = reusable.local_endpoint(); assert(reusable_endpoint);
  vix::CancellationSource source;
  auto cancelled = std::make_shared<Seen<TcpConnection>>(); auto cancelled_task = await(reusable.accept({source.token(), {}}), cancelled);
  assert(source.request_cancel()); assert(done(cancelled));
  { std::lock_guard lock(cancelled->mutex); assert(cancelled->value->status() == vix::CompletionStatus::Cancelled); }
  int stale_peer = connect_peer(reusable_endpoint.value());
  auto after_cancel = std::make_shared<Seen<TcpConnection>>(); auto after_cancel_task = await(reusable.accept(), after_cancel);
  assert(done(after_cancel)); auto stale_connection = take_connection(after_cancel);
  assert(stale_connection.close()); ::close(stale_peer);

  auto expired = std::make_shared<Seen<TcpConnection>>();
  auto expired_task = await(reusable.accept({{}, vix::Deadline::after(vix::Duration::milliseconds(10))}), expired);
  assert(done(expired)); { std::lock_guard lock(expired->mutex); assert(expired->value->status() == vix::CompletionStatus::Failure && expired->value->error().identity() == timed_out); }
  int deadline_peer = connect_peer(reusable_endpoint.value());
  auto after_deadline = std::make_shared<Seen<TcpConnection>>(); auto after_deadline_task = await(reusable.accept(), after_deadline);
  assert(done(after_deadline)); auto deadline_connection = take_connection(after_deadline);
  assert(deadline_connection.close()); ::close(deadline_peer);

  auto active = std::make_shared<Seen<TcpConnection>>(); auto active_task = await(reusable.accept(), active);
  auto close_busy = reusable.close(); assert(!close_busy && close_busy.error().identity() == operation_in_progress);
  assert(!source.request_cancel()); // already cancelled; this must not affect the new operation.
  vix::CancellationSource final_cancel; // use a deadline to finish the active operation.
  (void)final_cancel;
  // The active operation was deliberately left pending only long enough to validate close.
  int final_peer = connect_peer(reusable_endpoint.value()); assert(done(active));
  auto final_connection = take_connection(active);
  assert(final_connection.close()); ::close(final_peer);
  assert(reusable.close());
  auto after_close = std::make_shared<Seen<TcpConnection>>(); auto after_close_task = await(reusable.accept(), after_close); assert(done(after_close));
  { std::lock_guard lock(after_close->mutex); assert(after_close->value->status() == vix::CompletionStatus::Failure && after_close->value->error().identity() == peer_closed); }
  assert(reuse_provider.shutdown());

  auto collision_provider_result = Provider::create(); assert(collision_provider_result); auto collision_provider = collision_provider_result.take();
  auto first_listener_result = collision_provider.listen(loopback()); assert(first_listener_result); auto first_listener = first_listener_result.take();
  auto first_endpoint = first_listener.local_endpoint(); assert(first_endpoint);
  auto collision = collision_provider.listen(first_endpoint.value()); assert(!collision && collision.error().identity() == address_in_use);
  assert(first_listener.close()); assert(collision_provider.shutdown());

  // Refusal is reported through the same Standard error contract on every
  // readiness backend (the closed ephemeral port cannot accept the connect).
  auto refused_listener_result = Provider::create(); assert(refused_listener_result);
  auto refused_provider = refused_listener_result.take();
  auto refused_listener = refused_provider.listen(loopback()); assert(refused_listener);
  auto refused_endpoint = refused_listener.value().local_endpoint(); assert(refused_endpoint);
  assert(refused_listener.value().close());
  auto refused = std::make_shared<Seen<TcpConnection>>();
  auto refused_task = await(refused_provider.connect(refused_endpoint.value()), refused);
  assert(done(refused));
  { std::lock_guard lock(refused->mutex); assert(refused->value->status() == vix::CompletionStatus::Failure && refused->value->error().identity() == connection_refused); }
  assert(refused_provider.shutdown());

  // Exercise IPv6 TCP when loopback is enabled by the host. A failed listen
  // denotes an unavailable host capability; once it succeeds, transport
  // failures are test failures rather than skips.
  auto ipv6_address = Address::parse("::1"); assert(ipv6_address);
  auto ipv6_endpoint = Endpoint::create(ipv6_address.take(), 0); assert(ipv6_endpoint);
  auto ipv6_provider_result = Provider::create(); assert(ipv6_provider_result);
  auto ipv6_provider = ipv6_provider_result.take();
  auto ipv6_listener_result = ipv6_provider.listen(ipv6_endpoint.value());
  if (ipv6_listener_result)
  {
    auto ipv6_listener = ipv6_listener_result.take();
    auto ipv6_local = ipv6_listener.local_endpoint(); assert(ipv6_local);
    auto ipv6_accepted = std::make_shared<Seen<TcpConnection>>();
    auto ipv6_accept_task = await(ipv6_listener.accept(), ipv6_accepted);
    int ipv6_peer = connect_peer_v6(ipv6_local.value());
    assert(done(ipv6_accepted));
    auto ipv6_connection = take_connection(ipv6_accepted);
    assert(ipv6_connection.close()); ::close(ipv6_peer);
    assert(ipv6_listener.close());
  }
  else
  {
    const auto identity = ipv6_listener_result.error().identity();
    assert(identity == unsupported || identity == address_unavailable);
    std::fprintf(stderr, "IPv6 loopback unavailable on this host; IPv6 transport not exercised\n");
  }
  assert(ipv6_provider.shutdown());

  const auto overlap = spawn_overlap(argv[0]); assert(WIFEXITED(overlap) && WEXITSTATUS(overlap) == 42);
}
