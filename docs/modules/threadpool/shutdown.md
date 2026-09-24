# Shutdown

`vix::threadpool` provides explicit, safe, and idempotent shutdown behavior.

Shutdown is important because a thread pool owns worker threads. Those workers must be stopped and joined cleanly before the application exits.

The recommended include is:

```cpp
#include <vix/threadpool.hpp>
```

## Basic usage

The normal shutdown pattern is:

```cpp
pool.wait_idle();
pool.shutdown();
```

`wait_idle()` waits for accepted work to finish. `shutdown()` stops the pool and joins worker threads.

### Simple example

```cpp
#include <chrono>
#include <iostream>
#include <thread>
#include <vix/threadpool.hpp>

int main()
{
  vix::threadpool::ThreadPool pool(2);

  for (int i = 0; i < 4; ++i)
  {
    pool.post(
        [i]()
        {
          std::this_thread::sleep_for(std::chrono::milliseconds{50});
          std::cout << "task finished: " << i << '\n';
        });
  }

  pool.wait_idle();

  std::cout << "pool idle: " << (pool.idle() ? "yes" : "no") << '\n';

  pool.shutdown();

  std::cout << "pool running: " << (pool.running() ? "yes" : "no") << '\n';

  return 0;
}
```

## `shutdown()`

`shutdown()` requests the pool to stop and joins its workers. After shutdown, new tasks are rejected:

```cpp
pool.shutdown();

pool.running(); // false

const bool accepted = pool.post([]() {});
accepted; // false
```

### Shutdown is idempotent

Calling `shutdown()` multiple times is safe:

```cpp
pool.shutdown();
pool.shutdown();
pool.shutdown();
```

Only the first call performs the shutdown work. This is useful for destructors, error paths, and service cleanup.

### Destructor behavior

`ThreadPool` shuts down in its destructor, so this is safe:

```cpp
{
  vix::threadpool::ThreadPool pool(4);
  pool.post([]() { /* Work */ });
}
```

But explicit shutdown is clearer in examples, tests, services, and long-running applications:

```cpp
pool.wait_idle();
pool.shutdown();
```

## `wait_idle()`

`wait_idle()` waits until the pool has no pending or active tasks. It returns when `pending_tasks == 0` and `active_tasks == 0`.

```cpp
pool.wait_idle();
pool.shutdown();
```

## `idle()`

`idle()` is a non-blocking snapshot check:

```cpp
if (pool.idle())
{
  std::cout << "pool is idle\n";
}
```

## Shutdown with drain enabled

`ThreadPoolConfig::drain_on_shutdown` controls whether queued tasks should finish during shutdown:

```cpp
vix::threadpool::ThreadPoolConfig config;

config.thread_count = 2;
config.drain_on_shutdown = true;

vix::threadpool::ThreadPool pool(config);
```

### Example with drain

```cpp
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <vix/threadpool.hpp>

int main()
{
  vix::threadpool::ThreadPoolConfig config;

  config.thread_count = 1;
  config.drain_on_shutdown = true;

  vix::threadpool::ThreadPool pool(config);

  std::atomic<int> counter{0};

  for (int i = 0; i < 6; ++i)
  {
    pool.post(
        [&counter]()
        {
          std::this_thread::sleep_for(std::chrono::milliseconds{2});
          counter.fetch_add(1, std::memory_order_relaxed);
        });
  }

  pool.shutdown();

  std::cout << "completed: " << counter.load() << '\n';

  return 0;
}
```

## Shutdown without drain

When `drain_on_shutdown = false`, workers may stop before all queued tasks finish. Useful for best-effort background work where pending tasks can be dropped.

```cpp
#include <atomic>
#include <iostream>
#include <vix/threadpool.hpp>

int main()
{
  vix::threadpool::ThreadPoolConfig config;

  config.thread_count = 1;
  config.drain_on_shutdown = false;

  vix::threadpool::ThreadPool pool(config);

  std::atomic<int> counter{0};

  for (int i = 0; i < 20; ++i)
  {
    pool.post(
        [&counter]()
        {
          counter.fetch_add(1, std::memory_order_relaxed);
        });
  }

  pool.shutdown();

  std::cout << "executed: " << counter.load() << '\n';

  return 0;
}
```

The executed count may be less than 20. That is expected when draining is disabled.

## Shutdown does not kill running C++ code

Vix does not forcibly kill worker threads in the middle of user code. A running task is always allowed to return normally. Shutdown controls the pool lifecycle — it does not interrupt unsafe user code.

## Rejection after shutdown

After shutdown, new work is rejected. For `post()`, the return value is `false`. For `submit()`, a rejected future is returned:

```cpp
pool.shutdown();

auto future = pool.submit([]() { return 42; });

future.ready();  // true
future.status(); // rejected
future.result(); // rejected
```

Calling `get()` on a rejected future throws `std::system_error`:

```cpp
try
{
  int value = future.get();
}
catch (const std::system_error &e)
{
  // task was rejected
}
```

For `handle()`, the handle's future similarly reports rejection.

## `clear()`

`clear()` removes queued tasks that have not started yet. It does not stop tasks that are already running:

```cpp
const std::size_t removed = pool.clear();
```

A common pattern before dropping queued work:

```cpp
pool.clear();
pool.shutdown();
```

## `wait_idle` vs `shutdown`

| Method | Purpose |
|---|---|
| `wait_idle()` | Waits for no pending or active tasks |
| `shutdown()` | Stops workers and rejects new tasks |

When work must finish: `pool.wait_idle(); pool.shutdown();`

When pending work can be dropped: `pool.clear(); pool.shutdown();`

## Recommended shutdown order

### With PeriodicTask

Stop the periodic task before shutting down the pool:

```cpp
// Good:
periodic.stop();
periodic.join();
pool.wait_idle();
pool.shutdown();

// Bad:
pool.shutdown();
periodic.stop();
periodic.join();
```

### With Scope

```cpp
{
  vix::threadpool::Scope scope(pool);

  scope.spawn(work_a);
  scope.spawn(work_b);

  scope.wait_and_rethrow();
}

pool.shutdown();
```

### With futures

Consume futures before shutdown when results matter:

```cpp
auto first = pool.submit(work_a);
auto second = pool.submit(work_b);

auto a = first.get();
auto b = second.get();

pool.shutdown();
```

### For services

```cpp
stop_accepting_new_work();

periodic.stop();
periodic.join();

pool.wait_idle();

log_final_metrics();

pool.shutdown();
```

## Metrics before and after shutdown

Metrics and stats remain readable after shutdown:

```cpp
pool.wait_idle();

const auto metrics = pool.metrics();

std::cout << "completed: " << metrics.completed_tasks << '\n';
std::cout << "failed: " << metrics.failed_tasks << '\n';
std::cout << "rejected: " << metrics.rejected_tasks << '\n';

pool.shutdown();

// Still readable:
const auto stats = pool.stats();
```

## Shutdown state checks

```cpp
pool.running(); // pool is still running
pool.idle();    // no pending or active work
pool.pending(); // number of queued tasks
```

## Common mistakes

### Posting after shutdown

```cpp
// Bad:
pool.shutdown();
pool.post([]() { do_work(); }); // rejected

// Better:
if (pool.running()) { pool.post(fn); }
// Or:
if (!pool.post(fn)) { /* fallback */ }
```

### Shutting down while PeriodicTask is still running

```cpp
// Bad:
pool.shutdown();
task.stop();
task.join();

// Good:
task.stop();
task.join();
pool.wait_idle();
pool.shutdown();
```

### Expecting shutdown to cancel running tasks

Shutdown does not forcibly cancel running C++ code. For cooperative early exit, use cancellation tokens inside task code:

```cpp
if (token.cancelled()) { return; }
```

### Ignoring futures

```cpp
// Bad: exception is stored in future but future is dropped
pool.submit([]() { throw std::runtime_error{"failed"}; });
pool.shutdown();

// Good:
auto future = pool.submit([]() -> int { throw std::runtime_error{"failed"}; });
try { future.get(); }
catch (const std::exception &e) { std::cout << e.what() << '\n'; }
pool.shutdown();
```

## Shutdown with cancellation

Cancellation and shutdown are different. Cancellation asks tasks to stop cooperatively. Shutdown stops the pool lifecycle. You can combine both:

```cpp
vix::threadpool::CancellationSource source;

vix::threadpool::TaskOptions options;
options.set_cancellation(source.token());

auto future =
    pool.submit(
        [token = source.token()]()
        {
          while (token.can_continue())
          {
            do_one_unit_of_work();
          }
        },
        options);

source.request_cancel();
future.wait();
pool.shutdown();
```

## Simple mental model

```txt
wait_idle()   waits for accepted work to finish
clear()       removes queued work that has not started
shutdown()    stops workers, joins threads, rejects new work, idempotent
destructor    calls shutdown automatically
```

The safest default pattern:

```cpp
pool.wait_idle();
pool.shutdown();
```

For service shutdown:

```cpp
stop_accepting_new_work();
periodic.stop();
periodic.join();
pool.wait_idle();
log_final_metrics();
pool.shutdown();
```
