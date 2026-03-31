# when

This page documents `when_all()` and `when_any()` from `vix::async::core`.

They let you run multiple `task<T>` concurrently and wait for:
- all tasks to finish (`when_all`)
- the first task to finish (`when_any`)

Both utilities are scheduler-driven and integrate with Vix's `task` and `scheduler` types.

## Header

```cpp
#include <vix/async/core/when.hpp>
```

## Mental model

### What these functions do

- `when_all(sched, t1, t2, ...)`
  - starts all tasks concurrently
  - resumes when every task finishes
  - returns a tuple of results in the same order as inputs

- `when_any(sched, t1, t2, ...)`
  - starts all tasks concurrently
  - resumes when the first task finishes
  - returns:
    - the winner index
    - a tuple where the winner slot contains its result

### Why the scheduler is required

Both functions:
- post work onto the scheduler
- resume the awaiting coroutine by posting the continuation back onto the scheduler thread

So the scheduler is the "home thread" for resumption and coordination.

## Return types

### void tasks are mapped to std::monostate

To keep a consistent tuple type, `task<void>` results become `std::monostate` in output tuples.

Example:
```cpp
task<void> a();
task<int>  b();

auto tup = co_await when_all(sched, a(), b());
// type: std::tuple<std::monostate, int>
```

### when_all return type

```cpp
template <typename... Ts>
task<std::tuple<std::conditional_t<std::is_void_v<Ts>, std::monostate, Ts>...>>
when_all(scheduler& sched, task<Ts>... ts);
```

### when_any return type

```cpp
template <typename... Ts>
task<std::pair<std::size_t,
               std::tuple<std::conditional_t<std::is_void_v<Ts>, std::monostate, Ts>...>>>
when_any(scheduler& sched, task<Ts>... ts);
```

- `.first` is the winner index in `[0..N-1]`
- `.second` is the tuple of results (only the winner is guaranteed to be populated meaningfully)

## Exception behavior

### when_all

- each runner captures exceptions
- the first captured exception is rethrown when `when_all` resumes
- result tuple is returned only if no exception was captured

### when_any

- an exception captured by any runner is stored in `st->ex`
- the awaiter rethrows if `st->ex` is set when resuming

Important:
- This design is simple and deterministic.
- It means a losing task that throws could still cause `when_any` to throw, depending on timing.
- If you want "only winner decides", evolve the state to store the winner's exception only.

## Concurrency and lifetime notes

- Internally, both functions start runner coroutines in detached mode using `task<void>::start(scheduler&)`.
- Shared state (`std::shared_ptr`) owns:
  - remaining counters / done flag
  - continuation handle
  - stored results
  - first exception (if any)

This keeps coordination safe even if tasks finish on different timings.

## Examples

### Example 1: when_all with values

```cpp
#include <vix/async/core/io_context.hpp>
#include <vix/async/core/task.hpp>
#include <vix/async/core/when.hpp>

using vix::async::core::task;

task<int> fetch_a()
{
  co_return 10;
}

task<std::string> fetch_b()
{
  co_return std::string("ok");
}

task<void> demo(vix::async::core::io_context& ctx)
{
  auto& sched = ctx.get_scheduler();

  auto [a, b] = co_await when_all(sched, fetch_a(), fetch_b());
  // a == 10
  // b == "ok"

  co_return;
}
```

### Example 2: when_all with void tasks

```cpp
task<void> step1()
{
  co_return;
}

task<int> step2()
{
  co_return 42;
}

task<void> demo(vix::async::core::io_context& ctx)
{
  auto& sched = ctx.get_scheduler();

  auto tup = co_await when_all(sched, step1(), step2());
  // type: std::tuple<std::monostate, int>

  auto x = std::get<1>(tup); // 42
  (void)x;

  co_return;
}
```

### Example 3: when_any (race)

```cpp
task<int> fast()
{
  co_return 1;
}

task<int> slow()
{
  co_return 2;
}

task<void> demo(vix::async::core::io_context& ctx)
{
  auto& sched = ctx.get_scheduler();

  auto [idx, tup] = co_await when_any(sched, slow(), fast());

  // idx is either 0 or 1
  // winner result is in std::get<idx>(tup)

  int winner = (idx == 0) ? std::get<0>(tup) : std::get<1>(tup);
  (void)winner;

  co_return;
}
```

### Example 4: running end-to-end

Minimal pattern:
- build your top-level coroutine
- schedule it on the scheduler
- run the context loop

```cpp
#include <vix/async/core/io_context.hpp>
#include <vix/async/core/spawn.hpp>

vix::async::core::task<void> main_task(vix::async::core::io_context& ctx);

int main()
{
  vix::async::core::io_context ctx;

  vix::async::core::spawn_detached(ctx, main_task(ctx));
  ctx.run();

  return 0;
}
```

## Design notes

- `when_all` uses a `remaining` counter and resumes the continuation when it reaches zero.
- `when_any` uses an atomic `done` flag and only the first finisher posts the continuation.
- Resumption is posted back to the scheduler so you get deterministic thread affinity.

If you later want cancellation of losers for `when_any`, pair it with `cancel_source` and pass `cancel_token` into the tasks you start, then cancel the losers when the winner completes.

