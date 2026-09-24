# Async

`vix::async` provides asynchronous execution primitives for Vix applications.

Use it when you need coroutine tasks, an async runtime context, timers, cancellation, background work, signals, or async networking.

## Public header

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>
```

## What Async provides

Vix Async provides a small coroutine-based runtime.

It includes:

- `io_context` for runtime coordination
- `task<T>` and `task<void>` for coroutine work
- `spawn` for starting tasks
- `timer` for delayed execution
- `cancel_source` and `cancel_token` for cooperative cancellation
- `thread_pool` for CPU-heavy or blocking work
- `when_all` and `when_any` for task composition
- `signal_set` for async signal handling
- TCP, UDP, and DNS helpers backed by the async runtime

## Basic idea

Most async programs use an `io_context`.

```cpp
vix::async::core::io_context ctx;
```

The context owns the scheduler and lazily creates services when needed:

```cpp
ctx.timers();
ctx.cpu_pool();
ctx.signals();
ctx.net();
```

Then the context runs the scheduler loop:

```cpp
ctx.run();
```

Stop it when the async work is done:

```cpp
ctx.stop();
```

## Minimal async program

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

int main()
{
  vix::async::core::io_context ctx;

  ctx.post([&ctx]()
  {
    vix::print("hello from async");
    ctx.stop();
  });

  ctx.run();

  return 0;
}
```

Run:

```
vix run main.cpp
```

Expected output:

```
hello from async
```

## Coroutine task

A `task<T>` represents async work that can produce a value.

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

vix::async::core::task<int> compute()
{
  co_return 42;
}

vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  const int value = co_await compute();

  vix::print("value =", value);

  ctx.stop();
  co_return;
}

int main()
{
  vix::async::core::io_context ctx;

  auto t = app(ctx);
  std::move(t).start(ctx.get_scheduler());

  ctx.run();

  return 0;
}
```

Expected output:

```
value = 42
```

## Timers

Use timers when a coroutine needs to wait.

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

#include <chrono>

using namespace std::chrono_literals;

vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  vix::print("before sleep");

  co_await ctx.timers().sleep_for(100ms);

  vix::print("after sleep");

  ctx.stop();
  co_return;
}

int main()
{
  vix::async::core::io_context ctx;

  auto t = app(ctx);
  std::move(t).start(ctx.get_scheduler());

  ctx.run();

  return 0;
}
```

Expected output:

```
before sleep
after sleep
```

## Cancellation

Use `cancel_source` to request cancellation and `cancel_token` to observe it.

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

int main()
{
  vix::async::core::cancel_source source;
  vix::async::core::cancel_token token = source.token();

  vix::print("can cancel =", token.can_cancel() ? "yes" : "no");

  source.request_cancel();

  vix::print("cancelled =", token.is_cancelled() ? "yes" : "no");

  return 0;
}
```

Expected output:

```
can cancel = yes
cancelled = yes
```

## Thread pool

Use the CPU pool when a coroutine needs to offload blocking or CPU-heavy work.

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  int value = co_await ctx.cpu_pool().submit([]()
  {
    return 21 * 2;
  });

  vix::print("value =", value);

  ctx.stop();
  co_return;
}

int main()
{
  vix::async::core::io_context ctx;

  auto t = app(ctx);
  std::move(t).start(ctx.get_scheduler());

  ctx.run();

  return 0;
}
```

Expected output:

```
value = 42
```

## Network helpers

The async module also includes networking APIs.

| API            | Purpose                                |
| -------------- | -------------------------------------- |
| `dns_resolver` | Resolve hostnames asynchronously.      |
| `tcp_stream`   | Connect, read, and write TCP streams.  |
| `tcp_listener` | Listen and accept TCP connections.     |
| `udp_socket`   | Bind, send, and receive UDP datagrams. |

The network backend is attached to `io_context` and is created lazily when networking is used.

## Core components

| Component       | Purpose                                     |
| --------------- | ------------------------------------------- |
| `io_context`    | Owns the scheduler and async services.      |
| `scheduler`     | Runs callbacks and resumes coroutines.      |
| `task<T>`       | Coroutine task that produces a value.       |
| `task<void>`    | Coroutine task with no returned value.      |
| `timer`         | Runs callbacks later or sleeps a coroutine. |
| `cancel_source` | Requests cancellation.                      |
| `cancel_token`  | Observes cancellation.                      |
| `thread_pool`   | Runs work outside the scheduler thread.     |
| `signal_set`    | Waits for system signals.                   |

## Typical flow

```
create io_context
create or spawn async work
run the context
async work completes
stop the context
```

In code:

```cpp
vix::async::core::io_context ctx;

auto t = app(ctx);
std::move(t).start(ctx.get_scheduler());

ctx.run();
```

## Common workflows

### Run a posted callback

```cpp
vix::async::core::io_context ctx;

ctx.post([&ctx]()
{
  vix::print("callback");
  ctx.stop();
});

ctx.run();
```

### Run a coroutine task

```cpp
auto t = app(ctx);
std::move(t).start(ctx.get_scheduler());

ctx.run();
```

### Sleep inside a coroutine

```cpp
co_await ctx.timers().sleep_for(std::chrono::milliseconds(100));
```

### Offload CPU work

```cpp
int value = co_await ctx.cpu_pool().submit([]()
{
  return 42;
});
```

### Request cancellation

```cpp
vix::async::core::cancel_source source;

source.request_cancel();
```

## Common mistakes

### Forgetting to run the context

Wrong:

```cpp
vix::async::core::io_context ctx;

ctx.post([]()
{
  vix::print("hello");
});
```

Correct:

```cpp
vix::async::core::io_context ctx;

ctx.post([&ctx]()
{
  vix::print("hello");
  ctx.stop();
});

ctx.run();
```

### Forgetting to stop the context

If no code calls `stop()`, `run()` may keep waiting for work.

```cpp
ctx.post([&ctx]()
{
  vix::print("done");
  ctx.stop();
});
```

### Destroying a task before starting it

`task<T>` is lazy. Create it, then start it or await it.

```cpp
auto t = app(ctx);
std::move(t).start(ctx.get_scheduler());
```

### Using blocking work on the scheduler thread

Avoid running heavy blocking work directly inside the scheduler. Use the CPU pool:

```cpp
auto result = co_await ctx.cpu_pool().submit([]()
{
  return expensive_work();
});
```

### Ignoring cancellation

When an API accepts a `cancel_token`, pass one if the operation should be cancellable.

```cpp
vix::async::core::cancel_source source;

co_await ctx.timers().sleep_for(1s, source.token());
```

## Best practices

Use one `io_context` as the owner of async services. Use `task<T>` for coroutine-based async work. Use timers for delays instead of blocking sleeps. Use the CPU pool for blocking or CPU-heavy functions. Use cancellation tokens for long-running operations. Call `ctx.stop()` when the main async work is complete. Call `ctx.shutdown()` when you want to explicitly release services.

## Related pages

| Page                                          | Purpose                                       |
| --------------------------------------------- | --------------------------------------------- |
| [io_context](/modules/async/io-context)       | Learn the runtime context and scheduler loop. |
| [Tasks](/modules/async/tasks)                 | Learn `task<T>` and `task<void>`.             |
| [Spawn](/modules/async/spawn)                 | Learn how to start detached tasks.            |
| [Timers](/modules/async/timers)               | Learn delays and timer callbacks.             |
| [Cancellation](/modules/async/cancellation)   | Learn `cancel_source` and `cancel_token`.     |
| [Thread pool](/modules/async/thread-pool)     | Learn background work.                        |
| [when_all / when_any](/modules/async/when)    | Learn task composition.                       |
| [Signals](/modules/async/signals)             | Learn async signal handling.                  |
| [TCP](/modules/async/tcp)                     | Learn TCP streams and listeners.              |
| [UDP](/modules/async/udp)                     | Learn UDP sockets.                            |
| [DNS](/modules/async/dns)                     | Learn async DNS resolution.                   |
| [API Reference](/modules/async/api-reference) | See the public API surface.                   |

## Next step

Continue with [io_context](/modules/async/io-context).
