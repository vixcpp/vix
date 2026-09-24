# Periodic Tasks

`PeriodicTask` submits a callback repeatedly to an executor.

It is useful for background work that must run again and again, such as metrics flushing, cache cleanup, health checks, polling, or internal maintenance.

The recommended include is:

```cpp
#include <vix/threadpool/threadpool.hpp>
```

## Important idea

`PeriodicTask` owns a small scheduler thread that does not execute the user callback directly. Instead, it submits the callback to an executor:

```txt
PeriodicTask scheduler thread
  → waits for interval
  → posts callback to Executor

Executor
  → runs callback
```

This keeps timing logic separate from task execution.

## Basic usage

```cpp
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <vix/threadpool.hpp>

int main()
{
  vix::threadpool::ThreadPool pool(2);

  std::atomic<int> ticks{0};

  vix::threadpool::PeriodicTaskConfig config;
  config.interval = std::chrono::milliseconds{100};
  config.run_immediately = true;

  vix::threadpool::PeriodicTask task(
      pool,
      [&ticks]()
      {
        const int current = ticks.fetch_add(1, std::memory_order_relaxed) + 1;
        std::cout << "tick: " << current << '\n';
      },
      config);

  task.start();

  std::this_thread::sleep_for(std::chrono::milliseconds{350});

  task.stop();
  task.join();

  pool.wait_idle();
  pool.shutdown();

  return 0;
}
```

## Configuration

Periodic tasks are configured with `PeriodicTaskConfig`:

```cpp
vix::threadpool::PeriodicTaskConfig config;
```

### `interval`

Controls how long the periodic scheduler waits between submissions. Default is 1000 ms.

```cpp
config.interval = std::chrono::milliseconds{100};
```

Intervals ≤ 0 are normalized to 1 ms. You can also create a config with:

```cpp
auto config = vix::threadpool::PeriodicTaskConfig::every(std::chrono::milliseconds{250});
```

### `run_immediately`

By default the first callback is not submitted immediately (`false`). Set to `true` to submit on start:

```cpp
config.run_immediately = false; // wait → submit → wait → submit ...
config.run_immediately = true;  // submit → wait → submit → wait ...
```

### `stop_on_post_failure`

By default (`true`), periodic submission stops if the executor rejects a callback. Most applications should keep this default:

```cpp
config.stop_on_post_failure = true;  // safest default
config.stop_on_post_failure = false; // continue after failed posts
```

### `task_options`

Every submitted callback receives these task options:

```cpp
config.task_options.set_priority(vix::threadpool::TaskPriority::low);
```

## Create and start a periodic task

The constructor does not start the task automatically:

```cpp
vix::threadpool::PeriodicTask task(pool, []() { flush_metrics(); }, config);

task.start();
```

### `start()`

Returns `true` if the scheduler thread was started. Returns `false` if: no executor, empty callback, already running, or thread creation failed.

```cpp
if (!task.start())
{
  std::cout << "failed to start periodic task\n";
}
```

Calling `start()` while already running returns `false` (idempotent while running).

## Stop and join

```cpp
task.stop(); // requests stop (non-blocking)
task.join(); // waits for scheduler thread to exit
```

The destructor calls `stop()` and `join()` automatically, but explicit lifecycle management is clearer in services and tests.

### State checks

```cpp
task.running();   // scheduler is active
task.joinable();  // scheduler thread can be joined
```

## Counters

```cpp
task.submitted_ticks(); // callbacks successfully posted to executor
task.failed_posts();    // callback submissions that failed
```

A post fails when the executor is stopped, rejects the task, or is no longer valid.

## Examples

### Metrics flushing

```cpp
#include <chrono>
#include <iostream>
#include <thread>
#include <vix/threadpool.hpp>

int main()
{
  vix::threadpool::ThreadPool pool(2);

  vix::threadpool::PeriodicTaskConfig config;
  config.interval = std::chrono::milliseconds{100};
  config.run_immediately = true;
  config.task_options.set_priority(vix::threadpool::TaskPriority::low);

  vix::threadpool::PeriodicTask flushTask(
      pool,
      []() { std::cout << "flush metrics\n"; },
      config);

  flushTask.start();

  std::this_thread::sleep_for(std::chrono::milliseconds{350});

  flushTask.stop();
  flushTask.join();

  pool.wait_idle();

  std::cout << "submitted: " << flushTask.submitted_ticks() << '\n';

  pool.shutdown();

  return 0;
}
```

### Cache cleanup

```cpp
vix::threadpool::PeriodicTaskConfig config;
config.interval = std::chrono::seconds{5};
config.run_immediately = false;
config.task_options.set_priority(vix::threadpool::TaskPriority::low);

vix::threadpool::PeriodicTask cleanup(
    pool, []() { cleanup_expired_cache_entries(); }, config);

cleanup.start();
```

### Health check

```cpp
vix::threadpool::PeriodicTaskConfig config;
config.interval = std::chrono::seconds{1};
config.run_immediately = true;
config.task_options.set_priority(vix::threadpool::TaskPriority::high);

vix::threadpool::PeriodicTask health(
    pool, []() { refresh_health_snapshot(); }, config);

health.start();
```

## Using InlineExecutor

For deterministic tests, use `InlineExecutor`. Callbacks run directly on the periodic scheduler thread:

```cpp
vix::threadpool::InlineExecutor executor;

std::atomic<int> ticks{0};

vix::threadpool::PeriodicTaskConfig config;
config.interval = std::chrono::milliseconds{10};
config.run_immediately = true;

vix::threadpool::PeriodicTask task(
    executor, [&ticks]() { ticks.fetch_add(1, std::memory_order_relaxed); }, config);

task.start();

std::this_thread::sleep_for(std::chrono::milliseconds{30});

task.stop();
task.join();
```

## Callback overlap

`PeriodicTask` does not wait for the previous callback to finish before submitting the next one. If the interval is short and the callback is slow, multiple callbacks can run at the same time.

### Prevent overlapping callbacks

```cpp
std::atomic<bool> running{false};

vix::threadpool::PeriodicTask task(
    pool,
    [&running]()
    {
      bool expected = false;

      if (!running.compare_exchange_strong(expected, true))
      {
        return; // previous callback still running
      }

      try { do_work(); }
      catch (...) { running.store(false); throw; }

      running.store(false);
    },
    config);
```

## Combining with other features

### Cancellation token

```cpp
vix::threadpool::CancellationSource source;
auto token = source.token();

vix::threadpool::PeriodicTask task(
    pool,
    [token]()
    {
      if (token.cancelled()) { return; }
      do_work();
    },
    config);

task.start();

source.request_cancel();

task.stop();
task.join();
```

`stop()` controls periodic submission. The token controls callback logic.

### Timeout on each submission

```cpp
vix::threadpool::PeriodicTaskConfig config;
config.interval = std::chrono::milliseconds{100};
config.task_options.set_timeout(vix::threadpool::Timeout::milliseconds(50));
```

Timeouts are observational — Vix does not forcibly kill running C++ code.

## Shutdown order

Always stop periodic tasks **before** shutting down the pool:

```cpp
// Good:
task.stop();
task.join();
pool.wait_idle();
pool.shutdown();

// Bad:
pool.shutdown();
task.stop();
task.join();
```

`stop()` stops future submissions. Already-submitted callbacks may still be running. Use `pool.wait_idle()` to let them finish.

## Common mistakes

### Forgetting join

```cpp
// Bad:
task.stop();
// missing task.join()

// Good:
task.stop();
task.join();
```

### Callback takes longer than interval

If the callback consistently takes longer than the interval, callbacks will overlap. Use a non-overlap guard if that is not acceptable.

### Using tiny intervals in production

Very small intervals (e.g., 1 ms) may create too many submissions. Prefer realistic intervals:

```txt
metrics flush:  1s – 10s
cache cleanup:  5s – 60s
health refresh: 500ms – 5s
```

## When to use PeriodicTask

Good use cases: metrics flushing, cache cleanup, health checks, periodic polling, background maintenance, runtime housekeeping, retry ticks, queue draining checks.

Avoid when: you need precise real-time scheduling, callbacks must never overlap but you don't guard them, the interval is extremely small, the callback blocks forever, or the executor may be destroyed before the periodic task stops.

## Simple mental model

```txt
PeriodicTask
  owns scheduler thread
  waits for interval
  posts callback to Executor
  tracks submitted ticks and failed posts

stop()           stops future submissions
join()           waits for scheduler thread
pool.wait_idle() waits for submitted callbacks
```

The normal pattern:

```cpp
vix::threadpool::PeriodicTask task(pool, callback, config);

task.start();

// ... later ...

task.stop();
task.join();

pool.wait_idle();
pool.shutdown();
```
