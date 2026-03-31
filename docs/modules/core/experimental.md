# Experimental: ThreadPoolExecutor

The experimental module provides a concrete `IExecutor` implementation
backed by `vix::threadpool::ThreadPool`.

This executor is intended for:

- general-purpose HTTP handler execution
- controlled concurrency environments
- production-style workloads requiring thread pooling
- explicit control over worker scaling

Namespace:

```cpp
vix::experimental
```

---

## ThreadPoolExecutor

`ThreadPoolExecutor` is a final implementation of `vix::executor::IExecutor`.

```cpp
class ThreadPoolExecutor final : public vix::executor::IExecutor
```

It wraps a `vix::threadpool::ThreadPool` and exposes it through the
standard executor interface.

---

## Construction

```cpp
ThreadPoolExecutor(
  std::size_t threads,
  std::size_t maxThreads,
  int defaultPriority
);
```

### Parameters

- `threads`
  Initial number of worker threads.

- `maxThreads`
  Maximum allowed worker threads.

- `defaultPriority`
  Default task priority used when none is provided.

The executor internally owns the thread pool via:

```cpp
std::unique_ptr<vix::threadpool::ThreadPool>
```

---

## Core API

### `post(fn, opt)`

Posts a task to the thread pool.

```cpp
bool post(std::function<void()> fn, TaskOptions opt = {});
```

- Respects task options such as priority and blocking hints.
- Returns `false` if the pool rejects the task.

---

### `metrics()`

Returns a snapshot of executor state:

```cpp
vix::executor::Metrics metrics() const;
```

Typical fields:

- `pending`
- `active`
- `timed_out`

---

### `wait_idle()`

Blocks until the underlying pool becomes idle.

Useful for:

- deterministic shutdown
- draining background tasks
- test synchronization

---

## Introspection

```cpp
std::size_t threads() const noexcept;
std::size_t max_threads() const noexcept;
```

These methods expose configured limits for monitoring and diagnostics.

---

## Factory Helper

Vix provides a convenience factory:

```cpp
std::unique_ptr<vix::executor::IExecutor>
make_threadpool_executor(
  std::size_t threads,
  std::size_t maxThreads,
  int defaultPriority
);
```

Example:

```cpp
auto exec = vix::experimental::make_threadpool_executor(
  4,      // initial threads
  16,     // max threads
  0       // default priority
);

vix::App app(std::move(exec));
```

---

## How it integrates with App

When injected into `vix::App`, the executor becomes responsible
for running all HTTP route handlers.

This ensures:

- networking threads remain responsive
- user code does not block accept loops
- concurrency is controlled via pool sizing

---

## Design Notes

- The executor strictly adheres to `IExecutor`.
- It isolates concurrency from HTTP parsing.
- It allows scaling policies to evolve independently from the core runtime.

---

## When to use it

Use `ThreadPoolExecutor` when:

- you need predictable concurrency limits
- your handlers may block or perform CPU-heavy work
- you want explicit control over scaling behavior

If your application requires custom scheduling, priority routing,
or integration with another async runtime, implement your own
`IExecutor` and inject it into `App`.


