#include "Engine.hpp"
#include "CoroutineTask.hpp"
#include "TcpLoopbackFixture.hpp"
#include <vix/network/Address.hpp>
#include <vix/network/Endpoint.hpp>
#include <vix/network/Provider.hpp>
#include <vix/network/TcpConnection.hpp>
#include <sys/socket.h>
#include <sys/wait.h>
#include <spawn.h>
#include <fcntl.h>
#include <unistd.h>
#include <atomic>
#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <exception>
#include <cerrno>
#include <cstdio>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>
#if defined(__APPLE__) || (defined(__linux__) && defined(__GLIBC__))
#include <execinfo.h>
#endif

using namespace vix::network;
using namespace vix::network::detail;
extern char **environ;
static std::atomic<const char *> active_scenario{"test startup"};
static void set_scenario(const char *scenario) noexcept
{
  active_scenario.store(scenario, std::memory_order_relaxed);
}
static void write_diagnostic(const char *text, std::size_t remaining) noexcept
{
  while (remaining > 0)
  {
    const ssize_t n = ::write(STDERR_FILENO, text, remaining);

    if (n < 0 && errno == EINTR)
      continue;

    if (n <= 0)
      return;

    const auto written = static_cast<std::size_t>(n);
    text += written;
    remaining -= written;
  }
}

static void phase2b_terminate_handler() noexcept
{
  const char *scenario = active_scenario.load(std::memory_order_relaxed);

  constexpr char prefix[] =
      "vix_network_phase2b_test: terminate during scenario: ";

  write_diagnostic(prefix, sizeof(prefix) - 1);

  if (scenario)
    write_diagnostic(scenario, std::strlen(scenario));
  else
    write_diagnostic("unknown", sizeof("unknown") - 1);

  write_diagnostic("\n", 1);

#if defined(__APPLE__) || (defined(__linux__) && defined(__GLIBC__))
  void *frames[64]{};
  const int count = ::backtrace(frames, 64);

  if (count > 0)
    ::backtrace_symbols_fd(frames, count, STDERR_FILENO);
#endif

  std::fflush(stderr);
  std::abort();
}
template <class T>
struct Seen
{
  std::mutex mutex;
  std::optional<vix::Completion<T>> value;
  std::atomic<bool> done{};
};
template <class T>
struct Task
{
  struct promise_type
  {
    std::shared_ptr<vix::network::test_detail::FinalSuspendState> final_suspend_state = std::make_shared<vix::network::test_detail::FinalSuspendState>();
    Task get_return_object() { return Task{std::coroutine_handle<promise_type>::from_promise(*this)}; }
    std::suspend_never initial_suspend() noexcept { return {}; }
    vix::network::test_detail::FinalSuspendAwaiter<promise_type> final_suspend() noexcept { return {}; }
    void return_void() noexcept {}
    void unhandled_exception() noexcept { std::terminate(); }
  };
  std::coroutine_handle<promise_type> h;
  explicit Task(std::coroutine_handle<promise_type> handle) : h(handle) {}
  ~Task() { vix::network::test_detail::destroy_after_final_suspend(h); }
};
template <class T>
Task<T> await(vix::network::Operation<T> operation, std::shared_ptr<Seen<T>> seen)
{
  auto completion = co_await std::move(operation);
  {
    std::lock_guard lock(seen->mutex);
    seen->value.emplace(std::move(completion));
  }
  seen->done = true;
}
constexpr auto short_completion_timeout = std::chrono::milliseconds(500);
template <class T>
bool wait_done_for(const std::shared_ptr<Seen<T>> &seen, std::chrono::milliseconds timeout)
{
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  while (!seen->done && std::chrono::steady_clock::now() < deadline)
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  return seen->done;
}
template <class T>
bool stays_pending_for(const std::shared_ptr<Seen<T>> &seen, std::chrono::milliseconds duration)
{
  const auto deadline = std::chrono::steady_clock::now() + duration;
  while (std::chrono::steady_clock::now() < deadline)
  {
    if (seen->done)
      return false;
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return !seen->done;
}
template <class T>
bool wait(const std::shared_ptr<Seen<T>> &seen) { return wait_done_for(seen, short_completion_timeout); }
static TcpConnection connect_loopback(TcpLoopbackFixture &fixture, Provider &provider)
{
  auto address = Address::parse("127.0.0.1");
  assert(address);
  auto endpoint = Endpoint::create(address.take(), fixture.port);
  assert(endpoint);
  auto seen = std::make_shared<Seen<TcpConnection>>();
  auto task = await(provider.connect(endpoint.take()), seen);
  assert(wait_done_for(seen, short_completion_timeout));
  fixture.accept_peer();
  std::lock_guard lock(seen->mutex);
  assert(seen->value->status() == vix::CompletionStatus::Success);
  return seen->value->take();
}
static void write_all(int fd, std::span<const std::byte> bytes)
{
  std::size_t offset{};
  while (offset < bytes.size())
  {
    auto n = ::write(fd, bytes.data() + offset, bytes.size() - offset);
    assert(n > 0);
    offset += std::size_t(n);
  }
}
static std::vector<std::byte> large_payload() { return std::vector<std::byte>(4 * 1024 * 1024, std::byte{0x5a}); }
static void constrain_send_buffer(TcpConnection &connection)
{
  int size = 4096;
  assert(::setsockopt(NetworkAccess::native_fd_for_test(connection), SOL_SOCKET, SO_SNDBUF, &size, sizeof size) == 0);
}
static void drain_exact(int fd, std::span<const std::byte> expected)
{
  std::vector<std::byte> got(expected.size());
  std::size_t at{};
  while (at < got.size())
  {
    auto n = ::read(fd, got.data() + at, got.size() - at);
    assert(n > 0);
    at += std::size_t(n);
  }
  assert(std::equal(got.begin(), got.end(), expected.begin()));
}
static void drain_available(int fd)
{
  int flags = fcntl(fd, F_GETFL);
  assert(flags >= 0 && fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0);
  std::array<std::byte, 8192> bytes{};
  while (::read(fd, bytes.data(), bytes.size()) > 0)
  {
  }
  assert(errno == EAGAIN || errno == EWOULDBLOCK);
  assert(fcntl(fd, F_SETFL, flags) == 0);
}
template <class T>
static void assert_one_terminal_winner(const std::shared_ptr<OperationRecord<T>> &record)
{
  const auto trace = record->terminal_trace_for_test();
  assert(trace.size() == 8);
  for (auto event : {OperationRecord<T>::TerminalEvent::readiness_revoked, OperationRecord<T>::TerminalEvent::deadline_invalidated, OperationRecord<T>::TerminalEvent::slot_released, OperationRecord<T>::TerminalEvent::cancellation_released, OperationRecord<T>::TerminalEvent::borrow_revoked, OperationRecord<T>::TerminalEvent::completion_stored, OperationRecord<T>::TerminalEvent::provider_released, OperationRecord<T>::TerminalEvent::resume_queued})
    assert(std::count(trace.begin(), trace.end(), event) == 1);
  assert(!record->native_access_allowed_for_test());
}
static int run_fault_child(bool write)
{
  std::set_terminate([]
                     { _Exit(42); });
  TcpLoopbackFixture fixture;
  auto made = Provider::create();
  if (!made)
    return 43;
  auto provider = made.take();
  auto connection = connect_loopback(fixture, provider);
  if (write)
  {
    constrain_send_buffer(connection);
    auto payload = large_payload();
    auto first = std::make_shared<Seen<void>>();
    auto first_task = await(connection.send(payload), first);
    if (!stays_pending_for(first, std::chrono::milliseconds(20)))
      return 44;
    auto second = connection.send(std::span<const std::byte>{payload.data(), 1});
    auto second_task = await(std::move(second), std::make_shared<Seen<void>>());
  }
  else
  {
    std::byte bytes[1]{};
    auto first = std::make_shared<Seen<std::size_t>>();
    auto first_task = await(connection.receive(bytes), first);
    auto second = connection.receive(bytes);
    auto second_task = await(std::move(second), std::make_shared<Seen<std::size_t>>());
  }
  return 45;
}
static int run_sigpipe_child()
{
  TcpLoopbackFixture fixture;
  auto made = Provider::create();
  if (!made)
    return 43;
  auto provider = made.take();
  auto connection = connect_loopback(fixture, provider);
  linger reset{1, 0};
  setsockopt(fixture.peer, SOL_SOCKET, SO_LINGER, &reset, sizeof reset);
  close(fixture.peer);
  fixture.peer = -1;
  std::array<std::byte, 1> byte{std::byte{'x'}};
  auto sent = std::make_shared<Seen<void>>();
  auto task = await(connection.send(byte), sent);
  if (!wait_done_for(sent, short_completion_timeout))
    return 44;
  {
    std::lock_guard lock(sent->mutex);
    if (sent->value->status() != vix::CompletionStatus::Failure)
      return 45;
  }
  if (!connection.close())
    return 46;
  return provider.shutdown() ? 0 : 47;
}
static int spawn_child(const char *self, const char *mode)
{
  pid_t child{};
  char *const args[] = {const_cast<char *>(self), const_cast<char *>(mode), nullptr};
  assert(posix_spawn(&child, self, nullptr, nullptr, args, environ) == 0);
  int status{};
  assert(waitpid(child, &status, 0) == child);
  return status;
}
int main(int argc, char **argv)
{
  if (argc == 2 && std::strcmp(argv[1], "--child-fault-send-overlap") == 0)
    return run_fault_child(true);
  if (argc == 2 && std::strcmp(argv[1], "--child-fault-receive-overlap") == 0)
    return run_fault_child(false);
  if (argc == 2 && std::strcmp(argv[1], "--child-sigpipe-send") == 0)
    return run_sigpipe_child();
  std::set_terminate(phase2b_terminate_handler);
  set_scenario("errno mapping");
  // C2: private errno mapping is stable and never exposes errno as identity.
  struct Mapping
  {
    int native;
    vix::ErrorIdentity expected;
  };
  const Mapping mappings[] = {{ECONNRESET, connection_reset}, {ECONNABORTED, connection_aborted}, {EPIPE, peer_closed}, {ENOTCONN, peer_closed}, {ENETUNREACH, network_unreachable}, {EHOSTUNREACH, host_unreachable}, {ETIMEDOUT, timed_out}, {ENOMEM, resource_exhausted}, {ENOBUFS, resource_exhausted}, {EACCES, permission_denied}, {EPERM, permission_denied}, {EINVAL, io_failure}};
  for (const auto &mapping : mappings)
    assert(transport_error_for_test(mapping.native).identity() == mapping.expected);

  // C1: receive facts enter the same terminal gate as recv() progression.
  // Holding evaluation makes native > cancellation > deadline deterministic.
  set_scenario("terminal arbitration");
  auto precedence_core_result = ProviderCore::create();
  assert(precedence_core_result);
  auto precedence_core = precedence_core_result.take();
  auto terminal = [](const auto &record)
  {for(int n=0;n<200;++n){if(record->terminal_trace_for_test().size()==8)return true;std::this_thread::sleep_for(std::chrono::milliseconds(1));}return false; };
  auto precedence = [&](std::optional<vix::Completion<std::size_t>> native, bool cancel, bool deadline)
  {auto record=std::make_shared<OperationRecord<std::size_t>>(precedence_core);record->begin(std::noop_coroutine());assert(record->commit_suspend());record->hold_evaluation_for_test(true);if(native)record->publish_native_for_test(std::move(*native));if(cancel)record->publish_cancellation_for_test();if(deadline)record->publish_deadline_for_test();record->release_evaluation_for_test();assert(terminal(record));assert(precedence_core->active_operations_for_test()==0);return record->take_completion(); };
  // Native receive data and native EOF both beat Cancellation and Deadline.
  auto data = precedence(vix::Completion<std::size_t>{3}, true, true);
  assert(data.status() == vix::CompletionStatus::Success && data.take() == 3);
  auto held_eof = precedence(vix::Completion<std::size_t>{0}, true, true);
  assert(held_eof.status() == vix::CompletionStatus::Success && held_eof.take() == 0);
  auto failure = precedence(vix::Completion<std::size_t>{vix::Error{connection_reset, "recv"}}, true, true);
  assert(failure.status() == vix::CompletionStatus::Failure && failure.error().identity() == connection_reset);
  auto cancelled = precedence({}, true, true);
  assert(cancelled.status() == vix::CompletionStatus::Cancelled);
  auto expired = precedence({}, false, true);
  assert(expired.status() == vix::CompletionStatus::Failure && expired.error().identity() == timed_out);
  assert(precedence_core->shutdown());

  // H1: descriptor arm failure terminalizes after the connection mutex is
  // released.  The private force hook avoids timing or native socket I/O.
  auto arm_failure = [&](bool read)
  {
    int fds[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    auto made_h1 = Provider::create();
    assert(made_h1);
    auto h1_provider = made_h1.take();
    auto h1_core = NetworkAccess::core_for_test(h1_provider);
    auto h1_connection = NetworkAccess::adopt_tcp_for_test(h1_provider, fds[0]);
    NetworkAccess::fail_descriptor_arm_for_test(h1_connection, true);
    if (read)
    {
      std::byte byte{};
      auto seen = std::make_shared<Seen<std::size_t>>();
      auto task = await(h1_connection.receive(std::span<std::byte>{&byte, 1}), seen);
      assert(wait_done_for(seen, short_completion_timeout));
      std::lock_guard lock(seen->mutex);
      assert(seen->value->status() == vix::CompletionStatus::Failure && seen->value->error().identity() == io_failure);
    }
    else
    {
      const std::byte byte{};
      auto seen = std::make_shared<Seen<void>>();
      auto task = await(h1_connection.send(std::span<const std::byte>{&byte, 1}), seen);
      assert(wait_done_for(seen, short_completion_timeout));
      std::lock_guard lock(seen->mutex);
      assert(seen->value->status() == vix::CompletionStatus::Failure && seen->value->error().identity() == io_failure);
    }
    auto counters = NetworkAccess::transport_counters_for_test(h1_connection);
    assert(h1_core->active_operations_for_test() == 0);
    assert(read ? counters.read_slot_releases == 1 : counters.write_slot_releases == 1);
    assert(h1_connection.close());
    close(fds[1]);
    assert(h1_provider.shutdown());
  };
  set_scenario("descriptor arm failure: receive");
  arm_failure(true);
  set_scenario("descriptor arm failure: send");
  arm_failure(false);

  // H1: a connect-style one-shot write handler that declines rearm retires
  // its registration before the same fd can become a TcpConnection resource.
  set_scenario("descriptor registration handoff");
  {
    int fds[2];
    assert(pipe2(fds, O_NONBLOCK | O_CLOEXEC) == 0);
    auto made_handoff = Provider::create();
    assert(made_handoff);
    auto handoff_provider = made_handoff.take();
    auto handoff_core = NetworkAccess::core_for_test(handoff_provider);
    auto old = handoff_core->register_descriptor(fds[0], ReadinessHandler{}, ReadinessHandler{[](std::uint32_t)
                                                                                              { return false; }});
    assert(old);
    assert(handoff_core->dispatch_for_test(old, reactor_writable));
    assert(handoff_core->live_descriptor_registrations_for_test() == 0);
    auto replacement = handoff_core->register_descriptor(fds[0], ReadinessHandler{[](std::uint32_t)
                                                                                  { return false; }},
                                                         ReadinessHandler{});
    assert(replacement);
    handoff_core->retire(replacement);
    close(fds[0]);
    close(fds[1]);
    assert(handoff_provider.shutdown());
  }

  // H2: one-shot readiness retains and rearms a live WRITE direction after every
  // readiness dispatch.  The terminal WRITE callback removes only itself;
  // an independently active READ direction remains registered and rearmed.
  set_scenario("reactor rearming");
  {
    int fds[2];
    assert(pipe2(fds, O_NONBLOCK | O_CLOEXEC) == 0);
    auto made_rearm = Provider::create();
    assert(made_rearm);
    auto rearm_provider = made_rearm.take();
    auto rearm_core = NetworkAccess::core_for_test(rearm_provider);
    std::atomic<unsigned> reads{}, writes{};
    auto token = rearm_core->register_descriptor(fds[0], ReadinessHandler{[&](std::uint32_t)
                                                                          {++reads;return true; }},
                                                 ReadinessHandler{[&](std::uint32_t)
                                                                  { return ++writes < 3; }});
    assert(token);
    const auto before = rearm_core->reactor_control_count_for_test(ReactorControl::modify);
    assert(rearm_core->dispatch_for_test(token, reactor_writable));
    assert(writes == 1 && rearm_core->accepts(token));
    assert(rearm_core->reactor_control_count_for_test(ReactorControl::modify) == before + 1);
    assert(rearm_core->dispatch_for_test(token, reactor_writable));
    assert(writes == 2 && rearm_core->accepts(token));
    assert(rearm_core->reactor_control_count_for_test(ReactorControl::modify) == before + 2);
    assert(rearm_core->dispatch_for_test(token, reactor_writable));
    assert(writes == 3 && rearm_core->accepts(token));
    assert(rearm_core->reactor_control_count_for_test(ReactorControl::modify) == before + 3);
    assert(rearm_core->dispatch_for_test(token, reactor_readable));
    assert(reads == 1 && rearm_core->accepts(token));
    rearm_core->retire(token);
    close(fds[0]);
    close(fds[1]);
    assert(rearm_provider.shutdown());
  }

  // C3: hold the normal starter progression, terminalize through the normal
  // transport record, then inject the stale queued progression.  No test path
  // invokes recv/send: the counters therefore prove the post-terminal gate is
  // before both native I/O and the borrowed span access.
  set_scenario("stale send progression after terminal completion");
  {
    int fds[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    auto made_c3 = Provider::create();
    assert(made_c3);
    auto c3_provider = made_c3.take();
    auto c3_connection = NetworkAccess::adopt_tcp_for_test(c3_provider, fds[0]);
    NetworkAccess::hold_transport_progression_for_test(c3_connection, true);
    const std::array<std::byte, 3> bytes{std::byte{'c'}, std::byte{'3'}, std::byte{'!'}};
    auto c3_core = NetworkAccess::core_for_test(c3_provider);
    auto seen = std::make_shared<Seen<void>>();
    std::shared_ptr<OperationRecord<void>> record;
    NetworkAccess::capture_next_send_record_for_test(c3_connection, record);
    auto operation = c3_connection.send(bytes);
    assert(record);
    auto task = await(std::move(operation), seen);
    NetworkAccess::publish_send_for_test(c3_connection, vix::Completion<void>::success());
    assert(wait_done_for(seen, short_completion_timeout));
    assert_one_terminal_winner(record);
    auto before = NetworkAccess::transport_counters_for_test(c3_connection);
    assert(c3_core->active_operations_for_test() == 0);
    assert(before.native_send_attempts == 0 && before.write_buffer_accesses == 0 && before.write_slot_releases == 1 && before.write_offset == 0 && before.write_offset_at_terminal_release == bytes.size());
    assert(NetworkAccess::post_write_progression_for_test(c3_connection));
    for (int n = 0; n < 200 && NetworkAccess::transport_counters_for_test(c3_connection).write_progressions == before.write_progressions; ++n)
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    auto after = NetworkAccess::transport_counters_for_test(c3_connection);
    assert(after.write_progressions == before.write_progressions + 1);
    assert(after.native_send_attempts == before.native_send_attempts && after.write_buffer_accesses == before.write_buffer_accesses && after.write_offset == before.write_offset && after.write_offset_at_terminal_release == before.write_offset_at_terminal_release && after.write_slot_releases == before.write_slot_releases);
    assert(c3_core->active_operations_for_test() == 0);
    assert_one_terminal_winner(record);
    assert(c3_connection.close());
    close(fds[1]);
    assert(c3_provider.shutdown());
  }
  // Receive success, EOF, native failure, cancellation, and deadline all use
  // the same connection-owned READ terminal cleanup.  Their stale readiness
  // callbacks remain before recv() and before the borrowed destination span.
  auto c3_receive = [&](std::optional<vix::Completion<std::size_t>> native, std::optional<OperationOptions> options, std::function<void()> trigger = {})
  {
    int fds[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    auto made_c3 = Provider::create();
    assert(made_c3);
    auto c3_provider = made_c3.take();
    auto c3_core = NetworkAccess::core_for_test(c3_provider);
    auto c3_connection = NetworkAccess::adopt_tcp_for_test(c3_provider, fds[0]);
    NetworkAccess::hold_transport_progression_for_test(c3_connection, true);
    std::array<std::byte, 4> bytes{};
    auto seen = std::make_shared<Seen<std::size_t>>();
    std::shared_ptr<OperationRecord<std::size_t>> record;
    NetworkAccess::capture_next_receive_record_for_test(c3_connection, record);
    auto operation = c3_connection.receive(bytes, options.value_or(OperationOptions{}));
    assert(record);
    auto task = await(std::move(operation), seen);
    if (trigger)
      trigger();
    if (native)
      NetworkAccess::publish_receive_for_test(c3_connection, std::move(*native));
    assert(wait_done_for(seen, short_completion_timeout));
    assert_one_terminal_winner(record);
    auto before = NetworkAccess::transport_counters_for_test(c3_connection);
    assert(c3_core->active_operations_for_test() == 0);
    assert(before.native_recv_attempts == 0 && before.read_buffer_accesses == 0 && before.read_slot_releases == 1);
    assert(NetworkAccess::post_read_progression_for_test(c3_connection));
    for (int n = 0; n < 200 && NetworkAccess::transport_counters_for_test(c3_connection).read_progressions == before.read_progressions; ++n)
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    auto after = NetworkAccess::transport_counters_for_test(c3_connection);
    assert(after.read_progressions == before.read_progressions + 1);
    assert(after.native_recv_attempts == before.native_recv_attempts && after.read_buffer_accesses == before.read_buffer_accesses && after.read_slot_releases == before.read_slot_releases);
    assert(c3_core->active_operations_for_test() == 0);
    assert_one_terminal_winner(record);
    assert(c3_connection.close());
    close(fds[1]);
    assert(c3_provider.shutdown());
    return seen;
  };
  set_scenario("stale receive progression: data");
  {
    auto seen = c3_receive(vix::Completion<std::size_t>{2}, {});
    std::lock_guard lock(seen->mutex);
    assert(seen->value->status() == vix::CompletionStatus::Success && seen->value->take() == 2);
  }
  set_scenario("stale receive progression: EOF");
  {
    auto seen = c3_receive(vix::Completion<std::size_t>{0}, {});
    std::lock_guard lock(seen->mutex);
    assert(seen->value->status() == vix::CompletionStatus::Success && seen->value->take() == 0);
  }
  set_scenario("stale receive progression: native failure");
  {
    auto seen = c3_receive(vix::Completion<std::size_t>{vix::Error{connection_reset, "recv"}}, {});
    std::lock_guard lock(seen->mutex);
    assert(seen->value->status() == vix::CompletionStatus::Failure && seen->value->error().identity() == connection_reset);
  }
  set_scenario("stale receive progression: cancellation");
  {
    vix::CancellationSource source;
    auto options = OperationOptions{source.token(), {}};
    auto seen = c3_receive({}, options, [&]
                           { assert(source.request_cancel()); });
    std::lock_guard lock(seen->mutex);
    assert(seen->value->status() == vix::CompletionStatus::Cancelled);
  }
  set_scenario("stale receive progression: deadline");
  {
    auto options = OperationOptions{{}, vix::Deadline::after(vix::Duration::milliseconds(10))};
    auto seen = c3_receive({}, options);
    std::lock_guard lock(seen->mutex);
    assert(seen->value->status() == vix::CompletionStatus::Failure && seen->value->error().identity() == timed_out);
  }

  set_scenario("socketpair TCP receive");
  int pair[2];
  assert(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
  assert(fcntl(pair[0], F_SETFL, fcntl(pair[0], F_GETFL) | O_NONBLOCK) == 0);
  auto made = Provider::create();
  assert(made);
  auto provider = made.take();
  auto connection = NetworkAccess::adopt_tcp_for_test(provider, pair[0]);
  std::byte inbound[8]{};
  assert(::write(pair[1], "abc", 3) == 3);
  auto received = std::make_shared<Seen<std::size_t>>();
  auto receive_task = await(connection.receive(inbound), received);
  assert(wait(received));
  {
    std::lock_guard lock(received->mutex);
    assert(received->value->status() == vix::CompletionStatus::Success && received->value->take() == 3);
  }
  assert(inbound[0] == std::byte{'a'} && inbound[2] == std::byte{'c'});
  set_scenario("socketpair TCP send");
  const std::byte outbound[]{std::byte{'x'}, std::byte{'y'}};
  auto sent = std::make_shared<Seen<void>>();
  auto send_task = await(connection.send(outbound), sent);
  assert(wait(sent));
  {
    std::lock_guard lock(sent->mutex);
    if (sent->value->status() == vix::CompletionStatus::Failure && sent->value->error().identity() == permission_denied)
    {
      connection.close();
      close(pair[1]);
      provider.shutdown();
      return 77;
    }
    assert(sent->value->status() == vix::CompletionStatus::Success);
  }
  char peer[2]{};
  assert(::read(pair[1], peer, 2) == 2 && peer[0] == 'x' && peer[1] == 'y');
  set_scenario("socketpair empty receive/send");
  auto empty_receive = std::make_shared<Seen<std::size_t>>();
  auto empty_task = await(connection.receive({}), empty_receive);
  assert(wait(empty_receive));
  {
    std::lock_guard lock(empty_receive->mutex);
    assert(empty_receive->value->take() == 0);
  }
  auto empty_send = std::make_shared<Seen<void>>();
  auto empty_send_task = await(connection.send({}), empty_send);
  assert(wait(empty_send));
  set_scenario("socketpair EOF");
  ::shutdown(pair[1], SHUT_WR);
  std::byte eof_buffer[1]{};
  auto eof = std::make_shared<Seen<std::size_t>>();
  auto eof_task = await(connection.receive(eof_buffer), eof);
  assert(wait(eof));
  {
    std::lock_guard lock(eof->mutex);
    assert(eof->value->status() == vix::CompletionStatus::Success && eof->value->take() == 0);
  }
  assert(connection.close());
  close(pair[1]);
  assert(provider.shutdown());

  // real_tcp_receive: the peer sends before Vix performs its one logical receive.
  set_scenario("real TCP receive");
  {
    TcpLoopbackFixture fixture;
    auto made_real = Provider::create();
    assert(made_real);
    auto real_provider = made_real.take();
    auto real_connection = connect_loopback(fixture, real_provider);
    std::array<std::byte, 64> expected{};
    for (std::size_t i = 0; i < expected.size(); ++i)
      expected[i] = std::byte(i);
    write_all(fixture.peer, expected);
    std::array<std::byte, 64> actual{};
    auto seen = std::make_shared<Seen<std::size_t>>();
    auto task = await(real_connection.receive(actual), seen);
    assert(wait(seen));
    {
      std::lock_guard lock(seen->mutex);
      assert(seen->value->status() == vix::CompletionStatus::Success && seen->value->take() == expected.size());
    }
    assert(actual == expected);
    assert(real_connection.close());
    assert(real_provider.shutdown());
  }

  // real_tcp_send: the native peer accumulates rather than assuming one recv is enough.
  set_scenario("real TCP send");
  {
    TcpLoopbackFixture fixture;
    auto made_real = Provider::create();
    assert(made_real);
    auto real_provider = made_real.take();
    auto real_connection = connect_loopback(fixture, real_provider);
    std::array<std::byte, 257> expected{};
    for (std::size_t i = 0; i < expected.size(); ++i)
      expected[i] = std::byte(i);
    auto seen = std::make_shared<Seen<void>>();
    auto task = await(real_connection.send(expected), seen);
    assert(wait(seen));
    {
      std::lock_guard lock(seen->mutex);
      assert(seen->value->status() == vix::CompletionStatus::Success);
    }
    std::array<std::byte, 257> actual{};
    std::size_t offset{};
    while (offset < actual.size())
    {
      auto n = ::read(fixture.peer, actual.data() + offset, actual.size() - offset);
      assert(n > 0);
      offset += std::size_t(n);
    }
    assert(actual == expected);
    assert(real_connection.close());
    assert(real_provider.shutdown());
  }

  // Empty operations complete without peer activity or a persistent direction.
  set_scenario("real TCP empty receive/send");
  {
    TcpLoopbackFixture fixture;
    auto made_real = Provider::create();
    assert(made_real);
    auto real_provider = made_real.take();
    auto real_connection = connect_loopback(fixture, real_provider);
    std::span<std::byte> empty_receive_buffer;
    auto read = std::make_shared<Seen<std::size_t>>();
    auto read_task = await(real_connection.receive(empty_receive_buffer), read);
    assert(wait(read));
    {
      std::lock_guard lock(read->mutex);
      assert(read->value->status() == vix::CompletionStatus::Success && read->value->take() == 0);
    }
    std::span<const std::byte> empty_send_buffer;
    auto written = std::make_shared<Seen<void>>();
    auto write_task = await(real_connection.send(empty_send_buffer), written);
    assert(wait(written));
    {
      std::lock_guard lock(written->mutex);
      assert(written->value->status() == vix::CompletionStatus::Success);
    }
    assert(real_connection.close());
    assert(real_provider.shutdown());
  }

  // tcp_eof: orderly peer write shutdown is receive Success(0), not Failure.
  set_scenario("real TCP EOF");
  {
    TcpLoopbackFixture fixture;
    auto made_real = Provider::create();
    assert(made_real);
    auto real_provider = made_real.take();
    auto real_connection = connect_loopback(fixture, real_provider);
    assert(::shutdown(fixture.peer, SHUT_WR) == 0);
    std::byte byte{};
    auto eof_seen = std::make_shared<Seen<std::size_t>>();
    auto real_eof_task = await(real_connection.receive(std::span<std::byte>{&byte, 1}), eof_seen);
    assert(wait(eof_seen));
    {
      std::lock_guard lock(eof_seen->mutex);
      assert(eof_seen->value->status() == vix::CompletionStatus::Success && eof_seen->value->take() == 0);
    }
    assert(real_connection.close());
    assert(real_provider.shutdown());
  }

  // receive_cancellation_reuse: cancellation terminalizes only the borrow.
  set_scenario("receive cancellation and reuse");
  {
    TcpLoopbackFixture fixture;
    auto made_real = Provider::create();
    assert(made_real);
    auto real_provider = made_real.take();
    auto real_connection = connect_loopback(fixture, real_provider);
    vix::CancellationSource source;
    std::array<std::byte, 16> bytes{};
    auto receive_cancelled_seen = std::make_shared<Seen<std::size_t>>();
    auto cancelled_task = await(real_connection.receive(bytes, {source.token(), {}}), receive_cancelled_seen);
    assert(!real_provider.shutdown());
    assert(source.request_cancel());
    assert(wait(receive_cancelled_seen));
    {
      std::lock_guard lock(receive_cancelled_seen->mutex);
      assert(receive_cancelled_seen->value->status() == vix::CompletionStatus::Cancelled);
    }
    const std::array<std::byte, 5> payload{std::byte{'r'}, std::byte{'e'}, std::byte{'u'}, std::byte{'s'}, std::byte{'e'}};
    write_all(fixture.peer, payload);
    auto reused = std::make_shared<Seen<std::size_t>>();
    auto reused_task = await(real_connection.receive(bytes), reused);
    assert(wait(reused));
    {
      std::lock_guard lock(reused->mutex);
      assert(reused->value->status() == vix::CompletionStatus::Success && reused->value->take() == payload.size());
    }
    assert(std::equal(payload.begin(), payload.end(), bytes.begin()));
    assert(real_connection.close());
    assert(real_provider.shutdown());
  }

  // receive_deadline_reuse: timeout is Failure(timed_out), never connection close.
  set_scenario("receive deadline and reuse");
  {
    TcpLoopbackFixture fixture;
    auto made_real = Provider::create();
    assert(made_real);
    auto real_provider = made_real.take();
    auto real_connection = connect_loopback(fixture, real_provider);
    std::array<std::byte, 16> bytes{};
    auto receive_expired_seen = std::make_shared<Seen<std::size_t>>();
    auto expired_task = await(real_connection.receive(bytes, {{}, vix::Deadline::after(vix::Duration::milliseconds(10))}), receive_expired_seen);
    assert(!real_provider.shutdown());
    assert(wait(receive_expired_seen));
    {
      std::lock_guard lock(receive_expired_seen->mutex);
      assert(receive_expired_seen->value->status() == vix::CompletionStatus::Failure && receive_expired_seen->value->error().identity() == timed_out);
    }
    const std::array<std::byte, 5> payload{std::byte{'a'}, std::byte{'f'}, std::byte{'t'}, std::byte{'e'}, std::byte{'r'}};
    write_all(fixture.peer, payload);
    auto reused = std::make_shared<Seen<std::size_t>>();
    auto reused_task = await(real_connection.receive(bytes), reused);
    assert(wait(reused));
    {
      std::lock_guard lock(reused->mutex);
      assert(reused->value->status() == vix::CompletionStatus::Success && reused->value->take() == payload.size());
    }
    assert(std::equal(payload.begin(), payload.end(), bytes.begin()));
    assert(real_connection.close());
    assert(real_provider.shutdown());
  }

  // bidirectional_send_receive: READ and WRITE direction slots are independent.
  set_scenario("bidirectional TCP send and receive");
  {
    TcpLoopbackFixture fixture;
    auto made_real = Provider::create();
    assert(made_real);
    auto real_provider = made_real.take();
    auto real_connection = connect_loopback(fixture, real_provider);
    std::array<std::byte, 8> response{};
    auto read = std::make_shared<Seen<std::size_t>>();
    auto read_task = await(real_connection.receive(response), read);
    const std::array<std::byte, 4> request{std::byte{'p'}, std::byte{'i'}, std::byte{'n'}, std::byte{'g'}};
    auto written = std::make_shared<Seen<void>>();
    auto write_task = await(real_connection.send(request), written);
    std::array<std::byte, 4> peer_request{};
    for (std::size_t at{}; at < peer_request.size();)
    {
      auto n = ::read(fixture.peer, peer_request.data() + at, peer_request.size() - at);
      assert(n > 0);
      at += std::size_t(n);
    }
    assert(peer_request == request);
    const std::array<std::byte, 4> reply{std::byte{'p'}, std::byte{'o'}, std::byte{'n'}, std::byte{'g'}};
    write_all(fixture.peer, reply);
    assert(wait(read) && wait(written));
    {
      std::lock_guard lock(read->mutex);
      assert(read->value->status() == vix::CompletionStatus::Success && read->value->take() == reply.size());
    }
    assert(std::equal(reply.begin(), reply.end(), response.begin()));
    assert(real_connection.close());
    assert(real_provider.shutdown());
  }

  // close_during_receive and Provider accounting: operation and Resource hold distinct dependencies.
  set_scenario("close during receive and provider accounting");
  {
    TcpLoopbackFixture fixture;
    auto made_real = Provider::create();
    assert(made_real);
    auto real_provider = made_real.take();
    auto real_connection = connect_loopback(fixture, real_provider);
    vix::CancellationSource source;
    std::array<std::byte, 1> byte{};
    auto pending = std::make_shared<Seen<std::size_t>>();
    auto pending_task = await(real_connection.receive(byte, {source.token(), {}}), pending);
    auto close_busy = real_connection.close();
    assert(!close_busy && close_busy.error().identity() == operation_in_progress);
    auto pending_shutdown = real_provider.shutdown();
    assert(!pending_shutdown && pending_shutdown.error().identity() == provider_in_use);
    assert(source.request_cancel());
    assert(wait(pending));
    {
      std::lock_guard lock(pending->mutex);
      assert(pending->value->status() == vix::CompletionStatus::Cancelled);
    }
    auto live_shutdown = real_provider.shutdown();
    assert(!live_shutdown && live_shutdown.error().identity() == provider_in_use);
    assert(real_connection.close());
    assert(real_provider.shutdown());
  }

  // tcp_send_backpressure_complete: withholding peer reads leaves the large send pending.
  set_scenario("TCP send backpressure completion");
  {
    TcpLoopbackFixture fixture;
    auto made_real = Provider::create();
    assert(made_real);
    auto real_provider = made_real.take();
    auto real_connection = connect_loopback(fixture, real_provider);
    constrain_send_buffer(real_connection);
    auto payload = large_payload();
    auto large_send_seen = std::make_shared<Seen<void>>();
    auto task = await(real_connection.send(payload), large_send_seen);
    assert(stays_pending_for(large_send_seen, std::chrono::milliseconds(20)));
    std::thread drain([&]
                      { drain_exact(fixture.peer, payload); });
    assert(wait_done_for(large_send_seen, std::chrono::seconds(5)));
    drain.join();
    {
      std::lock_guard lock(large_send_seen->mutex);
      assert(large_send_seen->value->status() == vix::CompletionStatus::Success);
    }
    assert(real_connection.close());
    assert(real_provider.shutdown());
  }

  // tcp_send_cancel_backpressure, close arbitration, Provider accounting, and reuse.
  set_scenario("TCP send cancellation and backpressure reuse");
  {
    TcpLoopbackFixture fixture;
    auto made_real = Provider::create();
    assert(made_real);
    auto real_provider = made_real.take();
    auto real_connection = connect_loopback(fixture, real_provider);
    constrain_send_buffer(real_connection);
    auto payload = large_payload();
    vix::CancellationSource source;
    auto cancelled_send_seen = std::make_shared<Seen<void>>();
    auto task = await(real_connection.send(payload, {source.token(), {}}), cancelled_send_seen);
    assert(stays_pending_for(cancelled_send_seen, std::chrono::milliseconds(20)));
    auto close_busy = real_connection.close();
    assert(!close_busy && close_busy.error().identity() == operation_in_progress);
    auto pending_shutdown = real_provider.shutdown();
    assert(!pending_shutdown && pending_shutdown.error().identity() == provider_in_use);
    assert(source.request_cancel());
    assert(wait_done_for(cancelled_send_seen, short_completion_timeout));
    {
      std::lock_guard lock(cancelled_send_seen->mutex);
      assert(cancelled_send_seen->value->status() == vix::CompletionStatus::Cancelled);
    }
    drain_available(fixture.peer);
    const std::array<std::byte, 3> marker{std::byte{'o'}, std::byte{'k'}, std::byte{'!'}};
    auto marker_seen = std::make_shared<Seen<void>>();
    auto marker_task = await(real_connection.send(marker), marker_seen);
    assert(wait(marker_seen));
    drain_exact(fixture.peer, marker);
    auto live_shutdown = real_provider.shutdown();
    assert(!live_shutdown && live_shutdown.error().identity() == provider_in_use);
    assert(real_connection.close());
    assert(real_provider.shutdown());
  }

  // tcp_send_deadline_backpressure and same-connection marker reuse.
  set_scenario("TCP send deadline and backpressure reuse");
  {
    TcpLoopbackFixture fixture;
    auto made_real = Provider::create();
    assert(made_real);
    auto real_provider = made_real.take();
    auto real_connection = connect_loopback(fixture, real_provider);
    constrain_send_buffer(real_connection);
    auto payload = large_payload();
    auto deadline_send_seen = std::make_shared<Seen<void>>();
    auto task = await(real_connection.send(payload, {{}, vix::Deadline::after(vix::Duration::milliseconds(250))}), deadline_send_seen);
    assert(stays_pending_for(deadline_send_seen, std::chrono::milliseconds(20)));
    assert(wait_done_for(deadline_send_seen, std::chrono::seconds(1)));
    {
      std::lock_guard lock(deadline_send_seen->mutex);
      assert(deadline_send_seen->value->status() == vix::CompletionStatus::Failure && deadline_send_seen->value->error().identity() == timed_out);
    }
    drain_available(fixture.peer);
    const std::array<std::byte, 3> marker{std::byte{'t'}, std::byte{'m'}, std::byte{'o'}};
    auto marker_seen = std::make_shared<Seen<void>>();
    auto marker_task = await(real_connection.send(marker), marker_seen);
    assert(wait(marker_seen));
    drain_exact(fixture.peer, marker);
    assert(real_connection.close());
    assert(real_provider.shutdown());
  }

  // Same-direction overlap Faults run in a fresh process image.  Raw fork is
  // unsafe here because the parent has already initialized reactor and
  // continuation threads whose mutex owners do not exist in the child.
  set_scenario("child-process overlap fault contracts");
  {
    const auto send_status = spawn_child(argv[0], "--child-fault-send-overlap");
    assert(WIFEXITED(send_status) && WEXITSTATUS(send_status) == 42);
    const auto receive_status = spawn_child(argv[0], "--child-fault-receive-overlap");
    assert(WIFEXITED(receive_status) && WEXITSTATUS(receive_status) == 42);
  }

  // sigpipe_survival: a fresh process proves MSG_NOSIGNAL prevents a signal
  // death while the send still reports transport Failure.
  set_scenario("child-process SIGPIPE contract");
  {
    const auto status = spawn_child(argv[0], "--child-sigpipe-send");
    assert(!WIFSIGNALED(status) && WIFEXITED(status) && WEXITSTATUS(status) == 0);
  }
}
