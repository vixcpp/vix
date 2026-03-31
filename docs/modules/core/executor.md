# Executor

Vix uses an executor abstraction to run work outside the networking code.

The executor is responsible for:

- accepting tasks (`post`)
- exposing a lightweight metrics snapshot (`metrics`)
- allowing idle synchronization (`wait_idle`)
- supporting a future-based API (`submit`) on top of `post`

In the HTTP stack, the server and the `App` use an `IExecutor` to execute request handlers.

---

## IExecutor

`vix::executor::IExecutor` is the generic task execution interface.

### Core API

```cpp
struct IExecutor
{
  virtual ~IExecutor() = default;

  virtual bool post(std::function<void()> fn, TaskOptions opt = {}) = 0;
  virtual Metrics metrics() const = 0;
  virtual void wait_idle() = 0;

  template <class F, class... Args>
  auto submit(F&& f, Args&&... args, TaskOptions opt = {})
    -> std::future<std::invoke_result_t<F, Args...>>;
};
```

### `post(fn, opt)`

Posts a task for execution.

- Returns `true` if accepted by the executor.
- Returns `false` when rejected (queue full, shutting down, etc).

Task options allow priority and time constraints.

### `metrics()`

Returns an instantaneous snapshot of executor state:

- `pending`: tasks waiting to run
- `active`: tasks currently executing
- `timed_out`: tasks that timed out (implementation-defined)

### `wait_idle()`

Blocks the caller until the executor becomes idle.

This is useful for:

- clean shutdown sequences
- tests that need deterministic completion
- draining background work before exit

---

## Futures with `submit`

`submit` is implemented in the interface as a helper built on top of `post`.

It wraps the callable into a `std::packaged_task` and returns a `std::future`.

```cpp
auto fut = ex.submit([]{
  return 42;
});

int x = fut.get();
```

### Rejection behavior

If `post` rejects the task, `submit` returns a future already set with an exception:

- `std::runtime_error("submit rejected by executor")`

This ensures the API is safe: you always get a future, and failures are explicit.

---

## TaskOptions

`TaskOptions` are execution hints associated with a task.

```cpp
struct TaskOptions
{
  int priority = 0;
  std::chrono::milliseconds timeout{0};
  std::chrono::milliseconds deadline{0};
  bool may_block = false;
};
```

### Fields

- `priority`
  Scheduling hint. Higher means more important (implementation-defined).

- `timeout`
  Maximum allowed runtime or queue time (implementation-defined).

- `deadline`
  Absolute time constraint (implementation-defined).

- `may_block`
  Indicates the task may block (I/O, locks, sleeps). Executors can route it differently.

---

## Metrics

`Metrics` is a small, copyable snapshot struct:

```cpp
struct Metrics
{
  std::uint64_t pending{0};
  std::uint64_t active{0};
  std::uint64_t timed_out{0};
};
```

This is intentionally minimal: it is safe to read frequently and cheap to copy.

---

## Lightweight wrappers

Vix provides lightweight wrappers around an executor pointer for convenience.

### Stand

`Stand` is a simple forwarding wrapper:

```cpp
auto s = vix::executor::makeStand(ex);
s.post([]{
  // work
});
```

### LimitedExecutor

`limit(ex, n)` builds a limited view over an executor.

```cpp
auto limited = vix::executor::limit(ex, 64);
limited.post([]{
  // work
});
```

Note: the wrapper stores `maxPending`, but the real enforcement policy is owned by
the executor implementation. The wrapper is meant to express intent and provide
a consistent API surface.

---

## How it fits in core

- The HTTP server accepts connections and parses requests.
- The router resolves a handler for the request.
- The handler is executed via the executor.
- This separation prevents networking threads from being blocked by user work.

---

## Practical guidance

- Keep request handlers non-blocking when possible.
- Mark tasks that may block via `TaskOptions{ .may_block = true }`.
- Use `wait_idle()` in tests and controlled shutdown paths.
- Use `metrics()` to expose basic runtime health in dev tooling.


