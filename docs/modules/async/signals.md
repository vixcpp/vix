# Signals

This guide shows how to use async signal handling with Vix Async.

Use this page when you want to wait for system signals such as `SIGINT` and stop an `io_context` cleanly.

## Public header

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>
```

## What signals provide

The signal service is owned by `io_context`.

```cpp
ctx.signals()
```

It lets an async program:

- register signal numbers
- wait for the next signal
- react to signals from a coroutine
- stop the runtime cleanly

The signal service is useful for command-line apps, servers, workers, and long-running processes.

## Basic signal handling

This example waits for `SIGINT`. Press Ctrl+C to send the signal.

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

#include <csignal>

vix::async::core::task<void> wait_for_signal(
    vix::async::core::io_context &ctx)
{
  ctx.signals().add(SIGINT);

  vix::print("waiting for SIGINT");
  vix::print("press Ctrl+C to stop");

  const int sig = co_await ctx.signals().async_wait();

  vix::print("received signal =", sig);

  ctx.stop();
  co_return;
}

int main()
{
  vix::async::core::io_context ctx;

  auto t = wait_for_signal(ctx);
  std::move(t).start(ctx.get_scheduler());

  ctx.run();

  return 0;
}
```

Run:

```
vix run main.cpp
```

Example output:

```
waiting for SIGINT
press Ctrl+C to stop
received signal = 2
```

## Add a signal

Use `add` to register a signal number.

```cpp
ctx.signals().add(SIGINT);
```

You can register more than one signal.

```cpp
ctx.signals().add(SIGINT);
ctx.signals().add(SIGTERM);
```

## Remove a signal

Use `remove` to stop watching a signal.

```cpp
ctx.signals().remove(SIGINT);
```

After removal, the signal is no longer part of the watched set.

## async_wait

Use `async_wait` inside a coroutine.

```cpp
const int sig = co_await ctx.signals().async_wait();
```

It completes with the received signal number.

```cpp
vix::print("signal =", sig);
```

Complete example:

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

#include <csignal>

vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  ctx.signals().add(SIGINT);
  ctx.signals().add(SIGTERM);

  const int sig = co_await ctx.signals().async_wait();

  vix::print("received signal =", sig);

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

## Stop on Ctrl+C

The most common use case is graceful shutdown.

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

#include <csignal>

vix::async::core::task<void> shutdown_on_ctrl_c(
    vix::async::core::io_context &ctx)
{
  ctx.signals().add(SIGINT);

  co_await ctx.signals().async_wait();

  vix::print("shutdown requested");

  ctx.stop();
  co_return;
}

int main()
{
  vix::async::core::io_context ctx;

  auto t = shutdown_on_ctrl_c(ctx);
  std::move(t).start(ctx.get_scheduler());

  ctx.run();

  vix::print("stopped");

  return 0;
}
```

Example output:

```
shutdown requested
stopped
```

## Signal callback

Use `on_signal` when you want a callback to run when a signal is received.

```cpp
ctx.signals().on_signal([](int sig)
{
  vix::print("signal callback =", sig);
});
```

Complete example:

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

#include <csignal>

vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  ctx.signals().add(SIGINT);

  ctx.signals().on_signal([](int sig)
  {
    vix::print("callback signal =", sig);
  });

  const int sig = co_await ctx.signals().async_wait();

  vix::print("awaited signal =", sig);

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

## Cancellation

`async_wait` accepts a cancellation token.

```cpp
const int sig = co_await ctx.signals().async_wait(source.token());
```

If cancellation is requested before the signal is received, the wait throws a `std::system_error`.

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

#include <chrono>
#include <csignal>
#include <system_error>

using namespace std::chrono_literals;

vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  vix::async::core::cancel_source source;

  ctx.signals().add(SIGINT);

  ctx.timers().after(100ms, [&source]()
  {
    source.request_cancel();
  });

  try
  {
    const int sig = co_await ctx.signals().async_wait(source.token());

    vix::print("signal =", sig);
  }
  catch (const std::system_error &ex)
  {
    vix::eprint("signal wait cancelled:", ex.code().message());
  }

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
signal wait cancelled: canceled
```

## Stop watching signals

Use `stop` to stop signal watching.

```cpp
ctx.signals().stop();
```

A normal shutdown flow:

```cpp
ctx.signals().stop();
ctx.stop();
```

Example:

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

#include <csignal>

vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  ctx.signals().add(SIGINT);

  const int sig = co_await ctx.signals().async_wait();

  vix::print("signal =", sig);

  ctx.signals().stop();
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

## Multiple signals

Register multiple signals when the app should react to more than one shutdown signal.

```cpp
ctx.signals().add(SIGINT);
ctx.signals().add(SIGTERM);
```

Example:

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

#include <csignal>

vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  ctx.signals().add(SIGINT);
  ctx.signals().add(SIGTERM);

  vix::print("waiting for SIGINT or SIGTERM");

  const int sig = co_await ctx.signals().async_wait();

  if (sig == SIGINT)
  {
    vix::print("SIGINT received");
  }
  else if (sig == SIGTERM)
  {
    vix::print("SIGTERM received");
  }
  else
  {
    vix::print("signal received =", sig);
  }

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

## Signals and servers

Signals are useful for long-running servers.

A common server pattern is: start server task, start signal task, run `io_context`, when `SIGINT` arrives stop server and context.

Example skeleton:

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

#include <chrono>
#include <csignal>

using namespace std::chrono_literals;

vix::async::core::task<void> server_loop(
    vix::async::core::io_context &ctx,
    vix::async::core::cancel_token token)
{
  while (!token.is_cancelled())
  {
    vix::print("server tick");

    co_await ctx.timers().sleep_for(500ms);
  }

  vix::print("server stopped");
  co_return;
}

vix::async::core::task<void> wait_for_shutdown(
    vix::async::core::io_context &ctx,
    vix::async::core::cancel_source &source)
{
  ctx.signals().add(SIGINT);

  co_await ctx.signals().async_wait();

  vix::print("shutdown signal received");

  source.request_cancel();
  ctx.stop();

  co_return;
}

int main()
{
  vix::async::core::io_context ctx;
  vix::async::core::cancel_source source;

  auto server = server_loop(ctx, source.token());
  auto shutdown = wait_for_shutdown(ctx, source);

  std::move(server).start(ctx.get_scheduler());
  std::move(shutdown).start(ctx.get_scheduler());

  ctx.run();

  return 0;
}
```

## Platform behavior

Signal support depends on the platform. On unsupported platforms, `async_wait` can throw a `std::system_error`.

Handle errors when writing portable code.

```cpp
try
{
  const int sig = co_await ctx.signals().async_wait();
  vix::print("signal =", sig);
}
catch (const std::system_error &ex)
{
  vix::eprint(ex.code().message());
}
```

## Common workflows

### Wait for Ctrl+C

```cpp
ctx.signals().add(SIGINT);

const int sig = co_await ctx.signals().async_wait();

vix::print("signal =", sig);
ctx.stop();
```

### Watch SIGINT and SIGTERM

```cpp
ctx.signals().add(SIGINT);
ctx.signals().add(SIGTERM);
```

### Register a callback

```cpp
ctx.signals().on_signal([](int sig)
{
  vix::print("signal =", sig);
});
```

### Cancel a signal wait

```cpp
vix::async::core::cancel_source source;

const int sig = co_await ctx.signals().async_wait(source.token());
```

### Stop signal service

```cpp
ctx.signals().stop();
```

## Common mistakes

### Forgetting to add a signal

Wrong:

```cpp
const int sig = co_await ctx.signals().async_wait();
```

Correct:

```cpp
ctx.signals().add(SIGINT);

const int sig = co_await ctx.signals().async_wait();
```

### Forgetting to run the context

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

### Forgetting to stop the context

After receiving the shutdown signal, stop the context.

```cpp
ctx.stop();
```

### Doing heavy work in a signal callback

Keep signal callbacks short.

Wrong:

```cpp
ctx.signals().on_signal([](int)
{
  expensive_work();
});
```

Better:

```cpp
ctx.signals().on_signal([](int sig)
{
  vix::print("signal =", sig);
});
```

Use a task or the thread pool for heavier work.

### Assuming all platforms behave the same

Signal delivery is platform-dependent. Handle `std::system_error` when writing portable code.

## Best practices

Use signals for graceful shutdown. Register only the signals your app needs. Keep signal callbacks short. Use `async_wait` from a coroutine for clean async flow. Call `ctx.stop()` after receiving a shutdown signal. Use `cancel_source` when a signal should cancel long-running tasks. Handle `std::system_error` for portable code.

## Related pages

| Page                                          | Purpose                        |
| --------------------------------------------- | ------------------------------ |
| [io_context](/modules/async/io-context)       | Learn the runtime context.     |
| [Tasks](/modules/async/tasks)                 | Learn coroutine tasks.         |
| [Spawn](/modules/async/spawn)                 | Learn how to start async work. |
| [Timers](/modules/async/timers)               | Learn timers and sleeps.       |
| [Cancellation](/modules/async/cancellation)   | Learn cancellation tokens.     |
| [Thread pool](/modules/async/thread-pool)     | Learn background execution.    |
| [API Reference](/modules/async/api-reference) | See the public API surface.    |

## Next step

Continue with [TCP](/modules/async/tcp).
