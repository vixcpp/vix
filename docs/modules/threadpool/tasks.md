# Tasks

A task is one unit of work executed by `vix::threadpool`.

Most users do not create `Task` objects directly. They write normal C++ callables, then Vix wraps them internally.

```cpp
pool.post(
    []()
    {
      // Task body.
    });
```

Or with a result:

```cpp
auto future =
    pool.submit(
        []()
        {
          return 42;
        });
```

## What a task contains

Internally, a task stores: task id, callable, task options, status, result, priority, sequence number, captured exception, creation time, start time, and finish time.

This gives the threadpool enough information to execute, order, observe, cancel, reject, or report a task safely.

## Task id

Each task has a `TaskId`:

```cpp
vix::threadpool::TaskId id = pool.next_task_id();
```

Most users do not need to allocate ids manually. Task ids are mainly useful for diagnostics, metrics, task handles, task groups, tests, and runtime integrations.

An invalid task id is represented by `vix::threadpool::invalid_task_id`. A valid task id can be checked with:

```cpp
vix::threadpool::is_valid_task_id(id);
```

## Task function

A task stores a callable with this internal type: `vix::threadpool::TaskFunction`.

`TaskFunction` is move-only. That means tasks can safely own packaged tasks, promises, unique pointers, move-only captures, and internal state used by futures:

```cpp
auto ptr = std::make_unique<int>(42);

pool.post(
    [value = std::move(ptr)]()
    {
      // Owns value.
    });
```

## `post` tasks

`post()` creates a detached fire-and-forget task.

```cpp
pool.post(
    []()
    {
      save_logs();
    });
```

Use `post()` when you do not need a result, do not need `future.get()`, and the task is background work. `post()` returns whether the task was accepted:

```cpp
const bool accepted = pool.post([]() { /* Work */ });

if (!accepted)
{
  // Task was rejected.
}
```

## `submit` tasks

`submit()` creates a task connected to a `Future<T>`.

```cpp
auto future =
    pool.submit(
        []()
        {
          return 42;
        });

int value = future.get();
```

Use `submit()` when you need a return value, need to wait for a specific task, or need exception propagation.

### void submit

`submit()` also supports tasks that return void:

```cpp
auto future =
    pool.submit(
        []()
        {
          write_file();
        });

future.get();
```

For `Future<void>`, `get()` waits and rethrows exceptions, but returns no value.

## Task lifecycle

A task moves through lifecycle states represented by `TaskStatus`.

A typical successful task:

```txt
created -> queued -> running -> completed
```

A task that throws:

```txt
created -> queued -> running -> failed
```

A task cancelled before execution:

```txt
created -> queued -> cancelled
```

A task submitted after shutdown:

```txt
created -> rejected
```

## Task result

`TaskResult` represents the final outcome: `none`, `success`, `failure`, `cancelled`, `timeout`, or `rejected`.

`TaskStatus` describes the lifecycle state. `TaskResult` describes the final outcome. Example:

```cpp
auto future = pool.submit([]() { return 42; });

future.get();

std::cout << vix::threadpool::to_string(future.status()) << '\n';
std::cout << vix::threadpool::to_string(future.result()) << '\n';
// completed
// success
```

## Task priority

```cpp
vix::threadpool::TaskOptions options;
options.set_priority(vix::threadpool::TaskPriority::high);

pool.post([]() { /* High-priority work. */ }, options);
```

Priority levels: `lowest`, `low`, `normal`, `high`, `highest`.

Higher priority tasks are selected before lower priority tasks in the same queue. Priority does not interrupt a task that is already running.

When two tasks have the same priority, Vix uses a sequence number to keep stable FIFO order within that priority level.

## Task options

`TaskOptions` controls task behavior:

```cpp
vix::threadpool::TaskOptions options;

options.set_priority(vix::threadpool::TaskPriority::high);
options.set_timeout(vix::threadpool::Timeout::milliseconds(100));
```

Available options: priority, timeout, deadline, cancellation, affinity, `allow_after_stop`, detached, flags.

### Timeout

Timeouts are observational. Vix does not forcibly kill running C++ code. If a task exceeds its timeout, it can be marked as timed out after execution.

```cpp
vix::threadpool::TaskOptions options;
options.set_timeout(vix::threadpool::Timeout::milliseconds(50));
```

### Deadline

A deadline is an absolute time limit. If the deadline is already expired before the task starts, the task can be skipped and marked as timed out.

```cpp
vix::threadpool::TaskOptions options;
options.set_deadline(
    vix::threadpool::Deadline::after(std::chrono::milliseconds{100}));
```

### Cancellation

Cancellation is cooperative:

```cpp
vix::threadpool::CancellationSource source;

vix::threadpool::TaskOptions options;
options.set_cancellation(source.token());

source.request_cancel();
```

If cancellation is requested before a task runs, the task can be skipped. Running C++ code is not forcefully interrupted.

## TaskHandle

For user-facing cancellation, use `handle()`:

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
  const int value = handle.get();
}
catch (const std::exception &e)
{
  std::cout << e.what() << '\n';
}
```

A `TaskHandle` contains a task id, `Future<T>`, and a `CancellationSource`. Useful methods: `id()`, `cancel()`, `cancelled()`, `wait()`, `get()`, `status()`, `result()`, `error()`.

## Worker affinity

Affinity is a hint for the scheduler, not a guarantee:

```cpp
vix::threadpool::TaskOptions options;
options.set_affinity(vix::threadpool::WorkerId{1});
```

## Rejected tasks

A task may be rejected when the pool is stopped, the queue is full, or no worker can be selected:

```cpp
pool.shutdown();

const bool accepted = pool.post([]() {});

if (!accepted)
{
  // Rejected.
}
```

For `submit()`, rejection is reported through the future:

```cpp
auto future = pool.submit([]() { return 42; });

std::cout << vix::threadpool::to_string(future.status()) << '\n'; // rejected
std::cout << vix::threadpool::to_string(future.result()) << '\n'; // rejected
```

## Exceptions

Task exceptions never escape worker threads.

For `submit()`, exceptions are stored in the future:

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
  std::cout << e.what() << '\n';
}
```

For `post()`, there is no future. The exception is captured internally and reflected in metrics as a failure.

## Task timing

A task stores: `created_at`, `started_at`, `finished_at`, and `execution_duration`. This is useful for timeout observation, metrics, diagnostics, and future runtime tracing. Most users access timing indirectly through metrics and stats.

## Task queue

Tasks are stored in `TaskQueue`. The default queue behavior is priority-based: higher priority first, with same priority ordered by sequence number. The queue rejects invalid tasks, terminal tasks, and tasks beyond configured capacity.

### Bounded queues

```cpp
vix::threadpool::ThreadPoolConfig config;

config.thread_count = 4;
config.max_queue_size = 1024;

vix::threadpool::ThreadPool pool(config);
```

A bounded queue prevents unbounded memory growth. When the queue is full, new tasks may be rejected.

## Scope tasks

`Scope` is the structured way to group tasks in application code:

```cpp
vix::threadpool::Scope scope(pool);

scope.spawn([]() { /* Work A */ });
scope.spawn([]() { /* Work B */ });
scope.wait();
```

Use `scope.wait_and_rethrow()` when you want the first task exception to be rethrown after all tasks finish.

## Best practices

Use `post()` for background tasks, `submit()` for result-producing tasks, and `handle()` when cancellation is needed:

```cpp
pool.post(fn);
auto future = pool.submit(fn);
auto handle = pool.handle(fn);
handle.cancel();
```

Use `Scope` when several tasks must finish before the current function returns. Use `TaskOptions` instead of hidden behavior. Always protect shared mutable data with a mutex.

## What tasks do not do

Tasks do not forcibly stop running C++ code. That means: cancellation is cooperative, timeout is observational, priority does not preempt running work, and shutdown does not kill a thread mid-execution. This is intentional — forcefully killing C++ functions can leave memory, locks, files, or shared state corrupted.

## Simple mental model

```txt
User callable  →  Task
Task           →  TaskQueue (ordered by priority + sequence)
Worker         →  pops Task, runs callable
Future         →  receives result or exception
```

The public API stays simple:

```cpp
pool.post(fn);
auto future = pool.submit(fn);
future.get();
```
