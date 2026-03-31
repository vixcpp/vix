# Async

The `vix::async` module provides a coroutine-based asynchronous runtime.

This guide explains the core building blocks and how they fit together.
It focuses on patterns, not full showcase applications.

---

# Core Concepts

Main components:

- `io_context` → event loop
- `task<T>` → coroutine task
- `timers()` → async sleep
- `cpu_pool()` → offload CPU work
- `signals()` → signal handling
- `spawn_detached()` → fire-and-forget coroutine

Async in Vix is:

- Explicit
- Coroutine-based
- No hidden threads
- Fully controlled by your event loop

---

# 1. Event loop and task

Every async program starts with an `io_context`.

Minimal structure:

```cpp
#include <vix/async/core/io_context.hpp>
#include <vix/async/core/task.hpp>

using vix::async::core::io_context;
using vix::async::core::task;

task<void> app(io_context& ctx)
{
  // async logic here
  ctx.stop();
  co_return;
}

int main()
{
  io_context ctx;

  auto t = app(ctx);
  ctx.post(t.handle());

  ctx.run();
}
```

Flow:

1. Create `io_context`
2. Create a `task`
3. Post the coroutine handle
4. Call `run()`

Nothing runs automatically. You control the loop.

---

# 2. Timers

Use `ctx.timers().sleep_for()` for async delays.

```cpp
co_await ctx.timers().sleep_for(
  std::chrono::milliseconds(50)
);
```

This does not block the thread.
The coroutine suspends and resumes later.

---

# 3. CPU pool

Heavy CPU work should not block the event loop.

Use:

```cpp
int result = co_await ctx.cpu_pool().submit([]{
  int sum = 0;
  for (int i = 0; i < 100000; ++i)
    sum += (i % 7);
  return sum;
});
```

This runs the lambda on a worker thread pool.
The coroutine resumes when the result is ready.

Rule:

- I/O and coordination stay in the event loop.
- CPU-heavy work goes to `cpu_pool()`.

---

# 4. Signals

To handle SIGINT or SIGTERM:

```cpp
auto& sig = ctx.signals();

sig.add(SIGINT);
sig.add(SIGTERM);

sig.on_signal([&](int s){
  ctx.stop();
});

co_await sig.async_wait();
```

This allows clean shutdown logic.

---

# 5. TCP server pattern

Async servers follow this structure:

1. Create listener
2. `async_listen`
3. Loop with `async_accept`
4. Spawn per-connection task

Pattern:

```cpp
auto listener = make_tcp_listener(ctx);
co_await listener->async_listen({"0.0.0.0", 9090}, 128);

while (ctx.is_running())
{
  auto client = co_await listener->async_accept();

  spawn_detached(
    ctx,
    handle_client(std::move(client))
  );
}
```

Each client is handled in its own coroutine.
No hidden threads are created.

---

# 6. when_all and when_any

Run multiple tasks concurrently.

```cpp
auto tup = co_await when_all(sched, a(), b());
```

- Waits for all tasks
- Returns a tuple of results

```cpp
auto [index, values] =
  co_await when_any(sched, a(), b());
```

- Returns when the first task completes
- `index` tells which one finished

Useful for parallel I/O or timeouts.

---

# Execution Model

Vix async is:

- Single explicit event loop
- Cooperative scheduling
- Coroutine suspension via `co_await`
- Deterministic control over lifecycle

You decide:

- When to stop
- Where CPU work runs
- How tasks are composed

---

# Recommended Usage

- Use `io_context` as your runtime root
- Offload heavy work to `cpu_pool()`
- Always stop the context explicitly
- Prefer structured concurrency (`when_all`, `when_any`)
- Use `spawn_detached` carefully (fire-and-forget)

Async in Vix follows the same philosophy as the rest of the runtime:

Explicit control. No hidden magic.


