# Timeouts

`vix::threadpool` supports timeout and deadline observation for tasks.

Timeouts help detect tasks that took longer than expected.

The recommended include is:

```cpp
#include <vix/threadpool/threadpool.hpp>
```

## Important rule

Timeouts are observational. Vix does not forcibly kill running C++ code. If a task exceeds its timeout, Vix can mark it as timed out after execution.

This is intentional because forcibly stopping C++ code can leave memory, locks, files, database transactions, or shared state corrupted.

## Timeout vs Deadline

There are two time-related concepts. A **timeout** is a relative duration — "this task should not take longer than 100 ms". A **deadline** is an absolute time point — "this task should not start or finish after this exact time point".

## Timeout

A timeout is a relative duration:

```cpp
vix::threadpool::Timeout timeout =
    vix::threadpool::Timeout::milliseconds(100);

// Or with seconds:
vix::threadpool::Timeout timeout =
    vix::threadpool::Timeout::seconds(2);
```

### Disabled timeout

The default timeout is disabled and never expires:

```cpp
vix::threadpool::Timeout timeout;

timeout.enabled();        // false
timeout.disabled_value(); // true
timeout.count();          // 0
timeout.expired(std::chrono::seconds{10}); // false
```

### Check timeout expiration

A timeout is checked against an elapsed duration. A task is considered timed out only when elapsed time is **greater than** the timeout.

```cpp
auto timeout = vix::threadpool::Timeout::milliseconds(50);

timeout.expired(std::chrono::milliseconds{40}); // false
timeout.expired(std::chrono::milliseconds{50}); // false
timeout.expired(std::chrono::milliseconds{51}); // true
```

### Negative timeout values

Negative timeout values are normalized to zero, preventing invalid durations from creating undefined behavior:

```cpp
auto timeout = vix::threadpool::Timeout::milliseconds(-10);

timeout.enabled(); // false
timeout.count();   // 0
```

## Use timeout with TaskOptions

Timeouts are attached to tasks through `TaskOptions`:

```cpp
vix::threadpool::TaskOptions options;
options.set_timeout(vix::threadpool::Timeout::milliseconds(100));

// With submit():
auto future = pool.submit([]() { return 42; }, options);

// With post():
pool.post([]() { do_background_work(); }, options);
```

### Example: task completes before timeout

```cpp
#include <chrono>
#include <iostream>
#include <thread>
#include <vix/threadpool.hpp>

int main()
{
  vix::threadpool::ThreadPool pool(2);

  vix::threadpool::TaskOptions options;
  options.set_timeout(vix::threadpool::Timeout::milliseconds(100));

  auto future =
      pool.submit(
          []()
          {
            std::this_thread::sleep_for(std::chrono::milliseconds{10});
            return 42;
          },
          options);

  std::cout << "value: " << future.get() << '\n';
  std::cout << "status: " << vix::threadpool::to_string(future.status()) << '\n';
  std::cout << "result: " << vix::threadpool::to_string(future.result()) << '\n';

  pool.shutdown();

  return 0;
}
```

Expected final state: `completed` / `success`.

### Example: task exceeds timeout

```cpp
#include <chrono>
#include <iostream>
#include <thread>
#include <vix/threadpool.hpp>

int main()
{
  vix::threadpool::ThreadPool pool(2);

  vix::threadpool::TaskOptions options;
  options.set_timeout(vix::threadpool::Timeout::milliseconds(50));

  auto future =
      pool.submit(
          []()
          {
            std::this_thread::sleep_for(std::chrono::milliseconds{100});
            return 42;
          },
          options);

  try
  {
    std::cout << "value: " << future.get() << '\n';
  }
  catch (const std::exception &e)
  {
    std::cout << "task failed: " << e.what() << '\n';
  }

  std::cout << "status: " << vix::threadpool::to_string(future.status()) << '\n';
  std::cout << "result: " << vix::threadpool::to_string(future.result()) << '\n';

  pool.shutdown();

  return 0;
}
```

The task body is not killed while it is running. After it finishes, Vix observes that it exceeded the timeout and marks it as timed out. Expected final state: `timed_out` / `timeout`.

## Timeout with `post`

For `post()` tasks, the timeout result is visible through metrics:

```cpp
vix::threadpool::TaskOptions options;
options.set_timeout(vix::threadpool::Timeout::milliseconds(50));

pool.post([]() { std::this_thread::sleep_for(std::chrono::milliseconds{100}); }, options);

pool.wait_idle();

const auto metrics = pool.metrics();
std::cout << metrics.timed_out_tasks << '\n';
```

## Timeout with `submit`

With `submit()`, timeout state can be inspected through the future:

```cpp
auto future = pool.submit(fn, options);

try { future.get(); } catch (...) {}

// For timeout: TaskStatus::timed_out, TaskResult::timeout
std::cout << vix::threadpool::to_string(future.status()) << '\n';
std::cout << vix::threadpool::to_string(future.result()) << '\n';
```

## Timeout with `handle`

`handle()` also accepts `TaskOptions`:

```cpp
vix::threadpool::TaskOptions options;
options.set_timeout(vix::threadpool::Timeout::milliseconds(100));

auto handle = pool.handle([]() { return 42; }, options);

handle.wait();

std::cout << vix::threadpool::to_string(handle.status()) << '\n';
std::cout << vix::threadpool::to_string(handle.result()) << '\n';
```

## Deadline

A deadline is an absolute time point:

```cpp
vix::threadpool::Deadline deadline =
    vix::threadpool::Deadline::after(std::chrono::milliseconds{100});
```

### Disabled deadline

The default deadline is disabled and never expires:

```cpp
vix::threadpool::Deadline deadline;

deadline.enabled();        // false
deadline.disabled_value(); // true
deadline.expired();        // false
```

### Create deadline from timeout

```cpp
auto timeout = vix::threadpool::Timeout::milliseconds(100);
auto deadline = vix::threadpool::Deadline::from_timeout(timeout);

// Disabled timeout → disabled deadline:
auto deadline = vix::threadpool::Deadline::from_timeout(
    vix::threadpool::Timeout::disabled());

deadline.enabled(); // false
```

### Use deadline with TaskOptions

```cpp
vix::threadpool::TaskOptions options;
options.set_deadline(
    vix::threadpool::Deadline::after(std::chrono::milliseconds{100}));

auto future = pool.submit([]() { return 42; }, options);
```

### Expired deadline before execution

If the deadline is already expired before the task starts, the task can be skipped:

```cpp
vix::threadpool::TaskOptions options;
options.set_deadline(
    vix::threadpool::Deadline::after(std::chrono::milliseconds{-1}));

auto future = pool.submit([]() { return 42; }, options);
// Can finish as: timed_out / timeout — without running the callable.
```

### Deadline utility methods

```cpp
auto deadline = vix::threadpool::Deadline::after(std::chrono::seconds{1});

// Remaining time:
auto remaining = deadline.remaining();

// Expiration check:
deadline.expired(); // true/false

// Check at a specific time point:
auto now = vix::threadpool::Deadline::clock::now();
deadline.expired_at(now);
```

## Timeout and default pool configuration

`ThreadPoolConfig` can define a default timeout. Tasks without their own timeout use the pool default; tasks with their own timeout override it:

```cpp
vix::threadpool::ThreadPoolConfig config;

config.thread_count = 4;
config.default_timeout = std::chrono::milliseconds{100};

vix::threadpool::ThreadPool pool(config);
```

## Timeout vs cancellation

Timeout means the task took longer than expected. Cancellation means someone requested that the task should stop. Both are safe and neither forcibly kills running C++ code.

```cpp
// Timeout:
options.set_timeout(vix::threadpool::Timeout::milliseconds(100));

// Cancellation:
handle.cancel();
```

## Timeout vs rejection

**Timeout** means the task was accepted but exceeded a time limit. **Rejection** means the task was never accepted (pool stopped, queue full, etc.).

Timeout state: `timed_out` / `timeout`. Rejected state: `rejected` / `rejected`.

## Why timeout is not forced interruption

Forcefully killing a running C++ task is unsafe. A task may be holding a mutex, file handle, database transaction, network socket, allocator state, or shared data structure. Stopping it mid-execution can corrupt the process. Vix uses timeout observation instead, keeping the runtime safe and predictable.

## Patterns

### Task with internal timeout checks

For long-running loops, check time explicitly inside user code:

```cpp
auto started = std::chrono::steady_clock::now();
auto limit = std::chrono::milliseconds{100};

auto future =
    pool.submit(
        [started, limit]()
        {
          while (true)
          {
            if (std::chrono::steady_clock::now() - started > limit)
            {
              return -1;
            }

            do_one_unit_of_work();
          }
        });
```

### Timeout with cancellation token

Combine a timeout with a cancellation token for both caller-driven cancellation and runtime timeout observation:

```cpp
vix::threadpool::CancellationSource source;

vix::threadpool::TaskOptions options;
options.set_cancellation(source.token());
options.set_timeout(vix::threadpool::Timeout::milliseconds(100));

auto token = source.token();

auto future =
    pool.submit(
        [token]()
        {
          while (token.can_continue())
          {
            do_one_unit_of_work();
          }

          return -1;
        },
        options);
```

### Deadline before expensive work

Use a deadline to avoid starting stale work:

```cpp
vix::threadpool::TaskOptions options;
options.set_deadline(
    vix::threadpool::Deadline::after(std::chrono::milliseconds{50}));

auto future = pool.submit([]() { return expensive_work(); }, options);
```

## Metrics and stats

Timed-out tasks are counted in both metrics (current state) and stats (cumulative):

```cpp
const auto metrics = pool.metrics();
std::cout << metrics.timed_out_tasks << '\n';

const auto stats = pool.stats();
std::cout << stats.timed_out_tasks << '\n';
```

## Best practices

- Use timeout to detect slow work
- Use deadline to avoid starting stale work
- Use cancellation for user-driven stop requests
- Do not expect timeout to interrupt blocking system calls
- Do not hold locks during long-running work if timeout matters
- For long loops, check time or cancellation manually
- Use `metrics()` and `stats()` to observe timeout behavior

## What timeouts do not do

Timeouts do not kill threads, interrupt blocking I/O, unlock mutexes, stop CPU loops automatically, rollback transactions, cancel futures by force, or guarantee immediate stop. They only observe whether a task exceeded its configured time budget.

## Simple mental model

```txt
Timeout      relative duration, checked against elapsed execution time
Deadline     absolute time point, checked before execution
TaskOptions  attaches timeout/deadline to task
Task         can finish as timed_out
ThreadPool   reports timed_out_tasks through metrics and stats
```

The safe pattern:

```cpp
vix::threadpool::TaskOptions options;
options.set_timeout(vix::threadpool::Timeout::milliseconds(100));

auto future = pool.submit(fn, options);

try { future.get(); } catch (...) {}

std::cout << vix::threadpool::to_string(future.status()) << '\n';
std::cout << vix::threadpool::to_string(future.result()) << '\n';
```
