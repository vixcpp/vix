# asio_net_service

`asio_net_service` is an internal networking service used by the Vix async runtime. It owns an independent `asio::io_context` that runs on a dedicated network thread, and it is created lazily by `vix::async::core::io_context::net()`.

This file lives in the `detail` namespace on purpose. End users typically interact with higher-level networking APIs (`tcp_stream`, `udp_socket`, `dns_resolver`, etc.), not with `asio_net_service` directly.

---

## What problem it solves

Networking backends like Asio need an event loop (`asio::io_context::run()`) to drive async I/O completions.

Vix already has its own scheduler (`vix::async::core::scheduler`) for coroutine continuations and posted tasks. Rather than mixing scheduler internals with socket readiness, `asio_net_service` isolates networking into:

- its own `asio::io_context`
- its own dedicated thread that runs `ioc_.run()`
- a work guard to keep `run()` alive

This separation keeps the core scheduler minimal and makes the networking backend an optional service.

---

## Where it sits in the architecture

- `vix::async::core::io_context`
  - owns a `scheduler` (core coroutine resumption and task queue)
  - lazily owns services:
    - `thread_pool` (CPU jobs)
    - `timer` (deadlines)
    - `signal_set` (signals)
    - `net()` -> `asio_net_service` (network backend)

So the model is:

- scheduler thread: your main event loop calling `io_context.run()`
- network thread: Asio loop used by socket primitives
- worker threads: optional thread pool (CPU compute)

---

## Key design points

### 1) Dedicated thread for networking

`asio_net_service` runs:

- `net_thread_` calls `ioc_.run()`

This means networking completions happen on the net thread. Concrete networking primitives should then "bridge back" to the Vix scheduler thread to resume user coroutines in a predictable place.

### 2) Work guard keeps Asio alive

Without a guard, `asio::io_context::run()` returns as soon as there is no pending work.

The service keeps a `executor_work_guard` alive so the Asio loop stays running even when no sockets are active yet.

### 3) stop() is explicit

`stop()` is responsible for shutting down the Asio loop and letting the net thread exit. Destruction calls stop and joins the thread, guaranteeing a clean shutdown when `io_context` is destroyed.

---

## Public surface

### Constructor

```cpp
explicit asio_net_service(vix::async::core::io_context& ctx);
```

- binds to the core `io_context`
- creates a work guard
- starts the network thread

### Accessor

```cpp
asio::io_context& asio_ctx() noexcept;
```

Used only by internal implementations to bind sockets, resolvers, timers, etc.

### stop()

```cpp
void stop() noexcept;
```

- releases the guard (so Asio is allowed to finish)
- calls `ioc_.stop()`
- requests thread shutdown

---

## Typical lifecycle

### Lazy creation

`io_context` should create it on first `ctx.net()` call. This keeps startup cheap if the app never uses networking.

### Shutdown behavior

A typical shutdown sequence is:

1. app calls `ctx.stop()` (stops Vix scheduler loop)
2. `io_context` destructor triggers service cleanup
3. `asio_net_service::~asio_net_service()` calls `stop()` and joins `net_thread_`

Because `net_thread_` runs independently, joining is important to avoid a dangling thread during program exit.

---

## How networking primitives should integrate with it

Concrete networking types (TCP/UDP/DNS implementations) will usually:

1. create Asio objects bound to `asio_net_service::asio_ctx()`
2. start async operations on the net thread
3. when Asio completes, post the continuation to the Vix scheduler thread

Example pattern (conceptual):

```cpp
// PSEUDO CODE for a TCP read awaitable
void on_asio_complete(std::error_code ec, std::size_t n) {
  // store ec/n
  // then resume coroutine on Vix scheduler thread
  ctx.get_scheduler().post(h);
}
```

This keeps the rule simple for users:

- your coroutines resume on the Vix scheduler thread
- networking backends run on their own threads

---

## Threading model and safety notes

- `asio::io_context` is safe to use from multiple threads, but Vix uses one dedicated net thread for predictability.
- `asio_ctx()` must outlive any sockets/resolvers created from it.
- `stop()` must be safe to call more than once (idempotent behavior is recommended).
- Avoid calling `join()` from the net thread itself. Destruction should happen from the owning context thread.

---

## Common pitfalls

### 1) Forgetting the guard

If you do not keep the work guard alive, `ioc_.run()` may immediately return and networking operations will never complete.

### 2) Resuming coroutines on the wrong thread

If you resume user continuations directly from the net thread, you can create subtle race conditions because the rest of Vix assumes resumption happens on the scheduler thread.

The recommended rule is:

- Asio callbacks run on net thread
- coroutine resumption posts back to `ctx.get_scheduler()`

### 3) stop order

If `io_context` is destroyed while networking primitives still hold references to Asio objects, you can get crashes.

Practical rule:

- destroy all network objects (streams, listeners, sockets) before `io_context` destruction
- or ensure those objects internally handle `asio_ctx().stopped()` and early exit

---

## Suggested testing checklist

When implementing the `.cpp` for `asio_net_service`, verify:

- creating the service starts the thread and `ioc_.run()` stays alive
- `stop()` stops `ioc_` and the thread exits
- `stop()` is safe to call multiple times
- destructor always joins the thread
- lazy creation from `io_context::net()` works and does not create services unless needed

---

## Next docs that depend on this

Once `asio_net_service` exists, it becomes the backend for:

- `tcp_stream` and `tcp_listener` implementations
- `udp_socket` implementation
- `dns_resolver` implementation

Those guides should reference the same threading rule: Asio completes on net thread, coroutine resumes on scheduler thread.

