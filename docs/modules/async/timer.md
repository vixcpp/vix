# Timer

`vix::async::core::timer` is a small timer service integrated with `io_context`.

It provides two primary features:

- `after()`: schedule a callback to run after a delay
- `sleep_for()`: coroutine-friendly delay that resumes on the `io_context` scheduler thread

The timer is designed to be explicit and deterministic:
- it uses a dedicated worker thread to wait for deadlines
- it posts completions back onto the `io_context` scheduler thread
- it supports per-entry cancellation via `cancel_token`

---

## Key types

- `timer::clock`: `std::chrono::steady_clock` (monotonic, safe for timeouts)
- `timer::time_point`: deadline timestamp
- `timer::duration`: delay duration

Because the clock is steady, timers are not affected by wall clock changes.

---

## Lifecycle and threading model

When you construct a `timer`, it starts a worker thread that:

1. Waits until the earliest deadline in the ordered queue.
2. When a deadline fires, it checks cancellation.
3. If not cancelled, it posts the job back to the `io_context` scheduler thread.

Important:
- Scheduled callbacks do NOT run on the timer worker thread.
- They run on the scheduler thread via `io_context::post()` (through `ctx_post()`).
- This keeps all your async callbacks serialized on the runtime thread, matching the rest of the async core.

Destruction:
- `~timer()` stops the worker and releases queued jobs.
- You can also call `stop()` explicitly.

---

## Schedule a callback with `after()`

### Signature

```cpp
template <typename Fn>
void after(duration d, Fn&& fn, cancel_token ct = {});
```

### Behavior

- Computes `deadline = clock::now() + d`
- Wraps your callable into a type-erased job
- Inserts it into the ordered queue
- Wakes the worker so it can recalculate the next deadline
- If `ct.is_cancelled()` is true when the deadline fires, the job is skipped

### Example

```cpp
#include <vix/async/core/io_context.hpp>
#include <vix/async/core/timer.hpp>

using namespace vix::async::core;

static void run_example()
{
  io_context ctx;
  auto& t = ctx.timers();

  t.after(std::chrono::seconds(1), [&]()
  {
    // Runs on the io_context scheduler thread
    // not on the timer worker thread
    // Do short work here
  });

  ctx.run();
}

int main()
{
  run_example();
}
```

Tip:
- Keep `after()` callbacks short. If you need CPU-heavy work, submit it to `ctx.cpu_pool()` and continue from there.

---

## Coroutine sleep with `sleep_for()`

### Signature

```cpp
task<void> sleep_for(duration d, cancel_token ct = {});
```

### Behavior

- Suspends the awaiting coroutine
- Schedules a timer entry that posts the coroutine handle back onto the scheduler thread
- When resumed, it continues on the `io_context` scheduler thread

This gives you an ergonomic delay inside coroutines without blocking any thread.

### Example

```cpp
#include <vix/async/core/io_context.hpp>
#include <vix/async/core/task.hpp>
#include <vix/async/core/timer.hpp>
#include <vix/async/core/spawn.hpp>

using namespace vix::async::core;

static task<void> job(io_context& ctx)
{
  auto& t = ctx.timers();

  // Hop onto scheduler thread (optional but common in this design)
  co_await ctx.get_scheduler().schedule();

  co_await t.sleep_for(std::chrono::milliseconds(250));
  co_await t.sleep_for(std::chrono::milliseconds(250));

  co_return;
}

static void run_example()
{
  io_context ctx;

  // Run coroutine in detached mode
  spawn_detached(ctx, job(ctx));

  ctx.run();
}

int main()
{
  run_example();
}
```

---

## Cancellation

Both `after()` and `sleep_for()` accept a `cancel_token`.

- If the token is cancelled before the deadline, the entry is skipped.
- If you want cancellation errors (instead of "skip"), implement that at the call site:
  - request cancel
  - decide whether you treat "not fired" as success or as cancellation

### Example with cancel_source

```cpp
#include <vix/async/core/cancel.hpp>
#include <vix/async/core/io_context.hpp>
#include <vix/async/core/task.hpp>
#include <vix/async/core/timer.hpp>
#include <vix/async/core/spawn.hpp>

using namespace vix::async::core;

static task<void> cancellable_sleep(io_context& ctx)
{
  cancel_source src;
  cancel_token ct = src.token();

  // Cancel after 100ms
  ctx.timers().after(std::chrono::milliseconds(100), [&]()
  {
    src.request_cancel();
  });

  // This sleep will be skipped if cancellation is observed before firing
  co_await ctx.timers().sleep_for(std::chrono::seconds(1), ct);

  co_return;
}

static void run_example()
{
  io_context ctx;
  spawn_detached(ctx, cancellable_sleep(ctx));
  ctx.run();
}

int main()
{
  run_example();
}
```

Note:
- The current timer contract is "skip on cancel".
- If you want `sleep_for()` to throw `std::system_error(cancelled_ec())`, you can implement that in `sleep_for()` by checking the token and capturing an exception for the awaiting coroutine.

---

## Ordering and tie breaking

Timer entries are stored in a `std::multiset` ordered by:

1. `when` (deadline time)
2. `id` (monotonic sequence number)

This makes ordering stable even when multiple entries share the same deadline.

---

## Common pitfalls

- Using long work inside `after()` callback
  - Fix: offload to `cpu_pool().submit(...)` and resume later.
- Forgetting to run the scheduler
  - `timer` posts completions onto the scheduler thread, so you must call `ctx.run()`.
- Expecting exact timing
  - Like any timer, wakeups depend on OS scheduling and load. Use it for delays and timeouts, not for precise real-time scheduling.

---

## Recommended pattern

Use the timer for:
- timeouts around I/O
- retries with backoff
- periodic tasks (reschedule inside the callback)
- coroutine delays

Keep business logic:
- on the scheduler thread for short operations
- on the CPU pool for heavy compute

