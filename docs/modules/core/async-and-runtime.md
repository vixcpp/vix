# Async and runtime

This page explains how `vix::async`, `vix::runtime`, and Vix Core work together.

Use it when you want to understand the difference between asynchronous I/O, coroutine scheduling, runtime workers, HTTP sessions, and application execution.

## Public header

```cpp
#include <vix.hpp>
```

For lower-level usage, the related headers are:

```cpp
#include <vix/async/core/io_context.hpp>
#include <vix/async/core/task.hpp>
#include <vix/async/core/spawn.hpp>
#include <vix/executor/RuntimeExecutor.hpp>
#include <vix/runtime/Runtime.hpp>
```

## The short version

Vix Core uses two execution models:

```text
vix::async
  -> network I/O
  -> timers
  -> cancellation
  -> coroutine tasks
  -> TCP accept/read/write

vix::runtime
  -> internal task execution
  -> worker threads
  -> scheduling
  -> run queues
  -> runtime metrics
```

Core connects both:

```text
vix::core
  -> App
  -> HTTPServer
  -> Session
  -> Router
  -> RuntimeExecutor
```

## Why there are two systems

An HTTP server needs to do two different kinds of work.

First, it must wait for I/O.

```text
accept connection
read request bytes
write response bytes
wait for timers
handle cancellation
```

This is the job of `vix::async`.

Second, it must execute work.

```text
run tasks
schedule work
track pending tasks
manage workers
reschedule yielded tasks
collect metrics
```

This is the job of `vix::runtime`.

Keeping them separate makes the architecture easier to reason about.

## Mental model

Think of the system like this:

```text
vix::async keeps the network moving.

vix::runtime executes scheduled work.

vix::core connects both into an HTTP application model.
```

## Core request flow

A simplified HTTP request flow looks like this:

```text
client
  -> TCP connection
  -> HTTPServer
  -> vix::async accept
  -> Session
  -> vix::async read
  -> Request parser
  -> Router
  -> RequestHandler
  -> user handler
  -> Response
  -> vix::async write
  -> client
```

The runtime executor is part of the Core execution model:

```text
App
  -> RuntimeExecutor
  -> vix::runtime::Runtime
  -> Scheduler
  -> Workers
```

## vix::async role

`vix::async` is used for asynchronous I/O.

In Core, it handles:

- TCP listener creation
- async listen
- async accept
- async read
- async write
- session coroutines
- timers
- cancellation tokens
- detached async tasks
- I/O context execution

The HTTP server owns an async context.

```text
HTTPServer
  -> io_context
```

The I/O threads run that context.

```text
io_thread_0 -> io_context.run()
io_thread_1 -> io_context.run()
io_thread_N -> io_context.run()
```

## vix::runtime role

`vix::runtime` is the internal task execution engine.

It handles:

- runtime startup
- runtime shutdown
- task submission
- task scheduling
- worker threads
- local run queues
- work stealing
- task yielding
- metrics

The runtime model is exposed to Core through `RuntimeExecutor`.

```text
RuntimeExecutor
  -> Runtime
  -> Scheduler
  -> Worker
  -> RunQueue
  -> Task
```

## RuntimeExecutor role

`RuntimeExecutor` is the bridge between Core and `vix::runtime`.

It turns higher-level callable work into runtime tasks.

```cpp
executor.post([]()
{
  // work
});
```

Internally:

```text
callable
  -> RuntimeExecutor
  -> runtime task
  -> Runtime::submit
  -> Scheduler
  -> Worker
```

## HTTPServer and async

The HTTP server uses `vix::async` directly.

```text
HTTPServer::run
  -> start I/O threads
  -> spawn start_server
  -> start accept loop
```

The accept loop is asynchronous.

```text
accept_loop
  -> co_await listener.async_accept
  -> spawn handle_client
```

Each accepted client is handled as an async task.

```text
handle_client
  -> create Session or TlsSession
  -> co_await session.run()
```

## Session and async

A session is asynchronous.

It uses `vix::async::core::task<void>`.

```text
Session::run
  -> co_await read_request
  -> co_await dispatch_request
  -> co_await send_response
  -> co_await close_stream_gracefully
```

The session awaits transport reads and writes.

```text
co_await transport.async_read(...)
co_await transport.async_write(...)
```

## Transport and async

Transports also use `vix::async`.

The transport interface returns async tasks.

```cpp
task<std::size_t> async_read(
    std::span<std::byte> buffer,
    cancel_token token);

task<std::size_t> async_write(
    std::span<const std::byte> buffer,
    cancel_token token);
```

This lets the session read and write without blocking the whole server.

## Timers and cancellation

Core uses async timers and cancellation for session timeouts.

Conceptually:

```text
start timer
  -> wait for configured timeout
  -> cancel token
  -> close transport
```

The same cancellation token can be passed to transport operations.

```text
transport.async_read(buffer, token)
transport.async_write(buffer, token)
```

## Runtime worker model

`vix::runtime` runs work on workers.

```text
Runtime
  -> Scheduler
    -> Worker 0
    -> Worker 1
    -> Worker N
```

Each worker owns a local queue.

```text
Worker
  -> RunQueue
  -> Task
```

When a worker has no local work, it can try to steal work from another worker.

```text
idle worker
  -> try steal
  -> execute stolen task
```

## Runtime tasks

A runtime task returns a task result.

```cpp
vix::runtime::TaskResult::complete
vix::runtime::TaskResult::yield
vix::runtime::TaskResult::failed
```

Meaning:

| Result     | Meaning                                   |
| ---------- | ----------------------------------------- |
| `complete` | The task finished.                        |
| `yield`    | The task should be scheduled again later. |
| `failed`   | The task failed.                          |

## Yielding work

A low-level runtime task can yield.

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

This lets long work be split into smaller steps.

## Async task vs runtime task

These are different concepts.

An async task is coroutine-based.

```cpp
vix::async::core::task<void>
```

It is used for operations like:

```text
co_await sleep
co_await accept
co_await read
co_await write
```

A runtime task is a lower-level scheduled unit of work.

```cpp
vix::runtime::Task
```

It returns a `TaskResult`.

```text
complete
yield
failed
```

## Comparison

| Concept                          | Used for                           |
| -------------------------------- | ---------------------------------- |
| `vix::async::core::task<T>`      | Coroutine-based async operations.  |
| `vix::async::core::io_context`   | Running async I/O tasks.           |
| `vix::async::net::tcp_listener`  | Accepting TCP connections.         |
| `vix::async::net::tcp_stream`    | Reading and writing TCP bytes.     |
| `vix::runtime::Runtime`          | Running internal scheduled tasks.  |
| `vix::runtime::Task`             | Low-level runtime work unit.       |
| `vix::executor::RuntimeExecutor` | Adapter from Core to runtime.      |
| `vix::server::HTTPServer`        | Native HTTP server built on async. |
| `vix::session::Session`          | One client connection lifecycle.   |

## Thread model

A Core app can involve several thread groups.

```text
main thread
  -> constructs App
  -> calls run or listen

server thread
  -> started by App::listen
  -> calls HTTPServer::run

I/O threads
  -> run io_context
  -> process async network work

runtime workers
  -> owned by RuntimeExecutor
  -> execute runtime tasks
```

Simplified:

```text
I/O threads handle network progress.

Runtime workers handle scheduled runtime work.
```

## Why not run everything on one thread pool

Network I/O and scheduled work have different needs.

I/O needs:

```text
wait for sockets
resume coroutines
handle timers
react to cancellation
```

Runtime work needs:

```text
execute tasks
balance work
reschedule yielded tasks
track metrics
avoid blocking I/O progress
```

Separating them avoids mixing two different responsibilities.

## HTTP server thread flow

When the app starts:

```text
App::listen
  -> starts server thread
  -> HTTPServer::run
  -> starts I/O threads
  -> spawns start_server coroutine
  -> starts accept loop
```

The accept loop then handles connections.

```text
listener.async_accept
  -> handle_client
  -> Session::run
```

## Runtime executor startup

The app creates and starts the executor.

```text
App constructor
  -> make RuntimeExecutor
  -> executor.start()
  -> Runtime::start()
  -> Scheduler starts workers
```

After that, runtime workers are ready to execute submitted work.

## Runtime executor shutdown

Shutdown is designed to stop accepting work first.

```text
RuntimeExecutor::stop
  -> accepting = false
  -> Runtime::stop
  -> workers join
```

For graceful draining:

```text
RuntimeExecutor::stop_and_wait
  -> accepting = false
  -> wait_idle
  -> stop
```

## HTTP server shutdown

The HTTP server shutdown flow is:

```text
HTTPServer::stop_async
  -> mark stop requested
  -> close listener
  -> stop io_context
  -> wake monitor thread
```

Then the blocking path joins threads.

```text
HTTPServer::stop_blocking
  -> stop_async
  -> join_threads
```

## App shutdown

`App::close()` coordinates server shutdown.

```text
App::close
  -> mark stop requested
  -> run shutdown callback
  -> server.stop_async
  -> server.stop_blocking
  -> join server thread
```

The executor is stopped safely by its lifecycle.

## I/O work example

This kind of work belongs to `vix::async`.

```text
accept new connection
read request bytes
write response bytes
sleep for timeout
cancel slow connection
```

Example shape:

```cpp
co_await listener.async_accept();
co_await stream.async_read(buffer, token);
co_await stream.async_write(buffer, token);
co_await ctx.timers().sleep_for(timeout, token);
```

## Runtime work example

This kind of work belongs to `vix::runtime`.

```text
execute submitted task
reschedule yielded task
track active tasks
count timed-out tasks
stop workers
```

Example shape:

```cpp
executor.post([]()
{
  // scheduled work
});
```

Direct runtime task:

```cpp
executor.submit([]() -> vix::runtime::TaskResult
{
  return vix::runtime::TaskResult::complete;
});
```

## Core handler example

Most user code does not interact directly with the low-level async or runtime pieces.

```cpp
#include <vix.hpp>

int main()
{
  vix::App app;

  app.get("/", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.text("Hello from Vix");
  });

  app.run(8080);

  return 0;
}
```

Internally:

```text
HTTPServer uses async.
RuntimeExecutor connects to runtime.
Session dispatches request.
Router runs handler.
```

## Custom executor example

Use a custom executor when you need explicit runtime worker control.

```cpp
#include <vix.hpp>

int main()
{
  auto executor = std::make_shared<vix::executor::RuntimeExecutor>(4);

  vix::App app{executor};

  app.get("/", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.text("custom executor");
  });

  app.run(8080);

  return 0;
}
```

## Executor metrics route

You can expose runtime metrics from an app.

```cpp
#include <vix.hpp>

int main()
{
  vix::App app;

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

## Direct runtime task example

Advanced code can submit runtime tasks directly.

```cpp
#include <vix.hpp>

int main()
{
  vix::App app;

  app.executor().submit([]() -> vix::runtime::TaskResult
  {
    vix::print("runtime task");

    return vix::runtime::TaskResult::complete;
  });

  app.run(8080);

  return 0;
}
```

This is advanced usage.

Most routes should use normal handlers.

## Fast HTTP task path

`RuntimeExecutor` provides a fast path for short HTTP work.

```cpp
app.executor().post_http_fast([]()
{
  // short hot-path work
});
```

This path avoids extra bookkeeping used by generic `post(...)`.

Use it only when you know the task is short and does not need generic executor accounting.

## What should go where

Use `vix::async` for:

- waiting for sockets
- async reads
- async writes
- timers
- cancellation
- coroutine-based operations

Use `vix::runtime` for:

- scheduled internal work
- worker-based execution
- task queues
- task yielding
- runtime metrics

Use `vix::core` for:

- routes
- handlers
- middleware
- request parsing
- response writing
- app lifecycle
- HTTP server behavior

## Avoid blocking I/O threads

Long blocking work should not run on I/O threads.

Bad pattern:

```cpp
app.get("/slow", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  std::this_thread::sleep_for(std::chrono::seconds(5));

  res.text("done");
});
```

This can reduce server responsiveness.

Prefer moving expensive work into a runtime-managed path or making the work asynchronous when possible.

## Short handlers are best

Keep handlers short and clear.

```cpp
app.get("/status", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.json({"status", "ok"});
});
```

Use separate services for heavier logic.

```cpp
app.get("/reports", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  // Call a service that handles the expensive work.
  res.json({"report", "ready"});
});
```

## Heavy routes

Core can mark routes as heavy.

```cpp
app.get_heavy("/reports", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.json({"report", "ready"});
});
```

Heavy route metadata lets the runtime and router model identify expensive routes.

## Async I/O lifecycle

Simplified I/O lifecycle:

```text
create io_context
  -> start I/O threads
  -> bind listener
  -> start accept loop
  -> accept connections
  -> run sessions
  -> stop io_context
  -> join I/O threads
```

## Runtime lifecycle

Simplified runtime lifecycle:

```text
create RuntimeExecutor
  -> create Runtime
  -> start workers
  -> accept tasks
  -> execute tasks
  -> stop accepting
  -> stop runtime
  -> join workers
```

## Combined lifecycle

The combined lifecycle in `App` is:

```text
construct App
  -> create RuntimeExecutor
  -> start runtime workers
  -> create HTTPServer
  -> create async io_context

listen/run
  -> start server thread
  -> start I/O threads
  -> accept connections

request
  -> async read
  -> router handler
  -> async write

shutdown
  -> stop server
  -> stop I/O context
  -> stop runtime executor
```

## Architecture diagram

```text
                 user code
                    |
                    v
                  App
                    |
        +-----------+------------+
        |                        |
        v                        v
     Router              RuntimeExecutor
        |                        |
        v                        v
 RequestHandler              Runtime
        |                        |
        v                        v
     Session               Scheduler
        |                        |
        v                        v
    Transport                 Workers
        |
        v
   vix::async I/O
```

## Full request architecture

```text
client
  -> TCP socket
  -> vix::async::net::tcp_listener
  -> HTTPServer::accept_loop
  -> HTTPServer::handle_client
  -> Session or TlsSession
  -> Transport
  -> Session::read_request
  -> vix::http::Request
  -> Router::handle_request
  -> RequestHandler
  -> user handler
  -> vix::http::Response
  -> Session::send_response
  -> Transport::async_write
  -> client
```

## Complete example

```cpp
#include <vix.hpp>

int main()
{
  auto executor = std::make_shared<vix::executor::RuntimeExecutor>(4);

  vix::App app{executor};

  app.get("/", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.text("Hello from Vix");
  });

  app.get("/metrics", [&app](vix::Request &req, vix::Response &res)
  {
    (void)req;

    const auto metrics = app.executor().metrics();

    res.json({
      "pending", metrics.pending,
      "active", metrics.active,
      "timed_out", metrics.timed_out
    });
  });

  app.get_heavy("/report", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.json({
      "report", "ready"
    });
  });

  app.run(8080);

  return 0;
}
```

## API summary

| API                                | Purpose                           |
| ---------------------------------- | --------------------------------- |
| `vix::async::core::io_context`     | Runs async I/O tasks.             |
| `vix::async::core::task<T>`        | Coroutine-based async result.     |
| `vix::async::core::spawn_detached` | Start a detached async task.      |
| `vix::async::net::tcp_listener`    | Accept TCP connections.           |
| `vix::async::net::tcp_stream`      | Read and write TCP bytes.         |
| `vix::executor::RuntimeExecutor`   | Bridge Core to `vix::runtime`.    |
| `RuntimeExecutor::post`            | Submit normal callable work.      |
| `RuntimeExecutor::submit`          | Submit low-level runtime work.    |
| `RuntimeExecutor::post_http_fast`  | Submit short HTTP fast-path work. |
| `RuntimeExecutor::metrics`         | Read runtime executor metrics.    |
| `vix::runtime::Runtime`            | Internal runtime engine.          |
| `vix::runtime::Task`               | Runtime task unit.                |
| `vix::runtime::TaskResult`         | Runtime task result.              |

## Best practices

Use `vix::App` for normal application code.

```cpp
vix::App app;
```

Let Core manage `vix::async` and `vix::runtime`.

```cpp
app.run(8080);
```

Keep handlers short.

```cpp
app.get("/status", handler);
```

Use route metadata for expensive routes.

```cpp
app.get_heavy("/reports", handler);
```

Use the app executor only when you need advanced runtime control.

```cpp
auto &executor = app.executor();
```

Do not block I/O paths with long synchronous work.

```text
Keep I/O responsive.
Move expensive work to appropriate runtime-managed code.
```

## Next steps

Read the next pages:

- [Runtime executor](./runtime-executor.md)
- [HTTP server](./http-server.md)
- [Sessions](./sessions.md)
- [Transports](./transports.md)
- [Configuration](./configuration.md)
- [API Reference](./api-reference.md)
