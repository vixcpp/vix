# Priorities

`vix::threadpool` supports task priorities.

Priorities let the scheduler prefer important queued work before less important queued work.

The recommended include is:

```cpp
#include <vix/threadpool.hpp>
```

## Important rule

Priority is not preemption. A high-priority task does not interrupt a task that is already running. Priority only affects tasks that are **waiting in a queue**.

```txt
running task    → continues normally
queued tasks    → ordered by priority
```

## Priority levels

The module defines five priority levels:

```txt
lowest   (-2)
low      (-1)
normal    (0)   ← default
high     (+1)
highest  (+2)
```

## Basic usage

```cpp
vix::threadpool::TaskOptions options;
options.set_priority(vix::threadpool::TaskPriority::high);

pool.post([]() { /* High-priority background work. */ }, options);

// Or with submit():
auto future = pool.submit([]() { return 42; }, options);
```

## Full example

```cpp
#include <iostream>
#include <mutex>
#include <vector>
#include <vix/threadpool.hpp>

int main()
{
  vix::threadpool::ThreadPool pool(2);

  std::mutex mutex;
  std::vector<int> order;

  vix::threadpool::TaskOptions lowPriority;
  lowPriority.set_priority(vix::threadpool::TaskPriority::low);

  vix::threadpool::TaskOptions highPriority;
  highPriority.set_priority(vix::threadpool::TaskPriority::high);

  pool.post([&mutex, &order]() { std::lock_guard<std::mutex> lock(mutex); order.push_back(1); }, lowPriority);
  pool.post([&mutex, &order]() { std::lock_guard<std::mutex> lock(mutex); order.push_back(2); }, highPriority);
  pool.post([&mutex, &order]() { std::lock_guard<std::mutex> lock(mutex); order.push_back(3); }, highPriority);

  pool.wait_idle();

  std::cout << "execution order:";
  for (const int value : order) { std::cout << ' ' << value; }
  std::cout << '\n';

  pool.shutdown();

  return 0;
}
```

## How ordering works

Inside a worker queue, tasks are ordered first by priority, then by sequence number for same-priority tasks:

```txt
Task A: priority low,    sequence 1
Task B: priority high,   sequence 2
Task C: priority normal, sequence 3

Queue order: B → C → A
```

When two tasks have the same priority, the earlier task runs first (stable FIFO within a priority level):

```txt
Task A: priority normal, sequence 10
Task B: priority normal, sequence 11

→ Task A runs before Task B
```

## Priority with `post`, `submit`, and `handle`

```cpp
vix::threadpool::TaskOptions options;
options.set_priority(vix::threadpool::TaskPriority::high);

// post()
pool.post(fn, options);

// submit()
auto future = pool.submit(fn, options);

// handle()
auto handle = pool.handle(fn, options);
```

## Priority with parallel algorithms

### `parallel_for`

```cpp
vix::threadpool::ParallelForOptions options;
options.chunk_size = 128;
options.task_options.set_priority(vix::threadpool::TaskPriority::high);

vix::threadpool::parallel_for(pool, std::size_t{0}, values.size(), fn, options);
```

### `parallel_map`

```cpp
vix::threadpool::ParallelMapOptions options;
options.chunk_size = 256;
options.task_options.set_priority(vix::threadpool::TaskPriority::normal);

auto output = vix::threadpool::parallel_map(pool, values, fn, options);
```

## Priority with periodic tasks

```cpp
vix::threadpool::PeriodicTaskConfig config;

config.interval = std::chrono::milliseconds{100};
config.task_options.set_priority(vix::threadpool::TaskPriority::low);

vix::threadpool::PeriodicTask task(pool, []() { flush_metrics(); }, config);
```

This is useful for background maintenance work that should not compete with urgent tasks.

## Priority utilities

### Numeric value

```cpp
std::int32_t value =
    vix::threadpool::to_priority_value(vix::threadpool::TaskPriority::high);
// → 1
```

### Compare priorities

```cpp
const bool highWins =
    vix::threadpool::priority_higher_than(
        vix::threadpool::TaskPriority::high,
        vix::threadpool::TaskPriority::normal);
// → true
```

### Convert to string

```cpp
std::cout << vix::threadpool::to_string(vix::threadpool::TaskPriority::highest) << '\n';
// → "highest"
```

## Default priority

A default `TaskOptions` uses `normal` priority, so `pool.post(fn)` is equivalent to submitting with normal priority.

## When to use each level

### High priority

For work that should be preferred when queued: health checks, control-plane messages, short user-facing tasks, runtime coordination, latency-sensitive callbacks.

```cpp
vix::threadpool::TaskOptions options;
options.set_priority(vix::threadpool::TaskPriority::high);

pool.post([]() { update_health_snapshot(); }, options);
```

### Low priority

For best-effort background work: metrics flushing, cache cleanup, background indexing, log compaction, non-urgent telemetry.

```cpp
vix::threadpool::TaskOptions options;
options.set_priority(vix::threadpool::TaskPriority::low);

pool.post([]() { flush_metrics(); }, options);
```

## Avoid priority abuse

Do not make every task high priority. If every task uses `highest`, priority becomes meaningless. Use priority to separate real urgency levels.

## Priority and fairness

Constant submission of high-priority tasks can delay low-priority tasks. Use low priority only for work that genuinely can wait.

## Priority and queue capacity

Priority does not bypass queue capacity. If a queue is full, a high-priority task can still be rejected:

```cpp
vix::threadpool::ThreadPoolConfig config;
config.thread_count = 1;
config.max_queue_size = 4;

vix::threadpool::ThreadPool pool(config);

vix::threadpool::TaskOptions options;
options.set_priority(vix::threadpool::TaskPriority::highest);

const bool accepted = pool.post(fn, options); // may be false
```

## Combining priority with other options

```cpp
vix::threadpool::TaskOptions options;

// Priority + affinity:
options.set_priority(vix::threadpool::TaskPriority::high);
options.set_affinity(vix::threadpool::WorkerId{1});

// Priority + deadline:
options.set_priority(vix::threadpool::TaskPriority::high);
options.set_deadline(vix::threadpool::Deadline::after(std::chrono::milliseconds{100}));

// Priority + timeout:
options.set_priority(vix::threadpool::TaskPriority::high);
options.set_timeout(vix::threadpool::Timeout::milliseconds(100));

// Priority + cancellation (high-priority tasks can still be cancelled):
auto handle = pool.handle(fn, options);
handle.cancel();
```

## Limitations

Priority does not: interrupt running tasks, bypass queue capacity, force execution after shutdown, guarantee real-time scheduling, or override OS thread scheduling. Priority is a **queue ordering** feature, not an OS scheduling feature.

## Best practices

- Use `normal` priority by default
- Use `high` priority for short, urgent work
- Use `low` priority for best-effort background work
- Keep high-priority tasks short
- Combine priority with deadlines when stale work should be skipped
- Verify behavior with `metrics()` and `stats()`

## Simple mental model

```txt
TaskPriority  describes importance
TaskOptions   attaches priority to a task
TaskQueue     orders queued tasks by priority then sequence
Worker        pops the best queued task
Running task  is never preempted by priority
```

The usual pattern:

```cpp
vix::threadpool::TaskOptions options;
options.set_priority(vix::threadpool::TaskPriority::high);

pool.submit(fn, options);
```
