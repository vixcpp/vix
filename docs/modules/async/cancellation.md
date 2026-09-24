# Cancellation

This guide shows how cancellation works in Vix Async.

Use this page when you want to stop long-running async operations, cancel timers, or pass cancellation state through your coroutine code.

## Public header

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>
```

## What cancellation provides

Vix Async uses cooperative cancellation.

It provides:

- `cancel_source` to request cancellation
- `cancel_token` to observe cancellation
- `request_cancel()` to signal cancellation
- `is_cancelled()` to check cancellation state
- `cancelled_ec()` to create the standard cancellation error code

Cancellation is cooperative. That means an operation must receive a token and check it.

## Basic cancellation

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

int main()
{
  vix::async::core::cancel_source source;
  vix::async::core::cancel_token token = source.token();

  vix::print("can cancel =", token.can_cancel() ? "yes" : "no");
  vix::print("cancelled =", token.is_cancelled() ? "yes" : "no");

  source.request_cancel();

  vix::print("cancelled =", token.is_cancelled() ? "yes" : "no");

  return 0;
}
```

Run:

```
vix run main.cpp
```

Expected output:

```
can cancel = yes
cancelled = no
cancelled = yes
```

## cancel_source

`cancel_source` owns the cancellation state.

Use it when your code needs to request cancellation.

```cpp
vix::async::core::cancel_source source;
```

Request cancellation:

```cpp
source.request_cancel();
```

Check whether cancellation was requested:

```cpp
if (source.is_cancelled())
{
  vix::print("cancelled");
}
```

## cancel_token

`cancel_token` observes cancellation state.

Use it when an async operation should be cancellable.

```cpp
vix::async::core::cancel_token token = source.token();
```

Check if the token is connected to a source:

```cpp
if (token.can_cancel())
{
  vix::print("token is cancellable");
}
```

Check if cancellation was requested:

```cpp
if (token.is_cancelled())
{
  vix::print("cancel requested");
}
```

## Source and token

The source requests cancellation. The token observes it.

```cpp
vix::async::core::cancel_source source;
auto token = source.token();

source.request_cancel();

if (token.is_cancelled())
{
  vix::print("operation should stop");
}
```

## Cancel a timer sleep

Timer sleeps accept a cancellation token.

```cpp
co_await ctx.timers().sleep_for(1s, source.token());
```

If cancellation is requested before the timer resumes, `sleep_for` throws a `std::system_error`.

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

#include <chrono>
#include <system_error>

using namespace std::chrono_literals;

vix::async::core::task<void> sleeper(
    vix::async::core::io_context &ctx,
    vix::async::core::cancel_token token)
{
  try
  {
    co_await ctx.timers().sleep_for(1s, token);

    vix::print("sleep completed");
  }
  catch (const std::system_error &ex)
  {
    vix::eprint("sleep cancelled:", ex.code().message());
  }

  ctx.stop();
  co_return;
}

int main()
{
  vix::async::core::io_context ctx;
  vix::async::core::cancel_source source;

  auto t = sleeper(ctx, source.token());
  std::move(t).start(ctx.get_scheduler());

  ctx.timers().after(100ms, [&source]()
  {
    source.request_cancel();
  });

  ctx.run();

  return 0;
}
```

Expected output:

```
sleep cancelled: canceled
```

## Cancel a timer callback

`after` also accepts a cancellation token.

```cpp
ctx.timers().after(1s, []()
{
  vix::print("this should not run");
}, source.token());
```

Complete example:

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

#include <chrono>

using namespace std::chrono_literals;

int main()
{
  vix::async::core::io_context ctx;
  vix::async::core::cancel_source source;

  ctx.timers().after(1s, []()
  {
    vix::print("callback fired");
  }, source.token());

  ctx.timers().after(100ms, [&ctx, &source]()
  {
    source.request_cancel();

    vix::print("callback cancelled");

    ctx.stop();
  });

  ctx.run();

  return 0;
}
```

Expected output:

```
callback cancelled
```

## Cancellation error code

Use `cancelled_ec()` when you need the standard async cancellation error code.

```cpp
const std::error_code ec = vix::async::core::cancelled_ec();

vix::print("category =", ec.category().name());
vix::print("message =", ec.message());
```

Example:

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

int main()
{
  const std::error_code ec = vix::async::core::cancelled_ec();

  vix::print("category =", ec.category().name());
  vix::print("message =", ec.message());

  return 0;
}
```

Expected output:

```
category = async
message = canceled
```

## Manual cancellation checks

For your own long-running coroutine, check the token regularly.

```cpp
if (token.is_cancelled())
{
  co_return;
}
```

Example:

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

#include <chrono>

using namespace std::chrono_literals;

vix::async::core::task<void> worker(
    vix::async::core::io_context &ctx,
    vix::async::core::cancel_token token)
{
  for (int i = 0; i < 10; ++i)
  {
    if (token.is_cancelled())
    {
      vix::print("worker cancelled");
      ctx.stop();
      co_return;
    }

    vix::print("step =", i);

    co_await ctx.timers().sleep_for(50ms);
  }

  vix::print("worker completed");

  ctx.stop();
  co_return;
}

int main()
{
  vix::async::core::io_context ctx;
  vix::async::core::cancel_source source;

  auto t = worker(ctx, source.token());
  std::move(t).start(ctx.get_scheduler());

  ctx.timers().after(130ms, [&source]()
  {
    source.request_cancel();
  });

  ctx.run();

  return 0;
}
```

Example output:

```
step = 0
step = 1
step = 2
worker cancelled
```

## Cancellation with thread pool work

`thread_pool::submit` accepts a cancellation token.

```cpp
auto value = co_await ctx.cpu_pool().submit([]()
{
  return 42;
}, source.token());
```

If the token is already cancelled before execution starts, the operation throws a cancellation error.

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

## Cancellation with networking

Networking APIs also accept `cancel_token`.

```cpp
co_await stream->async_connect(endpoint, source.token());
auto bytes = co_await stream->async_read(buffer, source.token());
auto bytes = co_await socket->async_send_to(
    buffer,
    endpoint,
    source.token());
```

Use cancellation for long-running network operations.

## Empty token

A default `cancel_token` is valid but not connected to a source.

```cpp
vix::async::core::cancel_token token;

vix::print("can cancel =", token.can_cancel() ? "yes" : "no");
vix::print("cancelled =", token.is_cancelled() ? "yes" : "no");
```

Expected output:

```
can cancel = no
cancelled = no
```

This is useful for APIs where cancellation is optional.

## Cancellation flow

A normal cancellation flow looks like this:

```
create cancel_source
pass source.token() to async operation
later call source.request_cancel()
operation observes token
operation exits or throws cancellation error
```

In code:

```cpp
vix::async::core::cancel_source source;

auto token = source.token();

source.request_cancel();

if (token.is_cancelled())
{
  vix::print("cancelled");
}
```

## Common workflows

### Create a cancellable operation

```cpp
vix::async::core::cancel_source source;

auto t = worker(ctx, source.token());
std::move(t).start(ctx.get_scheduler());
```

### Request cancellation later

```cpp
ctx.timers().after(std::chrono::milliseconds(100), [&source]()
{
  source.request_cancel();
});
```

### Check cancellation manually

```cpp
if (token.is_cancelled())
{
  co_return;
}
```

### Catch cancellation errors

```cpp
try
{
  co_await ctx.timers().sleep_for(1s, token);
}
catch (const std::system_error &ex)
{
  vix::eprint(ex.code().message());
}
```

### Use optional cancellation

```cpp
vix::async::core::cancel_token token{};

co_await ctx.timers().sleep_for(100ms, token);
```

## Common mistakes

### Creating a source but not passing the token

Wrong:

```cpp
vix::async::core::cancel_source source;

co_await ctx.timers().sleep_for(1s);

source.request_cancel();
```

Correct:

```cpp
vix::async::core::cancel_source source;

co_await ctx.timers().sleep_for(1s, source.token());
```

### Expecting cancellation to kill a running function

Cancellation is cooperative. A function must observe the token.

```cpp
if (token.is_cancelled())
{
  co_return;
}
```

### Forgetting to catch cancellation errors

Some async operations throw `std::system_error` when cancellation is observed.

```cpp
try
{
  co_await ctx.timers().sleep_for(1s, token);
}
catch (const std::system_error &ex)
{
  vix::eprint(ex.code().message());
}
```

### Requesting cancellation too late

If an operation already completed, cancellation has no effect on that operation. Only pending or future token checks can observe it.

### Reusing a cancelled source unintentionally

Once a source is cancelled, its tokens stay cancelled.

```cpp
source.request_cancel();

if (source.token().is_cancelled())
{
  vix::print("still cancelled");
}
```

Create a new `cancel_source` for a new independent operation.

## Best practices

Create one `cancel_source` per cancellable operation group. Pass `cancel_token` to long-running operations. Check `token.is_cancelled()` in custom loops. Handle `std::system_error` for cancellable async APIs. Use `cancelled_ec()` when creating your own cancellation errors. Do not treat cancellation as forced thread termination. Keep cancellation cooperative and explicit.

## Related pages

| Page                                          | Purpose                            |
| --------------------------------------------- | ---------------------------------- |
| [io_context](/modules/async/io-context)       | Learn the runtime context.         |
| [Tasks](/modules/async/tasks)                 | Learn coroutine tasks.             |
| [Timers](/modules/async/timers)               | Learn cancellable timer sleeps.    |
| [Thread pool](/modules/async/thread-pool)     | Learn cancellable background work. |
| [TCP](/modules/async/tcp)                     | Learn cancellable TCP operations.  |
| [UDP](/modules/async/udp)                     | Learn cancellable UDP operations.  |
| [API Reference](/modules/async/api-reference) | See the public API surface.        |

## Next step

Continue with the [thread pool](/modules/async/thread-pool).
