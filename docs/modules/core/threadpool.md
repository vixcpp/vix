# ThreadPool (vix::threadpool)

This module provides a priority-based thread pool used by Vix for background work and heavy workloads.

Key features:

- Priority scheduling via a `std::priority_queue<Task, ..., TaskCmp>`
- FIFO behavior inside the same priority using a monotonic sequence number (`seq`)
- Automatic worker growth when saturated (up to `maxThreads`)
- Lightweight metrics: pending, active, timed out
- Periodic scheduler threads that enqueue work at fixed intervals
- Linux CPU affinity for worker threads (best-effort)

## Components

### 1) Task

`Task` is the unit of work stored in the internal priority queue.

- `func`: `std::function<void()>` executed by a worker
- `priority`: integer priority used by the queue
- `seq`: increasing number used to keep deterministic ordering

Important detail:

- Higher `priority` runs first.
- If two tasks have the same `priority`, smaller `seq` runs first (earlier enqueued first).

### 2) TaskCmp

`TaskCmp` defines ordering for the priority queue.

- First compares `priority`
- Then compares `seq`

Because `std::priority_queue` keeps the "largest" element on top, the comparator is written so that:

- tasks with higher priority end up at the top
- for equal priority, the smaller sequence number is served first

### 3) TaskGuard

`TaskGuard` is a small RAII helper that increments a shared atomic counter on construction and decrements on destruction.

In this pool, it is used to track `activeTasks` safely even if the task throws.

### 4) ThreadPool

The pool has two kinds of threads:

1. Worker threads (`workers`)
   - Wait for tasks
   - Pop the highest priority task
   - Execute it
   - Update `activeTasks`
   - Notify `cvIdle` when the pool becomes idle

2. Periodic scheduler threads (`periodicWorkers`)
   - Sleep until the next tick
   - Enqueue a task into the main pool
   - Limit concurrency with `maxPeriodicThreads`

## Construction

Constructor:

- `threadCount`: initial worker threads (clamped to at least 1)
- `maxThreadCount`: upper bound for auto-growth (clamped to at least 1)
- `priority`: default priority used by `enqueue()` overloads without an explicit priority
- `maxPeriodic`: maximum concurrent periodic scheduler threads (defaults to 4, clamped to at least 1)

Behavior:

- Workers start immediately.
- If worker creation fails, the pool stops and joins threads before rethrowing.

## Enqueueing work

There are three overloads:

1. `enqueue(priority, timeout, f, args...)`
2. `enqueue(priority, f, args...)`
3. `enqueue(f, args...)` (uses default pool priority)

### Timeout behavior

The timeout is for warning and metrics only.

- The pool does not cancel the work.
- If the work finishes after `timeout`, a warning is logged and `tasksTimedOut` is incremented.

### Return values

`enqueue` wraps the callable in a `std::packaged_task` and returns a `std::future<ReturnType>`.

This means:

- Exceptions thrown inside the callable propagate through the future.
- You can `future.get()` to rethrow.

### Auto-scaling

On each enqueue, the pool checks:

- `wcount = workers.size()`
- `saturated = (activeTasks >= wcount)`
- `backlog = (tasks.size() > wcount)`

If:

- `wcount < maxThreads` and `saturated` and `backlog`

then it creates a new worker thread.

This is a simple heuristic: it grows when all workers appear busy and the queue is building.

## Periodic scheduling

`periodicTask(priority, func, interval)` creates a scheduler thread that:

- Ensures there are fewer than `maxPeriodicThreads` active periodic threads
- On each tick, enqueues `func` into the main pool using `enqueue(priority, ...)`
- Warns if the posted task is not completed by the time the next tick occurs

Important details:

- The scheduler itself does not run the job; it only posts it into the pool.
- Periodic threads stop when `stopPeriodic` becomes true.

## Idle waiting

Two helpers exist:

- `isIdle()` returns true when there are no pending tasks and `activeTasks == 0`
- `waitUntilIdle()` blocks until both conditions are true

Implementation note:

- Workers notify `cvIdle` whenever they see both `tasks.empty()` and `activeTasks == 0` after finishing a task.

## Shutdown and lifetime

The destructor stops everything and joins threads.

- Sets `stop = true` and `stopPeriodic = true`
- Wakes all waiters (`condition`, `cvPeriodic`)
- Joins periodic workers then main workers

Behavioral implications:

- Enqueue after stop triggers an exception (`ThreadPool is stopped; cannot enqueue new tasks`).
- Stopping does not attempt to cancel running tasks; it waits for workers to exit after the queue drains.

## Platform notes

### Linux CPU affinity

On Linux, worker threads attempt to pin themselves to a CPU core:

- Uses `pthread_setaffinity_np`
- Core index is `id % hardware_concurrency`
- If setting affinity fails, the pool logs a warning

On non-Linux platforms, the affinity call is a no-op.

### Windows macros

On Windows, the header temporarily undefines `min` and `max` macros to avoid conflicts with standard library headers.

## Usage example

Minimal usage pattern:

1. Create a pool with a reasonable thread count.
2. Enqueue work with priorities.
3. Optionally wait for idle during shutdown.

Example priorities convention (suggested):

- 100: latency-sensitive tasks
- 50: normal tasks
- 10: background tasks

## Integration note for Vix server

In the HTTP pipeline, heavy routes are executed via an executor abstraction. The executor can be backed by this thread pool (or another implementation). A heavy route should:

- Avoid blocking the I/O threads
- Run in the executor/pool
- Return the response via the session write path

That separation is what keeps the I/O threads responsive.

## Quick checklist

- Use a small `threadCount` for startup, allow growth with `maxThreads`.
- Use priorities consistently across the codebase.
- Use timeout warnings to find tasks that exceed your SLO.
- Prefer short periodic tasks, and keep periodic concurrency bounded.
- Call `waitUntilIdle()` only in controlled shutdown paths.

