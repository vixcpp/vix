# signal

`signal_set` is a small asynchronous signal watcher designed to integrate POSIX signals into the Vix async runtime (`io_context` + `scheduler`).

It lets you:

- register signals you care about (`add`, `remove`)
- `co_await` the next signal via `async_wait()`
- optionally run a callback on each received signal (`on_signal`)
- stop the watcher (`stop()`)

This guide focuses on how to use the API, how it behaves, and what the important constraints are.

---

## What problem does this solve?

Signals (like `SIGINT` when you press Ctrl+C) arrive outside your normal program flow.

If you want a clean shutdown in an async program, you typically want:

- a single place to observe signals
- a way to `co_await` a signal in coroutines
- delivery of completions on your scheduler thread (not on some random signal context)

`signal_set` provides that integration point.

---

## Design model

`signal_set` is:

- bound to an `io_context`
- potentially backed by a dedicated worker thread (lazy start)
- able to deliver signal events to:
  - a single awaiting coroutine (single waiter model)
  - an optional callback

Important behaviors implied by the header:

1. **Single waiter model**
   - `signal_set` stores one `std::coroutine_handle<> waiter_` and `bool waiter_active_`.
   - That strongly suggests only one `async_wait()` is intended at a time.
   - If you need multiple consumers, build a fan-out layer (e.g. channel/queue) on top.

2. **Queue for pending signals**
   - Captured signals are buffered in `pending_`.
   - If a signal arrives before you call `async_wait()`, it can be consumed later.

3. **Callbacks run on the scheduler thread**
   - `on_signal(fn)` says the callback is posted via `io_context` posting mechanism.
   - That means your callback runs on the same thread that executes `ctx.run()`.

4. **Cancellation support**
   - `async_wait(cancel_token)` integrates with Vix cancellation.
   - If cancellation is requested, the task should complete with a cancellation error or equivalent behavior (see your `task<>` contract).

5. **Stop is explicit**
   - `stop()` requests shutdown and should wake any active waiter.

---

## Typical usage

### 1) Basic Ctrl+C handling

In an async app, you often want Ctrl+C to request cancellation and let your tasks unwind.

```cpp
#include <vix/async/core/io_context.hpp>
#include <vix/async/core/signal.hpp>
#include <vix/async/core/cancel.hpp>
#include <vix/async/core/task.hpp>

using namespace vix::async::core;

task<void> app_main(io_context& ctx, signal_set& sigs, cancel_source& cs)
{
  // Wait for SIGINT (Ctrl+C)
  int sig = co_await sigs.async_wait(cs.token());
  (void)sig;

  // Request cancellation for the rest of the system
  cs.request_cancel();

  co_return;
}

int main()
{
  io_context ctx;

  // signal_set is a lazy watcher bound to ctx
  signal_set sigs(ctx);

  // Register SIGINT
  sigs.add(SIGINT);

  cancel_source cs;

  // Start your app tasks (depends on your task runner API)
  // Example shape:
  // vix::async::core::spawn(ctx, app_main(ctx, sigs, cs));

  ctx.run();
  return 0;
}
```

Notes:

- Register signals early (before `run`) so you do not miss early signals.
- Ensure `ctx.run()` is running so posted completions can execute.

### 2) Using a callback instead of awaiting

Sometimes you just want a lightweight handler that flips a flag and triggers shutdown.

```cpp
signal_set sigs(ctx);
sigs.add(SIGINT);
sigs.add(SIGTERM);

sigs.on_signal([&](int sig){
  // Runs on the scheduler thread
  // Keep it short and safe.
  cs.request_cancel();
});
```

This is convenient when you do not want a dedicated coroutine waiting on signals.

### 3) Supporting both callback and coroutine wait

You can do both:

- callback for immediate side effects
- coroutine wait for structured shutdown sequencing

Be careful to avoid duplicate actions (e.g. requesting cancel twice is usually fine).

---

## API reference

### `signal_set(io_context& ctx)`

Binds the signal watcher to a runtime context.

The watcher can post completions and callbacks back onto `ctx` so they execute on the scheduler thread.

### `void add(int sig)`

Register a signal number (e.g. `SIGINT`, `SIGTERM`) to observe.

The implementation may start the internal worker thread lazily on first registration or first wait.

### `void remove(int sig)`

Stop observing a given signal number.

If the signal is already pending in the queue, removal does not necessarily remove it from the pending queue.

### `task<int> async_wait(cancel_token ct = {})`

Asynchronously wait for the next received signal.

Key behaviors:

- If `pending_` already contains a signal, the task can complete quickly by consuming it.
- If no pending signal exists, the coroutine suspends and `waiter_` is stored.
- Cancellation token may cancel the wait.

Because the header stores a single waiter handle, assume:

- only one active `async_wait()` at a time
- calling `async_wait()` again concurrently is either rejected, undefined, or causes overwrites

If you need multiple waits, serialize them:

```cpp
for (;;)
{
  int sig = co_await sigs.async_wait(ct);
  // handle sig
}
```

### `void on_signal(std::function<void(int)> fn)`

Register a callback invoked for each received signal.

The callback is posted onto the scheduler thread.

Rules of thumb:

- Keep it fast (no blocking I/O, no heavy work).
- Delegate heavy work to other tasks using `ctx.post(...)` or your coroutine orchestration.

### `void stop() noexcept`

Requests shutdown of the internal worker (if any) and wakes the active waiter (if any).

Use this if your program is exiting and you want to ensure:

- worker thread terminates
- pending tasks can unblock

---

## Threading and safety notes

- Internal state is protected by a mutex (`m_`).
- The worker thread captures signals and pushes them to `pending_`.
- Delivery to coroutines/callbacks is done by posting onto `io_context` (scheduler thread).

Practical implications:

- You should treat `signal_set` methods as thread-safe unless your implementation says otherwise.
- Your callback runs on the scheduler thread, so it can safely touch scheduler-owned state if that state is also scheduler-thread-confined.
- Do not assume callbacks run immediately after the signal arrives: they are queued via `ctx_post`.

---

## Cancellation behavior

`async_wait(cancel_token)` indicates waiters can be cancelled.

Make sure your shutdown logic accounts for:

- cancellation requested before a signal arrives
- stop() called while waiting
- multiple signals arriving quickly

A robust pattern is:

- request cancellation on signal
- let other tasks watch the cancel token
- stop the io_context once tasks have drained

---

## Common patterns

### Graceful shutdown: signal cancels, main loop exits

- signal triggers cancellation
- your main coroutine or control loop stops the context

Pseudo:

```cpp
int sig = co_await sigs.async_wait(ct);
cs.request_cancel();
// wait for tasks to finish if you have a join mechanism
ctx.stop();
```

### Coalesce repeated signals

If you press Ctrl+C multiple times, you may get multiple pending signals.

You can ignore subsequent ones:

```cpp
bool first = true;
for (;;)
{
  int sig = co_await sigs.async_wait(ct);
  if (first)
  {
    first = false;
    cs.request_cancel();
  }
}
```

---

## Practical limitations

1. POSIX-only by intent
   - The header references POSIX signals (`<csignal>`).
   - On non-POSIX platforms, you may provide a stub or use `errc::not_supported`.

2. Signal semantics are platform-dependent
   - Delivery rules depend on process/thread masks.
   - If you do signal masking in other threads, document your expectation.

3. Single waiter model
   - If you need multiple consumers, build a small channel around it.

---

## Summary

`signal_set` is the async bridge from OS signals to Vix coroutines:

- register signals (`add`)
- await the next signal (`async_wait`)
- optional callback (`on_signal`) posted on the scheduler thread
- stop cleanly (`stop`)

It is small, explicit, and integrates into `io_context` without hiding complex behavior.

