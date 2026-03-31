# spawn

`spawn` is the tiny "fire and forget" helper for Vix async tasks.

It takes a `task<void>`, schedules it onto an `io_context` scheduler, and lets it run without returning anything to the caller.

This is useful for background work where you do not want to propagate a result, for example telemetry, cache warmups, periodic refresh, or "best effort" cleanup.

## What it provides

- `vix::async::core::spawn_detached(io_context&, task<void>)`
- An internal helper coroutine type: `detail::detached_task`

`spawn_detached` is intentionally small and deterministic:

- The coroutine is posted to `io_context` via `ctx.post(handle)`.
- The coroutine frame destroys itself at the end (final suspend).
- Exceptions are swallowed by design (detached tasks have no observer).

## API

```cpp
namespace vix::async::core {
  inline void spawn_detached(io_context& ctx, task<void> t);
}
```

### Behavior

1. Wrap the provided `task<void>` into an internal coroutine (`make_detached`).
2. Post the wrapper coroutine handle to the `io_context` scheduler.
3. The wrapper `co_await`s the original task.
4. At completion, `final_suspend` destroys the wrapper frame.

## How it works internally

### `detail::detached_task`

`detached_task` is a dedicated coroutine type with a promise that:

- starts suspended (`initial_suspend = suspend_always`)
- self-destroys at final suspend
- swallows exceptions in `unhandled_exception()`

That makes it safe as a scheduler posted coroutine handle:

- it will not leak (self-destruction)
- it will not crash the runtime due to an unobserved exception

### `detail::make_detached(task<void>)`

```cpp
inline detached_task make_detached(task<void> t) {
  co_await t;
  co_return;
}
```

This wrapper exists so `spawn_detached` can post a coroutine handle that has a self-destroying final suspend, even if the original task implementation is updated later.

## Example

A simple "background job" that runs on the scheduler thread:

```cpp
#include <vix/async/core/io_context.hpp>
#include <vix/async/core/spawn.hpp>

using namespace vix::async::core;

task<void> background()
{
  // do something best-effort
  // if you throw here, it will be swallowed (detached)
  co_return;
}

int main()
{
  io_context ctx;

  spawn_detached(ctx, background());

  ctx.run();
  return 0;
}
```

## Notes and best practices

- `spawn_detached` is for `task<void>` only. If you need a value, return a `task<T>` and `co_await` it somewhere.
- Because exceptions are swallowed, treat detached tasks as "best effort".
  - If you need observability, add logging inside the task body or later connect `unhandled_exception()` to the logger.
- The work runs on the scheduler thread unless the task uses other services (cpu pool, timers, net) and awaits them.

## Related

- `task<T>`: coroutine result type and ownership model
- `scheduler`: single-thread scheduler queue and event loop
- `io_context`: runtime context that owns the scheduler and lazy services

