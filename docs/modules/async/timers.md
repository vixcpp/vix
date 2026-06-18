# Timers

This guide shows how to use timers in Vix Async.

Use this page when you want to delay coroutine execution, schedule callbacks, or cancel timer-based work.

## Public header

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>
```

## What timers provide

The timer service is owned by `io_context`.

```cpp
ctx.timers()
```

It provides two common APIs:

| API                         | Purpose                              |
| --------------------------- | ------------------------------------ |
| `sleep_for(duration)`       | Suspends a coroutine for a duration. |
| `after(duration, callback)` | Runs a callback after a duration.    |

Use `sleep_for` inside coroutine tasks. Use `after` for simple delayed callbacks.

## Basic sleep

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

Run:

```
vix run main.cpp
```

Expected output:

```
before sleep
after sleep
```

## sleep_for

Use `sleep_for` when a coroutine must wait without blocking the scheduler thread.

```cpp
co_await ctx.timers().sleep_for(std::chrono::milliseconds(100));
```

The coroutine is suspended, then resumed through the `io_context` scheduler when the timer fires.

Example:

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

#include <chrono>

using namespace std::chrono_literals;

vix::async::core::task<void> wait_and_print(
    vix::async::core::io_context &ctx)
{
  co_await ctx.timers().sleep_for(250ms);

  vix::print("timer fired");

  ctx.stop();
  co_return;
}

int main()
{
  vix::async::core::io_context ctx;

  auto t = wait_and_print(ctx);
  std::move(t).start(ctx.get_scheduler());

  ctx.run();

  return 0;
}
```

Expected output:

```
timer fired
```

## after

Use `after` to run a callback after a delay.

```cpp
ctx.timers().after(std::chrono::milliseconds(100), []()
{
  vix::print("callback fired");
});
```

Complete example:

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

#include <chrono>

using namespace std::chrono_literals;

int main()
{
  vix::async::core::io_context ctx;

  ctx.timers().after(100ms, [&ctx]()
  {
    vix::print("callback fired");
    ctx.stop();
  });

  ctx.run();

  return 0;
}
```

Expected output:

```
callback fired
```

## sleep_for vs after

| API         | Use when                                                   |
| ----------- | ---------------------------------------------------------- |
| `sleep_for` | You are inside a coroutine and want to `co_await` a delay. |
| `after`     | You want to schedule a callback for later.                 |

Coroutine style:

```cpp
co_await ctx.timers().sleep_for(100ms);
```

Callback style:

```cpp
ctx.timers().after(100ms, []()
{
  vix::print("done");
});
```

## Multiple timers

You can schedule multiple timer tasks.

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

#include <chrono>

using namespace std::chrono_literals;

vix::async::core::task<void> first(vix::async::core::io_context &ctx)
{
  co_await ctx.timers().sleep_for(50ms);

  vix::print("first");
  co_return;
}

vix::async::core::task<void> second(vix::async::core::io_context &ctx)
{
  co_await ctx.timers().sleep_for(100ms);

  vix::print("second");
  ctx.stop();
  co_return;
}

int main()
{
  vix::async::core::io_context ctx;

  auto a = first(ctx);
  auto b = second(ctx);

  std::move(a).start(ctx.get_scheduler());
  std::move(b).start(ctx.get_scheduler());

  ctx.run();

  return 0;
}
```

Expected output:

```
first
second
```

## Timer cancellation

Timer operations support cooperative cancellation with `cancel_token`.

Create a cancellation source:

```cpp
vix::async::core::cancel_source source;
```

Pass its token to `sleep_for`:

```cpp
co_await ctx.timers().sleep_for(1s, source.token());
```

Request cancellation:

```cpp
source.request_cancel();
```

If cancellation is observed before resume, `sleep_for` throws a `std::system_error`.

## Cancel a timer sleep

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

#include <chrono>
#include <exception>
#include <system_error>

using namespace std::chrono_literals;

vix::async::core::task<void> sleeper(
    vix::async::core::io_context &ctx,
    vix::async::core::cancel_token token)
{
  try
  {
    co_await ctx.timers().sleep_for(1s, token);

    vix::print("sleep completed");
  }
  catch (const std::system_error &ex)
  {
    vix::eprint("sleep cancelled:", ex.code().message());
  }

  ctx.stop();
  co_return;
}

int main()
{
  vix::async::core::io_context ctx;
  vix::async::core::cancel_source source;

  auto t = sleeper(ctx, source.token());
  std::move(t).start(ctx.get_scheduler());

  ctx.timers().after(100ms, [&source]()
  {
    source.request_cancel();
  });

  ctx.run();

  return 0;
}
```

Expected output:

```
sleep cancelled: canceled
```

## Cancel a callback timer

`after` also accepts a cancellation token.

```cpp
ctx.timers().after(1s, []()
{
  vix::print("this should not run");
}, source.token());
```

Example:

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

#include <chrono>

using namespace std::chrono_literals;

int main()
{
  vix::async::core::io_context ctx;
  vix::async::core::cancel_source source;

  ctx.timers().after(1s, []()
  {
    vix::print("callback fired");
  }, source.token());

  ctx.timers().after(100ms, [&ctx, &source]()
  {
    source.request_cancel();

    vix::print("timer cancelled");

    ctx.stop();
  });

  ctx.run();

  return 0;
}
```

Expected output:

```
timer cancelled
```

## Zero duration

A zero duration sleep completes immediately.

```cpp
co_await ctx.timers().sleep_for(std::chrono::milliseconds(0));
```

Example:

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

#include <chrono>

vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  co_await ctx.timers().sleep_for(std::chrono::milliseconds(0));

  vix::print("done");

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
done
```

## Timer lifecycle

The timer service is created lazily.

```cpp
ctx.timers()
```

The service is owned by `io_context`. When the context shuts down, the timer service is stopped and released.

```cpp
ctx.shutdown();
```

Typical lifecycle:

```
create io_context
start task using timers
run context
timer resumes task
task calls stop
context exits run
```

## Avoid blocking sleeps

Do not block the scheduler thread with `std::this_thread::sleep_for`.

Wrong:

```cpp
ctx.post([&ctx]()
{
  std::this_thread::sleep_for(std::chrono::seconds(1));

  ctx.stop();
});
```

Correct:

```cpp
vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  co_await ctx.timers().sleep_for(std::chrono::seconds(1));

  ctx.stop();
  co_return;
}
```

## Common workflows

### Sleep inside a coroutine

```cpp
co_await ctx.timers().sleep_for(std::chrono::milliseconds(100));
```

### Schedule a callback

```cpp
ctx.timers().after(std::chrono::milliseconds(100), []()
{
  vix::print("done");
});
```

### Sleep and stop

```cpp
vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  co_await ctx.timers().sleep_for(std::chrono::milliseconds(100));

  vix::print("done");

  ctx.stop();
  co_return;
}
```

### Cancel a sleep

```cpp
vix::async::core::cancel_source source;

co_await ctx.timers().sleep_for(
    std::chrono::seconds(1),
    source.token());
```

### Cancel a callback timer

```cpp
vix::async::core::cancel_source source;

ctx.timers().after(
    std::chrono::seconds(1),
    []()
    {
      vix::print("done");
    },
    source.token());

source.request_cancel();
```

## Common mistakes

### Using sleep_for outside a coroutine

Wrong:

```cpp
ctx.timers().sleep_for(std::chrono::milliseconds(100));
```

Correct:

```cpp
co_await ctx.timers().sleep_for(std::chrono::milliseconds(100));
```

### Forgetting to run the context

Wrong:

```cpp
ctx.timers().after(std::chrono::milliseconds(100), []()
{
  vix::print("done");
});
```

Correct:

```cpp
ctx.timers().after(std::chrono::milliseconds(100), [&ctx]()
{
  vix::print("done");
  ctx.stop();
});

ctx.run();
```

### Forgetting to stop the context

If the timer callback or coroutine does not call `ctx.stop()`, `ctx.run()` may keep waiting.

```cpp
ctx.timers().after(std::chrono::milliseconds(100), [&ctx]()
{
  vix::print("done");
  ctx.stop();
});
```

### Blocking the scheduler thread

Avoid this:

```cpp
std::this_thread::sleep_for(std::chrono::seconds(1));
```

Use this:

```cpp
co_await ctx.timers().sleep_for(std::chrono::seconds(1));
```

### Ignoring cancellation exceptions

When using cancellation, handle `std::system_error`.

```cpp
try
{
  co_await ctx.timers().sleep_for(1s, token);
}
catch (const std::system_error &ex)
{
  vix::eprint(ex.code().message());
}
```

## Best practices

Use `sleep_for` inside coroutine tasks. Use `after` for simple delayed callbacks. Use cancellation tokens for long delays. Call `ctx.stop()` when the timer-based flow is complete. Avoid blocking sleeps on the scheduler thread. Keep timer callbacks short. Use the CPU pool for heavy work triggered by a timer.

## Related pages

| Page                                          | Purpose                                       |
| --------------------------------------------- | --------------------------------------------- |
| [io_context](/modules/async/io-context)       | Learn the runtime context and scheduler loop. |
| [Tasks](/modules/async/tasks)                 | Learn coroutine tasks.                        |
| [Spawn](/modules/async/spawn)                 | Learn how to start tasks.                     |
| [Cancellation](/modules/async/cancellation)   | Learn cancellation tokens.                    |
| [Thread pool](/modules/async/thread-pool)     | Learn background execution.                   |
| [API Reference](/modules/async/api-reference) | See the public API surface.                   |

## Next step

Continue with [cancellation](/modules/async/cancellation).
