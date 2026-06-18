# Thread pool

This guide shows how to use the Vix Async thread pool.

Use this page when you want to run CPU-heavy or blocking work outside the `io_context` scheduler thread.

## Public header

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>
```

## What the thread pool provides

The thread pool is owned by `io_context`.

```cpp
ctx.cpu_pool()
```

It is used to offload work that should not run directly on the scheduler thread.

Use it for:

- CPU-heavy functions
- blocking functions
- synchronous library calls
- work that should resume back into the async flow

## Basic example

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  int value = co_await ctx.cpu_pool().submit([]()
  {
    return 21 * 2;
  });

  vix::print("value =", value);

  ctx.stop();
  co_return;
}

int main()
{
  vix::async::core::io_context ctx;

  auto t = app(ctx);
  std::move(t).start(ctx.get_scheduler());

  ctx.run();

  return 0;
}
```

Run:

```
vix run main.cpp
```

Expected output:

```
value = 42
```

## Why use the thread pool?

The `io_context` scheduler should stay responsive.

Avoid doing heavy work directly inside `ctx.post(...)` or inside a coroutine before an await point.

Wrong:

```cpp
vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  int value = expensive_work();

  vix::print("value =", value);

  ctx.stop();
  co_return;
}
```

Better:

```cpp
vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  int value = co_await ctx.cpu_pool().submit([]()
  {
    return expensive_work();
  });

  vix::print("value =", value);

  ctx.stop();
  co_return;
}
```

## Submit work and await the result

Use `submit` inside a coroutine.

```cpp
int value = co_await ctx.cpu_pool().submit([]()
{
  return 42;
});
```

The callable runs on a worker thread. The coroutine resumes back through the owning `io_context`.

## Return a value

The callable can return a value.

```cpp
auto result = co_await ctx.cpu_pool().submit([]()
{
  return 42;
});
```

Complete example:

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

int compute_value()
{
  return 21 * 2;
}

vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  int value = co_await ctx.cpu_pool().submit([]()
  {
    return compute_value();
  });

  vix::print("computed =", value);

  ctx.stop();
  co_return;
}

int main()
{
  vix::async::core::io_context ctx;

  auto t = app(ctx);
  std::move(t).start(ctx.get_scheduler());

  ctx.run();

  return 0;
}
```

Expected output:

```
computed = 42
```

## Void work

The callable can also return void.

```cpp
co_await ctx.cpu_pool().submit([]()
{
  do_work();
});
```

Example:

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

void blocking_work()
{
  volatile int value = 0;

  for (int i = 0; i < 1000; ++i)
  {
    value += i;
  }
}

vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  co_await ctx.cpu_pool().submit([]()
  {
    blocking_work();
  });

  vix::print("work done");

  ctx.stop();
  co_return;
}

int main()
{
  vix::async::core::io_context ctx;

  auto t = app(ctx);
  std::move(t).start(ctx.get_scheduler());

  ctx.run();

  return 0;
}
```

Expected output:

```
work done
```

## Submit a plain callback

The thread pool also supports plain callback submission.

```cpp
ctx.cpu_pool().submit([]()
{
  // background callback
});
```

Use this when you do not need to await a result. For coroutine workflows, prefer the awaitable form:

```cpp
auto value = co_await ctx.cpu_pool().submit([]()
{
  return 42;
});
```

## Cancellation

`submit` accepts a `cancel_token`.

```cpp
auto value = co_await ctx.cpu_pool().submit([]()
{
  return 42;
}, source.token());
```

If the token is already cancelled before the work starts, the operation throws a cancellation error.

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

#include <system_error>

vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  vix::async::core::cancel_source source;

  source.request_cancel();

  try
  {
    int value = co_await ctx.cpu_pool().submit([]()
    {
      return 42;
    }, source.token());

    vix::print("value =", value);
  }
  catch (const std::system_error &ex)
  {
    vix::eprint("cancelled:", ex.code().message());
  }

  ctx.stop();
  co_return;
}

int main()
{
  vix::async::core::io_context ctx;

  auto t = app(ctx);
  std::move(t).start(ctx.get_scheduler());

  ctx.run();

  return 0;
}
```

Expected output:

```
cancelled: canceled
```

## Exceptions

If the callable throws, the exception is captured and rethrown when the coroutine resumes.

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

#include <exception>
#include <stdexcept>

vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  try
  {
    int value = co_await ctx.cpu_pool().submit([]()
    {
      throw std::runtime_error("worker failed");
      return 42;
    });

    vix::print("value =", value);
  }
  catch (const std::exception &ex)
  {
    vix::eprint(ex.what());
  }

  ctx.stop();
  co_return;
}

int main()
{
  vix::async::core::io_context ctx;

  auto t = app(ctx);
  std::move(t).start(ctx.get_scheduler());

  ctx.run();

  return 0;
}
```

Expected output:

```
worker failed
```

## Multiple jobs

You can await multiple pool jobs from one coroutine.

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  int a = co_await ctx.cpu_pool().submit([]()
  {
    return 20;
  });

  int b = co_await ctx.cpu_pool().submit([]()
  {
    return 22;
  });

  vix::print("sum =", a + b);

  ctx.stop();
  co_return;
}

int main()
{
  vix::async::core::io_context ctx;

  auto t = app(ctx);
  std::move(t).start(ctx.get_scheduler());

  ctx.run();

  return 0;
}
```

Expected output:

```
sum = 42
```

## Run background work then resume

A thread pool job runs away from the scheduler. After the job completes, the coroutine resumes on the `io_context` scheduler.

Typical flow:

```
coroutine starts on scheduler
co_await cpu_pool.submit(...)
callable runs on worker thread
result is stored
coroutine resumes on scheduler
```

This keeps async code simple while moving heavy work off the runtime thread.

## Thread pool lifecycle

The pool is created lazily.

```cpp
ctx.cpu_pool()
```

It is owned by `io_context`. When the context is shut down, the pool is released.

```cpp
ctx.shutdown();
```

You normally do not create the pool manually. Use:

```cpp
ctx.cpu_pool()
```

## Blocking work example

This example simulates blocking work.

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

#include <chrono>
#include <thread>

vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  vix::print("before blocking work");

  int value = co_await ctx.cpu_pool().submit([]()
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return 42;
  });

  vix::print("after blocking work");
  vix::print("value =", value);

  ctx.stop();
  co_return;
}

int main()
{
  vix::async::core::io_context ctx;

  auto t = app(ctx);
  std::move(t).start(ctx.get_scheduler());

  ctx.run();

  return 0;
}
```

Expected output:

```
before blocking work
after blocking work
value = 42
```

## Thread pool vs timers

Use timers for waiting.

```cpp
co_await ctx.timers().sleep_for(std::chrono::milliseconds(100));
```

Use the thread pool for work.

```cpp
auto value = co_await ctx.cpu_pool().submit([]()
{
  return compute_value();
});
```

Do not use the thread pool just to sleep.

## Thread pool vs post

Use `post` for small scheduler callbacks.

```cpp
ctx.post([&ctx]()
{
  vix::print("callback");
  ctx.stop();
});
```

Use the thread pool for work that should not block the scheduler.

```cpp
auto value = co_await ctx.cpu_pool().submit([]()
{
  return expensive_work();
});
```

## Common workflows

### Run CPU work

```cpp
int value = co_await ctx.cpu_pool().submit([]()
{
  return compute_value();
});
```

### Run blocking work

```cpp
auto result = co_await ctx.cpu_pool().submit([]()
{
  return blocking_call();
});
```

### Run void work

```cpp
co_await ctx.cpu_pool().submit([]()
{
  write_file();
});
```

### Use cancellation

```cpp
vix::async::core::cancel_source source;

auto value = co_await ctx.cpu_pool().submit([]()
{
  return 42;
}, source.token());
```

### Catch worker exceptions

```cpp
try
{
  auto value = co_await ctx.cpu_pool().submit([]()
  {
    return risky_work();
  });

  vix::print("value =", value);
}
catch (const std::exception &ex)
{
  vix::eprint(ex.what());
}
```

## Common mistakes

### Running heavy work on the scheduler

Wrong:

```cpp
vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  auto value = expensive_work();

  vix::print("value =", value);

  ctx.stop();
  co_return;
}
```

Correct:

```cpp
vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  auto value = co_await ctx.cpu_pool().submit([]()
  {
    return expensive_work();
  });

  vix::print("value =", value);

  ctx.stop();
  co_return;
}
```

### Forgetting to await submit

Wrong:

```cpp
ctx.cpu_pool().submit([]()
{
  return 42;
});
```

Correct:

```cpp
int value = co_await ctx.cpu_pool().submit([]()
{
  return 42;
});
```

Use plain `submit(std::function<void()>)` only when you intentionally want callback-style background work.

### Using the thread pool for timers

Wrong:

```cpp
co_await ctx.cpu_pool().submit([]()
{
  std::this_thread::sleep_for(std::chrono::seconds(1));
});
```

Correct:

```cpp
co_await ctx.timers().sleep_for(std::chrono::seconds(1));
```

### Forgetting to run the context

Wrong:

```cpp
auto t = app(ctx);
std::move(t).start(ctx.get_scheduler());
```

Correct:

```cpp
auto t = app(ctx);
std::move(t).start(ctx.get_scheduler());

ctx.run();
```

### Forgetting to stop the context

The main async task should stop the context when done.

```cpp
ctx.stop();
co_return;
```

### Expecting cancellation to stop a running function

Cancellation is checked before execution starts. If the callable is already running, it must cooperate manually.

```cpp
if (token.is_cancelled())
{
  return;
}
```

## Best practices

Use the thread pool for blocking or CPU-heavy work. Keep scheduler callbacks short. Prefer `co_await ctx.cpu_pool().submit(...)` inside coroutine tasks. Catch exceptions around awaited worker jobs. Use cancellation tokens when the work may become unnecessary. Use timers for delays, not the thread pool. Call `ctx.stop()` when the main async flow is complete.

## Related pages

| Page                                          | Purpose                           |
| --------------------------------------------- | --------------------------------- |
| [io_context](/modules/async/io-context)       | Learn the runtime context.        |
| [Tasks](/modules/async/tasks)                 | Learn coroutine tasks.            |
| [Spawn](/modules/async/spawn)                 | Learn how to start async work.    |
| [Timers](/modules/async/timers)               | Learn delays and timer callbacks. |
| [Cancellation](/modules/async/cancellation)   | Learn cancellation tokens.        |
| [when_all / when_any](/modules/async/when)    | Learn task composition.           |
| [API Reference](/modules/async/api-reference) | See the public API surface.       |

## Next step

Continue with [when_all and when_any](/modules/async/when).
