# ThreadPool

`ThreadPool` is the main public API of the Vix threadpool module.

It lets users run work concurrently without managing `std::thread`, worker loops, queues, condition variables, futures, shutdown, or metrics manually.

The recommended include is:

```cpp
#include <vix/threadpool.hpp>
```

## Basic idea

A thread pool owns a group of worker threads. Instead of creating a new thread every time you need to run work, you create a pool once and submit work to it:

```cpp
vix::threadpool::ThreadPool pool(4);

pool.post(
    []()
    {
      // Runs on a worker thread.
    });

pool.wait_idle();
pool.shutdown();
```

## Create a pool

### Default pool

```cpp
vix::threadpool::ThreadPool pool;
```

Uses `ThreadPoolConfig::default_thread_count()`, which usually maps to the number of available hardware threads with a safe fallback to 1.

### Fixed worker count

```cpp
vix::threadpool::ThreadPool pool(4);
```

### Custom configuration

```cpp
vix::threadpool::ThreadPoolConfig config;

config.thread_count = 4;
config.max_thread_count = 4;
config.max_queue_size = 1024;
config.drain_on_shutdown = true;

vix::threadpool::ThreadPool pool(config);
```

The configuration is normalized internally before the pool starts.

## Automatic start

`ThreadPool` starts automatically when constructed:

```cpp
vix::threadpool::ThreadPool pool(4);

if (pool.running())
{
  // Pool is ready.
}
```

## `post`

Use `post()` for fire-and-forget work.

```cpp
pool.post(
    []()
    {
      // Background work.
    });
```

`post()` returns `true` if the task was accepted:

```cpp
const bool accepted = pool.post([]() { /* Work */ });

if (!accepted)
{
  // The task was rejected.
}
```

Use `post()` when you do not need a return value, do not need to wait for a specific task, or want simple background execution.

```cpp
#include <atomic>
#include <iostream>
#include <vix/threadpool.hpp>

int main()
{
  vix::threadpool::ThreadPool pool(4);

  std::atomic<int> counter{0};

  for (int i = 0; i < 10; ++i)
  {
    pool.post(
        [&counter]()
        {
          counter.fetch_add(1, std::memory_order_relaxed);
        });
  }

  pool.wait_idle();

  std::cout << counter.load() << '\n';

  pool.shutdown();

  return 0;
}
```

## `submit`

Use `submit()` when you need a result.

```cpp
auto future =
    pool.submit(
        []()
        {
          return 42;
        });

int value = future.get();
```

`submit()` returns a `Future<T>` that stores the returned value, any thrown exception, task status, and task result.

```cpp
#include <iostream>
#include <vix/threadpool.hpp>

int main()
{
  vix::threadpool::ThreadPool pool(4);

  auto future =
      pool.submit(
          []()
          {
            return 10 + 32;
          });

  std::cout << future.get() << '\n';

  pool.shutdown();

  return 0;
}
```

### `submit` with void

```cpp
auto future = pool.submit([]() { save_file(); });
future.get();
```

For `Future<void>`, `get()` waits and rethrows any exception but returns no value.

### Exceptions

If a task submitted with `submit()` throws, the exception is captured and rethrown by `future.get()`:

```cpp
auto future =
    pool.submit(
        []() -> int
        {
          throw std::runtime_error{"failed"};
        });

try
{
  int value = future.get();
}
catch (const std::exception &e)
{
  std::cout << "task failed: " << e.what() << '\n';
}
```

Exceptions never escape worker threads.

## `handle`

Use `handle()` when you need both a future and cancellation control.

```cpp
auto handle =
    pool.handle(
        []()
        {
          return 42;
        });

handle.cancel();

try
{
  int value = handle.get();
}
catch (const std::exception &e)
{
  // Handle failure or cancellation.
}
```

A `TaskHandle<T>` contains a task id, `Future<T>`, and a cancellation source. Useful methods: `id()`, `cancel()`, `cancelled()`, `wait()`, `get()`, `status()`, `result()`, `error()`.

## `TaskOptions`

`post()`, `submit()`, and `handle()` all accept `TaskOptions`:

```cpp
vix::threadpool::TaskOptions options;

options.set_priority(vix::threadpool::TaskPriority::high);
options.set_timeout(vix::threadpool::Timeout::milliseconds(100));

auto future = pool.submit([]() { return 42; }, options);
```

### Priority

Priority affects which queued task is selected first. Levels: `lowest`, `low`, `normal`, `high`, `highest`. Higher priority tasks are selected before lower priority tasks. Priority does not interrupt a task that is already running.

```cpp
vix::threadpool::TaskOptions options;
options.set_priority(vix::threadpool::TaskPriority::high);

pool.post([]() { /* High priority work. */ }, options);
```

### Timeout

Timeouts are observational. Vix does not forcibly kill running C++ code. If a task takes longer than the configured timeout, it can be marked as timed out after it finishes.

```cpp
vix::threadpool::TaskOptions options;
options.set_timeout(vix::threadpool::Timeout::milliseconds(50));

auto future = pool.submit(
    []()
    {
      std::this_thread::sleep_for(std::chrono::milliseconds{100});
      return 42;
    },
    options);

// After get():
std::cout << vix::threadpool::to_string(future.status()) << '\n';
std::cout << vix::threadpool::to_string(future.result()) << '\n';
```

### Deadline

A deadline is an absolute time point. If expired before execution, the task is skipped and marked as timed out.

```cpp
vix::threadpool::TaskOptions options;
options.set_deadline(
    vix::threadpool::Deadline::after(std::chrono::milliseconds{100}));
```

### Cancellation

Cancellation is cooperative:

```cpp
auto handle = pool.handle([]() { return 42; });
handle.cancel();
```

If cancellation is requested before the task starts, the task can be skipped. If the task is already running, Vix does not forcibly stop it.

## `wait_idle`

Waits until the pool has no pending or active work.

```cpp
pool.wait_idle();
pool.shutdown();
```

## `idle`

Returns whether the pool has no pending or active work.

```cpp
if (pool.idle())
{
  std::cout << "pool is idle\n";
}
```

## `pending`

Returns the number of queued tasks that have not started yet.

```cpp
std::size_t queued = pool.pending();
```

## `clear`

Removes queued tasks that have not started yet.

```cpp
const std::size_t removed = pool.clear();
```

This does not stop tasks that are already running. Use it carefully — it is mainly useful during shutdown, tests, or best-effort background queues.

## `shutdown`

Requests shutdown and joins workers.

```cpp
pool.shutdown();
```

Shutdown is safe and idempotent. A pool can be configured to drain queued tasks before stopping:

```cpp
config.drain_on_shutdown = true;
```

### Destructor behavior

`ThreadPool` calls `shutdown()` in its destructor, so this is safe:

```cpp
{
  vix::threadpool::ThreadPool pool(4);
  pool.post([]() { /* Work */ });
}
```

But explicit shutdown is clearer in examples, tests, and services:

```cpp
pool.wait_idle();
pool.shutdown();
```

## Metrics

`metrics()` returns a live snapshot of the pool state:

```cpp
const auto metrics = pool.metrics();

std::cout << "workers: " << metrics.worker_count << '\n';
std::cout << "pending: " << metrics.pending_tasks << '\n';
std::cout << "active: " << metrics.active_tasks << '\n';
std::cout << "completed: " << metrics.completed_tasks << '\n';
std::cout << "rejected: " << metrics.rejected_tasks << '\n';
```

## Stats

`stats()` returns cumulative historical counters:

```cpp
const auto stats = pool.stats();

std::cout << "accepted: " << stats.accepted_tasks << '\n';
std::cout << "completed: " << stats.completed_tasks << '\n';
std::cout << "failed: " << stats.failed_tasks << '\n';
std::cout << "rejected: " << stats.rejected_tasks << '\n';
```

## Other methods

`thread_count()` returns the number of worker threads. `config()` returns the normalized configuration. `next_task_id()` returns a new unique task id (mainly useful for internals, tests, and integrations).

## Complete example

```cpp
#include <atomic>
#include <iostream>
#include <vix/threadpool.hpp>

int main()
{
  vix::threadpool::ThreadPoolConfig config;

  config.thread_count = 4;
  config.max_thread_count = 4;
  config.max_queue_size = 1024;
  config.drain_on_shutdown = true;

  vix::threadpool::ThreadPool pool(config);

  std::atomic<int> counter{0};

  for (int i = 0; i < 10; ++i)
  {
    pool.post(
        [&counter]()
        {
          counter.fetch_add(1, std::memory_order_relaxed);
        });
  }

  auto future =
      pool.submit(
          []()
          {
            return 42;
          });

  std::cout << "future result: " << future.get() << '\n';

  pool.wait_idle();

  const auto metrics = pool.metrics();

  std::cout << "counter: " << counter.load() << '\n';
  std::cout << "completed: " << metrics.completed_tasks << '\n';
  std::cout << "pending: " << metrics.pending_tasks << '\n';

  pool.shutdown();

  return 0;
}
```

## Best practices

Prefer one pool per subsystem instead of creating a pool for every task:

```cpp
// Good
vix::threadpool::ThreadPool pool(4);
pool.submit(work_a);
pool.submit(work_b);
pool.submit(work_c);
```

Always protect shared data:

```cpp
std::mutex mutex;
std::vector<int> values;

pool.post(
    [&]()
    {
      std::lock_guard<std::mutex> lock(mutex);
      values.push_back(42);
    });
```

Use `submit()` when you need a result. Use `post()` when you do not. Use `wait_idle()` before shutdown when you need all accepted work to finish. Use `metrics()` and `stats()` when debugging or tuning.

## Mental model

```txt
ThreadPool  →  Scheduler  →  Workers  →  TaskQueue  →  Tasks  →  Futures
```

The user-facing API stays simple:

```cpp
pool.post(fn);
auto future = pool.submit(fn);
future.get();
pool.shutdown();
```
