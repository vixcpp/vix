# io_context

This guide shows how to use `io_context` in Vix Async.

Use this page when you want to run async callbacks, start coroutine tasks, access runtime services, and stop the async runtime cleanly.

## Public header

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>
```

## What is io_context?

`io_context` is the central runtime object for Vix Async.

It owns the async services used by an application:

- scheduler
- CPU thread pool
- timer service
- signal service
- networking backend

A normal async program creates one `io_context`, starts async work, then runs the context.

```cpp
vix::async::core::io_context ctx;

ctx.run();
```

## Minimal example

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

int main()
{
  vix::async::core::io_context ctx;

  ctx.post([&ctx]()
  {
    vix::print("hello from io_context");
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
hello from io_context
```

## The scheduler

`io_context` contains a scheduler.

The scheduler runs callbacks and resumes coroutines.

```cpp
auto &scheduler = ctx.get_scheduler();
```

Most user code does not need to store the scheduler directly. Use it when starting a task:

```cpp
auto t = app(ctx);
std::move(t).start(ctx.get_scheduler());
```

## Post work

Use `post` to schedule a callback.

```cpp
ctx.post([]()
{
  vix::print("posted work");
});
```

The callback runs when `ctx.run()` processes it.

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

int main()
{
  vix::async::core::io_context ctx;

  ctx.post([&ctx]()
  {
    vix::print("first callback");
  });

  ctx.post([&ctx]()
  {
    vix::print("second callback");
    ctx.stop();
  });

  ctx.run();

  return 0;
}
```

Expected output:

```
first callback
second callback
```

## Run the context

`run()` starts the scheduler loop.

```cpp
ctx.run();
```

It blocks until `stop()` is called.

```cpp
ctx.stop();
```

Typical pattern:

```cpp
vix::async::core::io_context ctx;

ctx.post([&ctx]()
{
  vix::print("done");
  ctx.stop();
});

ctx.run();
```

## Stop the context

Use `stop()` to request the scheduler loop to exit.

```cpp
ctx.stop();
```

A common pattern is to stop the context when the main async work is done.

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  vix::print("app running");

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
app running
```

## Check running state

Use `is_running()` to check whether the scheduler is active.

```cpp
vix::print("running =", ctx.is_running() ? "yes" : "no");
```

Example:

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

int main()
{
  vix::async::core::io_context ctx;

  vix::print("before run =", ctx.is_running() ? "yes" : "no");

  ctx.post([&ctx]()
  {
    vix::print("inside run =", ctx.is_running() ? "yes" : "no");
    ctx.stop();
  });

  ctx.run();

  vix::print("after run =", ctx.is_running() ? "yes" : "no");

  return 0;
}
```

Expected output:

```
before run = no
inside run = yes
after run = no
```

## Services

`io_context` lazily creates services when they are first used.

| Service   | API               | Purpose                                      |
| --------- | ----------------- | -------------------------------------------- |
| Scheduler | `get_scheduler()` | Runs callbacks and resumes coroutines.       |
| CPU pool  | `cpu_pool()`      | Runs blocking or CPU-heavy work.             |
| Timers    | `timers()`        | Runs delayed callbacks and coroutine sleeps. |
| Signals   | `signals()`       | Watches system signals.                      |
| Network   | `net()`           | Provides the async networking backend.       |

### Timers service

Use `timers()` to access the timer service.

```cpp
co_await ctx.timers().sleep_for(std::chrono::milliseconds(100));
```

Complete example:

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

#include <chrono>

using namespace std::chrono_literals;

vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  vix::print("before timer");

  co_await ctx.timers().sleep_for(100ms);

  vix::print("after timer");

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
before timer
after timer
```

### CPU pool service

Use `cpu_pool()` to run blocking or CPU-heavy work outside the scheduler thread.

```cpp
int result = co_await ctx.cpu_pool().submit([]()
{
  return 42;
});
```

Complete example:

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

### Signals service

Use `signals()` to access async signal handling.

```cpp
ctx.signals().add(SIGINT);
```

A signal task can wait for the next signal:

```cpp
int sig = co_await ctx.signals().async_wait();
```

Signal handling is covered in the [signals guide](/modules/async/signals).

### Network service

The network backend is created when networking APIs are used.

```cpp
ctx.net();
```

Most user code should use the higher-level helpers instead:

```cpp
auto resolver = vix::async::net::make_dns_resolver(ctx);
auto stream = vix::async::net::make_tcp_stream(ctx);
auto socket = vix::async::net::make_udp_socket(ctx);
```

Networking is covered in the [TCP](/modules/async/tcp), [UDP](/modules/async/udp), and [DNS](/modules/async/dns) guides.

## Shutdown

Use `shutdown()` when you want to stop the scheduler and release all services explicitly.

```cpp
ctx.shutdown();
```

The destructor also shuts down the context.

A safe pattern:

```cpp
vix::async::core::io_context ctx;

ctx.post([&ctx]()
{
  vix::print("done");
  ctx.stop();
});

ctx.run();
ctx.shutdown();
```

## Lifecycle

A normal `io_context` lifecycle looks like this:

```
create io_context
start async work
run scheduler
stop context
shutdown services
```

In code:

```cpp
vix::async::core::io_context ctx;

auto t = app(ctx);
std::move(t).start(ctx.get_scheduler());

ctx.run();
ctx.shutdown();
```

## Common workflows

### Run one callback

```cpp
vix::async::core::io_context ctx;

ctx.post([&ctx]()
{
  vix::print("callback");
  ctx.stop();
});

ctx.run();
```

### Run one coroutine task

```cpp
vix::async::core::io_context ctx;

auto t = app(ctx);
std::move(t).start(ctx.get_scheduler());

ctx.run();
```

### Stop after timer

```cpp
vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  co_await ctx.timers().sleep_for(std::chrono::milliseconds(100));

  vix::print("done");
  ctx.stop();

  co_return;
}
```

### Offload work and stop

```cpp
vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  int value = co_await ctx.cpu_pool().submit([]()
  {
    return 42;
  });

  vix::print("value =", value);

  ctx.stop();
  co_return;
}
```

## Common mistakes

### Forgetting to call run

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

### Forgetting to call stop

`run()` waits for work. If nothing stops the context, the program may keep running.

```cpp
ctx.post([&ctx]()
{
  vix::print("done");
  ctx.stop();
});
```

### Starting a task without running the context

Wrong:

```cpp
auto t = app(ctx);
std::move(t).start(ctx.get_scheduler());
```

Correct:

```cpp
auto t = app(ctx);
std::move(t).start(ctx.get_scheduler());

ctx.run();
```

### Doing heavy work directly in post

Avoid this:

```cpp
ctx.post([&ctx]()
{
  expensive_work();
  ctx.stop();
});
```

Use the CPU pool inside a coroutine:

```cpp
auto value = co_await ctx.cpu_pool().submit([]()
{
  return expensive_work();
});
```

### Accessing services after shutdown

After `shutdown()`, services should not be recreated. Keep service access before shutdown.

## Best practices

Create one `io_context` for the async runtime. Start the main async task before calling `run()`. Call `ctx.stop()` when the main task is complete. Use `ctx.timers()` for delays. Use `ctx.cpu_pool()` for blocking or CPU-heavy work. Use `ctx.shutdown()` when you want explicit cleanup. Avoid using internal networking service APIs directly unless you are implementing lower-level integrations.

## Related pages

| Page                                          | Purpose                           |
| --------------------------------------------- | --------------------------------- |
| [Tasks](/modules/async/tasks)                 | Learn `task<T>` and `task<void>`. |
| [Spawn](/modules/async/spawn)                 | Learn how to start async work.    |
| [Timers](/modules/async/timers)               | Learn delayed work and sleeps.    |
| [Cancellation](/modules/async/cancellation)   | Learn cancellation tokens.        |
| [Thread pool](/modules/async/thread-pool)     | Learn background execution.       |
| [Signals](/modules/async/signals)             | Learn signal handling.            |
| [API Reference](/modules/async/api-reference) | See the public API surface.       |

## Next step

Continue with [tasks](/modules/async/tasks).
