# Shutdown

This page explains shutdown in the Vix WebSocket module.

Use it when you want to stop a WebSocket server safely, close active sessions, avoid blocking inside callbacks, and coordinate shutdown with an attached HTTP app.

## Header

```cpp
#include <vix/websocket/server.hpp>
#include <vix/websocket/AttachedRuntime.hpp>
```

Or use the umbrella header:

```cpp
#include <vix/websocket.hpp>
```

## Why shutdown matters

A WebSocket server keeps long-lived connections open.

That means shutdown must handle:

- active WebSocket sessions
- pending reads
- pending writes
- heartbeat loops
- I/O worker threads
- long-polling buffers
- shared runtime executor
- HTTP and WebSocket lifecycle coordination

The module separates non-blocking shutdown requests from final blocking cleanup.

## Basic rule

The most important rule is:

```txt
Use stop_async() when shutdown must not block.
Use stop() when final cleanup is allowed to block.
```

In attached HTTP + WebSocket applications:

```txt
HTTP shutdown callback
  -> request non-blocking WebSocket stop

safe finalization path
  -> perform blocking WebSocket stop
  -> stop shared executor
```

## Main shutdown APIs

| API                                    | Meaning                                         |
| -------------------------------------- | ----------------------------------------------- |
| `Server::stop_async()`                 | Request non-blocking WebSocket shutdown.        |
| `Server::stop()`                       | Stop the server and join internal threads.      |
| `Session::close(...)`                  | Close one session normally.                     |
| `Session::shutdown_now()`              | Force immediate session shutdown.               |
| `AttachedRuntime::request_stop()`      | Request non-blocking stop for attached runtime. |
| `AttachedRuntime::finalize_shutdown()` | Final blocking shutdown for attached runtime.   |

## Server shutdown model

`vix::websocket::Server` provides two shutdown paths:

```cpp
ws.stop_async();
ws.stop();
```

They are intentionally different.

## stop_async

Use `stop_async()` when shutdown must return quickly.

```cpp
ws.stop_async();
```

This function:

- collects live sessions
- requests immediate shutdown for each live session
- requests low-level server shutdown
- does not wait for all I/O threads to finish

Conceptually:

```txt
Server::stop_async
  -> collect live sessions
  -> session.shutdown_now()
  -> LowLevelServer::stop_async()
  -> return
```

Use this from callbacks or signal paths where blocking is unsafe.

## stop

Use `stop()` when you want final blocking cleanup.

```cpp
ws.stop();
```

This function stops the WebSocket server and joins internal server threads.

Conceptually:

```txt
Server::stop
  -> stop_async
  -> LowLevelServer::join_threads
```

Use it from a safe control path.

Do not call it from a callback that runs inside the WebSocket or HTTP server internals.

## Session close

Use `Session::close(...)` for normal application-level disconnection.

```cpp
session.close();
```

With a reason:

```cpp
session.close("normal shutdown");
```

Use this when:

- one client should be disconnected
- a user logs out
- a protocol rule is violated
- the session is no longer authorized
- the application wants to close a single connection

## Session immediate shutdown

Use `Session::shutdown_now()` for server shutdown paths.

```cpp
session.shutdown_now();
```

This is stronger than `close(...)`.

It is intended for cases where the server must not depend on async close work being scheduled later.

Normal application logic should prefer:

```cpp
session.close();
```

## Standalone server shutdown

For a standalone WebSocket server:

```cpp
#include <memory>
#include <string>

#include <vix/config/Config.hpp>
#include <vix/executor/RuntimeExecutor.hpp>
#include <vix/websocket.hpp>

int main()
{
  vix::config::Config config{".env"};

  auto executor =
      std::make_shared<vix::executor::RuntimeExecutor>(4);

  vix::websocket::Server ws{config, executor};

  ws.on_message(
    [](vix::websocket::Session &session, const std::string &message)
    {
      session.send_text("echo: " + message);
    });

  ws.start();

  ws.stop();
  executor->stop();

  return 0;
}
```

In real applications, shutdown is usually triggered by a signal handler, control route, or application lifecycle manager.

## Attached runtime shutdown

`AttachedRuntime` coordinates shutdown between:

```txt
vix::App
vix::websocket::Server
RuntimeExecutor
```

The attached runtime exists because HTTP and WebSocket often run together but must not block each other during shutdown.

## AttachedRuntime request_stop

Use `request_stop()` to request a non-blocking WebSocket stop.

```cpp
runtime.request_stop();
```

This is safe to call from shutdown callbacks.

Internally, it calls:

```cpp
ws.stop_async();
```

## AttachedRuntime finalize_shutdown

Use `finalize_shutdown()` for final blocking cleanup.

```cpp
runtime.finalize_shutdown();
```

Finalization order:

```txt
1. ws.stop()
2. executor.stop()
```

This function is idempotent.

It is safe to call more than once.

## Attached runtime shutdown flow

The attached shutdown flow is:

```txt
HTTP app shutdown requested
  -> HTTP shutdown callback runs
  -> AttachedRuntime requests ws.stop_async()
  -> callback returns quickly

Later, from safe control path:
  -> AttachedRuntime::finalize_shutdown()
  -> ws.stop()
  -> executor.stop()
```

This avoids deadlocks and avoids blocking inside HTTP shutdown callbacks.

## Why not block in shutdown callbacks

Shutdown callbacks may run inside an internal server thread or a path that must return quickly.

Blocking inside such a callback can cause:

- deadlocks
- stuck server shutdown
- thread join from the wrong thread
- partially stopped runtime state
- callbacks waiting on themselves

That is why the module separates:

```txt
non-blocking stop request
```

from:

```txt
blocking final cleanup
```

## Minimal attached runtime example

```cpp
#include <memory>
#include <string>

#include <vix.hpp>
#include <vix/websocket.hpp>

int main()
{
  vix::App app;

  auto executor =
      std::make_shared<vix::executor::RuntimeExecutor>(4);

  vix::websocket::Server ws{app.config(), executor};

  ws.on_message(
    [](vix::websocket::Session &session, const std::string &message)
    {
      session.send_text("echo: " + message);
    });

  vix::websocket::AttachedRuntime runtime{app, ws, executor};

  app.run(8080);

  return 0;
}
```

When `AttachedRuntime` is destroyed, it calls `finalize_shutdown()` defensively.

## Destructor behavior

`AttachedRuntime` finalizes shutdown in its destructor.

```txt
~AttachedRuntime
  -> finalize_shutdown()
```

This makes cleanup safer if the user forgets to finalize manually.

Still, when building advanced lifecycle code, explicit shutdown is clearer.

## Idempotent shutdown

Shutdown APIs are designed to be safe when called more than once.

For example:

```cpp
runtime.request_stop();
runtime.request_stop();

runtime.finalize_shutdown();
runtime.finalize_shutdown();
```

Only the first meaningful call performs the shutdown work.

This helps when shutdown can be triggered by several paths:

- Ctrl+C
- HTTP shutdown route
- process signal
- destructor cleanup
- internal error
- test teardown

## Session shutdown flow

A normal session lifecycle ends like this:

```txt
close requested
  -> close frame if possible
  -> stop heartbeat
  -> close stream
  -> notify router close once
  -> cleanup
```

During forced server shutdown:

```txt
server stop_async
  -> session.shutdown_now()
  -> transport closes immediately
  -> pending operations are cancelled or fail
```

## Close notification

A session should notify close handlers only once.

This avoids duplicate cleanup logic.

Conceptually:

```txt
close event
  -> compare close-notified flag
  -> notify router close once
```

This is important because close can happen from several paths:

- client close frame
- read error
- write error
- server shutdown
- forced shutdown
- protocol error

## Shutdown with rooms

When a session closes, the server removes it from all rooms.

Conceptually:

```txt
session close
  -> unregister session
  -> remove session from all rooms
  -> call user close handler
```

This prevents room lists from keeping dead sessions.

## Shutdown with long-polling

Long-polling uses HTTP polling sessions and in-memory buffers.

During application shutdown:

```txt
stop WebSocket server
  -> stop accepting realtime connections
  -> stop executor
  -> let process cleanup long-polling manager
```

If your application owns a `LongPollingBridge` or `LongPollingManager`, keep its lifetime longer than the server callbacks that use it.

## Shutdown with metrics exporter

`run_metrics_http_exporter(...)` is a blocking helper.

If you run it in a separate thread, coordinate shutdown for that thread in your application.

Example model:

```txt
main app
  -> starts metrics exporter thread
  -> starts HTTP + WebSocket
  -> shutdown requested
  -> stop WebSocket
  -> stop HTTP
  -> stop metrics exporter path
  -> join metrics thread
```

The current minimal exporter is intended as a simple metrics endpoint.

Advanced applications may expose metrics from the main HTTP app instead.

## Shutdown with message store

Message stores should outlive callbacks that use them.

Example:

```cpp
vix::websocket::SqliteMessageStore store{"messages.db"};

ws.on_typed_message(
  [&store](vix::websocket::Session &session,
           const std::string &type,
           const vix::json::kvs &payload)
  {
    (void)session;
    (void)type;
    (void)payload;

    // store is still alive while callback can run
  });
```

During shutdown, make sure no callback can use the store after it has been destroyed.

The easiest rule is:

```txt
create store before server starts
destroy store after server stops
```

## Recommended object lifetime

A safe object lifetime order is:

```txt
create config
create executor
create message store
create metrics
create long-polling bridge
create WebSocket server
register callbacks
start server
stop server
stop executor
destroy bridge
destroy metrics
destroy store
```

For attached runtime:

```txt
create app
create executor
create stores/metrics/bridges
create WebSocket server
register callbacks
create AttachedRuntime
run app
AttachedRuntime finalizes shutdown
destroy stores/metrics/bridges
```

## Signal handling model

A good signal handling model is:

```txt
signal received
  -> set atomic stop flag
  -> request non-blocking stop
  -> main control path notices flag
  -> finalize blocking shutdown
```

Avoid doing heavy shutdown directly inside a low-level signal handler.

## Example: manual shutdown flag

```cpp
#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

#include <vix/config/Config.hpp>
#include <vix/executor/RuntimeExecutor.hpp>
#include <vix/websocket.hpp>

int main()
{
  std::atomic<bool> stopRequested{false};

  vix::config::Config config{".env"};

  auto executor =
      std::make_shared<vix::executor::RuntimeExecutor>(4);

  vix::websocket::Server ws{config, executor};

  ws.on_message(
    [](vix::websocket::Session &session, const std::string &message)
    {
      session.send_text("echo: " + message);
    });

  ws.start();

  while (!stopRequested.load())
  {
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
  }

  ws.stop();
  executor->stop();

  return 0;
}
```

This example shows the control-path shape.

The code that sets `stopRequested` depends on your application.

## Example: close one client on command

```cpp
ws.on_message(
  [](vix::websocket::Session &session, const std::string &message)
  {
    if (message == "close")
    {
      session.close("client requested close");
      return;
    }

    session.send_text("echo: " + message);
  });
```

Use this when one client should be disconnected without stopping the server.

## Example: stop server from an external control path

```cpp
void stop_websocket_server(
    vix::websocket::Server &ws,
    std::shared_ptr<vix::executor::RuntimeExecutor> executor)
{
  ws.stop();

  if (executor)
  {
    executor->stop();
  }
}
```

Call this from a safe path that can block.

## Example: request attached stop

```cpp
void request_shutdown(vix::websocket::AttachedRuntime &runtime)
{
  runtime.request_stop();
}
```

This requests shutdown without blocking.

## Example: finalize attached shutdown

```cpp
void finalize_shutdown(vix::websocket::AttachedRuntime &runtime)
{
  runtime.finalize_shutdown();
}
```

Call this from a path where blocking cleanup is safe.

## Shutdown and executor

The executor should stop after the WebSocket server is stopped.

Recommended order:

```txt
ws.stop()
executor.stop()
```

This lets server cleanup happen before the shared executor is stopped.

`AttachedRuntime::finalize_shutdown()` follows this order.

## Shutdown and callbacks

Callbacks may still run while sessions are closing.

Make sure captured references remain valid.

Avoid capturing references to short-lived objects.

Good:

```cpp
auto sharedState = std::make_shared<MyState>();

ws.on_message([sharedState](vix::websocket::Session &session,
                            const std::string &message)
{
  (void)session;
  (void)message;

  sharedState->count++;
});
```

Risky:

```cpp
MyState state;

ws.on_message([&state](vix::websocket::Session &session,
                       const std::string &message)
{
  (void)session;
  (void)message;

  state.count++;
});
```

Reference captures are fine only when the referenced object clearly outlives the server.

## Shutdown and blocking work

Avoid blocking inside:

- `on_open`
- `on_message`
- `on_close`
- `on_error`
- HTTP shutdown callbacks
- low-level server callbacks

If heavy work is needed, schedule it through a safe application service or runtime path.

## Common shutdown mistakes

### Calling stop from inside a shutdown callback

Avoid blocking final shutdown inside a callback.

Prefer:

```cpp
ws.stop_async();
```

or:

```cpp
runtime.request_stop();
```

Then finalize later.

### Destroying dependencies too early

Avoid destroying a store, bridge, or metrics object while callbacks may still use it.

Stop the server first.

### Stopping executor too early

Avoid:

```cpp
executor->stop();
ws.stop();
```

Prefer:

```cpp
ws.stop();
executor->stop();
```

### Using shutdown_now for normal app logic

Avoid:

```cpp
session.shutdown_now();
```

for normal user disconnects.

Prefer:

```cpp
session.close("reason");
```

### Forgetting to join threads

For standalone usage, make sure final shutdown happens.

```cpp
ws.stop();
```

`stop_async()` alone only requests shutdown.

## Best practices

Use `session.close(...)` for one-client application closes.

Use `ws.stop_async()` for non-blocking server stop requests.

Use `ws.stop()` for final standalone cleanup.

Use `runtime.request_stop()` in attached non-blocking paths.

Use `runtime.finalize_shutdown()` for attached final cleanup.

Stop WebSocket before stopping the executor.

Keep stores, bridges, and metrics alive until after server shutdown.

Avoid blocking inside callbacks.

Make shutdown idempotent.

## Next steps

Continue with:

- [API Reference](./api-reference.md)
- [Attached runtime](./attached-runtime.md)
- [Server](./server.md)
- [Session](./session.md)
- [Metrics](./metrics.md)
