# Attached runtime

This page explains `vix::websocket::AttachedRuntime`.

Use it when you want to run an HTTP app and a WebSocket server together with one coordinated lifecycle.

## Header

```cpp
#include <vix/websocket/AttachedRuntime.hpp>
```

Or use the umbrella header:

```cpp
#include <vix/websocket.hpp>
```

## What AttachedRuntime provides

`AttachedRuntime` connects a `vix::App` with a `vix::websocket::Server`.

It provides:

- one HTTP app
- one WebSocket server
- one shared `RuntimeExecutor`
- WebSocket startup
- HTTP shutdown coordination
- non-blocking WebSocket stop from HTTP shutdown callbacks
- final blocking shutdown from a safe control path
- defensive cleanup in the destructor

The goal is to make HTTP and WebSocket run as one application without forcing the user to manually coordinate every lifecycle step.

## Basic model

```txt
vix::App
  -> HTTP server
  -> routes
  -> middleware
  -> static files

vix::websocket::Server
  -> WebSocket listener
  -> sessions
  -> rooms
  -> messages

AttachedRuntime
  -> starts WebSocket
  -> connects shutdown lifecycle
  -> stops WebSocket safely
  -> stops shared RuntimeExecutor
```

## Why AttachedRuntime exists

HTTP and WebSocket often belong to the same application, but they have different server loops.

Without `AttachedRuntime`, the user would need to manually coordinate:

```txt
create HTTP app
create WebSocket server
share RuntimeExecutor
start WebSocket server
start HTTP app
handle HTTP shutdown
stop WebSocket without blocking callbacks
join WebSocket threads
stop executor
avoid double shutdown
```

`AttachedRuntime` centralizes this lifecycle.

## Constructor

```cpp
vix::websocket::AttachedRuntime runtime{
    app,
    ws,
    executor};
```

The constructor receives:

| Argument   | Purpose                                     |
| ---------- | ------------------------------------------- |
| `app`      | Existing `vix::App` instance.               |
| `ws`       | Existing `vix::websocket::Server` instance. |
| `executor` | Shared runtime executor.                    |

The constructor starts the WebSocket server immediately and registers a shutdown callback on the HTTP app.

## Minimal example

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

  app.get("/", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.text("HTTP and WebSocket are running");
  });

  app.run(8080);

  return 0;
}
```

HTTP runs on the normal HTTP server port.

WebSocket runs on the configured WebSocket port.

Example:

```txt
HTTP:      http://localhost:8080
WebSocket: ws://localhost:9090
```

## Startup flow

When `AttachedRuntime` is created, it starts the WebSocket server.

```txt
create app
  -> create shared executor
  -> create WebSocket server
  -> register WebSocket callbacks
  -> create AttachedRuntime
  -> AttachedRuntime starts WebSocket server
  -> app.run starts HTTP server
```

Conceptually:

```txt
AttachedRuntime constructor
  -> show wait banner
  -> ws.start()
  -> app.set_shutdown_callback(...)
```

The WebSocket server starts before the HTTP app enters its blocking run loop.

## Shutdown flow

Shutdown is intentionally split into two phases.

First, the HTTP app shutdown callback requests a non-blocking WebSocket stop.

```txt
HTTP shutdown callback
  -> AttachedRuntime state
  -> stop_requested = true
  -> ws.stop_async()
```

Then final shutdown happens from a safe control path.

```txt
AttachedRuntime::finalize_shutdown()
  -> ws.stop()
  -> executor.stop()
```

This prevents blocking inside HTTP shutdown callbacks.

## Important lifecycle rule

The key rule is:

```txt
The HTTP shutdown callback only requests an asynchronous WebSocket stop.
The final blocking shutdown happens later from a safe control path.
```

Do this:

```cpp
runtime.request_stop();
```

Then finalize later:

```cpp
runtime.finalize_shutdown();
```

Do not perform a blocking WebSocket shutdown directly inside an HTTP shutdown callback.

## request_stop

Use `request_stop()` to request a non-blocking WebSocket shutdown.

```cpp
runtime.request_stop();
```

This function is:

- non-blocking
- idempotent
- safe to call more than once
- safe to call from shutdown callbacks

Internally, it calls:

```cpp
ws.stop_async();
```

Use it when shutdown is being requested from a path that must not block.

## finalize_shutdown

Use `finalize_shutdown()` to perform final blocking cleanup.

```cpp
runtime.finalize_shutdown();
```

Finalization order:

```txt
1. stop WebSocket server
2. stop shared RuntimeExecutor
```

This function is protected so it runs only once.

It is safe to call multiple times.

## Destructor behavior

The destructor calls `finalize_shutdown()` defensively.

```txt
~AttachedRuntime
  -> finalize_shutdown()
```

This means the runtime tries to clean up even if the user forgets to call finalization manually.

Still, explicit finalization is clearer in advanced lifecycle code.

## Shared shutdown state

`AttachedRuntime` stores shutdown state separately from the object itself.

Conceptually:

```txt
State
  -> stop_requested
  -> finalized
  -> finalize_mutex
```

The HTTP shutdown callback captures this shared state instead of capturing the full `AttachedRuntime` object.

This avoids unsafe lifetime assumptions.

## Why stop_async exists

`ws.stop_async()` only requests shutdown.

It does not join internal threads.

This is important because shutdown callbacks may run from internal server paths where blocking could cause deadlocks.

```txt
safe callback behavior
  -> request stop
  -> return quickly
```

## Why stop exists

`ws.stop()` performs final blocking shutdown.

It can join threads and finish cleanup.

This should happen from a safe control path.

```txt
safe finalization path
  -> ws.stop()
  -> executor.stop()
```

## AttachedRuntime with routes

You can register HTTP routes normally.

```cpp
app.get("/status", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.json({
    "status", "ok"
  });
});
```

And register WebSocket callbacks normally.

```cpp
ws.on_message(
  [](vix::websocket::Session &session, const std::string &message)
  {
    session.send_text("echo: " + message);
  });
```

`AttachedRuntime` only coordinates lifecycle.

It does not replace normal route registration.

## AttachedRuntime with typed messages

```cpp
ws.on_typed_message(
  [](vix::websocket::Session &session,
     const std::string &type,
     const vix::json::kvs &payload)
  {
    (void)payload;

    if (type == "ping")
    {
      session.send_text("pong");
    }
  });
```

Then create the attached runtime:

```cpp
vix::websocket::AttachedRuntime runtime{app, ws, executor};
```

## AttachedRuntime with rooms

```cpp
ws.on_open([&ws](vix::websocket::Session &session)
{
  ws.join_room(session.shared_from_this(), "general");
});

ws.on_message(
  [&ws](vix::websocket::Session &session, const std::string &message)
  {
    (void)session;

    ws.broadcast_text_to_room("general", "room: " + message);
  });
```

`AttachedRuntime` does not change how rooms work.

It only runs the WebSocket server beside the HTTP app.

## AttachedRuntime with long-polling

You can attach a long-polling bridge to the WebSocket server before creating the runtime.

```cpp
vix::websocket::WebSocketMetrics metrics;

vix::websocket::LongPollingBridge bridge{
    &metrics,
    std::chrono::seconds{60},
    256};

ws.attach_long_polling_bridge(&bridge);

vix::websocket::AttachedRuntime runtime{app, ws, executor};
```

The flow becomes:

```txt
WebSocket message
  -> Server parses JsonMessage
  -> LongPollingBridge
  -> HTTP fallback clients can poll messages
```

## AttachedRuntime with metrics

Metrics can be shared with WebSocket and long-polling components.

```cpp
vix::websocket::WebSocketMetrics metrics;
```

You can expose them through a dedicated metrics exporter:

```cpp
vix::websocket::run_metrics_http_exporter(
    metrics,
    "0.0.0.0",
    9100);
```

Then metrics are available at:

```txt
http://localhost:9100/metrics
```

## HTTP and WebSocket ports

The HTTP server and the WebSocket server use separate ports.

Example `.env`:

```dotenv
SERVER_PORT=8080

WEBSOCKET_HOST=0.0.0.0
WEBSOCKET_PORT=9090
```

Then clients connect to:

```txt
HTTP:      http://localhost:8080
WebSocket: ws://localhost:9090
```

## Full example

```cpp
#include <chrono>
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

  vix::websocket::WebSocketMetrics metrics;

  vix::websocket::LongPollingBridge bridge{
      &metrics,
      std::chrono::seconds{60},
      256};

  ws.attach_long_polling_bridge(&bridge);

  ws.on_open([](vix::websocket::Session &session)
  {
    session.send_text("welcome");
  });

  ws.on_message(
    [](vix::websocket::Session &session, const std::string &message)
    {
      session.send_text("echo: " + message);
    });

  ws.on_typed_message(
    [&ws](vix::websocket::Session &session,
          const std::string &type,
          const vix::json::kvs &payload)
    {
      (void)payload;

      if (type == "room.join")
      {
        ws.join_room(session.shared_from_this(), "general");
        session.send_text("joined general");
        return;
      }

      if (type == "chat.message")
      {
        ws.broadcast_text_to_room("general", "new chat message");
        return;
      }
    });

  vix::websocket::AttachedRuntime runtime{app, ws, executor};

  app.get("/", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.text("Vix HTTP + WebSocket runtime");
  });

  app.get("/status", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.json({
      "status", "ok",
      "http", true,
      "websocket", true
    });
  });

  app.run(8080);

  return 0;
}
```

## Recommended order

Create the objects in this order:

```txt
1. Create vix::App
2. Create shared RuntimeExecutor
3. Create vix::websocket::Server
4. Register WebSocket callbacks
5. Attach bridges or metrics if needed
6. Create AttachedRuntime
7. Register HTTP routes
8. Run the HTTP app
```

Example:

```cpp
vix::App app;

auto executor =
    std::make_shared<vix::executor::RuntimeExecutor>(4);

vix::websocket::Server ws{app.config(), executor};

ws.on_message(...);

vix::websocket::AttachedRuntime runtime{app, ws, executor};

app.run(8080);
```

## When to use AttachedRuntime

Use `AttachedRuntime` when:

- the same application needs HTTP and WebSocket
- WebSocket should share the app executor
- shutdown should be coordinated
- you want one lifecycle model
- you want to avoid manual server coordination

It is the recommended model for full Vix applications that combine REST APIs, frontend routes, and realtime behavior.

## When not to use AttachedRuntime

You may not need `AttachedRuntime` when:

- you only need a standalone WebSocket server
- you are testing the WebSocket module in isolation
- you are building a low-level integration
- you want to control every lifecycle step manually

For standalone WebSocket usage, use `vix::websocket::Server` directly.

## Standalone WebSocket alternative

```cpp
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
```

## Best practices

Use one shared executor for HTTP and WebSocket.

Register WebSocket callbacks before creating `AttachedRuntime`.

Use `request_stop()` for non-blocking shutdown requests.

Use `finalize_shutdown()` for final blocking cleanup.

Do not block inside HTTP shutdown callbacks.

Keep WebSocket and HTTP ports clear in `.env`.

Use long-polling bridge only when fallback delivery is needed.

Use metrics for production services.

## Common mistakes

### Creating AttachedRuntime before configuring WebSocket callbacks

Avoid:

```cpp
vix::websocket::AttachedRuntime runtime{app, ws, executor};

ws.on_message(...);
```

Prefer:

```cpp
ws.on_message(...);

vix::websocket::AttachedRuntime runtime{app, ws, executor};
```

### Blocking inside shutdown callback

Do not call blocking shutdown from inside the HTTP shutdown callback.

Prefer non-blocking stop first:

```cpp
runtime.request_stop();
```

Then final cleanup later:

```cpp
runtime.finalize_shutdown();
```

### Using different executors accidentally

Avoid creating unrelated executors for HTTP and WebSocket.

Prefer one shared executor:

```cpp
auto executor =
    std::make_shared<vix::executor::RuntimeExecutor>(4);
```

### Forgetting WebSocket port config

Make sure the WebSocket port is configured.

```dotenv
WEBSOCKET_PORT=9090
```

### Treating AttachedRuntime as a router

`AttachedRuntime` is not a routing API.

Use `vix::App` for HTTP routes.

Use `vix::websocket::Server` for WebSocket callbacks.

## Next steps

Continue with:

- [Metrics](./metrics.md)
- [Long polling](./long-polling.md)
- [Shutdown](./shutdown.md)
- [Server](./server.md)
- [OpenAPI](./openapi.md)
