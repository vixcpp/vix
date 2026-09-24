# Quick Start

This page shows the fastest way to use `vix::threadpool`.

The recommended include is:

```cpp
#include <vix/threadpool/threadpool.hpp>
```

## Create a pool

```cpp
#include <vix/threadpool/threadpool.hpp>

int main()
{
  vix::threadpool::ThreadPool pool(4);

  pool.shutdown();

  return 0;
}
```

This creates a thread pool with 4 worker threads. The pool starts automatically when constructed.

## Run a fire-and-forget task

Use `post()` when you want to run work in the background and you do not need a return value.

```cpp
#include <atomic>
#include <iostream>
#include <vix/threadpool.hpp>

int main()
{
  vix::threadpool::ThreadPool pool(4);

  std::atomic<int> counter{0};

  pool.post(
      [&counter]()
      {
        counter.fetch_add(1, std::memory_order_relaxed);
      });

  pool.wait_idle();

  std::cout << "counter: " << counter.load() << '\n';

  pool.shutdown();

  return 0;
}
```

Output:

```txt
counter: 1
```

## Submit a task and get a result

Use `submit()` when the task returns a value.

```cpp
#include <iostream>
#include <vix/threadpool.hpp>

int main()
{
  vix::threadpool::ThreadPool pool(4);

  auto future =
      pool.submit(
          []()
          {
            return 42;
          });

  std::cout << "result: " << future.get() << '\n';

  pool.shutdown();

  return 0;
}
```

Output:

```txt
result: 42
```

## Submit a void task

`submit()` also works with void tasks.

```cpp
#include <atomic>
#include <iostream>
#include <vix/threadpool.hpp>

int main()
{
  vix::threadpool::ThreadPool pool(4);

  std::atomic<int> counter{0};

  auto future =
      pool.submit(
          [&counter]()
          {
            counter.fetch_add(1, std::memory_order_relaxed);
          });

  future.get();

  std::cout << "counter: " << counter.load() << '\n';

  pool.shutdown();

  return 0;
}
```

## Use task options

`TaskOptions` lets you configure how a task is submitted.

```cpp
vix::threadpool::TaskOptions options;

options.set_priority(vix::threadpool::TaskPriority::high);
options.set_timeout(vix::threadpool::Timeout::milliseconds(100));

auto future =
    pool.submit(
        []()
        {
          return 42;
        },
        options);
```

## Use task priority

```cpp
#include <iostream>
#include <vix/threadpool.hpp>

int main()
{
  vix::threadpool::ThreadPool pool(2);

  vix::threadpool::TaskOptions options;
  options.set_priority(vix::threadpool::TaskPriority::high);

  pool.post(
      []()
      {
        std::cout << "high priority task\n";
      },
      options);

  pool.wait_idle();
  pool.shutdown();

  return 0;
}
```

Priorities are used by the task queue to decide which queued task should run first.

## Use timeout observation

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
    std::cout << "result: " << future.get() << '\n';
  }
  catch (const std::exception &e)
  {
    std::cout << "task failed: " << e.what() << '\n';
  }

  std::cout << "status: "
            << vix::threadpool::to_string(future.status())
            << '\n';

  std::cout << "result state: "
            << vix::threadpool::to_string(future.result())
            << '\n';

  pool.shutdown();

  return 0;
}
```

Timeouts are observational. Vix does not forcibly stop running C++ code. If a task runs longer than the timeout, the task can be marked as timed out after execution.

## Use cancellation

Use `handle()` when you want a cancellable task.

```cpp
#include <chrono>
#include <iostream>
#include <thread>
#include <vix/threadpool.hpp>

int main()
{
  vix::threadpool::ThreadPool pool(1);

  auto handle =
      pool.handle(
          []()
          {
            std::this_thread::sleep_for(std::chrono::milliseconds{100});
            return 42;
          });

  handle.cancel();

  try
  {
    const int value = handle.get();
    std::cout << "value: " << value << '\n';
  }
  catch (const std::exception &e)
  {
    std::cout << "task failed: " << e.what() << '\n';
  }

  std::cout << "cancelled: "
            << (handle.cancelled() ? "yes" : "no")
            << '\n';

  pool.shutdown();

  return 0;
}
```

Cancellation is cooperative. If cancellation is requested before the task starts, the task can be skipped. Running C++ code is not forcefully interrupted.

## Run a parallel for loop

```cpp
#include <cstddef>
#include <iostream>
#include <vector>
#include <vix/threadpool.hpp>

int main()
{
  vix::threadpool::ThreadPool pool(4);

  std::vector<int> values(10, 0);

  vix::threadpool::parallel_for(
      pool,
      std::size_t{0},
      values.size(),
      [&values](std::size_t index)
      {
        values[index] = static_cast<int>(index * index);
      });

  for (const int value : values)
  {
    std::cout << value << ' ';
  }

  std::cout << '\n';

  pool.shutdown();

  return 0;
}
```

## Run a parallel for-each

```cpp
#include <iostream>
#include <vector>
#include <vix/threadpool.hpp>

int main()
{
  vix::threadpool::ThreadPool pool(4);

  std::vector<int> values{1, 2, 3, 4};

  vix::threadpool::parallel_for_each(
      pool,
      values,
      [](int &value)
      {
        value *= 2;
      });

  for (const int value : values)
  {
    std::cout << value << ' ';
  }

  std::cout << '\n';

  pool.shutdown();

  return 0;
}
```

## Run a parallel map

```cpp
#include <iostream>
#include <vector>
#include <vix/threadpool.hpp>

int main()
{
  vix::threadpool::ThreadPool pool(4);

  std::vector<int> values{1, 2, 3, 4};

  std::vector<int> squares =
      vix::threadpool::parallel_map(
          pool,
          values,
          [](int value)
          {
            return value * value;
          });

  for (const int value : squares)
  {
    std::cout << value << ' ';
  }

  std::cout << '\n';

  pool.shutdown();

  return 0;
}
```

## Run a parallel reduce

```cpp
#include <iostream>
#include <vector>
#include <vix/threadpool.hpp>

int main()
{
  vix::threadpool::ThreadPool pool(4);

  std::vector<int> values{1, 2, 3, 4};

  const int sum =
      vix::threadpool::parallel_reduce(
          pool,
          values,
          0,
          [](int current, int value)
          {
            return current + value;
          });

  std::cout << "sum: " << sum << '\n';

  pool.shutdown();

  return 0;
}
```

Output:

```txt
sum: 10
```

## Use a scope

`Scope` groups tasks and waits for them safely.

```cpp
#include <atomic>
#include <iostream>
#include <vix/threadpool.hpp>

int main()
{
  vix::threadpool::ThreadPool pool(4);

  std::atomic<int> counter{0};

  {
    vix::threadpool::Scope scope(pool);

    scope.spawn(
        [&counter]()
        {
          counter.fetch_add(1, std::memory_order_relaxed);
        });

    scope.spawn(
        [&counter]()
        {
          counter.fetch_add(1, std::memory_order_relaxed);
        });

    scope.wait();
  }

  std::cout << "counter: " << counter.load() << '\n';

  pool.shutdown();

  return 0;
}
```

Output:

```txt
counter: 2
```

## Use a periodic task

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
        const int current =
            ticks.fetch_add(1, std::memory_order_relaxed) + 1;

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

## Read metrics

```cpp
#include <iostream>
#include <vix/threadpool.hpp>

int main()
{
  vix::threadpool::ThreadPool pool(4);

  pool.post([]() {});

  pool.wait_idle();

  const auto metrics = pool.metrics();

  std::cout << "workers: " << metrics.worker_count << '\n';
  std::cout << "pending: " << metrics.pending_tasks << '\n';
  std::cout << "completed: " << metrics.completed_tasks << '\n';
  std::cout << "rejected: " << metrics.rejected_tasks << '\n';

  pool.shutdown();

  return 0;
}
```

## Shutdown

Always wait or shut down when your work is done.

```cpp
pool.wait_idle();
pool.shutdown();
```

`shutdown()` is safe to call multiple times. The destructor also shuts down the pool, but explicit shutdown is clearer in examples, services, and tests.

## Best first example

Start with this:

```cpp
#include <iostream>
#include <vix/threadpool.hpp>

int main()
{
  vix::threadpool::ThreadPool pool(4);

  auto future =
      pool.submit(
          []()
          {
            return 42;
          });

  std::cout << future.get() << '\n';

  pool.shutdown();

  return 0;
}
```

Then move to `vix::threadpool::parallel_for(...)` when you need to process many items concurrently.
