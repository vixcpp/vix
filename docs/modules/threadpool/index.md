# Vix Threadpool

`vix::threadpool` is the Vix module for simple, explicit, and observable multithreaded execution in C++.

It provides a high-level API for running work concurrently without exposing users to low-level thread management, worker loops, condition variables, queues, futures, cancellation state, or shutdown details.

The goal is simple:

```cpp
#include <vix/threadpool.hpp>

int main()
{
  vix::threadpool::ThreadPool pool(4);

  auto result =
      pool.submit([](){
            return 42;
      });

  return result.get();
}
```

The user writes the work. Vix handles the concurrency.

## What this module provides

The threadpool module includes:

- `ThreadPool` for running tasks on worker threads
- `post()` for fire-and-forget tasks
- `submit()` for future-based tasks
- `TaskHandle` for cancellable submitted tasks
- `TaskOptions` for priority, timeout, deadline, cancellation, and affinity
- `TaskGroup` for coordinating related tasks
- `Scope` for structured concurrency
- `PeriodicTask` for repeated scheduled work
- `parallel_for`, `parallel_for_each`, `parallel_map`, `parallel_reduce`, `parallel_pipeline`
- `ThreadPoolMetrics` and `ThreadPoolStats` for observability
- `Latch` and `Barrier` for synchronization
- `InlineExecutor` and `ThreadPoolExecutor` for executor-based integrations

## Design goals

The module is designed around five principles.

### 1. Simple public API

Users should not need to understand the internal scheduler to use multithreading.

```cpp
vix::threadpool::ThreadPool pool;

pool.post([](){
      // background work
});

pool.wait_idle();
```

For result-producing tasks:

```cpp
auto future = pool.submit([](){
          return 10 + 32;
});

int value = future.get();
```

### 2. Explicit behavior

Threadpool behavior is configured through explicit types:

```cpp
vix::threadpool::TaskOptions options;
options.set_priority(vix::threadpool::TaskPriority::high);
options.set_timeout(vix::threadpool::Timeout::milliseconds(100));

auto future = pool.submit([](){
                return 42;
              },options);
```

### 3. Safe shutdown

The pool owns its worker threads and shuts them down safely.

```cpp
pool.wait_idle();
pool.shutdown();
```

Shutdown is idempotent, so calling it multiple times is safe.

### 4. Observability

The module exposes current metrics and cumulative stats:

```cpp
const auto metrics = pool.metrics();
const auto stats = pool.stats();

std::cout << "workers: " << metrics.worker_count << '\n';
std::cout << "completed: " << metrics.completed_tasks << '\n';
std::cout << "rejected: " << metrics.rejected_tasks << '\n';
```

This makes the pool useful not only for local apps, but also for runtime services, servers, background workers, and production systems.

### 5. Vix-compatible architecture

The module is built to integrate with the rest of Vix:

- runtime systems can depend on the `Executor` abstraction
- tests can use `InlineExecutor`
- higher-level APIs can use `ThreadPoolExecutor`
- apps can use `ThreadPool` directly
- future Vix modules can reuse the same task, cancellation, timeout, and metrics model

## Basic usage

### Fire-and-forget task

```cpp
#include <vix/threadpool.hpp>

int main()
{
  vix::threadpool::ThreadPool pool(4);

  pool.post(
      []()
      {
        // Run on a worker thread.
      });

  pool.wait_idle();
  pool.shutdown();

  return 0;
}
```

### Submit a task and get a result

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
            return 42;
          });

  std::cout << future.get() << '\n';

  pool.shutdown();

  return 0;
}
```

### Submit a task with priority

```cpp
vix::threadpool::TaskOptions options;
options.set_priority(vix::threadpool::TaskPriority::high);

pool.post(
    []()
    {
      // High-priority work.
    },
    options);
```

### Submit a task with timeout observation

```cpp
vix::threadpool::TaskOptions options;
options.set_timeout(vix::threadpool::Timeout::milliseconds(100));

auto future =
    pool.submit(
        []()
        {
          return 42;
        },
        options);
```

Timeouts are observational. Vix does not forcibly kill running C++ code. The task is marked as timed out after execution if the timeout was exceeded.

### Submit a cancellable task

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
  // Handle cancellation or failure.
}
```

Cancellation is cooperative. A task can be skipped before execution if cancellation is requested early. Running C++ code is not forcefully interrupted.

## Parallel algorithms

The module includes high-level helpers for common parallel operations.

### `parallel_for`

```cpp
std::vector<int> values(100);

vix::threadpool::parallel_for(
    pool,
    std::size_t{0},
    values.size(),
    [&values](std::size_t index)
    {
      values[index] = static_cast<int>(index * index);
    });
```

### `parallel_for_each`

```cpp
std::vector<int> values{1, 2, 3, 4};

vix::threadpool::parallel_for_each(
    pool,
    values,
    [](int &value)
    {
      value *= 2;
    });
```

### `parallel_map`

```cpp
std::vector<int> values{1, 2, 3, 4};

std::vector<int> squares =
    vix::threadpool::parallel_map(
        pool,
        values,
        [](int value)
        {
          return value * value;
        });
```

### `parallel_reduce`

```cpp
std::vector<int> values{1, 2, 3, 4};

int sum =
    vix::threadpool::parallel_reduce(
        pool,
        values,
        0,
        [](int current, int value)
        {
          return current + value;
        });
```

## Structured concurrency

`Scope` lets a function spawn tasks and guarantee that they finish before the scope exits.

```cpp
vix::threadpool::ThreadPool pool(4);

{
  vix::threadpool::Scope scope(pool);

  scope.spawn(
      []()
      {
        // Work A
      });

  scope.spawn(
      []()
      {
        // Work B
      });

  scope.wait();
}

pool.shutdown();
```

The destructor also waits for tracked tasks and swallows exceptions. Use `wait_and_rethrow()` when exceptions must be observed:

```cpp
scope.wait_and_rethrow();
```

## Periodic work

`PeriodicTask` submits a callback repeatedly to an executor.

```cpp
vix::threadpool::PeriodicTaskConfig config;
config.interval = std::chrono::milliseconds{100};
config.run_immediately = true;

vix::threadpool::PeriodicTask task(
    pool,
    []()
    {
      // Repeated work.
    },
    config);

task.start();

std::this_thread::sleep_for(std::chrono::seconds{1});

task.stop();
task.join();
```

## Configuration

A pool can be created from `ThreadPoolConfig`.

```cpp
vix::threadpool::ThreadPoolConfig config;

config.thread_count = 4;
config.max_thread_count = 4;
config.max_queue_size = 1024;
config.default_priority = vix::threadpool::TaskPriority::normal;
config.drain_on_shutdown = true;
config.swallow_task_exceptions = true;
config.default_timeout = std::chrono::milliseconds{0};

vix::threadpool::ThreadPool pool(config);
```

The configuration is normalized internally, so invalid thread counts are corrected safely.

## Metrics and stats

Use `metrics()` for current state:

```cpp
const auto metrics = pool.metrics();

std::cout << metrics.worker_count << '\n';
std::cout << metrics.pending_tasks << '\n';
std::cout << metrics.active_tasks << '\n';
```

Use `stats()` for cumulative counters:

```cpp
const auto stats = pool.stats();

std::cout << stats.completed_tasks << '\n';
std::cout << stats.failed_tasks << '\n';
std::cout << stats.rejected_tasks << '\n';
```

## Public include

The recommended include is:

```cpp
#include <vix/threadpool.hpp>
```

This exposes the complete public API. For smaller compile units, individual headers can be included directly:

```cpp
#include <vix/ThreadPool.hpp>
#include <vix/threadpool/ParallelFor.hpp>
#include <vix/threadpool/Scope.hpp>
```

## Module status

This module is designed as a complete Vix threadpool foundation. It includes:

- task execution
- futures and promises
- cancellation
- timeouts and deadlines
- priority queues
- workers and scheduling
- executors
- parallel algorithms
- scoped tasks
- synchronization primitives
- metrics, benchmarks, tests, and documentation

The implementation keeps the user-facing API simple while preserving the internal structure needed for production-grade Vix runtime integrations.
