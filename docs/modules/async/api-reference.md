# API Reference

This page is a compact reference for the public Vix Async API.

Use it when you want to quickly find the main types, functions, and operations exposed by `vix::async`.

## Public header

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>
```

## Namespaces

Core async APIs live in:

```
vix::async::core
```

Networking APIs live in:

```
vix::async::net
```

## Main components

| Component       | Purpose                                          |
| --------------- | ------------------------------------------------ |
| `io_context`    | Central async runtime context.                   |
| `scheduler`     | Runs callbacks and resumes coroutines.           |
| `task<T>`       | Coroutine task producing a value.                |
| `task<void>`    | Coroutine task with no returned value.           |
| `cancel_source` | Owns cancellation state.                         |
| `cancel_token`  | Observes cancellation state.                     |
| `timer`         | Delayed callbacks and coroutine sleeps.          |
| `thread_pool`   | Background worker pool for CPU or blocking work. |
| `signal_set`    | Async signal watching.                           |
| `when_all`      | Waits for multiple tasks to complete.            |
| `when_any`      | Waits for the first task to complete.            |
| `dns_resolver`  | Async hostname resolution.                       |
| `tcp_stream`    | Connected TCP stream.                            |
| `tcp_listener`  | TCP server listener.                             |
| `udp_socket`    | UDP datagram socket.                             |

## io_context

`io_context` is the central runtime object.

```cpp
vix::async::core::io_context ctx;
```

It owns the scheduler and async services.

### post

Posts a callback to the scheduler.

```cpp
ctx.post([&ctx]()
{
  vix::print("hello");
  ctx.stop();
});
```

### run

Runs the scheduler loop. Blocks until the context is stopped.

```cpp
ctx.run();
```

### stop

Requests the scheduler loop to stop.

```cpp
ctx.stop();
```

### is_running

Checks whether the scheduler is running.

```cpp
vix::print("running =", ctx.is_running() ? "yes" : "no");
```

### get_scheduler

Returns the internal scheduler.

```cpp
auto &sched = ctx.get_scheduler();
```

Use it when starting tasks.

```cpp
auto t = app(ctx);
std::move(t).start(ctx.get_scheduler());
```

### cpu_pool

Returns the CPU thread pool.

```cpp
auto &pool = ctx.cpu_pool();
```

Use it to run blocking or CPU-heavy work.

### timers

Returns the timer service.

```cpp
auto &timers = ctx.timers();
```

Use it for delayed callbacks and coroutine sleeps.

### signals

Returns the signal service.

```cpp
auto &signals = ctx.signals();
```

Use it for async signal handling.

### net

Returns the internal networking backend.

```cpp
ctx.net();
```

Most user code should prefer the higher-level networking factories:

```cpp
vix::async::net::make_dns_resolver(ctx);
vix::async::net::make_tcp_stream(ctx);
vix::async::net::make_tcp_listener(ctx);
vix::async::net::make_udp_socket(ctx);
```

### shutdown

Stops the scheduler and releases services.

```cpp
ctx.shutdown();
```

The destructor also shuts down the context.

## scheduler

`scheduler` runs callbacks and coroutine continuations.

Most application code uses it through `io_context`.

```cpp
auto &sched = ctx.get_scheduler();
```

### post

Posts a callable.

```cpp
sched.post([]()
{
  vix::print("callback");
});
```

### post_handle

Posts a coroutine handle. Most user code does not call this directly.

```cpp
sched.post_handle(handle);
```

### schedule

Creates an awaitable that resumes the coroutine on the scheduler.

```cpp
co_await ctx.get_scheduler().schedule();
```

### run

Runs the scheduler loop. In normal applications, prefer `ctx.run()`.

### stop

Stops the scheduler. In normal applications, prefer `ctx.stop()`.

## task

`task<T>` is the main coroutine task type.

### task\<T\>

Use `task<T>` when the coroutine returns a value.

```cpp
vix::async::core::task<int> compute()
{
  co_return 42;
}
```

Await it from another task:

```cpp
const int value = co_await compute();
```

### task\<void\>

Use `task<void>` when the coroutine returns no value.

```cpp
vix::async::core::task<void> work()
{
  vix::print("working");
  co_return;
}
```

### start

Starts a task on a scheduler. Tasks are lazy — they do not run until awaited or started.

```cpp
auto t = app(ctx);
std::move(t).start(ctx.get_scheduler());
```

### valid

Checks whether a task owns a coroutine handle.

```cpp
if (t.valid())
{
  vix::print("valid task");
}
```

Equivalent:

```cpp
if (t)
{
  vix::print("valid task");
}
```

### release

Releases the underlying coroutine handle. Most user code should prefer `start`.

```cpp
auto handle = t.release();
```

### Minimal task example

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

vix::async::core::task<int> compute()
{
  co_return 42;
}

vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  const int value = co_await compute();

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

## Cancellation

Vix Async cancellation is cooperative.

### cancel_source

Owns the cancellation state.

```cpp
vix::async::core::cancel_source source;
```

Request cancellation:

```cpp
source.request_cancel();
```

Check source state:

```cpp
if (source.is_cancelled())
{
  vix::print("cancelled");
}
```

### cancel_token

Observes cancellation state.

```cpp
vix::async::core::cancel_token token = source.token();
```

Check whether the token is connected to a source:

```cpp
if (token.can_cancel())
{
  vix::print("can cancel");
}
```

Check whether cancellation was requested:

```cpp
if (token.is_cancelled())
{
  vix::print("cancel requested");
}
```

### cancelled_ec

Returns the standard async cancellation error code.

```cpp
const std::error_code ec = vix::async::core::cancelled_ec();

vix::print("message =", ec.message());
```

## Timer

`timer` provides delayed callbacks and coroutine sleeps.

Access it through `io_context`.

```cpp
ctx.timers()
```

### sleep_for

Suspends a coroutine for a duration.

```cpp
co_await ctx.timers().sleep_for(std::chrono::milliseconds(100));
```

With cancellation:

```cpp
co_await ctx.timers().sleep_for(
    std::chrono::seconds(1),
    source.token());
```

### after

Schedules a callback after a duration.

```cpp
ctx.timers().after(std::chrono::milliseconds(100), []()
{
  vix::print("timer fired");
});
```

With cancellation:

```cpp
ctx.timers().after(
    std::chrono::seconds(1),
    []()
    {
      vix::print("callback");
    },
    source.token());
```

### Timer example

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

#include <chrono>

using namespace std::chrono_literals;

vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  vix::print("before sleep");

  co_await ctx.timers().sleep_for(100ms);

  vix::print("after sleep");

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

## Thread pool

`thread_pool` runs CPU-heavy or blocking work outside the scheduler thread.

Access it through `io_context`.

```cpp
ctx.cpu_pool()
```

### submit

Submits a plain callback.

```cpp
ctx.cpu_pool().submit([]()
{
  do_work();
});
```

### Awaitable submit

Submits a callable and awaits the result.

```cpp
int value = co_await ctx.cpu_pool().submit([]()
{
  return 42;
});
```

With cancellation:

```cpp
int value = co_await ctx.cpu_pool().submit([]()
{
  return 42;
}, source.token());
```

### Thread pool example

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

## when_all

`when_all` starts multiple tasks and waits until all of them complete.

```cpp
auto results = co_await vix::async::core::when_all(
    ctx.get_scheduler(),
    first(),
    second());
```

Results are returned as a tuple.

```cpp
const auto a = std::get<0>(results);
const auto b = std::get<1>(results);
```

Example:

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

#include <tuple>

vix::async::core::task<int> first()
{
  co_return 20;
}

vix::async::core::task<int> second()
{
  co_return 22;
}

vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  auto results = co_await vix::async::core::when_all(
      ctx.get_scheduler(),
      first(),
      second());

  vix::print("sum =", std::get<0>(results) + std::get<1>(results));

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

## when_any

`when_any` starts multiple tasks and waits for the first one to complete.

```cpp
auto result = co_await vix::async::core::when_any(
    ctx.get_scheduler(),
    fast(),
    slow());
```

The result contains:

| Field           | Purpose                            |
| --------------- | ---------------------------------- |
| `result.first`  | Index of the first completed task. |
| `result.second` | Tuple of optional result slots.    |

Example:

```cpp
const std::size_t index = result.first;
auto values = std::move(result.second);
```

## signal_set

`signal_set` watches system signals.

Access it through `io_context`.

```cpp
ctx.signals()
```

### add

Adds a signal to watch.

```cpp
ctx.signals().add(SIGINT);
```

### remove

Removes a watched signal.

```cpp
ctx.signals().remove(SIGINT);
```

### async_wait

Waits for the next signal.

```cpp
const int sig = co_await ctx.signals().async_wait();
```

With cancellation:

```cpp
const int sig = co_await ctx.signals().async_wait(source.token());
```

### on_signal

Registers a callback.

```cpp
ctx.signals().on_signal([](int sig)
{
  vix::print("signal =", sig);
});
```

### stop

Stops signal watching.

```cpp
ctx.signals().stop();
```

### Signal example

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

#include <csignal>

vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  ctx.signals().add(SIGINT);

  vix::print("press Ctrl+C to stop");

  const int sig = co_await ctx.signals().async_wait();

  vix::print("signal =", sig);

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

## DNS

DNS APIs live in `vix::async::net`.

### resolved_address

Represents one resolved endpoint.

| Field  | Purpose             |
| ------ | ------------------- |
| `ip`   | IP address as text. |
| `port` | Port number.        |

### make_dns_resolver

Creates a resolver attached to an `io_context`.

```cpp
auto resolver = vix::async::net::make_dns_resolver(ctx);
```

### async_resolve

Resolves a hostname and port.

```cpp
auto addresses = co_await resolver->async_resolve(
    "example.com",
    80);
```

With cancellation:

```cpp
auto addresses = co_await resolver->async_resolve(
    "example.com",
    80,
    source.token());
```

## TCP

TCP APIs live in `vix::async::net`.

### tcp_endpoint

Describes a TCP endpoint.

```cpp
vix::async::net::tcp_endpoint endpoint{
    "127.0.0.1",
    8080
};
```

| Field  | Purpose                 |
| ------ | ----------------------- |
| `host` | Hostname or IP address. |
| `port` | TCP port.               |

### tcp_stream

Represents a connected TCP stream.

| Method                           | Purpose                                      |
| -------------------------------- | -------------------------------------------- |
| `async_connect(endpoint, token)` | Connects to a remote endpoint.               |
| `async_read(buffer, token)`      | Reads bytes.                                 |
| `async_write(buffer, token)`     | Writes bytes.                                |
| `close()`                        | Closes the stream.                           |
| `is_open()`                      | Checks open state.                           |
| `native_handle()`                | Returns native socket handle when supported. |

### make_tcp_stream

Creates a TCP stream.

```cpp
auto stream = vix::async::net::make_tcp_stream(ctx);
```

### async_connect

Connects to a remote endpoint.

```cpp
co_await stream->async_connect({
    "example.com",
    80
});
```

With cancellation:

```cpp
co_await stream->async_connect(endpoint, source.token());
```

### async_read

Reads bytes into a mutable byte buffer.

```cpp
std::array<std::byte, 1024> buffer{};

const std::size_t bytes = co_await stream->async_read(
    std::span<std::byte>{buffer.data(), buffer.size()});
```

### async_write

Writes bytes from an immutable byte buffer.

```cpp
std::string message = "hello";

const std::size_t bytes = co_await stream->async_write(
    std::as_bytes(std::span{message}));
```

### tcp_listener

Represents a listening TCP socket.

| Method                            | Purpose                 |
| --------------------------------- | ----------------------- |
| `async_listen(endpoint, backlog)` | Starts listening.       |
| `async_accept(token)`             | Accepts one connection. |
| `close()`                         | Closes the listener.    |
| `is_open()`                       | Checks open state.      |

### make_tcp_listener

Creates a TCP listener.

```cpp
auto listener = vix::async::net::make_tcp_listener(ctx);
```

### async_listen

Starts listening on a local endpoint.

```cpp
co_await listener->async_listen({
    "127.0.0.1",
    8080
});
```

### async_accept

Accepts one incoming connection. The returned client is a `std::unique_ptr<tcp_stream>`.

```cpp
auto client = co_await listener->async_accept();
```

## UDP

UDP APIs live in `vix::async::net`.

### udp_endpoint

Describes a UDP endpoint.

```cpp
vix::async::net::udp_endpoint endpoint{
    "127.0.0.1",
    9000
};
```

| Field  | Purpose                 |
| ------ | ----------------------- |
| `host` | Hostname or IP address. |
| `port` | UDP port.               |

### udp_datagram

Describes a received datagram.

| Field   | Purpose                   |
| ------- | ------------------------- |
| `from`  | Sender endpoint.          |
| `bytes` | Number of received bytes. |

### udp_socket

Represents a UDP socket.

| Method                                   | Purpose                    |
| ---------------------------------------- | -------------------------- |
| `async_bind(endpoint)`                   | Binds to a local endpoint. |
| `async_send_to(buffer, endpoint, token)` | Sends one datagram.        |
| `async_recv_from(buffer, token)`         | Receives one datagram.     |
| `close()`                                | Closes the socket.         |
| `is_open()`                              | Checks open state.         |

### make_udp_socket

Creates a UDP socket.

```cpp
auto socket = vix::async::net::make_udp_socket(ctx);
```

### async_bind

Binds the socket to a local endpoint.

```cpp
co_await socket->async_bind({
    "127.0.0.1",
    9000
});
```

### async_send_to

Sends one datagram.

```cpp
std::string message = "hello";

const std::size_t bytes = co_await socket->async_send_to(
    std::as_bytes(std::span{message}),
    {
        "127.0.0.1",
        9000
    });
```

### async_recv_from

Receives one datagram.

```cpp
std::array<std::byte, 1024> buffer{};

const auto datagram = co_await socket->async_recv_from(
    std::span<std::byte>{buffer.data(), buffer.size()});
```

## Error codes

Async core errors use `vix::async::core::errc`.

| Error              | Meaning                                      |
| ------------------ | -------------------------------------------- |
| `ok`               | No error.                                    |
| `invalid_argument` | Invalid argument.                            |
| `not_ready`        | Operation cannot complete yet.               |
| `timeout`          | Operation timed out.                         |
| `canceled`         | Operation was canceled.                      |
| `closed`           | Resource was closed.                         |
| `overflow`         | Capacity or numeric overflow.                |
| `stopped`          | Runtime or scheduler was stopped.            |
| `queue_full`       | Internal task queue is full.                 |
| `rejected`         | Task submission was rejected.                |
| `not_supported`    | Operation is not supported on this platform. |

### make_error_code

Creates a `std::error_code` from an async error.

```cpp
std::error_code ec =
    vix::async::core::make_error_code(
        vix::async::core::errc::canceled);
```

## Operation table

| Goal                       | API                                          |
| -------------------------- | -------------------------------------------- |
| Create runtime             | `vix::async::core::io_context ctx`           |
| Post callback              | `ctx.post(...)`                              |
| Run scheduler              | `ctx.run()`                                  |
| Stop scheduler             | `ctx.stop()`                                 |
| Start task                 | `std::move(task).start(ctx.get_scheduler())` |
| Sleep coroutine            | `ctx.timers().sleep_for(duration)`           |
| Delayed callback           | `ctx.timers().after(duration, fn)`           |
| Create cancellation source | `cancel_source source`                       |
| Get cancellation token     | `source.token()`                             |
| Request cancellation       | `source.request_cancel()`                    |
| Offload work               | `ctx.cpu_pool().submit(fn)`                  |
| Wait all tasks             | `when_all(ctx.get_scheduler(), ...)`         |
| Wait first task            | `when_any(ctx.get_scheduler(), ...)`         |
| Wait signal                | `ctx.signals().async_wait()`                 |
| Resolve DNS                | `resolver->async_resolve(host, port)`        |
| Connect TCP                | `stream->async_connect(endpoint)`            |
| Read TCP                   | `stream->async_read(buffer)`                 |
| Write TCP                  | `stream->async_write(buffer)`                |
| Listen TCP                 | `listener->async_listen(endpoint)`           |
| Accept TCP                 | `listener->async_accept()`                   |
| Bind UDP                   | `socket->async_bind(endpoint)`               |
| Send UDP                   | `socket->async_send_to(buffer, endpoint)`    |
| Receive UDP                | `socket->async_recv_from(buffer)`            |

## Complete minimal runtime example

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

int main()
{
  vix::async::core::io_context ctx;

  ctx.post([&ctx]()
  {
    vix::print("hello from async");
    ctx.stop();
  });

  ctx.run();

  return 0;
}
```

## Complete coroutine example

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

#include <chrono>

using namespace std::chrono_literals;

vix::async::core::task<int> compute(
    vix::async::core::io_context &ctx)
{
  co_await ctx.timers().sleep_for(100ms);

  co_return 42;
}

vix::async::core::task<void> app(
    vix::async::core::io_context &ctx)
{
  const int value = co_await compute(ctx);

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

## Complete cancellation example

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

#include <chrono>
#include <system_error>

using namespace std::chrono_literals;

vix::async::core::task<void> app(
    vix::async::core::io_context &ctx)
{
  vix::async::core::cancel_source source;

  ctx.timers().after(100ms, [&source]()
  {
    source.request_cancel();
  });

  try
  {
    co_await ctx.timers().sleep_for(1s, source.token());

    vix::print("sleep completed");
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

## Common mistakes

### Creating a task but never starting it

Wrong:

```cpp
auto t = app(ctx);

ctx.run();
```

Correct:

```cpp
auto t = app(ctx);
std::move(t).start(ctx.get_scheduler());

ctx.run();
```

### Forgetting to run the context

Wrong:

```cpp
ctx.post([]()
{
  vix::print("hello");
});
```

Correct:

```cpp
ctx.post([&ctx]()
{
  vix::print("hello");
  ctx.stop();
});

ctx.run();
```

### Forgetting to stop the context

If nothing calls `ctx.stop()`, the context may continue waiting.

```cpp
ctx.stop();
```

### Blocking the scheduler thread

Wrong:

```cpp
std::this_thread::sleep_for(std::chrono::seconds(1));
```

Correct:

```cpp
co_await ctx.timers().sleep_for(std::chrono::seconds(1));
```

For CPU-heavy work:

```cpp
auto value = co_await ctx.cpu_pool().submit([]()
{
  return expensive_work();
});
```

### Passing text directly to network write APIs

Network write APIs expect byte spans.

Wrong:

```cpp
co_await stream->async_write(message);
```

Correct:

```cpp
co_await stream->async_write(
    std::as_bytes(std::span{message}));
```

### Ignoring network errors

Network operations can throw.

```cpp
try
{
  co_await stream->async_connect(endpoint);
}
catch (const std::exception &ex)
{
  vix::eprint(ex.what());
}
```

### Reusing moved tasks

Wrong:

```cpp
auto t = app(ctx);

std::move(t).start(ctx.get_scheduler());
std::move(t).start(ctx.get_scheduler());
```

Correct:

```cpp
auto t = app(ctx);

std::move(t).start(ctx.get_scheduler());
```

## Related pages

| Page                                        | Purpose                           |
| ------------------------------------------- | --------------------------------- |
| [Overview](/modules/async/)                 | Start with the async overview.    |
| [io_context](/modules/async/io-context)     | Learn the runtime context.        |
| [Tasks](/modules/async/tasks)               | Learn coroutine tasks.            |
| [Spawn](/modules/async/spawn)               | Learn how to start tasks.         |
| [Timers](/modules/async/timers)             | Learn timer sleeps and callbacks. |
| [Cancellation](/modules/async/cancellation) | Learn cooperative cancellation.   |
| [Thread pool](/modules/async/thread-pool)   | Learn background work.            |
| [when_all / when_any](/modules/async/when)  | Learn task composition.           |
| [Signals](/modules/async/signals)           | Learn signal handling.            |
| [TCP](/modules/async/tcp)                   | Learn TCP streams and listeners.  |
| [UDP](/modules/async/udp)                   | Learn UDP sockets.                |
| [DNS](/modules/async/dns)                   | Learn DNS resolution.             |
