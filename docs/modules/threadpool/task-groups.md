# Task Groups

`TaskGroup` coordinates several related tasks as one logical group.

It tracks how many tasks were registered, how many are still pending, and how many finished with success, failure, cancellation, timeout, or rejection.

The recommended include is:

```cpp
#include <vix/threadpool/threadpool.hpp>
```

## Basic idea

A task group is useful when several tasks belong to the same operation. For example: load users, load products, load orders. All three tasks are separate, but logically they belong to one larger operation. `TaskGroup` gives you one place to track that operation.

## Create a group

```cpp
vix::threadpool::TaskGroup group;
```

A new group is empty:

```cpp
group.empty();         // true
group.done();          // true
group.total_tasks();   // 0
group.pending_tasks(); // 0
```

## Register tasks

Use `add_task()` to register a task id.

```cpp
group.add_task(vix::threadpool::TaskId{1});
group.add_task(vix::threadpool::TaskId{2});
group.add_task(vix::threadpool::TaskId{3});
```

Now the group knows that 3 tasks are pending:

```cpp
group.total_tasks();   // 3
group.pending_tasks(); // 3
group.done();          // false
```

Invalid task ids are rejected:

```cpp
group.add_task(vix::threadpool::invalid_task_id); // false
```

## Finish a task

When a task completes, call `finish_task()`.

```cpp
group.finish_task(
    vix::threadpool::TaskStatus::completed,
    vix::threadpool::TaskResult::success);
```

This decrements the pending count and updates the result counters.

## Complete example

```cpp
#include <chrono>
#include <iostream>
#include <thread>
#include <vix/threadpool.hpp>

int main()
{
  vix::threadpool::TaskGroup group;

  group.add_task(vix::threadpool::TaskId{1});
  group.add_task(vix::threadpool::TaskId{2});
  group.add_task(vix::threadpool::TaskId{3});

  std::thread first(
      [&group]()
      {
        std::this_thread::sleep_for(std::chrono::milliseconds{20});
        group.finish_task(
            vix::threadpool::TaskStatus::completed,
            vix::threadpool::TaskResult::success);
      });

  std::thread second(
      [&group]()
      {
        std::this_thread::sleep_for(std::chrono::milliseconds{40});
        group.finish_task(
            vix::threadpool::TaskStatus::completed,
            vix::threadpool::TaskResult::success);
      });

  std::thread third(
      [&group]()
      {
        std::this_thread::sleep_for(std::chrono::milliseconds{60});
        group.finish_task(
            vix::threadpool::TaskStatus::cancelled,
            vix::threadpool::TaskResult::cancelled);
      });

  group.close();
  group.wait();

  first.join();
  second.join();
  third.join();

  std::cout << "total: " << group.total_tasks() << '\n';
  std::cout << "completed: " << group.completed_tasks() << '\n';
  std::cout << "cancelled: " << group.cancelled_tasks() << '\n';
  std::cout << "has error: " << (group.has_error() ? "yes" : "no") << '\n';

  return 0;
}
```

## Waiting for all tasks

Use `wait()` to block until all registered tasks finish.

```cpp
group.wait();
```

`wait()` returns when `pending_tasks == 0`. It does not throw.

## Wait and rethrow

Use `wait_and_rethrow()` when you want to wait for all tasks and then rethrow the first captured exception.

```cpp
group.finish_task(
    vix::threadpool::TaskStatus::failed,
    vix::threadpool::TaskResult::failure,
    std::make_exception_ptr(std::runtime_error{"task failed"}));

try
{
  group.wait_and_rethrow();
}
catch (const std::exception &e)
{
  std::cout << e.what() << '\n';
}
```

`wait_and_rethrow()` waits for all tasks first, then rethrows the first captured exception. This avoids leaving other tasks unfinished.

## Closing a group

Use `close()` to prevent new task registrations.

```cpp
group.close();
```

After closing, new calls to `add_task()` are rejected, but existing tasks are not cancelled.

## Cancellation

A group owns a shared cancellation source.

```cpp
group.cancel();
group.cancelled(); // true
```

You can also get a token:

```cpp
vix::threadpool::CancellationToken token = group.cancellation_token();

if (token.cancelled()) { ... }
```

Cancellation is cooperative. `TaskGroup` does not forcibly stop running C++ code. It only exposes shared cancellation state that tasks or higher-level systems can observe.

## Counters

`TaskGroup` tracks these counters:

```cpp
group.total_tasks()
group.pending_tasks()
group.completed_tasks()
group.failed_tasks()
group.cancelled_tasks()
group.timed_out_tasks()
group.rejected_tasks()
```

### Successful task

```cpp
group.finish_task(
    vix::threadpool::TaskStatus::completed,
    vix::threadpool::TaskResult::success);
// increments: completed_tasks
```

### Failed task

```cpp
group.finish_task(
    vix::threadpool::TaskStatus::failed,
    vix::threadpool::TaskResult::failure,
    std::make_exception_ptr(std::runtime_error{"failed"}));
// increments: failed_tasks
// stores first exception if none was stored yet
```

### Cancelled task

```cpp
group.finish_task(
    vix::threadpool::TaskStatus::cancelled,
    vix::threadpool::TaskResult::cancelled);
// increments: cancelled_tasks
```

### Timed out task

```cpp
group.finish_task(
    vix::threadpool::TaskStatus::timed_out,
    vix::threadpool::TaskResult::timeout);
// increments: timed_out_tasks
```

### Rejected task

```cpp
group.finish_task(
    vix::threadpool::TaskStatus::rejected,
    vix::threadpool::TaskResult::rejected);
// increments: rejected_tasks
```

## Checking group result

Use `has_failure()` when you only care about failed tasks:

```cpp
if (group.has_failure())
{
  // At least one task failed.
}
```

Use `has_error()` when you care about any non-success outcome (failed, cancelled, timed out, or rejected):

```cpp
if (group.has_error())
{
  // At least one task did not succeed.
}
```

## Task ids

A group stores all registered task ids:

```cpp
std::vector<vix::threadpool::TaskId> ids = group.task_ids();
```

This is useful for diagnostics, logs, tracing, testing, and runtime integrations.

## First exception

If a task reports an exception, the group stores the first one:

```cpp
std::exception_ptr exception = group.first_exception();

if (exception)
{
  std::rethrow_exception(exception);
}
```

Most code should use `group.wait_and_rethrow()` instead.

## Thread safety

`TaskGroup` is thread-safe. Multiple threads may call `finish_task()` while another thread calls `wait()`. The group uses an internal mutex and condition variable to protect state and wake waiters.

## TaskGroup vs Scope

Use `TaskGroup` when you need manual coordination:

```cpp
vix::threadpool::TaskGroup group;

group.add_task(id);
group.finish_task(status, result);
group.wait();
```

Use `Scope` when you want structured concurrency with automatic future tracking:

```cpp
vix::threadpool::Scope scope(pool);

scope.spawn(fn);
scope.spawn(fn);
scope.wait();
```

For most application code, `Scope` is simpler. For lower-level runtime integrations, `TaskGroup` is more explicit.

## Common pattern

```cpp
vix::threadpool::TaskGroup group;

for (int i = 0; i < 10; ++i)
{
  const auto id = static_cast<vix::threadpool::TaskId>(i + 1);

  group.add_task(id);

  pool.post(
      [&group]()
      {
        try
        {
          // Work

          group.finish_task(
              vix::threadpool::TaskStatus::completed,
              vix::threadpool::TaskResult::success);
        }
        catch (...)
        {
          group.finish_task(
              vix::threadpool::TaskStatus::failed,
              vix::threadpool::TaskResult::failure,
              std::current_exception());
        }
      });
}

group.close();
group.wait_and_rethrow();
```

This pattern provides: explicit registration, completion tracking, exception capture, a final wait, and aggregate result counters.

## Best practices

- Register every task before it can finish: `group.add_task(id);`
- Always call `finish_task()` exactly once for every registered task
- Close the group when no more tasks will be added: `group.close();`
- Use `wait()` when you only need completion
- Use `wait_and_rethrow()` when exceptions matter
- Use `cancel()` to request cooperative cancellation for the whole group
- Do not use `TaskGroup` to forcibly stop running tasks

## Simple mental model

```txt
TaskGroup
  stores task ids
  counts pending tasks
  records final outcomes
  stores first exception
  exposes shared cancellation
  wakes waiters when pending reaches zero
```

A task group is not the executor. It does not run tasks by itself. It coordinates tasks that are running somewhere else, usually inside a `ThreadPool`.
