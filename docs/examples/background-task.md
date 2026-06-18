# Background Task

This example shows how to run a small background worker with Vix Async.

It uses:

```txt id="d40b7e"
vix::async::core::io_context
vix::async::core::task<void>
timers
CPU pool
cooperative cancellation
signal handling
clean shutdown
```

Use this example when you want to build:

```txt id="xsyl6p"
periodic jobs
queue workers
cleanup jobs
email workers
cache refreshers
sync workers
metrics collectors
background maintenance tasks
```

## What this example builds

The program starts a background task that runs every second.

Each tick simulates a small job.

The program also listens for `SIGINT`.

Press:

```txt id="tmfa94"
Ctrl+C
```

and the worker stops cleanly.

The flow is:

```txt id="io0qr5"
create io_context
create cancellation source
start background worker
start signal watcher
run io_context
press Ctrl+C
request cancellation
worker exits
context stops
program exits
```

## Project structure

Create:

```txt id="set9cb"
background_task_demo/
└── background_task.cpp
```

Create the project directory:

```bash id="dv3seh"
mkdir background_task_demo
cd background_task_demo
touch background_task.cpp
```

## Source

Open:

```txt id="s6rn31"
background_task.cpp
```

Add:

```cpp id="kfnrm1"
#include <chrono>
#include <csignal>
#include <exception>
#include <string>
#include <system_error>

#include <vix/async.hpp>
#include <vix/print.hpp>

using namespace std::chrono_literals;

static int compute_job_result(int tick)
{
  return tick * 10;
}

static vix::async::core::task<void> run_background_worker(
  vix::async::core::io_context &ctx,
  vix::async::core::cancel_token token)
{
  int tick = 0;

  while (!token.is_cancelled())
  {
    ++tick;

    vix::print("worker tick =", tick);

    try
    {
      const int result = co_await ctx.cpu_pool().submit([tick]()
      {
        return compute_job_result(tick);
      }, token);

      vix::print("job result =", result);
    }
    catch (const std::system_error &ex)
    {
      if (token.is_cancelled())
      {
        vix::print("worker cancellation observed");
        break;
      }

      vix::eprint("worker system error:", ex.code().message());
    }
    catch (const std::exception &ex)
    {
      vix::eprint("worker error:", ex.what());
    }

    try
    {
      co_await ctx.timers().sleep_for(1s, token);
    }
    catch (const std::system_error &ex)
    {
      if (token.is_cancelled())
      {
        vix::print("worker timer cancelled");
        break;
      }

      vix::eprint("timer error:", ex.code().message());
    }
  }

  vix::print("background worker stopped");

  ctx.stop();
  co_return;
}

static vix::async::core::task<void> stop_on_signal(
  vix::async::core::io_context &ctx,
  vix::async::core::cancel_source &source)
{
  ctx.signals().add(SIGINT);
  ctx.signals().add(SIGTERM);

  vix::print("background worker running");
  vix::print("press Ctrl+C to stop");

  const int sig = co_await ctx.signals().async_wait();

  vix::print("shutdown signal received =", sig);

  source.request_cancel();

  co_return;
}

int main()
{
  vix::async::core::io_context ctx;
  vix::async::core::cancel_source source;

  auto worker = run_background_worker(ctx, source.token());
  auto signal = stop_on_signal(ctx, source);

  std::move(worker).start(ctx.get_scheduler());
  std::move(signal).start(ctx.get_scheduler());

  ctx.run();
  ctx.shutdown();

  vix::print("program stopped");

  return 0;
}
```

## Run it

Run:

```bash id="xktnit"
vix run background_task.cpp
```

Expected output shape:

```txt id="y186au"
background worker running
press Ctrl+C to stop
worker tick = 1
job result = 10
worker tick = 2
job result = 20
worker tick = 3
job result = 30
```

Press:

```txt id="hwqsbi"
Ctrl+C
```

Expected shutdown output shape:

```txt id="zdtvis"
shutdown signal received = 2
worker timer cancelled
background worker stopped
program stopped
```

The exact signal number can depend on the platform.

On many systems:

```txt id="nh0smu"
SIGINT = 2
SIGTERM = 15
```

## How it works

The program creates one async runtime:

```cpp id="bjup02"
vix::async::core::io_context ctx;
```

The runtime owns the scheduler and async services.

The worker is started with:

```cpp id="grfcm6"
auto worker = run_background_worker(ctx, source.token());

std::move(worker).start(ctx.get_scheduler());
```

The signal watcher is started the same way:

```cpp id="dh6oxr"
auto signal = stop_on_signal(ctx, source);

std::move(signal).start(ctx.get_scheduler());
```

Then the scheduler loop starts:

```cpp id="k803fq"
ctx.run();
```

The program keeps running until something calls:

```cpp id="dsvtv8"
ctx.stop();
```

## Why the task receives a cancellation token

The worker receives:

```cpp id="v271f7"
vix::async::core::cancel_token token
```

It checks the token in the loop:

```cpp id="o52vup"
while (!token.is_cancelled())
{
  // work
}
```

The signal task owns the cancellation source:

```cpp id="d8x1m7"
vix::async::core::cancel_source source;
```

When a signal is received, it requests cancellation:

```cpp id="f1u8uc"
source.request_cancel();
```

The worker observes this through the token.

That is the important model:

```txt id="yb8lyq"
cancel_source
  requests cancellation

cancel_token
  observes cancellation
```

## Why the worker uses timers

The worker waits between ticks with:

```cpp id="mi3kv2"
co_await ctx.timers().sleep_for(1s, token);
```

This is not the same as:

```cpp id="ek4gx5"
std::this_thread::sleep_for(...)
```

A timer suspends the coroutine without blocking the scheduler thread.

That means the async runtime can keep processing other work while the worker waits.

## Why the worker uses the CPU pool

The worker simulates a job with:

```cpp id="m2cvb6"
const int result = co_await ctx.cpu_pool().submit([tick]()
{
  return compute_job_result(tick);
}, token);
```

Use the CPU pool when the work is:

```txt id="pk4k5b"
CPU-heavy
blocking
synchronous
slow enough to hurt the scheduler thread
```

The scheduler should stay responsive.

Avoid this inside a coroutine:

```cpp id="fhgnxx"
const int result = compute_job_result(tick);
```

when the real function is expensive or blocking.

Prefer:

```cpp id="aeu75s"
const int result = co_await ctx.cpu_pool().submit([]()
{
  return expensive_work();
});
```

## Why signal handling is a separate task

The signal watcher is its own coroutine:

```cpp id="u20cpy"
static vix::async::core::task<void> stop_on_signal(
  vix::async::core::io_context &ctx,
  vix::async::core::cancel_source &source)
```

It registers signals:

```cpp id="xm6b5k"
ctx.signals().add(SIGINT);
ctx.signals().add(SIGTERM);
```

Then waits asynchronously:

```cpp id="fg8ig7"
const int sig = co_await ctx.signals().async_wait();
```

When a signal arrives, it requests cancellation:

```cpp id="movx69"
source.request_cancel();
```

This keeps shutdown logic clean.

The worker does the work.

The signal task only waits for shutdown.

## Why the worker calls ctx.stop()

The signal task requests cancellation.

The worker exits.

Then the worker calls:

```cpp id="ps88mw"
ctx.stop();
```

This makes the runtime exit once the main background work is complete.

The final shutdown path is:

```txt id="k8aqj1"
signal received
  -> request cancellation
  -> worker observes cancellation
  -> worker exits loop
  -> worker calls ctx.stop()
  -> ctx.run() returns
  -> ctx.shutdown()
```

## Add a second background task

You can start more than one worker.

Example:

```cpp id="b3qlb2"
auto worker_a = run_background_worker(ctx, source.token());
auto worker_b = run_background_worker(ctx, source.token());

std::move(worker_a).start(ctx.get_scheduler());
std::move(worker_b).start(ctx.get_scheduler());
```

Both workers use the same cancellation token.

When the source requests cancellation, both workers can stop.

For real applications, make sure only one task owns the final `ctx.stop()` decision, or use a coordinator task.

## Use a timer-only worker

For lightweight work, you may not need the CPU pool.

Example:

```cpp id="n38e9z"
static vix::async::core::task<void> heartbeat(
  vix::async::core::io_context &ctx,
  vix::async::core::cancel_token token)
{
  while (!token.is_cancelled())
  {
    vix::print("heartbeat");

    try
    {
      co_await ctx.timers().sleep_for(1s, token);
    }
    catch (const std::system_error &)
    {
      break;
    }
  }

  ctx.stop();
  co_return;
}
```

Use this shape for simple periodic logs, status checks, or metrics collection.

## Use after for a one-shot background callback

For simple delayed callback work, use `after`.

```cpp id="ic83hf"
#include <chrono>

#include <vix/async.hpp>
#include <vix/print.hpp>

using namespace std::chrono_literals;

int main()
{
  vix::async::core::io_context ctx;

  ctx.timers().after(500ms, [&ctx]()
  {
    vix::print("delayed callback");
    ctx.stop();
  });

  ctx.run();

  return 0;
}
```

Use `after` when the work is callback-based.

Use `sleep_for` when the work is inside a coroutine.

## Use post for immediate background work

For simple immediate work, use `ctx.post`.

```cpp id="ehca4r"
#include <vix/async.hpp>
#include <vix/print.hpp>

int main()
{
  vix::async::core::io_context ctx;

  ctx.post([&ctx]()
  {
    vix::print("posted background callback");
    ctx.stop();
  });

  ctx.run();

  return 0;
}
```

This is useful for simple callbacks that do not need `co_await`.

## Background task in an HTTP application

For a standalone worker, `io_context` is enough.

For an HTTP application, a common shape is:

```txt id="r4ql17"
vix::App
  handles HTTP

background worker
  runs periodic work

shutdown signal
  cancels background worker
```

Keep the responsibilities separate:

```txt id="tm0nn2"
HTTP routes
  request and response logic

background task
  scheduled work

cancellation source
  shutdown coordination
```

A background task should not block HTTP request handling.

Use the async timer or the CPU pool when needed.

## Common mistakes

### Forgetting to run the context

Wrong:

```cpp id="jw3iin"
vix::async::core::io_context ctx;

ctx.post([]()
{
  vix::print("hello");
});
```

Correct:

```cpp id="fbqkzd"
vix::async::core::io_context ctx;

ctx.post([&ctx]()
{
  vix::print("hello");
  ctx.stop();
});

ctx.run();
```

### Creating a task but not starting it

Wrong:

```cpp id="trx98f"
auto worker = run_background_worker(ctx, source.token());

ctx.run();
```

Correct:

```cpp id="mdh9vj"
auto worker = run_background_worker(ctx, source.token());

std::move(worker).start(ctx.get_scheduler());

ctx.run();
```

Tasks are lazy.

They must be started or awaited.

### Using blocking sleep

Avoid:

```cpp id="hcs6mi"
std::this_thread::sleep_for(std::chrono::seconds(1));
```

Prefer:

```cpp id="o31ep1"
co_await ctx.timers().sleep_for(1s);
```

The timer version does not block the scheduler thread.

### Ignoring cancellation

Avoid:

```cpp id="nb4aey"
while (true)
{
  co_await ctx.timers().sleep_for(1s);
}
```

Prefer:

```cpp id="q4bkup"
while (!token.is_cancelled())
{
  co_await ctx.timers().sleep_for(1s, token);
}
```

### Blocking inside the scheduler

Avoid expensive synchronous work directly inside the coroutine:

```cpp id="v5sycg"
expensive_work();
```

Prefer:

```cpp id="h5j1fe"
co_await ctx.cpu_pool().submit([]()
{
  return expensive_work();
});
```

### Calling ctx.stop too early

If the signal task calls `ctx.stop()` immediately, the worker may not finish cleanup.

Prefer this flow:

```txt id="uz66kp"
signal task
  requests cancellation

worker task
  exits cleanly
  stops context
```

## Best practices

Use one `io_context` as the owner of async services.

Use `task<void>` for long-running coroutine workers.

Use `sleep_for` for periodic delays.

Use `cancel_source` and `cancel_token` for shutdown.

Use `signals()` to react to Ctrl+C and SIGTERM.

Use `cpu_pool()` for blocking or CPU-heavy work.

Keep the scheduler thread responsive.

Keep background loops cancellable.

Call `ctx.shutdown()` after `ctx.run()` returns.

## Production notes

For production workers, add:

```txt id="hptcg5"
structured logs
error counters
retry policy
backoff
health status
metrics
job timeouts
clear shutdown policy
tests
```

For jobs that touch external systems, also add:

```txt id="l55y6h"
idempotency
deduplication
transaction boundaries
dead-letter handling
persistent queue state
```

A background task is simple when it only prints messages.

It becomes production-grade when it can survive retries, errors, cancellation, and process restarts.

## Complete test flow

Run:

```bash id="c8dsyh"
vix run background_task.cpp
```

Wait for a few ticks:

```txt id="pv8xtr"
worker tick = 1
job result = 10
worker tick = 2
job result = 20
```

Press:

```txt id="ddqf75"
Ctrl+C
```

Expected shape:

```txt id="svze4s"
shutdown signal received = 2
worker timer cancelled
background worker stopped
program stopped
```

## Summary

A background worker in Vix Async follows this shape:

```txt id="f8czil"
create io_context
create cancel_source
start worker task
start signal task
run context
request cancellation on signal
worker stops
context stops
shutdown context
```

The core code is:

```cpp id="xn82k0"
vix::async::core::io_context ctx;
vix::async::core::cancel_source source;

auto worker = run_background_worker(ctx, source.token());
auto signal = stop_on_signal(ctx, source);

std::move(worker).start(ctx.get_scheduler());
std::move(signal).start(ctx.get_scheduler());

ctx.run();
ctx.shutdown();
```

Use this pattern for small workers, periodic jobs, and background maintenance tasks in Vix applications.
