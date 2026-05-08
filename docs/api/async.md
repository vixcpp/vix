# Async API

The Async API is the low-level coroutine runtime of Vix. It provides C++20 `co_await`-based building blocks for asynchronous code.

```txt
event loop — tasks — timers — signals — CPU pool — async TCP/UDP/DNS — when_all/when_any
```

## Public header

```cpp
#include <vix/async.hpp>

// Or specific headers
#include <vix/async/core/io_context.hpp>
#include <vix/async/core/task.hpp>
#include <vix/async/core/timer.hpp>
#include <vix/async/core/thread_pool.hpp>
#include <vix/async/core/signal.hpp>
#include <vix/async/core/spawn.hpp>
#include <vix/async/core/when.hpp>
#include <vix/async/net/tcp.hpp>
#include <vix/async/net/udp.hpp>
#include <vix/async/net/dns.hpp>
```

## Mental model

```txt
io_context → scheduler → task<T> → co_await
```

`io_context` owns: scheduler, timer service, CPU thread pool, signal service, network backend.

A task starts **suspended**. Post it to the scheduler, then `ctx.run()` drives the event loop.

## Main concepts

| Concept | Purpose |
|---------|---------|
| `io_context` | Central async runtime context |
| `task<T>` | Coroutine task returning T |
| `task<void>` | Coroutine task with no return value |
| `scheduler` | Runs posted coroutine handles |
| `timer` | Non-blocking sleep and delayed callbacks |
| `thread_pool` | Offload CPU or blocking work |
| `signal_set` | Wait for POSIX signals |
| `spawn_detached` | Run a task without awaiting it |
| `when_all` | Wait for all tasks to complete |
| `when_any` | Wait for the first task to complete |
| `tcp_stream` | Async TCP connection |
| `tcp_listener` | Async TCP server listener |
| `udp_socket` | Async UDP socket |
| `dns_resolver` | Async DNS resolution |
| `cancel_token` | Observe cancellation |
| `cancel_source` | Request cancellation |

## Minimal async task

```cpp
#include <iostream>
#include <vix/async/core/io_context.hpp>
#include <vix/async/core/task.hpp>
using vix::async::core::io_context;
using vix::async::core::task;

static task<void> app(io_context &ctx)
{
  std::cout << "[async] hello from task\n";
  ctx.stop();
  co_return;
}

int main()
{
  io_context ctx;
  auto t = app(ctx);
  ctx.post(t.handle());  // task starts suspended — must post it
  ctx.run();             // drive the event loop
  std::cout << "[async] done\n";
  return 0;
}
```

## Timer service

```cpp
// Non-blocking sleep — do not use std::this_thread::sleep_for in async code
co_await ctx.timers().sleep_for(std::chrono::milliseconds(100));
co_await ctx.timers().sleep_for(std::chrono::milliseconds(200));

// Callback style
ctx.timers().after(std::chrono::milliseconds(150), []() {
  /* runs later */
});

// With cancellation token
co_await ctx.timers().sleep_for(std::chrono::seconds(5), token);
```

## CPU thread pool

```cpp
// Never block the event loop with heavy work
int result = co_await ctx.cpu_pool().submit([](){
                return heavy_work(42);
              });

// Fire-and-forget
ctx.cpu_pool().submit([]() { background_task(); });
```

## Signals

```cpp
#include <vix/async/core/signal.hpp>

auto &sig = ctx.signals();
sig.add(SIGINT);
sig.add(SIGTERM);

// Callback style
sig.on_signal([&](int s) {
  std::cout << "signal " << s << "\n"; ctx.stop();
});

// Coroutine style
int s = co_await sig.async_wait();
ctx.stop();
```

## Scheduler (minimal runtime)

```cpp
#include <vix/async/core/scheduler.hpp>
using vix::async::core::scheduler;

static task<void> app(scheduler &sched)
{
  co_await sched.schedule();
  std::cout << "running on scheduler\n";
  sched.stop();
  co_return;
}

int main()
{
  scheduler sched;

  std::thread worker([&]() {
    sched.run();
  });

  std::move(app(sched)).start(sched);

  worker.join();
  return 0;
}
```

Use `scheduler` for minimal coroutine scheduling without timers/signals/networking. Use `io_context` when you need those services.

## when_all

```cpp
#include <vix/async/core/when.hpp>
using vix::async::core::when_all;

auto result = co_await when_all(sched, a(), b());
std::cout << std::get<0>(result) << ", " << std::get<1>(result) << "\n";
```

## when_any

```cpp
using vix::async::core::when_any;

auto [idx, values] = co_await when_any(sched, a(), b());
if (idx == 0)
  std::cout << *std::get<0>(values);
else
  std::cout << *std::get<1>(values);
```

## Detached tasks

```cpp
#include <vix/async/core/spawn.hpp>

// Useful for independent background work (e.g. one task per client)
vix::async::core::spawn_detached(ctx, handle_client(std::move(client)));
```

Do not use detached tasks when you must observe the result.

## TCP echo server

```cpp
#include <vix/async/net/tcp.hpp>

static task<void> handle_client(std::unique_ptr<vix::async::net::tcp_stream> client)
{
  std::vector<std::byte> buffer(4096);
  while (client && client->is_open())
  {
    std::size_t n = co_await client->async_read(std::span<std::byte>(buffer.data(), buffer.size()));
    if (n == 0) break;
    co_await client->async_write(std::span<const std::byte>(buffer.data(), n));
  }
  client->close();
}

static task<void> server(io_context &ctx)
{
  auto listener = vix::async::net::make_tcp_listener(ctx);
  co_await listener->async_listen({"0.0.0.0", 9090}, 128);
  std::cout << "listening on 9090\n";

  while (ctx.is_running())
  {
    auto client = co_await listener->async_accept();
    vix::async::core::spawn_detached(ctx, handle_client(std::move(client)));
  }

  listener->close();
  ctx.stop();
}
```

## UDP overview

```cpp
#include <vix/async/net/udp.hpp>
co_await socket->async_bind(endpoint);
co_await socket->async_send_to(data, peer);
co_await socket->async_recv_from(buffer);
```

## DNS overview

```cpp
#include <vix/async/net/dns.hpp>
auto resolver = vix::async::net::make_dns_resolver(ctx);
auto addresses = co_await resolver->async_resolve("example.com", 443);
// addresses[i].ip, addresses[i].port
```

## Cancellation

```cpp
vix::async::core::cancel_source source;
vix::async::core::cancel_token token = source.token();

source.request_cancel();
if (token.is_cancelled()) {
  std::cout << "cancelled\n";
}
```

## Common mistakes

### Forgetting to post the task

```cpp
auto t = app(ctx);
// ctx.post(t.handle());  // missing — task never runs
ctx.run();
```

### Blocking the event loop

```cpp
// Wrong — blocks the scheduler thread
std::this_thread::sleep_for(std::chrono::seconds(1));

// Correct — yields to event loop
co_await ctx.timers().sleep_for(std::chrono::seconds(1));
```

### Heavy CPU work on event loop

```cpp
// Wrong
auto result = heavy_work();

// Correct
auto result = co_await ctx.cpu_pool().submit([]() {
  return heavy_work();
});
```

### Forgetting ctx.stop()

If nothing calls `ctx.stop()`, `ctx.run()` keeps waiting. Short demos should stop when done. Servers intentionally keep running until a signal.

## Production notes

- Keep coroutine tasks small
- Use timers instead of blocking sleep
- Offload CPU-heavy work to the CPU pool
- Handle `std::system_error` in networking code
- Support cancellation where needed
- Stop the runtime cleanly on SIGINT/SIGTERM

## What you should remember

```cpp
io_context ctx;
auto t = app(ctx);
ctx.post(t.handle());
ctx.run();

// Inside coroutine:
co_await ctx.timers().sleep_for(std::chrono::milliseconds(100));

int result = co_await ctx.cpu_pool().submit([]() {
  return heavy_work();
});

int sig = co_await ctx.signals().async_wait();
```

The core idea: Vix async is coroutine-first. A task starts suspended, the scheduler resumes it, and `io_context` coordinates runtime services.

Next: [P2P API](/api/p2p)
