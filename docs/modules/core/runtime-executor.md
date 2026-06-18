# Runtime executor

This page explains `vix::executor::RuntimeExecutor`.

Use it when you want to understand how Vix Core connects application work to the internal `vix::runtime` scheduler, workers, tasks, and metrics.

## Public header

```cpp
#include <vix.hpp>
```

You can also include the executor header directly:

```cpp
#include <vix/executor/RuntimeExecutor.hpp>
```

## What RuntimeExecutor provides

`RuntimeExecutor` is the executor adapter used by Vix Core.

It sits between the HTTP application layer and the internal runtime.

```text
Vix Core
  -> RuntimeExecutor
  -> vix::runtime::Runtime
  -> Scheduler
  -> Workers
  -> Tasks
```

It provides:

- runtime startup
- runtime shutdown
- task submission
- HTTP fast-path submission
- executor metrics
- idle waiting
- rejected task tracking
- timeout tracking
- safe lifecycle management

Most applications do not create it manually.

They use:

```cpp
vix::App app;
```

The app creates and starts a default executor.

## Why RuntimeExecutor exists

`vix::runtime` exposes a low-level task model.

Core needs a higher-level executor interface that can be used by the HTTP server, sessions, router, and future application work.

So `RuntimeExecutor` adapts this:

```cpp
vix::runtime::TaskResult()
```

into this:

```cpp
executor.post([]()
{
  // application work
});
```

This keeps application-level code cleaner while still using the internal runtime engine.

## Basic model

The model is:

```text
RuntimeExecutor
  -> owns Runtime
  -> starts Runtime
  -> accepts work
  -> submits tasks
  -> tracks metrics
  -> stops Runtime
```

The runtime then handles:

```text
Runtime
  -> Scheduler
  -> Worker
  -> RunQueue
  -> Task
```

## Default executor in App

A default `vix::App` creates an executor automatically.

```cpp
vix::App app;
```

Internally, the app creates:

```text
RuntimeExecutor
  -> Runtime
  -> worker count based on hardware concurrency
```

Then it starts the executor before the server begins handling requests.

## Access the app executor

You can access the executor from an app.

```cpp
#include <vix.hpp>

int main()
{
  vix::App app;

  auto &executor = app.executor();

  (void)executor;

  app.run(8080);

  return 0;
}
```

Most applications do not need direct executor access.

It is mainly useful for:

- advanced scheduling
- metrics
- diagnostics
- integrations
- internal tooling
- custom runtime behavior

## Create an executor manually

Advanced code can create an executor explicitly.

```cpp
#include <vix/executor/RuntimeExecutor.hpp>

int main()
{
  vix::executor::RuntimeExecutor executor{4};

  executor.start();

  executor.post([]()
  {
    // work
  });

  executor.stop();

  return 0;
}
```

The integer constructor creates a runtime with the requested number of workers.

## Use an external executor with App

You can create an executor and pass it to `App`.

```cpp
#include <vix.hpp>

int main()
{
  auto executor = std::make_shared<vix::executor::RuntimeExecutor>(4);

  vix::App app{executor};

  app.get("/", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.text("hello");
  });

  app.run(8080);

  return 0;
}
```

This is useful when the application wants to control the executor configuration.

## Start the executor

Use `start()` to start the underlying runtime.

```cpp
executor.start();
```

`start()` is safe to call multiple times.

If the executor is already started, the call has no effect.

```text
start
  -> if already started, return
  -> runtime.start
  -> accepting = true
```

## Stop the executor

Use `stop()` to stop the runtime.

```cpp
executor.stop();
```

`stop()` is safe to call multiple times.

It stops accepting new work and stops the underlying runtime.

```text
stop
  -> accepting = false
  -> runtime.stop
  -> started = false
```

`Runtime::stop()` is blocking because it waits for runtime workers to stop.

## Stop and wait

Use `stop_and_wait()` when you want current work to drain before stopping.

```cpp
executor.stop_and_wait();
```

The flow is:

```text
stop accepting new work
  -> wait until pending and active tasks reach zero
  -> stop runtime
```

This is useful for graceful shutdown.

## Check started state

Use `started()`.

```cpp
if (executor.started())
{
  vix::print("executor started");
}
```

## Check running state

Use `running()`.

```cpp
if (executor.running())
{
  vix::print("runtime is running");
}
```

`running()` reflects the underlying runtime state.

## Check accepting state

Use `accepting()`.

```cpp
if (executor.accepting())
{
  executor.post([]()
  {
    // work
  });
}
```

If the executor is no longer accepting work, new submissions are rejected.

## Submit a runtime task function

You can submit a runtime task function directly.

```cpp
executor.submit([]() -> vix::runtime::TaskResult
{
  return vix::runtime::TaskResult::complete;
});
```

A runtime task function returns:

```cpp
vix::runtime::TaskResult::complete
vix::runtime::TaskResult::yield
vix::runtime::TaskResult::failed
```

Use this when you want direct access to the low-level runtime task contract.

## Submit a void task

Use `post(...)` for normal callable work.

```cpp
executor.post([]()
{
  vix::print("hello from runtime executor");
});
```

`post(...)` adapts a `void()` callable into a runtime task.

Conceptually:

```text
void function
  -> wrapper
  -> TaskResult::complete
  -> Runtime::submit
```

## Task options

`post(...)` accepts task options.

```cpp
vix::executor::TaskOptions opt;

opt.timeout = std::chrono::milliseconds(100);
opt.may_block = false;

executor.post([]()
{
  // work
}, opt);
```

`TaskOptions` contains:

| Field       | Purpose                                        |
| ----------- | ---------------------------------------------- |
| `priority`  | Priority value reserved for scheduling policy. |
| `timeout`   | Timeout measurement threshold.                 |
| `deadline`  | Deadline value reserved for scheduling policy. |
| `may_block` | Marks whether the task may block.              |

In the current model, timeout is used for metrics.

## Timeout metrics

When a task is posted with a timeout, the executor measures how long the callable took.

```cpp
vix::executor::TaskOptions opt;
opt.timeout = std::chrono::milliseconds(10);

executor.post([]()
{
  // work
}, opt);
```

If the task takes longer than the timeout, the timeout counter is incremented.

This does not automatically kill the running task.

It records that the task exceeded the configured timeout.

## HTTP fast path

`post_http_fast(...)` is a lighter submission path for short HTTP work.

```cpp
executor.post_http_fast([]()
{
  // short HTTP work
});
```

It is designed to reduce executor overhead for hot-path HTTP tasks.

Compared with `post(...)`, it avoids extra active-task tracking and timeout measurement.

Use it for short work that does not need generic executor accounting.

## HTTP fast path with TaskResult

```cpp
executor.post_http_fast([]() -> vix::runtime::TaskResult
{
  return vix::runtime::TaskResult::complete;
});
```

This is useful when the caller already uses the runtime task contract.

## Accepted and rejected work

When a task is submitted, the executor checks whether it is accepting work.

```text
if accepting
  -> submit to runtime
  -> increment submitted count

if not accepting
  -> reject
  -> increment rejected count
```

This prevents new tasks from entering the runtime during shutdown.

## Metrics

Use `metrics()` to read a snapshot.

```cpp
const auto m = executor.metrics();

vix::print("pending:", m.pending);
vix::print("active:", m.active);
vix::print("timed out:", m.timed_out);
```

Metrics contain:

| Field       | Meaning                                                       |
| ----------- | ------------------------------------------------------------- |
| `pending`   | Number of pending runtime tasks.                              |
| `active`    | Number of tasks currently active through tracked `post(...)`. |
| `timed_out` | Number of tasks that exceeded their timeout threshold.        |

## Submitted task count

Use `submitted_tasks()`.

```cpp
const auto submitted = executor.submitted_tasks();
```

This returns the number of tasks accepted by the executor.

## Rejected task count

Use `rejected_tasks()`.

```cpp
const auto rejected = executor.rejected_tasks();
```

This returns the number of rejected submissions.

A task can be rejected when:

- the executor is not accepting work
- the task function is invalid
- the runtime refuses the task

## Fast HTTP submitted count

Use `fast_http_submitted_tasks()`.

```cpp
const auto fast = executor.fast_http_submitted_tasks();
```

This returns the number of tasks accepted through `post_http_fast(...)`.

## Wait until idle

Use `wait_idle()` to block until no pending or active work remains.

```cpp
executor.wait_idle();
```

The loop checks:

```text
pending == 0
active == 0
```

This does not stop the executor.

It only waits for current tracked work to finish.

## Executor lifecycle

The normal lifecycle is:

```text
construct RuntimeExecutor
  -> start
  -> submit work
  -> wait_idle if needed
  -> stop
```

Example:

```cpp
vix::executor::RuntimeExecutor executor{4};

executor.start();

executor.post([]()
{
  vix::print("work");
});

executor.wait_idle();

executor.stop();
```

## App lifecycle with executor

When using `App`, the lifecycle is handled for you.

```text
App constructor
  -> create RuntimeExecutor
  -> start executor
  -> create HTTPServer

App close
  -> stop HTTP server
  -> stop executor through destructor or lifecycle
```

Application code:

```cpp
vix::App app;

app.get("/", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.text("hello");
});

app.run(8080);
```

## RuntimeExecutor and Runtime

`RuntimeExecutor` owns a `vix::runtime::Runtime`.

```text
RuntimeExecutor
  -> unique_ptr<Runtime>
```

The runtime owns the lower-level scheduling system.

```text
Runtime
  -> Scheduler
  -> Workers
  -> RunQueue
  -> Task
```

`RuntimeExecutor` provides a safer and simpler interface on top.

## RuntimeExecutor and HTTPServer

`HTTPServer` receives a shared executor.

```text
App
  -> RuntimeExecutor
  -> HTTPServer
```

The server passes the executor to sessions.

```text
HTTPServer
  -> Session
  -> Router
  -> handlers
```

This keeps Core connected to the runtime execution model.

## RuntimeExecutor and Session

A session stores a shared executor.

```text
Session
  -> RuntimeExecutor
```

This makes it possible for session dispatch or handler execution paths to use runtime scheduling when needed.

The session still uses `vix::async` for network I/O.

## RuntimeExecutor and vix::async

The executor is not the same as `vix::async`.

They serve different roles.

```text
vix::async
  -> I/O context
  -> network reads and writes
  -> timers
  -> coroutine tasks

RuntimeExecutor
  -> runtime tasks
  -> worker scheduling
  -> task metrics
```

The HTTP server uses both.

```text
I/O work
  -> vix::async

application or runtime work
  -> RuntimeExecutor
```

## RuntimeExecutor and vix::runtime

The executor adapts higher-level callables into runtime tasks.

```text
std::function<void()>
  -> RuntimeExecutor::post
  -> runtime task function
  -> Runtime::submit
```

For direct runtime tasks:

```text
TaskFn
  -> RuntimeExecutor::submit
  -> Runtime::submit
```

This gives Core a clean bridge to the lower-level scheduler.

## Runtime task result

A direct runtime task returns a `TaskResult`.

```cpp
executor.submit([]() -> vix::runtime::TaskResult
{
  return vix::runtime::TaskResult::complete;
});
```

Results:

| Result     | Meaning                             |
| ---------- | ----------------------------------- |
| `complete` | The task finished.                  |
| `yield`    | The task should be scheduled again. |
| `failed`   | The task failed.                    |

## Yielding task

A low-level task can yield.

```cpp
std::size_t step = 0;

executor.submit([step]() mutable -> vix::runtime::TaskResult
{
  ++step;

  if (step < 10)
  {
    return vix::runtime::TaskResult::yield;
  }

  return vix::runtime::TaskResult::complete;
});
```

When a task yields, the runtime can reschedule it.

This is useful for work that can be split into small steps.

## Failed task

A direct task can report failure.

```cpp
executor.submit([]() -> vix::runtime::TaskResult
{
  return vix::runtime::TaskResult::failed;
});
```

A task submitted through `post(...)` returns failed if the callable throws.

```cpp
executor.post([]()
{
  throw std::runtime_error("failed");
});
```

The executor records failure internally.

## Safe destruction

`RuntimeExecutor` stops the runtime in its destructor.

Conceptually:

```text
~RuntimeExecutor
  -> stop
  -> ignore exceptions
```

This makes cleanup safer during application shutdown.

Still, explicit lifecycle management is recommended in advanced code.

```cpp
executor.stop();
```

## Complete manual example

```cpp
#include <vix/executor/RuntimeExecutor.hpp>
#include <vix/print.hpp>

#include <chrono>

int main()
{
  vix::executor::RuntimeExecutor executor{4};

  executor.start();

  vix::executor::TaskOptions options;
  options.timeout = std::chrono::milliseconds(100);

  executor.post([]()
  {
    vix::print("normal task");
  }, options);

  executor.submit([]() -> vix::runtime::TaskResult
  {
    vix::print("runtime task");
    return vix::runtime::TaskResult::complete;
  });

  executor.post_http_fast([]()
  {
    vix::print("fast HTTP task");
  });

  executor.wait_idle();

  const auto metrics = executor.metrics();

  vix::print("pending:", metrics.pending);
  vix::print("active:", metrics.active);
  vix::print("timed out:", metrics.timed_out);

  executor.stop();

  return 0;
}
```

## Complete App example

```cpp
#include <vix.hpp>

int main()
{
  auto executor = std::make_shared<vix::executor::RuntimeExecutor>(4);

  vix::App app{executor};

  app.get("/", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.text("App using a custom RuntimeExecutor");
  });

  app.get("/metrics", [&app](vix::Request &req, vix::Response &res)
  {
    (void)req;

    const auto m = app.executor().metrics();

    res.json({
      "pending", m.pending,
      "active", m.active,
      "timed_out", m.timed_out
    });
  });

  app.run(8080);

  return 0;
}
```

## API summary

| API                            | Purpose                                        |
| ------------------------------ | ---------------------------------------------- |
| `RuntimeExecutor(config)`      | Create an executor from runtime config.        |
| `RuntimeExecutor(workers)`     | Create an executor with a worker count.        |
| `RuntimeExecutor(runtime)`     | Create an executor from an existing runtime.   |
| `start()`                      | Start the underlying runtime.                  |
| `stop()`                       | Stop the underlying runtime.                   |
| `stop_and_wait()`              | Wait for current work to drain, then stop.     |
| `started()`                    | Return whether the executor has started.       |
| `running()`                    | Return whether the runtime is running.         |
| `accepting()`                  | Return whether new work is accepted.           |
| `submit(Task)`                 | Submit a pre-built runtime task.               |
| `submit(TaskFn, affinity)`     | Submit a low-level runtime task function.      |
| `post(fn, options)`            | Submit a normal void callable.                 |
| `post_http_fast(fn, affinity)` | Submit a short HTTP fast-path task.            |
| `metrics()`                    | Return executor metrics.                       |
| `wait_idle()`                  | Wait until pending and active work reach zero. |
| `submitted_tasks()`            | Return accepted submission count.              |
| `rejected_tasks()`             | Return rejected submission count.              |
| `fast_http_submitted_tasks()`  | Return fast HTTP submission count.             |

## Metrics summary

| Metric                        | Meaning                                                 |
| ----------------------------- | ------------------------------------------------------- |
| `pending`                     | Tasks waiting in the runtime.                           |
| `active`                      | Tasks currently active through tracked `post(...)`.     |
| `timed_out`                   | Tasks that exceeded their configured timeout threshold. |
| `submitted_tasks()`           | Total accepted submissions.                             |
| `rejected_tasks()`            | Total rejected submissions.                             |
| `fast_http_submitted_tasks()` | Accepted fast-path HTTP submissions.                    |

## Best practices

Use the app-managed executor for normal applications.

```cpp
vix::App app;
```

Use a custom executor only when you need explicit control.

```cpp
auto executor = std::make_shared<vix::executor::RuntimeExecutor>(4);
vix::App app{executor};
```

Use `post(...)` for normal work.

```cpp
executor.post(task);
```

Use `post_http_fast(...)` only for short hot-path HTTP work.

```cpp
executor.post_http_fast(task);
```

Use `submit(...)` when you need the low-level runtime task contract.

```cpp
executor.submit(task_fn);
```

Call `wait_idle()` before stopping if you need graceful draining.

```cpp
executor.stop_and_wait();
```

Do not submit new work after shutdown begins.

```cpp
if (executor.accepting())
{
  executor.post(task);
}
```

## Next steps

Read the next pages:

- [Async and runtime](./async-and-runtime.md)
- [HTTP server](./http-server.md)
- [Sessions](./sessions.md)
- [Configuration](./configuration.md)
- [API Reference](./api-reference.md)
