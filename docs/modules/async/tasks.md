# Tasks

This guide shows how to use `task<T>` and `task<void>` in Vix Async.

Use this page when you want to write coroutine-based async functions, return values, await other tasks, and start async work on an `io_context`.

## Public header

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>
```

## What is a task?

`vix::async::core::task<T>` represents an asynchronous computation that eventually produces a value of type `T`.

```cpp
vix::async::core::task<int>
```

`task<void>` represents async work that completes without returning a value.

```cpp
vix::async::core::task<void>
```

Tasks are coroutine-friendly.

```cpp
co_return 42;
co_await other_task();
```

## Minimal task

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

Run:

```
vix run main.cpp
```

Expected output:

```
value = 42
```

## task\<T\>

Use `task<T>` when the coroutine returns a value.

```cpp
vix::async::core::task<int> compute()
{
  co_return 21 * 2;
}
```

Await it from another task:

```cpp
const int value = co_await compute();
```

Complete example:

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

vix::async::core::task<int> get_number()
{
  co_return 42;
}

vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  const int number = co_await get_number();

  vix::print("number =", number);

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

## task\<void\>

Use `task<void>` when the coroutine performs async work but returns no value.

```cpp
vix::async::core::task<void> say_hello()
{
  vix::print("hello");
  co_return;
}
```

Await it from another task:

```cpp
co_await say_hello();
```

Complete example:

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

vix::async::core::task<void> say_hello()
{
  vix::print("hello from task");
  co_return;
}

vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  co_await say_hello();

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
hello from task
```

## Tasks are lazy

A task does not run just because it was created.

Wrong:

```cpp
auto t = app(ctx);
```

Correct:

```cpp
auto t = app(ctx);
std::move(t).start(ctx.get_scheduler());

ctx.run();
```

A task must be either awaited by another coroutine, or started on a scheduler.

## Start a task

Use `start` to schedule a task on the `io_context` scheduler.

```cpp
auto t = app(ctx);
std::move(t).start(ctx.get_scheduler());
```

Then run the context:

```cpp
ctx.run();
```

Complete example:

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  vix::print("task started");

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
task started
```

## Await another task

Tasks can call other tasks with `co_await`.

```cpp
vix::async::core::task<std::string> load_name()
{
  co_return "Ada";
}

vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  const std::string name = co_await load_name();

  vix::print("name =", name);

  ctx.stop();
  co_return;
}
```

Complete example:

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

#include <string>

vix::async::core::task<std::string> load_name()
{
  co_return "Ada";
}

vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  const std::string name = co_await load_name();

  vix::print("name =", name);

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
name = Ada
```

## Await a timer

Tasks become useful when combined with services like timers.

```cpp
co_await ctx.timers().sleep_for(std::chrono::milliseconds(100));
```

Example:

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

## Return values

Use `co_return` to return a value from `task<T>`.

```cpp
vix::async::core::task<int> add(int a, int b)
{
  co_return a + b;
}
```

Then await it:

```cpp
const int sum = co_await add(20, 22);
```

Complete example:

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

vix::async::core::task<int> add(int a, int b)
{
  co_return a + b;
}

vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  const int result = co_await add(20, 22);

  vix::print("result =", result);

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
result = 42
```

## Exceptions

If a task throws, the exception is captured and rethrown when the task is awaited.

```cpp
vix::async::core::task<int> fail()
{
  throw std::runtime_error("task failed");
  co_return 0;
}
```

Handle it in the awaiting task:

```cpp
try
{
  const int value = co_await fail();
  vix::print(value);
}
catch (const std::exception &ex)
{
  vix::eprint(ex.what());
}
```

Complete example:

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

#include <exception>
#include <stdexcept>

vix::async::core::task<int> fail()
{
  throw std::runtime_error("task failed");
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
task failed
```

## Move-only task

`task<T>` is move-only. This prevents accidental sharing of coroutine ownership.

Wrong:

```cpp
auto a = app(ctx);
auto b = a;
```

Correct:

```cpp
auto a = app(ctx);
auto b = std::move(a);
```

## Check if a task is valid

Use `valid()` or the boolean conversion.

```cpp
auto t = app(ctx);

if (t.valid())
{
  vix::print("task is valid");
}
```

Equivalent:

```cpp
if (t)
{
  vix::print("task is valid");
}
```

## Release a task handle

Advanced code can release the coroutine handle.

```cpp
auto h = t.release();
```

Most user code should prefer:

```cpp
std::move(t).start(ctx.get_scheduler());
```

## Common workflows

### Create a task that returns a value

```cpp
vix::async::core::task<int> compute()
{
  co_return 42;
}
```

### Create a task that returns nothing

```cpp
vix::async::core::task<void> work()
{
  vix::print("working");
  co_return;
}
```

### Await a value task

```cpp
const int value = co_await compute();
```

### Await a void task

```cpp
co_await work();
```

### Start the main task

```cpp
auto t = app(ctx);
std::move(t).start(ctx.get_scheduler());

ctx.run();
```

### Stop from the main task

```cpp
ctx.stop();
co_return;
```

## Common mistakes

### Creating a task but never starting it

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

If the main task finishes but nothing calls `ctx.stop()`, the scheduler may keep waiting.

```cpp
vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  vix::print("done");

  ctx.stop();
  co_return;
}
```

### Returning a value from task\<void\>

Wrong:

```cpp
vix::async::core::task<void> work()
{
  co_return 42;
}
```

Correct:

```cpp
vix::async::core::task<void> work()
{
  co_return;
}
```

### Forgetting co_return in a task

A task coroutine should complete with `co_return`.

```cpp
vix::async::core::task<void> work()
{
  vix::print("done");
  co_return;
}
```

### Copying a task

Wrong:

```cpp
auto t1 = app(ctx);
auto t2 = t1;
```

Correct:

```cpp
auto t1 = app(ctx);
auto t2 = std::move(t1);
```

## Best practices

Use `task<void>` for the main async application function. Use `task<T>` for async functions that return data. Keep tasks small and composable. Await tasks from other tasks. Start only the top-level task from `main`. Call `ctx.stop()` when the top-level async work is complete. Use timers and thread pools instead of blocking the scheduler thread.

## Related pages

| Page                                          | Purpose                                       |
| --------------------------------------------- | --------------------------------------------- |
| [io_context](/modules/async/io-context)       | Learn the runtime context and scheduler loop. |
| [Spawn](/modules/async/spawn)                 | Learn detached task startup.                  |
| [Timers](/modules/async/timers)               | Learn coroutine sleeps and delayed callbacks. |
| [Cancellation](/modules/async/cancellation)   | Learn cooperative cancellation.               |
| [Thread pool](/modules/async/thread-pool)     | Learn background execution.                   |
| [when_all / when_any](/modules/async/when)    | Learn task composition.                       |
| [API Reference](/modules/async/api-reference) | See the public API surface.                   |

## Next step

Continue with [spawn](/modules/async/spawn).
