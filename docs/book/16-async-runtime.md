# Async runtime

In the previous chapter, you learned WebSocket.
Now you will learn the async runtime.

```txt
event loop → coroutines → timers, signals, networking, CPU pool → completion resumes on event loop
```

Vix async is not a web framework.
It is a low-level C++20 asynchronous runtime core — the deterministic foundation for servers, networking, timers, workers, and real-time systems.

## Why async exists

C++ has coroutines as a language feature, but not a full runtime.
You still need:
event loop,
scheduler,
timers,
I/O integration,
CPU worker pool,
signal handling,
cancellation.

Vix async provides that foundation.

## The core idea

User coroutine code resumes on the event loop.
Blocking or CPU-heavy work is moved elsewhere.
Completion returns to the event loop.

```txt
event loop thread → runs user coroutine → starts timers/I/O/CPU jobs → completion → coroutine resumes
```

## Main components

| Component     | Purpose                                      |
|---------------|----------------------------------------------|
| `io_context`  | Owns the event loop and runtime services.    |
| `scheduler`   | Schedules coroutine resumption.              |
| `task<T>`     | Represents a coroutine return type.          |
| `timer`       | Provides non-blocking timers.                |
| `thread_pool` | Offloads CPU-bound work from the event loop. |
| `signal_set`  | Handles operating system signals.            |
| `net::*`      | Provides asynchronous networking primitives. |
| `when_all`    | Waits for multiple tasks to complete.        |
| `when_any`    | Waits for the first completed task.          |

## Minimal async example

```cpp
#include <vix/print>
#include <vix/async.hpp>

using vix::async::core::io_context;
using vix::async::core::task;

static task<void> app(io_context &ctx)
{
  vix::print("[async] hello from task");

  co_await ctx.timers().sleep_for(std::chrono::milliseconds(100));

  vix::print("[async] after timer");

  ctx.stop();
  co_return;
}

int main()
{
  io_context ctx;
  auto t = app(ctx);
  ctx.post(t.handle());  // schedule task
  ctx.run();              // start event loop
  vix::print("[async] done");
  return 0;
}
```

## Timers

Use `co_await` timers instead of blocking sleep:

```cpp
// Wrong — blocks event loop
std::this_thread::sleep_for(std::chrono::milliseconds(100));

// Correct — yields to event loop
co_await ctx.timers().sleep_for(std::chrono::milliseconds(100));

// Callback style
ctx.timers().after(std::chrono::milliseconds(150), []() { /* callback */ });
```

## CPU-bound work

CPU-heavy work belongs in the CPU pool:

```cpp
// Wrong — blocks event loop
for (int i = 0; i < 100000000; ++i) { /* heavy work */ }

// Correct — offload to worker thread
int result = co_await ctx.cpu_pool().submit([]() { return heavy_work(); });
```

## Signals

```cpp
#include <vix/async/core/signal.hpp>

auto &signals = ctx.signals();
signals.add(SIGINT);
signals.add(SIGTERM);

signals.on_signal([&](int signal) { ctx.stop(); });

int signal = co_await signals.async_wait();
ctx.stop();
```

## TCP echo server

```cpp
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

  while (ctx.is_running())
  {
    auto client = co_await listener->async_accept();
    vix::async::core::spawn_detached(ctx, handle_client(std::move(client)));
  }
}
```

## `when_all` and `when_any`

```cpp
// Wait for both to complete
auto values = co_await when_all(sched, a(), b());
std::get<0>(values);  // result from a()
std::get<1>(values);  // result from b()

// Wait for first to complete
auto [index, vals] = co_await when_any(sched, a(), b());
```

## Async and HTTP

For normal HTTP apps, you do not need low-level async:

```cpp
App app;
app.get("/", handler);
app.run(8080);
```

The runtime handles execution.
Understanding async helps you understand what happens underneath.

## Common mistakes

### Blocking the event loop

```cpp
// Wrong
std::this_thread::sleep_for(std::chrono::seconds(1));

// Correct
co_await ctx.timers().sleep_for(std::chrono::seconds(1));
```

### Running heavy CPU work on the event loop

```cpp
// Wrong
auto result = heavy_work();

// Correct
auto result = co_await ctx.cpu_pool().submit([]() { return heavy_work(); });
```

### Forgetting to schedule the task

```cpp
// A task starts suspended — you must schedule it
ctx.post(t.handle());
```

### Ignoring signal handling

Production services must handle `SIGINT` and `SIGTERM` for clean shutdown with systemd.

## What you should remember

The most important flow:

```cpp
io_context ctx;
auto t = app(ctx);
ctx.post(t.handle());  // schedule
ctx.run();              // run event loop
// co_await operations inside the coroutine
```

Use timers instead of blocking sleep.
Use CPU pool for heavy work.
Use signals for clean shutdown.

The core idea:
async code stays understandable when execution is explicit,
non-blocking,
and resumed through one clear runtime.

## Next chapter

[Next: Cache](/book/17-cache)
