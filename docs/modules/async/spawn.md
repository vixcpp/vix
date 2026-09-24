# Spawn

This guide shows how to start async tasks with Vix Async.

Use this page when you want to launch a coroutine task from `main`, run detached async work, or schedule work on an `io_context`.

## Public header

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>
```

## What spawn means

In Vix Async, tasks are lazy.

Creating a task does not run it.

```cpp
auto t = app(ctx);
```

To run the task, you must schedule it on a scheduler.

```cpp
std::move(t).start(ctx.get_scheduler());
```

This starts the coroutine on the `io_context` scheduler.

## Basic task startup

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  vix::print("app started");

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
app started
```

## Start on the scheduler

The most direct way to start a top-level task is:

```cpp
auto t = app(ctx);
std::move(t).start(ctx.get_scheduler());
```

Then run the context:

```cpp
ctx.run();
```

Full pattern:

```cpp
vix::async::core::io_context ctx;

auto t = app(ctx);
std::move(t).start(ctx.get_scheduler());

ctx.run();
```

## Why std::move is used

`task<T>` is move-only.

Starting a task transfers ownership of the coroutine frame to the scheduler.

```cpp
std::move(t).start(ctx.get_scheduler());
```

After this, do not use `t` again.

## Top-level task

A common pattern is to write one top-level `app` task.

```cpp
vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  vix::print("running");

  ctx.stop();
  co_return;
}
```

Then start it from `main`.

```cpp
int main()
{
  vix::async::core::io_context ctx;

  auto t = app(ctx);
  std::move(t).start(ctx.get_scheduler());

  ctx.run();

  return 0;
}
```

## Spawn with timer work

You can start a task that uses timers.

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

## Spawn with returned values

A started task can await other tasks.

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

## Fire-and-forget work

Use detached async work when you do not need to await the task result directly.

A detached task is useful for background work that will stop the context later.

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

#include <chrono>

using namespace std::chrono_literals;

vix::async::core::task<void> background(vix::async::core::io_context &ctx)
{
  co_await ctx.timers().sleep_for(100ms);

  vix::print("background done");

  ctx.stop();
  co_return;
}

int main()
{
  vix::async::core::io_context ctx;

  auto t = background(ctx);
  std::move(t).start(ctx.get_scheduler());

  ctx.run();

  return 0;
}
```

Expected output:

```
background done
```

## Multiple tasks

You can start multiple top-level tasks.

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

## Using post for simple callbacks

For simple callback work, use `ctx.post`.

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

int main()
{
  vix::async::core::io_context ctx;

  ctx.post([&ctx]()
  {
    vix::print("posted callback");
    ctx.stop();
  });

  ctx.run();

  return 0;
}
```

Expected output:

```
posted callback
```

Use `task` when the work needs `co_await`. Use `post` when the work is a simple callback.

## Start vs co_await

Use `start` at the top level.

```cpp
auto t = app(ctx);
std::move(t).start(ctx.get_scheduler());
```

Use `co_await` inside another task.

```cpp
const int value = co_await compute();
```

Typical layout:

```cpp
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

## Stopping the context

A started task should usually stop the context when the main async work is done.

```cpp
ctx.stop();
co_return;
```

Without `ctx.stop()`, `ctx.run()` may keep waiting for work.

```cpp
vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  vix::print("done");

  ctx.stop();
  co_return;
}
```

## Error handling

Exceptions thrown inside an awaited task are rethrown to the awaiting coroutine.

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

#include <exception>
#include <stdexcept>

vix::async::core::task<int> fail()
{
  throw std::runtime_error("failed");
  co_return 0;
}

vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  try
  {
    const int value = co_await fail();
    vix::print("value =", value);
  }
  catch (const std::exception &ex)
  {
    vix::eprint(ex.what());
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
failed
```

## Common workflows

### Start one task

```cpp
auto t = app(ctx);
std::move(t).start(ctx.get_scheduler());

ctx.run();
```

### Start two tasks

```cpp
auto a = first(ctx);
auto b = second(ctx);

std::move(a).start(ctx.get_scheduler());
std::move(b).start(ctx.get_scheduler());

ctx.run();
```

### Start a task that waits on a timer

```cpp
vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  co_await ctx.timers().sleep_for(std::chrono::milliseconds(100));

  ctx.stop();
  co_return;
}
```

### Start a task that awaits another task

```cpp
vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  const int value = co_await compute();

  vix::print("value =", value);

  ctx.stop();
  co_return;
}
```

### Use post for callbacks

```cpp
ctx.post([&ctx]()
{
  vix::print("callback");
  ctx.stop();
});
```

## Common mistakes

### Creating a task but not starting it

Wrong:

```cpp
auto t = app(ctx);

ctx.run();
```

Correct:

```cpp
auto t = app(ctx);
std::move(t).start(ctx.get_scheduler());

ctx.run();
```

### Starting a task but not running the context

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

### Forgetting std::move

Wrong:

```cpp
t.start(ctx.get_scheduler());
```

Correct:

```cpp
std::move(t).start(ctx.get_scheduler());
```

### Using a task after starting it

Wrong:

```cpp
auto t = app(ctx);

std::move(t).start(ctx.get_scheduler());

if (t.valid())
{
  vix::print("still valid");
}
```

Correct: after `start`, treat the task as transferred.

### Forgetting to stop the context

Wrong:

```cpp
vix::async::core::task<void> app(vix::async::core::io_context &)
{
  vix::print("done");
  co_return;
}
```

Correct:

```cpp
vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  vix::print("done");

  ctx.stop();
  co_return;
}
```

### Using post when you need co_await

Wrong:

```cpp
ctx.post([&ctx]()
{
  co_await ctx.timers().sleep_for(std::chrono::milliseconds(100));
});
```

Correct:

```cpp
vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  co_await ctx.timers().sleep_for(std::chrono::milliseconds(100));

  ctx.stop();
  co_return;
}
```

## Best practices

Use one top-level `task<void>` for the main async application. Start the top-level task from `main`. Use `co_await` to compose tasks inside other tasks. Use `ctx.post` only for simple callbacks. Call `ctx.stop()` when the top-level async flow is complete. Do not use a task after moving it into `start`. Keep detached work small and observable.

## Related pages

| Page                                          | Purpose                                       |
| --------------------------------------------- | --------------------------------------------- |
| [io_context](/modules/async/io-context)       | Learn the runtime context and scheduler loop. |
| [Tasks](/modules/async/tasks)                 | Learn `task<T>` and `task<void>`.             |
| [Timers](/modules/async/timers)               | Learn coroutine sleeps and delayed callbacks. |
| [Cancellation](/modules/async/cancellation)   | Learn cooperative cancellation.               |
| [Thread pool](/modules/async/thread-pool)     | Learn background execution.                   |
| [when_all / when_any](/modules/async/when)    | Learn task composition.                       |
| [API Reference](/modules/async/api-reference) | See the public API surface.                   |

## Next step

Continue with [timers](/modules/async/timers).
