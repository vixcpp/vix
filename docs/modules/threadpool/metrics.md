# Metrics

`vix::threadpool` exposes metrics and stats so applications can observe what the thread pool is doing.

Metrics help answer questions like: how many workers exist? how many tasks are pending? how many tasks completed? how many were rejected? is the pool idle?

The recommended include is:

```cpp
#include <vix/threadpool.hpp>
```

## Metrics vs Stats

Two related types serve different purposes:

```txt
ThreadPoolMetrics  →  current snapshot of the pool
ThreadPoolStats    →  cumulative counters over the pool lifetime
```

Use `metrics()` when you want to know the current state. Use `stats()` when you want to know what happened historically.

## `ThreadPoolMetrics`

A snapshot of the current pool state:

```cpp
const auto metrics = pool.metrics();
```

### Basic example

```cpp
#include <chrono>
#include <iostream>
#include <thread>
#include <vix/threadpool.hpp>

int main()
{
  vix::threadpool::ThreadPool pool(4);

  for (int i = 0; i < 8; ++i)
  {
    pool.post([]() { std::this_thread::sleep_for(std::chrono::milliseconds{20}); });
  }

  pool.wait_idle();

  const auto metrics = pool.metrics();

  std::cout << "workers: " << metrics.worker_count << '\n';
  std::cout << "pending: " << metrics.pending_tasks << '\n';
  std::cout << "active: " << metrics.active_tasks << '\n';
  std::cout << "completed: " << metrics.completed_tasks << '\n';
  std::cout << "failed: " << metrics.failed_tasks << '\n';
  std::cout << "rejected: " << metrics.rejected_tasks << '\n';
  std::cout << "idle: " << (metrics.idle() ? "yes" : "no") << '\n';

  pool.shutdown();

  return 0;
}
```

### Fields

`worker_count` — number of workers owned by the pool.

`pending_tasks` — queued tasks that have not started yet. After `wait_idle()`, this is normally 0.

`active_tasks` — tasks currently running. After `wait_idle()`, this is normally 0.

`idle_workers` — workers not currently executing a task.

`busy_workers` — workers currently executing a task. Usually `busy_workers <= worker_count`.

`submitted_tasks` — tasks submitted to the pool.

`completed_tasks` — tasks that finished successfully (`TaskStatus::completed`, `TaskResult::success`).

`failed_tasks` — tasks that threw an exception.

`cancelled_tasks` — tasks that finished as cancelled. Cancellation is cooperative.

`timed_out_tasks` — tasks that exceeded their configured timeout or deadline. Timeouts are observational.

`rejected_tasks` — tasks rejected by the pool or scheduler (pool stopped, queue full, invalid task, no available worker).

### Methods

`idle()` — returns `true` when `pending_tasks == 0` and `active_tasks == 0`.

`finished_tasks()` — returns `completed + failed + cancelled + timed_out` (excludes rejected, which never ran).

`error_tasks()` — returns `failed + cancelled + timed_out + rejected`.

## `ThreadPoolStats`

Cumulative counters over the pool lifetime:

```cpp
const auto stats = pool.stats();
```

### Stats example

```cpp
#include <iostream>
#include <vix/threadpool.hpp>

int main()
{
  vix::threadpool::ThreadPool pool(4);

  auto first = pool.submit([]() { return 10; });
  auto second = pool.submit([]() { return 32; });

  std::cout << "result: " << first.get() + second.get() << '\n';

  const auto stats = pool.stats();

  std::cout << "accepted: " << stats.accepted_tasks << '\n';
  std::cout << "completed: " << stats.completed_tasks << '\n';
  std::cout << "failed: " << stats.failed_tasks << '\n';
  std::cout << "rejected: " << stats.rejected_tasks << '\n';

  pool.shutdown();

  return 0;
}
```

### Fields

`accepted_tasks` — tasks that entered the execution system (may later complete, fail, be cancelled, or time out).

`rejected_tasks` — tasks that could not be accepted (pool stopped, queue full, etc.).

`completed_tasks`, `failed_tasks`, `cancelled_tasks`, `timed_out_tasks` — cumulative outcome counters.

`total_execution_time`, `max_execution_time` — aggregate execution timing.

`idle_waits` — number of idle wait events.

### Methods

`finished_tasks()` and `error_tasks()` work the same as on `ThreadPoolMetrics`.

## Metrics are snapshots

A metrics object is a copy at the time you called `metrics()`. It does not update automatically:

```cpp
const auto a = pool.metrics();
const auto b = pool.metrics(); // may differ from a
```

For live monitoring, call `metrics()` repeatedly:

```cpp
for (int i = 0; i < 10; ++i)
{
  const auto metrics = pool.metrics();
  std::cout << "pending: " << metrics.pending_tasks << '\n';
  std::this_thread::sleep_for(std::chrono::milliseconds{100});
}
```

## Metrics after `wait_idle`

```cpp
pool.wait_idle();

const auto metrics = pool.metrics();

// Expected:
// pending_tasks == 0
// active_tasks == 0
// idle() == true
```

## Metrics and stats after shutdown

Both remain readable after shutdown — useful for final logs:

```cpp
pool.shutdown();

const auto metrics = pool.metrics();
const auto stats = pool.stats();
```

## Metrics with different submission methods

### With `post`

`post()` tasks do not return futures, so metrics are the main way to observe aggregate outcomes:

```cpp
pool.post([]() { do_work(); });
pool.wait_idle();

const auto metrics = pool.metrics();
std::cout << metrics.completed_tasks << '\n';
std::cout << metrics.failed_tasks << '\n';
```

### With `submit`

Both individual result (via `Future`) and aggregate result (via metrics) are available:

```cpp
auto future = pool.submit([]() { return 42; });
std::cout << future.get() << '\n';

const auto metrics = pool.metrics();
std::cout << metrics.completed_tasks << '\n';
```

### With parallel algorithms

Parallel algorithms submit chunk tasks. Metrics count chunks, not input elements:

```cpp
vix::threadpool::parallel_for(pool, 0, 1000, [](int) {},
    vix::threadpool::ParallelForOptions::with_chunk_size(100));

// ~10 chunk tasks submitted for 1000 items with chunk_size=100
const auto metrics = pool.metrics();
std::cout << metrics.completed_tasks << '\n';
```

### With timeouts and cancellation

```cpp
// Timeouts:
try { future.get(); } catch (...) {}
const auto metrics = pool.metrics();
std::cout << "timed out: " << metrics.timed_out_tasks << '\n';

// Cancellation:
handle.cancel();
try { handle.get(); } catch (...) {}
std::cout << "cancelled: " << metrics.cancelled_tasks << '\n';
```

### With bounded queues

```cpp
vix::threadpool::ThreadPoolConfig config;
config.thread_count = 1;
config.max_queue_size = 4;

vix::threadpool::ThreadPool pool(config);

// If many tasks submitted quickly, some may be rejected:
const auto metrics = pool.metrics();
std::cout << "rejected: " << metrics.rejected_tasks << '\n';
```

## Observability patterns

### Logging metrics

```cpp
void log_threadpool_metrics(vix::threadpool::ThreadPool &pool)
{
  const auto metrics = pool.metrics();

  std::cout << "workers=" << metrics.worker_count
            << " pending=" << metrics.pending_tasks
            << " active=" << metrics.active_tasks
            << " completed=" << metrics.completed_tasks
            << " failed=" << metrics.failed_tasks
            << " rejected=" << metrics.rejected_tasks
            << '\n';
}
```

### Health check

```cpp
bool threadpool_healthy(vix::threadpool::ThreadPool &pool)
{
  const auto metrics = pool.metrics();

  return pool.running() && metrics.rejected_tasks == 0;
}
```

### Backpressure

```cpp
const auto metrics = pool.metrics();

if (metrics.pending_tasks > 1000)
{
  // Skip best-effort work.
}
else
{
  pool.post(best_effort_task);
}
```

### Final shutdown log

```cpp
pool.wait_idle();

const auto metrics = pool.metrics();
const auto stats = pool.stats();

std::cout << "completed=" << metrics.completed_tasks << '\n';
std::cout << "failed=" << metrics.failed_tasks << '\n';
std::cout << "timed_out=" << metrics.timed_out_tasks << '\n';
std::cout << "rejected=" << metrics.rejected_tasks << '\n';

pool.shutdown();
```

## Best practices

- Use `metrics()` for current state
- Use `stats()` for lifetime counters
- Read metrics after `wait_idle()` in tests
- Read metrics before and after load in benchmarks
- Use `pending_tasks` for backpressure decisions
- Use `rejected_tasks` as a warning signal
- Use `failed_tasks`, `cancelled_tasks`, `timed_out_tasks` for reliability checks
- Remember parallel algorithms count chunk tasks, not input elements
- Do not treat metrics as synchronization primitives — they are observability snapshots

## Common mistakes

### Expecting live updates from a copied snapshot

```cpp
// Bad:
const auto metrics = pool.metrics();
pool.post(fn);
std::cout << metrics.completed_tasks << '\n'; // stale

// Good:
pool.post(fn);
pool.wait_idle();
const auto metrics = pool.metrics();
std::cout << metrics.completed_tasks << '\n';
```

### Confusing submitted and completed

A submitted task is not necessarily completed. A task may be submitted and later complete, fail, be cancelled, or time out.

### Assuming rejected tasks ran

Rejected tasks did not run. For `submit()`, a rejected future is ready immediately and `get()` throws.

### Using metrics as synchronization

```cpp
// Bad:
while (pool.metrics().active_tasks > 0) {}

// Good:
pool.wait_idle();
```

## Simple mental model

```txt
ThreadPoolMetrics   tells what the pool looks like now
ThreadPoolStats     tells what happened over time
metrics()           returns a snapshot
stats()             returns cumulative counters
wait_idle()         makes pending and active tasks reach zero
```

The usual pattern:

```cpp
pool.post(fn);
pool.wait_idle();

const auto metrics = pool.metrics();
std::cout << metrics.completed_tasks << '\n';
```
