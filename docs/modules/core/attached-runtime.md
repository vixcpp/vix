# Attached runtime

This page explains the attached runtime model used to run HTTP and WebSocket together in Vix.

Use it when you want to understand how a Vix HTTP app, a WebSocket server, a shared runtime executor, OpenAPI docs, and shutdown lifecycle are connected.

## Public header

```cpp
#include <vix/websocket/AttachedRuntime.hpp>
```

## What AttachedRuntime provides

`AttachedRuntime` connects a Vix HTTP app with a Vix WebSocket server.

It provides:

- one HTTP application
- one WebSocket server
- one shared `RuntimeExecutor`
- shared shutdown coordination
- WebSocket server startup
- non-blocking WebSocket stop from the HTTP shutdown callback
- final blocking shutdown from a safe control path
- OpenAPI documentation registration for HTTP and WebSocket routes
- a combined runtime banner showing HTTP and WebSocket information

Even though the class lives under `vix::websocket`, it belongs to the Core architecture because it connects the HTTP core with the WebSocket runtime model.

## Basic model

The attached runtime model is:

```text
App
  -> HTTPServer
  -> Router
  -> RuntimeExecutor

WebSocket Server
  -> same RuntimeExecutor

AttachedRuntime
  -> starts WebSocket server
  -> connects shutdown lifecycle
  -> finalizes shutdown safely
```

The shared executor is important:

```text
HTTP and WebSocket use the same runtime execution foundation.
```

## Why this exists

HTTP and WebSocket have different server loops, but they often belong to the same application.

Without an attached runtime, the user would need to manually coordinate:

```text
start HTTP
start WebSocket
share executor
register docs
show runtime banner
wait for shutdown
stop WebSocket
stop HTTP
stop executor
avoid blocking inside callbacks
```

`AttachedRuntime` centralizes that lifecycle.

## AttachedRuntime

`vix::websocket::AttachedRuntime` attaches a WebSocket server to an existing HTTP app.

```cpp
vix::websocket::AttachedRuntime runtime{app, ws, exec};
```

Construction does two important things:

```text
1. starts the WebSocket server
2. registers an HTTP shutdown callback
```

The shutdown callback requests a non-blocking WebSocket stop.

This is important because shutdown callbacks may run from a server worker or internal server path.

## Lifecycle rule

The key rule is:

```text
The HTTP shutdown callback only requests an asynchronous WebSocket stop.
The final blocking shutdown happens from an external safe control path.
```

This avoids deadlocks.

The callback calls:

```cpp
ws.stop_async();
```

The final control path calls:

```cpp
ws.stop();
exec->stop();
```

## Startup flow

When `AttachedRuntime` is created:

```text
AttachedRuntime constructor
  -> show wait banner
  -> start WebSocket server
  -> register HTTP shutdown callback
```

Simplified:

```text
HTTP app exists
WebSocket server exists
shared executor exists
  -> AttachedRuntime starts WebSocket
  -> HTTP app starts later
```

## Shutdown flow

The shutdown flow is intentionally split.

First, the HTTP app requests shutdown:

```text
App shutdown callback
  -> AttachedRuntime state
  -> stop_requested = true
  -> ws.stop_async()
```

Then final shutdown happens outside the callback:

```text
runtime.finalize_shutdown()
  -> ws.stop()
  -> exec.stop()
```

This makes shutdown:

- idempotent
- safe
- non-blocking in callbacks
- blocking only from the final control path

## Shared shutdown state

`AttachedRuntime` stores shutdown state separately from the object itself.

```text
State
  -> stop_requested
  -> finalized
  -> finalize_mutex
```

This avoids capturing the `AttachedRuntime` instance directly inside the HTTP shutdown callback.

That is safer because callbacks can outlive the original call stack.

## request_stop

`request_stop()` asks the WebSocket server to stop asynchronously.

```cpp
runtime.request_stop();
```

It is:

- non-blocking
- idempotent
- safe to call more than once
- safe to call from shutdown paths

Internally it calls:

```cpp
ws.stop_async();
```

## finalize_shutdown

`finalize_shutdown()` performs the final blocking shutdown exactly once.

```cpp
runtime.finalize_shutdown();
```

Finalization order:

```text
1. stop WebSocket server blocking
2. stop shared RuntimeExecutor
```

It is protected by:

```text
finalized atomic flag
finalize_mutex
```

This prevents double shutdown.

## Destructor behavior

The destructor calls `finalize_shutdown()` defensively.

```text
~AttachedRuntime
  -> finalize_shutdown
```

This makes cleanup safer if the user forgets to call finalization manually.

Still, the normal combined runtime helpers call it explicitly.

## register_ws_openapi_docs_once

`register_ws_openapi_docs_once()` registers WebSocket OpenAPI docs once per process.

```cpp
vix::register_ws_openapi_docs_once();
```

It uses `std::call_once`.

That prevents duplicate WebSocket OpenAPI route documentation.

The registered docs describe routes such as:

```text
/ws
/ws/poll
/ws/send
/metrics
```

## run_http_and_ws

Use `run_http_and_ws(...)` to run HTTP and WebSocket together.

```cpp
vix::run_http_and_ws(app, ws, exec, cfg);
```

It performs the combined lifecycle:

```text
register WebSocket OpenAPI docs once
register OpenAPI and Swagger UI routes on the HTTP router
create AttachedRuntime
start HTTP app
wait for shutdown
close HTTP app
finalize WebSocket and executor shutdown
```

## run_http_and_ws with port

There is also a port overload:

```cpp
vix::run_http_and_ws(app, ws, exec, 8080);
```

It creates a default config, sets the HTTP port, then runs the config-based version.

## Combined docs registration

The combined runner registers:

```text
WebSocket OpenAPI docs
HTTP OpenAPI docs route
Swagger UI docs route
```

That means the combined app can expose:

```text
/openapi.json
/docs
/docs/
/docs/index.html
/docs/swagger-ui.css
/docs/swagger-ui-bundle.js
```

The OpenAPI document can include both HTTP and WebSocket-related docs.

## Runtime banner

When HTTP starts, the combined runner updates the ready information to include WebSocket data.

```text
show_ws = true
ws_scheme = ws or wss
ws_host = localhost
ws_port = websocket server port
ws_path = /
```

If TLS is enabled in the HTTP config, the WebSocket scheme becomes:

```text
wss
```

Otherwise it becomes:

```text
ws
```

## serve_http_and_ws

`serve_http_and_ws(...)` is the high-level helper.

It creates:

```text
Config
RuntimeExecutor
App
WebSocket Server
```

Then it lets the caller configure both HTTP and WebSocket.

```cpp
vix::serve_http_and_ws([](vix::App &app, vix::websocket::Server &ws)
{
  app.get("/", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.text("HTTP + WebSocket");
  });

  (void)ws;
});
```

## serve_http_and_ws with config path

Use this overload when you want an explicit config file and port.

```cpp
vix::serve_http_and_ws(".env", 8080, [](vix::App &app, vix::websocket::Server &ws)
{
  app.get("/", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.text("combined runtime");
  });

  (void)ws;
});
```

## Default serve_http_and_ws

The default overload uses:

```text
config path = .env
HTTP port   = 8080
```

```cpp
vix::serve_http_and_ws([](vix::App &app, vix::websocket::Server &ws)
{
  (void)ws;

  app.get("/", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.text("hello");
  });
});
```

## Architecture

The attached runtime architecture is:

```text
                   shared RuntimeExecutor
                         /        \
                        /          \
                       v            v
                HTTP App        WebSocket Server
                    |                 |
                    v                 v
              HTTPServer        WebSocket runtime
                    |
                    v
                 Router
                    |
                    v
              HTTP handlers
```

`AttachedRuntime` coordinates the lifecycle between both sides.

```text
AttachedRuntime
  -> starts WebSocket
  -> listens to HTTP shutdown
  -> stops WebSocket async
  -> finalizes WebSocket blocking
  -> stops shared executor
```

## HTTP + WebSocket flow

A combined app can receive HTTP requests and WebSocket traffic at the same time.

```text
HTTP request
  -> HTTPServer
  -> Session
  -> Router
  -> Handler

WebSocket connection
  -> WebSocket Server
  -> WebSocket handlers

Both
  -> shared RuntimeExecutor
```

## TLS and WebSocket scheme

When TLS is enabled:

```text
HTTP  -> https
WS    -> wss
```

When TLS is disabled:

```text
HTTP  -> http
WS    -> ws
```

The combined runtime banner uses the HTTP config to select the WebSocket scheme.

## OpenAPI and WebSocket

WebSocket routes may not always exist as normal HTTP router routes.

That is why WebSocket docs are registered through the OpenAPI registry.

```text
register_ws_openapi_docs_once
  -> websocket docs
  -> OpenAPI Registry
  -> build_from_router
  -> /openapi.json
```

This lets `/openapi.json` include documentation for HTTP routes and WebSocket-related endpoints.

## Complete example

```cpp
#include <vix.hpp>
#include <vix/websocket/AttachedRuntime.hpp>
#include <vix/websocket/server.hpp>

int main()
{
  vix::serve_http_and_ws([](vix::App &app, vix::websocket::Server &ws)
  {
    app.get("/", [](vix::Request &req, vix::Response &res)
    {
      (void)req;

      res.text("HTTP and WebSocket are running together");
    });

    app.get("/api/status", [](vix::Request &req, vix::Response &res)
    {
      (void)req;

      res.json({
        "http", true,
        "websocket", true
      });
    });

    (void)ws;
  });

  return 0;
}
```

## Manual combined runtime example

```cpp
#include <vix.hpp>
#include <vix/websocket/AttachedRuntime.hpp>
#include <vix/websocket/server.hpp>

int main()
{
  vix::config::Config cfg;
  cfg.setServerPort(8080);

  auto exec = std::make_shared<vix::executor::RuntimeExecutor>(1u);

  vix::App app{exec};
  vix::websocket::Server ws{cfg, exec};

  app.get("/", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.text("manual HTTP + WebSocket runtime");
  });

  vix::run_http_and_ws(app, ws, exec, cfg);

  return 0;
}
```

## API summary

| API                                       | Purpose                                                      |
| ----------------------------------------- | ------------------------------------------------------------ |
| `vix::websocket::AttachedRuntime`         | Attach a WebSocket server to an HTTP app.                    |
| `AttachedRuntime(app, ws, exec)`          | Start WebSocket and register HTTP shutdown callback.         |
| `request_stop()`                          | Request non-blocking WebSocket shutdown.                     |
| `finalize_shutdown()`                     | Perform final blocking shutdown exactly once.                |
| `register_ws_openapi_docs_once()`         | Register WebSocket OpenAPI docs once per process.            |
| `run_http_and_ws(app, ws, exec, cfg)`     | Run HTTP and WebSocket together with a shared config.        |
| `run_http_and_ws(app, ws, exec, port)`    | Run HTTP and WebSocket together on a port.                   |
| `serve_http_and_ws(configPath, port, fn)` | Build and run HTTP + WebSocket from a config path.           |
| `serve_http_and_ws(fn)`                   | Build and run HTTP + WebSocket using `.env` and port `8080`. |

## Best practices

Use `serve_http_and_ws(...)` when you want the simplest combined HTTP + WebSocket setup.

```cpp
vix::serve_http_and_ws([](vix::App &app, vix::websocket::Server &ws)
{
  (void)ws;

  app.get("/", handler);
});
```

Use `run_http_and_ws(...)` when you already created the app, WebSocket server, executor, and config.

```cpp
vix::run_http_and_ws(app, ws, exec, cfg);
```

Use one shared executor for HTTP and WebSocket.

```cpp
auto exec = std::make_shared<vix::executor::RuntimeExecutor>(1u);
```

Do not perform blocking WebSocket shutdown inside the HTTP shutdown callback.

```text
Use stop_async in callbacks.
Use finalize_shutdown from the safe final control path.
```

Register docs once.

```cpp
vix::register_ws_openapi_docs_once();
```

## Next steps

Read the related pages:

- [Runtime executor](./runtime-executor.md)
- [HTTP server](./http-server.md)
- [Async and runtime](./async-and-runtime.md)
- [OpenAPI](./openapi.md)
- [WebSocket API](../../api/websocket.md)
