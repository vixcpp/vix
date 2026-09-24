# Concepts

This page explains the main concepts behind `vix::threadpool`.

The goal of the module is to make concurrent and parallel C++ programming simple for users, while keeping the complex runtime details inside Vix.

## Thread pool

A thread pool owns a fixed number of worker threads. Instead of creating a new `std::thread` for every task, the pool creates workers once, then reuses them:

```cpp
vix::threadpool::ThreadPool pool(4);

pool.post([]() { /* Runs on a worker thread. */ });
```

The user does not manage thread creation, joining, task queues, worker loops, condition variables, or shutdown logic. Vix handles that internally.

## Task

A task is one unit of work submitted to the pool. Internally it stores: a task id, a callable, task options, status, result, priority, sequence number, timing information, and a captured exception.

A user normally writes:

```cpp
pool.post([]() { /* Work */ });

// or:
auto future = pool.submit([]() { return 42; });
```

Vix wraps the callable into an internal task.

## `post`

Use `post()` for fire-and-forget work:

```cpp
std::atomic<int> counter{0};

pool.post([&counter]() { counter.fetch_add(1, std::memory_order_relaxed); });

pool.wait_idle();
```

Use `post()` when you do not need a return value, do not need `get()`, and the task can run in the background.

## `submit`

Use `submit()` when the task returns a result. If the task throws, the exception is stored in the future and rethrown by `get()`:

```cpp
auto future = pool.submit([]() { return 42; });

int value = future.get();
```

## `Future`

A `Future<T>` represents a result that will be available later:

```cpp
vix::threadpool::Future<int> future = pool.submit([]() { return 42; });

future.wait();

int value = future.get();

auto status = future.status();
auto result = future.result();
auto error  = future.error();
```

`get()` consumes the result. Calling `get()` more than once is an error.

## `Promise`

`Promise<T>` is the producer side of a future. The threadpool uses promises internally. Conceptually:

```cpp
Promise<int> promise;
Future<int> future = promise.get_future();

promise.set_value(42);

int value = future.get();
```

## Task status and result

`TaskStatus` describes the lifecycle state. `TaskResult` describes the final outcome.

Common statuses: `created`, `queued`, `running`, `completed`, `failed`, `cancelled`, `timed_out`, `rejected`.

Common results: `none`, `success`, `failure`, `cancelled`, `timeout`, `rejected`.

```cpp
if (future.result() == vix::threadpool::TaskResult::success)
{
  // Task completed successfully.
}
```

## Task options

`TaskOptions` controls how a task is submitted. It can configure priority, timeout, deadline, cancellation, worker affinity, detached mode, and allow-after-stop behavior:

```cpp
vix::threadpool::TaskOptions options;

options.set_priority(vix::threadpool::TaskPriority::high);
options.set_timeout(vix::threadpool::Timeout::milliseconds(100));

auto future = pool.submit([]() { return 42; }, options);
```

## Priority

Priority affects the order in which queued tasks are selected. Levels: `lowest`, `low`, `normal`, `high`, `highest`.

Higher priority tasks are selected before lower priority tasks in the same queue. Priority does not interrupt a task that is already running.

## Sequence number

The sequence number keeps stable ordering between tasks with the same priority — the task submitted earlier runs first (predictable FIFO within a priority level).

## Worker

A worker is one execution unit inside the pool. Each worker owns one worker thread, one local task queue, worker metrics, and a lifecycle state. The pool and scheduler manage workers internally.

## Scheduler

The scheduler distributes tasks across workers. Scheduling policies include: `round_robin`, `least_loaded`, `affinity`, `affinity_then_least_loaded`. The default is `affinity_then_least_loaded`.

## Queue and rejection policies

The queue policy describes task ordering: `priority`, `fifo`, `lifo`. The default is priority-based.

Rejection policies when a task cannot be accepted: `reject` (default), `caller_runs`, `discard`. A task can be rejected when the pool is stopped, the queue is full, the task is invalid, or no worker is available.

## Cancellation

Cancellation is cooperative — Vix does not forcibly kill running C++ code.

```txt
CancellationSource  →  requests cancellation
CancellationToken   →  observes cancellation
```

```cpp
auto handle = pool.handle([]() { return 42; });
handle.cancel();
```

The handle owns a cancellation source. The submitted task receives a matching cancellation token through its options.

For long-running work, pass the token and check it:

```cpp
if (token.cancelled()) { return; }
```

## Timeout

A timeout describes a maximum expected duration. Timeouts are observational — Vix marks a task as timed out after it finishes if the elapsed time exceeded the timeout. Vix does not stop the task while it is running.

```cpp
vix::threadpool::TaskOptions options;
options.set_timeout(vix::threadpool::Timeout::milliseconds(50));
```

## Deadline

A deadline is an absolute time limit. If the deadline is already expired before a task starts, the task can be skipped and marked as timed out.

```cpp
auto deadline = vix::threadpool::Deadline::after(std::chrono::milliseconds{100});
```

## `TaskHandle`

`TaskHandle<T>` combines a task id, future, and cancellation source:

```cpp
auto handle = pool.handle([]() { return 42; });

handle.cancel();
handle.wait();
int value = handle.get();
```

Useful when the caller needs both a result and cancellation control.

## `TaskGroup`

`TaskGroup` coordinates several related tasks. It tracks total/pending/completed/failed/cancelled/timed-out/rejected tasks, a first exception, and shared cancellation:

```cpp
vix::threadpool::TaskGroup group;

group.add_task(vix::threadpool::TaskId{1});
group.add_task(vix::threadpool::TaskId{2});

group.finish_task(vix::threadpool::TaskStatus::completed,
                  vix::threadpool::TaskResult::success);
group.finish_task(vix::threadpool::TaskStatus::completed,
                  vix::threadpool::TaskResult::success);

group.wait();
```

## `Scope`

`Scope` provides structured concurrency. It is bound to a thread pool and tracks tasks spawned inside it:

```cpp
vix::threadpool::Scope scope(pool);

scope.spawn([]() { /* Work A */ });
scope.spawn([]() { /* Work B */ });

scope.wait();
```

The destructor waits for spawned tasks and swallows exceptions. Use `wait_and_rethrow()` when exceptions must be observed.

## Parallel algorithms

### `parallel_for` — numeric index ranges

```cpp
vix::threadpool::parallel_for(pool, std::size_t{0}, values.size(),
    [&values](std::size_t index) { values[index] = static_cast<int>(index); });
```

### `parallel_for_each` — containers or iterator ranges

```cpp
vix::threadpool::parallel_for_each(pool, values,
    [](int &value) { value *= 2; });
```

### `parallel_map` — transform to new vector

```cpp
std::vector<int> squares = vix::threadpool::parallel_map(pool, values,
    [](int value) { return value * value; });
```

### `parallel_reduce` — reduce to one result

```cpp
int sum = vix::threadpool::parallel_reduce(pool, values, 0,
    [](int current, int value) { return current + value; });
```

### `parallel_pipeline` — independent stages concurrently

```cpp
vix::threadpool::parallel_pipeline(pool,
    []() { load_data(); },
    []() { warm_cache(); },
    []() { prepare_logs(); });
```

## Chunk size

Parallel algorithms split work into chunks. Each chunk is processed by one submitted task. If chunk size is zero, Vix chooses a safe default.

Small chunks: better load balancing, more overhead. Large chunks: less overhead, may balance less well.

```cpp
vix::threadpool::ParallelForOptions options;
options.chunk_size = 1024;
```

## Executor

`Executor` is a small abstraction for fire-and-forget work, allowing higher-level modules to depend on an execution interface without depending directly on `ThreadPool`.

`InlineExecutor` runs tasks immediately on the caller thread — useful for tests. `ThreadPoolExecutor` adapts a `ThreadPool` to the `Executor` interface (non-owning; the pool must outlive the adapter).

## `PeriodicTask`

Submits a callback repeatedly to an executor:

```cpp
vix::threadpool::PeriodicTaskConfig config;
config.interval = std::chrono::milliseconds{100};
config.run_immediately = true;

vix::threadpool::PeriodicTask task(pool, callback, config);

task.start();

std::this_thread::sleep_for(std::chrono::seconds{1});

task.stop();
task.join();
```

## `Latch` and `Barrier`

**Latch** — one-shot synchronization. Waits until a counter reaches zero; once open, stays open:

```cpp
vix::threadpool::Latch latch(3);
latch.count_down(); // × 3
latch.wait();
```

**Barrier** — reusable synchronization. Waits until a fixed number of participants arrive, then resets for the next generation:

```cpp
vix::threadpool::Barrier barrier(4);
barrier.arrive_and_wait();
```

## Metrics and stats

`ThreadPoolMetrics` — current state snapshot: `worker_count`, `pending_tasks`, `active_tasks`, `completed_tasks`, `failed_tasks`, `timed_out_tasks`, `rejected_tasks`, etc.

`ThreadPoolStats` — cumulative historical counters: `accepted_tasks`, `total_execution_time`, `max_execution_time`, etc.

```cpp
const auto metrics = pool.metrics();
const auto stats = pool.stats();
```

## Shutdown

Shutdown is safe and idempotent. A pool configured with `drain_on_shutdown = true` lets workers finish queued tasks before stopping. When drain is disabled, workers may stop before all queued tasks finish.

```cpp
pool.shutdown();
pool.shutdown(); // safe
```

## What Vix does not do

The threadpool does not forcibly kill running C++ functions. That means: cancellation is cooperative, timeout is observational, shutdown waits or stops according to configuration, user code must avoid infinite loops, and user code must protect shared data.

## Mental model

```txt
ThreadPool  →  owns Scheduler
Scheduler   →  owns Workers
Worker      →  owns WorkerThread + TaskQueue
TaskQueue   →  stores Tasks
Task        →  runs user callable
Future      →  returns result
TaskOptions →  configures behavior
```

The public API stays simple:

```cpp
pool.post(fn);
pool.submit(fn).get();
parallel_for(pool, first, last, fn);
pool.shutdown();
```

The internal system keeps the structure needed for reliability, metrics, cancellation, scheduling, and integration with the rest of Vix.
