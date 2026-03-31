# io_context

`vix::async::core::io_context` is the **core runtime context** for Vix async.
It owns a `scheduler` and exposes **lazy services** used by higher-level features: CPU thread pool, timers, signal handling, and networking.

This type is designed to be:
- Explicit: you drive it via `run()` and stop it via `stop()`
- Deterministic: no hidden background loops
- Lightweight: services are created only when you ask for them

---

## Header

```cpp
#include <vix/async/core/io_context.hpp>
```

---

## What it provides

### 1) Scheduler ownership
`io_context` owns a `scheduler` instance and exposes it via:

```cpp
scheduler&       get_scheduler() noexcept;
const scheduler& get_scheduler() const noexcept;
```

You can also post work directly:

```cpp
template <typename Fn>
void post(Fn&& fn);

void post(std::coroutine_handle<> h);
```

### 2) Event loop control

```cpp
void run();           // processes queued tasks, typically blocks
void stop() noexcept; // requests the scheduler to stop
bool is_running() const noexcept;
```

### 3) Lazy services
These are created on first access and owned by the `io_context`:

```cpp
thread_pool& cpu_pool();
timer&       timers();
signal_set&  signals();
vix::async::net::detail::asio_net_service& net();
```

Notes:
- `net()` returns an internal service type in a `detail` namespace. The goal is to keep the public API light and avoid pulling networking details into headers.

---

## Minimal example

```cpp
#include <iostream>
#include <vix/async/core/io_context.hpp>

static void demo_basic(vix::async::core::io_context& ctx)
{
  ctx.post([&]{
    std::cout << "hello from scheduler\n";
    ctx.stop();
  });

  ctx.run();
}

int main()
{
  vix::async::core::io_context ctx;
  demo_basic(ctx);
  return 0;
}
```

---

## Using lazy services

### CPU pool (compute-bound work)
You typically keep IO on the scheduler and push heavy CPU work to the pool.

```cpp
static void demo_cpu_pool(vix::async::core::io_context& ctx)
{
  auto& pool = ctx.cpu_pool(); // created here if needed
  (void)pool;

  // Example usage depends on thread_pool API (submit, schedule, etc.)
}
```

### Timers
```cpp
static void demo_timers(vix::async::core::io_context& ctx)
{
  auto& t = ctx.timers(); // created here if needed
  (void)t;

  // Example usage depends on timer API (sleep_for, at, etc.)
}
```

### Signals
```cpp
static void demo_signals(vix::async::core::io_context& ctx)
{
  auto& sig = ctx.signals(); // created here if needed
  (void)sig;

  // Example usage depends on signal_set API.
}
```

### Networking
```cpp
static void demo_net(vix::async::core::io_context& ctx)
{
  auto& n = ctx.net(); // internal Asio-backed service
  (void)n;

  // Networking APIs are intentionally built on top of this service.
}
```

---

## Lifecycle rules and best practices

- Create one `io_context` per subsystem that needs a runtime loop.
- Call `run()` from the thread you want to own the loop.
- Use `post()` to inject work safely from other parts of the program.
- Stop the loop using `stop()` (or a higher-level shutdown mechanism).
- Avoid constructing lazy services unless you need them. They are owned by the context and destroyed with it.

---

## Common pitfalls

- Forgetting to call `run()`: posting tasks does nothing until the scheduler is driven.
- Calling `run()` twice concurrently: only one thread should drive a given context loop unless the scheduler explicitly supports multi-threaded driving.
- Storing references to lazy services after context destruction: they are owned by `io_context`.

---

## Related

- `scheduler` (task queue + coroutine resumption)
- `thread_pool` (compute scheduling)
- `timer` (time-based scheduling)
- `signal_set` (signal handling)
- Networking module built on top of `net()` service

