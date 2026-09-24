# ThreadPool API

The Vix ThreadPool API provides simple, explicit, and observable multithreaded execution for C++ applications.

It gives you a high-level API for running work concurrently without managing worker threads, queues, futures, cancellation state, or shutdown details manually.

## Include

Use the main public header:

```cpp
#include <vix/threadpool.hpp>
```

Namespace:

```cpp
vix::threadpool
```

## Quick example

```cpp
#include <vix/threadpool.hpp>

#include <iostream>

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

Output:

```txt
42
```

## Main API

For most application code, these are the most important APIs:

```cpp
vix::threadpool::ThreadPool pool(4);

pool.post(fn);

auto future = pool.submit(fn);
auto value = future.get();

auto handle = pool.handle(fn);
handle.cancel();

pool.wait_idle();
pool.shutdown();
```

## Main types

The public ThreadPool API includes:

- `ThreadPool`
- `ThreadPoolConfig`
- `ThreadPoolMetrics`
- `ThreadPoolStats`
- `ThreadPoolErrc`
- `Task`
- `TaskId`
- `TaskOptions`
- `TaskPriority`
- `TaskStatus`
- `TaskResult`
- `TaskHandle`
- `TaskGroup`
- `CancellationToken`
- `CancellationSource`
- `Timeout`
- `Deadline`
- `Future<T>`
- `Promise<T>`
- `Executor`
- `ExecutorRef`
- `InlineExecutor`
- `ThreadPoolExecutor`
- `PeriodicTask`
- `Scope`
- `Latch`
- `Barrier`

## Parallel helpers

The module also provides high-level parallel algorithms:

```cpp
vix::threadpool::parallel_for(pool, first, last, fn);
vix::threadpool::parallel_for_each(pool, values, fn);
vix::threadpool::parallel_map(pool, values, fn);
vix::threadpool::parallel_reduce(pool, values, initial, reduce);
vix::threadpool::parallel_pipeline(pool, stage1, stage2, stage3);
```

## Fire-and-forget tasks

Use `post()` when the task does not return a value:

```cpp
vix::threadpool::ThreadPool pool(4);

pool.post(
    []()
    {
      // Background work.
    });

pool.wait_idle();
pool.shutdown();
```

## Future-based tasks

Use `submit()` when the task returns a value:

```cpp
auto future =
    pool.submit(
        []()
        {
          return 10 + 32;
        });

int value = future.get();
```

## Task options

Use `TaskOptions` to configure priority, timeout, deadline, cancellation, or affinity.

```cpp
vix::threadpool::TaskOptions options;

options.set_priority(vix::threadpool::TaskPriority::high);
options.set_timeout(vix::threadpool::Timeout::milliseconds(100));

auto future =
    pool.submit(
        []()
        {
          return 42;
        },
        options);
```

## Cancellation

Use `TaskHandle` when you need a cancellable task:

```cpp
auto handle =
    pool.handle(
        []()
        {
          return 42;
        });

handle.cancel();
```

Cancellation is cooperative. Vix does not forcibly interrupt running C++ code.

## Metrics

Use `metrics()` for current runtime state:

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

## Shutdown

Always shut down the pool when you are done:

```cpp
pool.wait_idle();
pool.shutdown();
```

`shutdown()` is safe to call more than once.

## Full ThreadPool documentation

The complete ThreadPool documentation is available in the dedicated section:

- [ThreadPool overview](/modules/threadpool/)
- [Quick start](/modules/threadpool/quick-start)
- [Concepts](/modules/threadpool/concepts)
- [Tasks](/modules/threadpool/tasks)
- [Futures](/modules/threadpool/futures)
- [Cancellation](/modules/threadpool/cancellation)
- [Timeouts](/modules/threadpool/timeouts)
- [Priorities](/modules/threadpool/priorities)
- [Task groups](/modules/threadpool/task-groups)
- [Parallel for](/modules/threadpool/parallel-for)
- [Parallel map](/modules/threadpool/parallel-map)
- [Parallel reduce](/modules/threadpool/parallel-reduce)
- [Metrics](/modules/threadpool/metrics)
- [Shutdown](/modules/threadpool/shutdown)
- [API reference](/modules/threadpool/api-reference)
- [Recommended usage](#recommended-usage)

## Recommended usage

For most users:

```cpp
vix::threadpool::ThreadPool pool(4);

auto future = pool.submit(fn);
auto result = future.get();

pool.shutdown();
```

For fire-and-forget work:

```cpp
pool.post(fn);
pool.wait_idle();
```

For parallel data processing:

```cpp
auto results =
    vix::threadpool::parallel_map(
        pool,
        values,
        [](const auto &value)
        {
          return transform(value);
        });
```

## Design note

The preferred user-facing API stays small:

```cpp
pool.post(fn);
pool.submit(fn);
pool.handle(fn);
pool.wait_idle();
pool.shutdown();
```

Lower-level types exist so Vix can keep the implementation explicit, testable, observable, and extensible.

