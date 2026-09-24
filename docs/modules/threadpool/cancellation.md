# Cancellation

`vix::threadpool` supports cooperative cancellation.

Cancellation lets the caller request that a task should stop, skip execution, or finish early when the task code chooses to observe the cancellation token.

The recommended include is:

```cpp
#include <vix/threadpool.hpp>
```

## Important rule

Cancellation is cooperative. Vix does not forcibly kill running C++ code. This is intentional because forcibly stopping a C++ function can leave memory, locks, files, or shared state corrupted.

Cancellation can be observed: before a task starts, inside user code if the user checks a token, or after task execution if cancellation was requested.

## Basic idea

Cancellation has two sides:

```txt
CancellationSource  →  requests cancellation (owner)
CancellationToken   →  observes cancellation (read-only)
```

```cpp
vix::threadpool::CancellationSource source;

vix::threadpool::CancellationToken token = source.token();

source.request_cancel();

token.cancelled(); // true
```

## `CancellationSource`

The owner side. Can request cancellation:

```cpp
vix::threadpool::CancellationSource source;

source.request_cancel();

source.cancelled();    // true
source.is_cancelled(); // true
```

Calling `request_cancel()` multiple times is safe.

### Reset

A cancellation source can be reset to create a new cancellation state. Old tokens keep observing the old state:

```cpp
auto oldToken = source.token();

source.reset();

auto newToken = source.token();

source.request_cancel();

oldToken.cancelled(); // false
newToken.cancelled(); // true
```

## `CancellationToken`

The observer side. Cannot request cancellation — can only check the state:

```cpp
vix::threadpool::CancellationToken token = source.token();

if (token.cancelled()) { /* Stop early. */ }
```

Useful methods:

```cpp
token.can_cancel();
token.cancelled();
token.is_cancelled();
token.stop_requested();
token.can_continue();
```

### Default token

A default token is not connected to any source and is always safe to pass when cancellation is optional:

```cpp
vix::threadpool::CancellationToken token;

token.can_cancel();   // false
token.cancelled();    // false
token.can_continue(); // true
```

### Token reset

```cpp
token.reset(); // detaches from its state
token.can_cancel(); // false
```

## Cancellation through `TaskOptions`

```cpp
vix::threadpool::CancellationSource source;

vix::threadpool::TaskOptions options;
options.set_cancellation(source.token());

auto future = pool.submit([]() { return 42; }, options);

source.request_cancel();
```

If cancellation is requested before the task starts, the task can be skipped and marked as cancelled.

## Cancellation with `handle`

The simplest user-facing API is `handle()`. A `TaskHandle<T>` contains a task id, `Future<T>`, and a `CancellationSource`:

```cpp
auto handle = pool.handle([]() { return 42; });

handle.cancel();

try { int value = handle.get(); }
catch (const std::exception &e)
{
  // Task was cancelled, rejected, failed, or otherwise unavailable.
}
```

## Example: cancel before execution

```cpp
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <vix/threadpool/threadpool.hpp>

int main()
{
  vix::threadpool::ThreadPool pool(1);

  // Block the only worker:
  auto blocker = pool.submit([]()
  {
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
  });

  auto handle = pool.handle([]() { return 42; });

  handle.cancel();

  blocker.get();

  try
  {
    const int value = handle.get();
    std::cout << "value: " << value << '\n';
  }
  catch (const std::exception &e)
  {
    std::cout << "task did not complete normally: " << e.what() << '\n';
  }

  std::cout << "cancelled: " << (handle.cancelled() ? "yes" : "no") << '\n';

  pool.shutdown();

  return 0;
}
```

## Example: cooperative cancellation inside a task

For long-running tasks, pass a token explicitly into the task body:

```cpp
#include <chrono>
#include <iostream>
#include <thread>
#include <vix/threadpool/threadpool.hpp>

int main()
{
  vix::threadpool::ThreadPool pool(2);

  vix::threadpool::CancellationSource source;
  vix::threadpool::CancellationToken token = source.token();

  vix::threadpool::TaskOptions options;
  options.set_cancellation(token);

  auto future =
      pool.submit(
          [token]()
          {
            for (int i = 0; i < 100; ++i)
            {
              if (token.cancelled()) { return -1; }
              std::this_thread::sleep_for(std::chrono::milliseconds{10});
            }

            return 42;
          },
          options);

  std::this_thread::sleep_for(std::chrono::milliseconds{50});

  source.request_cancel();

  std::cout << "result: " << future.get() << '\n';

  pool.shutdown();

  return 0;
}
```

Vix does not interrupt the loop automatically. The task cooperates by checking the token.

## Why Vix does not kill running code

Forcefully killing a C++ function is unsafe. A task might be holding a mutex, file handle, database transaction, network socket, memory allocator state, or shared data structure. Stopping it mid-execution could corrupt the process.

## Cancellation status

A cancelled task ends with `TaskStatus::cancelled` / `TaskResult::cancelled`:

```cpp
std::cout << vix::threadpool::to_string(handle.status()) << '\n'; // cancelled
std::cout << vix::threadpool::to_string(handle.result()) << '\n'; // cancelled
```

## Cancellation and `Future`

A `Future<T>` can expose cancellation state through `status()` and `result()`, but cannot request cancellation itself. Use `handle()` or a custom `CancellationSource` through `TaskOptions` for cancellation control.

## Cancellation vs timeout

**Cancellation** — someone requested the task to stop. **Timeout** — the task exceeded an expected time limit. Both are cooperative/observational. Neither forcibly kills running C++ code.

## Cancellation and deadline

A deadline can skip a task before it starts if it has already expired. Cancellation is manually requested. Deadline is time-based. They can be combined in `TaskOptions`.

## Cancellation and `TaskGroup`

`TaskGroup` has a shared cancellation source:

```cpp
vix::threadpool::TaskGroup group;

auto token = group.cancellation_token();

group.cancel();

token.cancelled(); // true
```

Useful when many related tasks should observe the same cancellation request.

## Cancellation and `Scope`

`Scope` also has shared cancellation:

```cpp
vix::threadpool::Scope scope(pool);

scope.cancel();

scope.cancelled(); // true
```

Tasks spawned after cancellation may be skipped before execution.

## Cancellation and `PeriodicTask`

`task.stop()` stops periodic submissions but does not forcibly stop already-submitted callbacks. Use cancellation tokens inside the callback if the callback itself needs to stop early.

## Common patterns

### Long work with token

```cpp
vix::threadpool::CancellationSource source;
auto token = source.token();

vix::threadpool::TaskOptions options;
options.set_cancellation(token);

auto future =
    pool.submit(
        [token]()
        {
          while (token.can_continue())
          {
            do_one_unit_of_work();
          }
        },
        options);

source.request_cancel();
```

### With `TaskHandle`

```cpp
auto handle = pool.handle([]() { return run_job(); });

handle.cancel();

try { auto value = handle.get(); }
catch (const std::exception &e) { /* Handle cancellation or failure. */ }
```

### With `Scope`

```cpp
vix::threadpool::Scope scope(pool);

scope.spawn([]() { run_part_a(); });
scope.spawn([]() { run_part_b(); });

scope.cancel();
scope.wait();
```

## Best practices

- Use `handle()` when the caller needs cancellation control
- Use `TaskOptions` when cancellation state is owned outside the task
- For long-running loops, check the token periodically: `if (token.cancelled()) { return; }`
- Do not expect cancellation to interrupt blocking or CPU-bound code automatically
- Do not hold locks while waiting for cancellation
- Do not ignore the result of important cancelled tasks — use `get()` and inspect status/result

## What cancellation does not do

Cancellation does not: kill threads, interrupt blocking system calls, unlock mutexes, rollback database transactions, stop CPU loops automatically, or guarantee that already-running code stops immediately.

Cancellation only requests that work should stop. The task code must cooperate.

## Simple mental model

```txt
CancellationSource  owns state, can request cancellation
CancellationToken   observes state, can be copied into tasks
TaskOptions         attaches token to a task
Task                checks token before running, may be skipped if cancelled
User code           can check token during long work
```

The safe pattern:

```cpp
auto handle = pool.handle(fn);
handle.cancel();
handle.wait();
```

For long work:

```cpp
if (token.cancelled()) { return; }
```
