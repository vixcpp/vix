# task

Coroutine task type for the async core runtime.

`task<T>` is Vix.cpp’s minimal coroutine return type: it represents an asynchronous computation that eventually produces a value of type `T` (or throws). Tasks are lazy by default: they start suspended and only run when awaited or explicitly scheduled.

This page documents the public behavior of `vix::async::core::task` as implemented in `task.hpp`.

---

## Header

```cpp
#include <vix/async/core/task.hpp>
```

Namespace:

```cpp
vix::async::core
```

---

## What `task<T>` is

`task<T>` is a move-only coroutine handle owner with these properties:

- **Lazy**: created suspended (`initial_suspend()` is `suspend_always`).
- **Single-consumer result**: the produced value is moved out once in `await_resume()`.
- **Exception-aware**: exceptions are captured and rethrown from `await_resume()`.
- **Continuation-based**: `co_await task` resumes the awaiting coroutine at final suspend.
- **Detachable**: `std::move(t).start(sched)` schedules the coroutine and releases ownership.

There is also a specialization:

- `task<void>`: same behavior, but no value.

---

## Lifecycle and suspension model

### Lazy start

A task does not start running when you create it. It starts when:

- You `co_await` it, or
- You call `std::move(task).start(scheduler)`.

This comes from:

- `promise_common::initial_suspend()` returns `std::suspend_always`.

### Continuation wiring

When you `co_await` a task:

1. The awaiter stores the awaiting coroutine handle into `promise.continuation`.
2. The runtime resumes the task coroutine.
3. At `final_suspend`, the task returns the continuation handle so the runtime resumes it.

If there is no continuation and the task is detached, the coroutine frame destroys itself at final suspend.

---

## Basic usage

### Await a task that returns a value

```cpp
#include <vix/async/core/task.hpp>

using vix::async::core::task;

task<int> compute()
{
  co_return 42;
}

task<void> demo()
{
  int v = co_await compute();
  (void)v;
  co_return;
}
```

### Await a task that returns void

```cpp
using vix::async::core::task;

task<void> do_work()
{
  co_return;
}

task<void> demo()
{
  co_await do_work();
  co_return;
}
```

---

## Error propagation

If a task throws, the exception is captured in the promise via `unhandled_exception()` and rethrown in `await_resume()`.

Example:

```cpp
#include <stdexcept>
#include <vix/async/core/task.hpp>

using vix::async::core::task;

task<int> fails()
{
  throw std::runtime_error("boom");
  co_return 0;
}

task<void> demo()
{
  try
  {
    int x = co_await fails();
    (void)x;
  }
  catch (const std::exception &)
  {
    // handle error
  }
  co_return;
}
```

## Detaching and scheduling

### `start(scheduler&)`

`start()` detaches the task and posts it onto a scheduler:

- Marks `promise.detached = true`
- Posts the coroutine handle to `scheduler::post(handle)`
- Releases ownership (`task` becomes empty)

Signature (both `task<T>` and `task<void>`):

```cpp
void start(scheduler &sched) && noexcept;
```

Important implications:

- You must call it on an rvalue: `std::move(t).start(sched)`.
- After `start()`, the task object no longer owns the coroutine frame.
- If nobody awaits it (no continuation), the frame self-destroys at final suspend.

Example:

```cpp
#include <vix/async/core/scheduler.hpp>
#include <vix/async/core/task.hpp>

using vix::async::core::scheduler;
using vix::async::core::task;

task<void> background_job()
{
  // do something
  co_return;
}

void run_detached()
{
  scheduler sched;

  // fire-and-forget
  std::move(background_job()).start(sched);

  // drive the scheduler on this thread
  sched.run();
}
```

---

## Awaiting from coroutines

### `operator co_await`

Both lvalue and rvalue `co_await` are supported:

```cpp
auto operator co_await() & noexcept;
auto operator co_await() && noexcept;
```

Behavior:

- `await_ready()` is true if the handle is empty or already completed.
- `await_suspend(awaiting)` stores the continuation and resumes the task.
- `await_resume()` rethrows captured exceptions and returns/moves the result.

---

## Ownership rules

- `task` is **move-only** (copy is deleted).
- Destroying a non-detached `task` destroys its coroutine frame (`h_.destroy()`).
- After `start()`, the task releases its handle, so its destructor does nothing.

You can inspect:

```cpp
bool valid() const noexcept;
explicit operator bool() const noexcept;
handle_type handle() const noexcept;
```

---

## Notes and constraints

- `task<T>` rejects reference result types at compile time:

  `static_assert(!std::is_reference_v<T>)`

  If you need to return references, use `task<std::reference_wrapper<T>>`.

- The implementation is intentionally minimal: it does not provide cancellation, timeouts, or executors directly. Those are built around it (e.g. `cancel_token`, `io_context`, timers).

---

## Related

- `scheduler` for driving posted tasks and coroutine resumptions
- `io_context` as the higher-level runtime that owns a scheduler and services
- `cancel_source` / `cancel_token` for cooperative cancellation

