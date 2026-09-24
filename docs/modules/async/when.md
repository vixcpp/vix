# when_all / when_any

This guide shows how to compose multiple async tasks with `when_all` and `when_any`.

Use this page when you want to run several tasks concurrently and wait until all of them finish, or until the first one finishes.

## Public header

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>
```

## What when_all and when_any provide

Vix Async provides two task composition helpers.

| API        | Purpose                                                   |
| ---------- | --------------------------------------------------------- |
| `when_all` | Run multiple tasks and wait for all results.              |
| `when_any` | Run multiple tasks and wait for the first completed task. |

Both APIs need a scheduler.

```cpp
ctx.get_scheduler()
```

Both APIs work with `task<T>` and `task<void>`.

## Basic when_all

Use `when_all` when every task result is needed.

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

#include <tuple>

vix::async::core::task<int> first()
{
  co_return 20;
}

vix::async::core::task<int> second()
{
  co_return 22;
}

vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  auto results = co_await vix::async::core::when_all(
      ctx.get_scheduler(),
      first(),
      second());

  const int a = std::get<0>(results);
  const int b = std::get<1>(results);

  vix::print("sum =", a + b);

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
sum = 42
```

## when_all

`when_all` starts all tasks and resumes when all of them complete.

```cpp
auto results = co_await vix::async::core::when_all(
    ctx.get_scheduler(),
    task_one(),
    task_two());
```

Results are returned in the same order as the input tasks.

```cpp
const auto first_result = std::get<0>(results);
const auto second_result = std::get<1>(results);
```

## when_all with timers

This example runs two timer-based tasks.

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

#include <chrono>
#include <tuple>

using namespace std::chrono_literals;

vix::async::core::task<int> load_a(vix::async::core::io_context &ctx)
{
  co_await ctx.timers().sleep_for(50ms);
  co_return 20;
}

vix::async::core::task<int> load_b(vix::async::core::io_context &ctx)
{
  co_await ctx.timers().sleep_for(100ms);
  co_return 22;
}

vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  auto results = co_await vix::async::core::when_all(
      ctx.get_scheduler(),
      load_a(ctx),
      load_b(ctx));

  const int a = std::get<0>(results);
  const int b = std::get<1>(results);

  vix::print("result =", a + b);

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

## when_all with void tasks

When a task returns void, the result slot is represented as `std::monostate`.

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

#include <chrono>
#include <tuple>
#include <variant>

using namespace std::chrono_literals;

vix::async::core::task<void> first(vix::async::core::io_context &ctx)
{
  co_await ctx.timers().sleep_for(50ms);

  vix::print("first done");
  co_return;
}

vix::async::core::task<void> second(vix::async::core::io_context &ctx)
{
  co_await ctx.timers().sleep_for(100ms);

  vix::print("second done");
  co_return;
}

vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  auto results = co_await vix::async::core::when_all(
      ctx.get_scheduler(),
      first(ctx),
      second(ctx));

  (void)std::get<0>(results);
  (void)std::get<1>(results);

  vix::print("all tasks completed");

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
first done
second done
all tasks completed
```

## when_all with mixed results

You can combine value tasks and void tasks.

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

#include <chrono>
#include <string>
#include <tuple>
#include <variant>

using namespace std::chrono_literals;

vix::async::core::task<std::string> load_name(
    vix::async::core::io_context &ctx)
{
  co_await ctx.timers().sleep_for(50ms);
  co_return "Ada";
}

vix::async::core::task<int> load_score(
    vix::async::core::io_context &ctx)
{
  co_await ctx.timers().sleep_for(100ms);
  co_return 42;
}

vix::async::core::task<void> log_step(
    vix::async::core::io_context &ctx)
{
  co_await ctx.timers().sleep_for(25ms);

  vix::print("loading data");
  co_return;
}

vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  auto results = co_await vix::async::core::when_all(
      ctx.get_scheduler(),
      load_name(ctx),
      load_score(ctx),
      log_step(ctx));

  const std::string name = std::get<0>(results);
  const int score = std::get<1>(results);

  vix::print("name =", name);
  vix::print("score =", score);

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
loading data
name = Ada
score = 42
```

## Basic when_any

Use `when_any` when you only need the first completed task.

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

#include <chrono>
#include <tuple>

using namespace std::chrono_literals;

vix::async::core::task<int> fast(vix::async::core::io_context &ctx)
{
  co_await ctx.timers().sleep_for(50ms);
  co_return 1;
}

vix::async::core::task<int> slow(vix::async::core::io_context &ctx)
{
  co_await ctx.timers().sleep_for(200ms);
  co_return 2;
}

vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  auto result = co_await vix::async::core::when_any(
      ctx.get_scheduler(),
      fast(ctx),
      slow(ctx));

  const std::size_t index = result.first;
  auto values = std::move(result.second);

  vix::print("winner =", index);

  if (index == 0 && std::get<0>(values).has_value())
  {
    vix::print("value =", *std::get<0>(values));
  }

  if (index == 1 && std::get<1>(values).has_value())
  {
    vix::print("value =", *std::get<1>(values));
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
winner = 0
value = 1
```

## when_any return value

`when_any` returns a pair.

```cpp
auto result = co_await vix::async::core::when_any(
    ctx.get_scheduler(),
    first(),
    second());
```

| Field           | Meaning                            |
| --------------- | ---------------------------------- |
| `result.first`  | Index of the first completed task. |
| `result.second` | Tuple of optional result slots.    |

Example:

```cpp
const std::size_t index = result.first;
auto values = std::move(result.second);
```

For the winning task, the corresponding slot contains a value.

```cpp
if (index == 0 && std::get<0>(values).has_value())
{
  vix::print(*std::get<0>(values));
}
```

## when_any with timeout-style behavior

You can use `when_any` to race a real task against a timer.

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

#include <chrono>
#include <tuple>
#include <variant>

using namespace std::chrono_literals;

vix::async::core::task<int> load_data(vix::async::core::io_context &ctx)
{
  co_await ctx.timers().sleep_for(500ms);
  co_return 42;
}

vix::async::core::task<void> timeout(vix::async::core::io_context &ctx)
{
  co_await ctx.timers().sleep_for(100ms);
  co_return;
}

vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  auto result = co_await vix::async::core::when_any(
      ctx.get_scheduler(),
      load_data(ctx),
      timeout(ctx));

  const std::size_t index = result.first;
  auto values = std::move(result.second);

  if (index == 0 && std::get<0>(values).has_value())
  {
    vix::print("data =", *std::get<0>(values));
  }
  else
  {
    vix::print("timeout");
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
timeout
```

## when_all vs when_any

| API        | Completes when       | Return value                           |
| ---------- | -------------------- | -------------------------------------- |
| `when_all` | All tasks finish.    | Tuple of all results.                  |
| `when_any` | First task finishes. | Pair of winner index and result slots. |

Use `when_all` when all results are required. Use `when_any` when the first result is enough.

## Exceptions

If a task throws, the exception is captured and rethrown when the composition resumes.

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

#include <exception>
#include <stdexcept>
#include <tuple>

vix::async::core::task<int> ok()
{
  co_return 42;
}

vix::async::core::task<int> fail()
{
  throw std::runtime_error("task failed");
  co_return 0;
}

vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  try
  {
    auto results = co_await vix::async::core::when_all(
        ctx.get_scheduler(),
        ok(),
        fail());

    vix::print("value =", std::get<0>(results));
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

## Scheduler requirement

Both helpers require a scheduler.

```cpp
ctx.get_scheduler()
```

This is because the helpers start runner coroutines internally.

```cpp
auto results = co_await vix::async::core::when_all(
    ctx.get_scheduler(),
    first(),
    second());
```

For normal application code, pass the scheduler from the same `io_context` that owns the task.

## Task ownership

Tasks passed to `when_all` or `when_any` are moved. Do not try to reuse those task objects after passing them to the composition helper.

## Common workflows

### Run two tasks and collect both results

```cpp
auto results = co_await vix::async::core::when_all(
    ctx.get_scheduler(),
    first(),
    second());

const auto a = std::get<0>(results);
const auto b = std::get<1>(results);
```

### Run tasks and wait for the first result

```cpp
auto result = co_await vix::async::core::when_any(
    ctx.get_scheduler(),
    fast(),
    slow());

const std::size_t index = result.first;
```

### Race work against a timeout

```cpp
auto result = co_await vix::async::core::when_any(
    ctx.get_scheduler(),
    load_data(ctx),
    timeout(ctx));

if (result.first == 1)
{
  vix::print("timeout");
}
```

### Combine mixed return types

```cpp
auto results = co_await vix::async::core::when_all(
    ctx.get_scheduler(),
    load_name(ctx),
    load_score(ctx),
    log_step(ctx));

const auto name = std::get<0>(results);
const auto score = std::get<1>(results);
```

### Catch composition errors

```cpp
try
{
  auto results = co_await vix::async::core::when_all(
      ctx.get_scheduler(),
      first(),
      risky());
}
catch (const std::exception &ex)
{
  vix::eprint(ex.what());
}
```

## Common mistakes

### Forgetting to pass the scheduler

Wrong:

```cpp
auto results = co_await vix::async::core::when_all(
    first(),
    second());
```

Correct:

```cpp
auto results = co_await vix::async::core::when_all(
    ctx.get_scheduler(),
    first(),
    second());
```

### Expecting when_any to return the value directly

Wrong:

```cpp
auto value = co_await vix::async::core::when_any(
    ctx.get_scheduler(),
    fast(),
    slow());
```

Correct:

```cpp
auto result = co_await vix::async::core::when_any(
    ctx.get_scheduler(),
    fast(),
    slow());

const std::size_t index = result.first;
auto values = std::move(result.second);
```

### Forgetting optional checks in when_any

Wrong:

```cpp
vix::print(*std::get<0>(values));
```

Correct:

```cpp
if (std::get<0>(values).has_value())
{
  vix::print(*std::get<0>(values));
}
```

### Reusing moved tasks

Wrong:

```cpp
auto a = first();
auto b = second();

auto results = co_await vix::async::core::when_all(
    ctx.get_scheduler(),
    std::move(a),
    std::move(b));

std::move(a).start(ctx.get_scheduler());
```

Correct:

```cpp
auto results = co_await vix::async::core::when_all(
    ctx.get_scheduler(),
    first(),
    second());
```

### Forgetting to stop the context

The parent task should stop the context when the composition is done.

```cpp
ctx.stop();
co_return;
```

## Best practices

Use `when_all` when every result is required. Use `when_any` for races, fallback behavior, and timeout-style flows. Pass `ctx.get_scheduler()` from the same runtime context. Keep tasks independent when running them concurrently. Handle exceptions around composition calls. Check optional result slots when using `when_any`. Call `ctx.stop()` when the top-level async flow is complete.

## Related pages

| Page                                          | Purpose                                  |
| --------------------------------------------- | ---------------------------------------- |
| [io_context](/modules/async/io-context)       | Learn the runtime context and scheduler. |
| [Tasks](/modules/async/tasks)                 | Learn `task<T>` and `task<void>`.        |
| [Spawn](/modules/async/spawn)                 | Learn how to start tasks.                |
| [Timers](/modules/async/timers)               | Learn timer-based tasks.                 |
| [Cancellation](/modules/async/cancellation)   | Learn cooperative cancellation.          |
| [Thread pool](/modules/async/thread-pool)     | Learn background execution.              |
| [API Reference](/modules/async/api-reference) | See the public API surface.              |

## Next step

Continue with [signals](/modules/async/signals).
