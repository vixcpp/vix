# Architecture

This page explains the architecture of Vix Core.

Use it when you want to understand how `vix::App`, the HTTP server, router, sessions, transports, `vix::async`, and `vix::runtime` work together.

## Public header

```cpp
#include <vix.hpp>
```

You can also include the core header directly:

```cpp
#include <vix/core.hpp>
```

## What Core architecture provides

Vix Core is the native application layer of Vix.

It connects:

- `vix::App`
- `vix::router::Router`
- `vix::server::HTTPServer`
- `vix::session::Session`
- `vix::session::Transport`
- `vix::executor::RuntimeExecutor`
- `vix::async`
- `vix::runtime`

The goal is to keep the HTTP application model simple while separating network I/O from application execution.

## High-level model

A Vix Core application starts with `vix::App`.

```cpp
vix::App app;
```

The app owns or coordinates the main runtime pieces.

```text
App
  -> Config
  -> Router
  -> RuntimeExecutor
  -> HTTPServer
```

The HTTP server owns the async I/O context.

```text
HTTPServer
  -> io_context
  -> TCP listener
  -> I/O threads
  -> accept loop
```

The session owns one client connection.

```text
Session
  -> Transport
  -> Request parser
  -> Router dispatch
  -> Response writer
```

The executor adapts application work to the internal runtime.

```text
RuntimeExecutor
  -> vix::runtime::Runtime
  -> Scheduler
  -> Workers
  -> Run queues
```

## Main layers

Vix Core is organized into layers.

```text
Application layer
  -> App
  -> route registration
  -> middleware
  -> static files
  -> templates

HTTP layer
  -> HTTPServer
  -> Session
  -> Request
  -> Response
  -> Router

Transport layer
  -> Transport
  -> PlainTransport
  -> TlsTransport

Async I/O layer
  -> vix::async::core::io_context
  -> vix::async::net::tcp_listener
  -> vix::async::net::tcp_stream
  -> timers
  -> cancellation

Runtime execution layer
  -> RuntimeExecutor
  -> vix::runtime::Runtime
  -> Scheduler
  -> Worker
  -> Task
```

## Request flow

A typical request moves through the system like this:

```text
client
  -> TCP connection
  -> HTTPServer accept loop
  -> Session
  -> Transport read
  -> HTTP request parser
  -> Request
  -> Router
  -> RequestHandler
  -> user handler
  -> Response
  -> Transport write
  -> client
```

In code, the user usually sees only this:

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

Internally, Core connects that handler to the server, router, session, transport, async I/O, and runtime execution model.

## App layer

`vix::App` is the main developer-facing object.

It is responsible for:

- route registration
- middleware registration
- route groups
- static files
- templates
- server startup
- server shutdown
- access to config, router, server, and executor

Example:

```cpp
vix::App app;

app.get("/", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.text("home");
});

app.run(8080);
```

`App` hides the lower-level server setup.

The user does not need to manually create the router, HTTP server, sessions, or async context.

## Router layer

The router maps HTTP methods and paths to handlers.

```text
GET /users/{id}
POST /users
DELETE /users/{id}
```

A route is registered through the app:

```cpp
app.get("/users/{id}", [](vix::Request &req, vix::Response &res)
{
  res.json({
    "id", req.param("id")
  });
});
```

Internally, the route is stored in a route tree.

```text
Router
  -> RouteNode
  -> handler
  -> route metadata
```

The router supports:

- static paths
- parameter paths
- `HEAD` fallback to `GET`
- `OPTIONS` responses
- custom not-found handler
- route documentation metadata
- heavy route metadata

## Handler layer

User handlers are adapted into a common internal interface.

The public handler looks like this:

```cpp
[](vix::Request &req, vix::Response &res)
{
  res.text("OK");
}
```

Internally, Core wraps it into a request handler object.

```text
user handler
  -> RequestHandler
  -> IRequestHandler
  -> Router
```

This keeps the router independent from the exact user handler type.

## Middleware layer

Middleware runs before the final route handler.

```cpp
app.use([](vix::Request &req, vix::Response &res, vix::App::Next next)
{
  vix::print(req.method(), req.path());

  next();
});
```

The middleware chain is collected when a route is registered.

```text
route path
  -> matching middleware prefixes
  -> middleware chain
  -> final handler
```

A middleware can continue:

```cpp
next();
```

Or stop the request early:

```cpp
res.status(401).json({
  "error", "unauthorized"
});
```

## HTTP server layer

`HTTPServer` is the native async HTTP server.

It is responsible for:

- creating the async I/O context
- creating the TCP listener
- launching I/O threads
- accepting client connections
- creating sessions
- stopping the server cleanly

The server runs the native async context on I/O threads.

```text
HTTPServer
  -> io_context
  -> io_thread_0
  -> io_thread_1
  -> io_thread_N
```

The accept loop waits for TCP connections.

```text
accept connection
  -> create Session or TlsSession
  -> run session coroutine
```

## Session layer

A session represents one client connection.

It is responsible for:

- reading bytes from the transport
- parsing HTTP request headers
- reading the request body
- building `vix::http::Request`
- dispatching the request to the router
- writing `vix::http::Response`
- handling keep-alive or close behavior
- closing the connection safely

Simplified flow:

```text
Session::run
  -> read_request
  -> dispatch_request
  -> send_response
  -> repeat while connection is open
```

A session can handle multiple requests on the same connection when keep-alive is active.

## Transport layer

The session does not directly depend on plain TCP or TLS.

It depends on the abstract `Transport` interface.

```text
Transport
  -> async_read
  -> async_write
  -> is_open
  -> close
```

There are two main transport implementations:

```text
PlainTransport
  -> normal TCP stream

TlsTransport
  -> encrypted TLS stream
```

This means the HTTP parser and response writer do not need to know whether the connection is plain HTTP or HTTPS.

## TLS layer

TLS is optional.

When TLS is disabled:

```text
TCP stream
  -> PlainTransport
  -> Session
```

When TLS is enabled:

```text
TCP stream
  -> TlsTransport
  -> TLS handshake
  -> Session
```

The HTTP session remains the same after the transport is ready.

This keeps TLS separate from HTTP parsing.

## Request object

`vix::Request` is the native request object.

It contains:

- method
- target
- path
- query string
- headers
- body
- route parameters
- request state

Example:

```cpp
app.get("/users/{id}", [](vix::Request &req, vix::Response &res)
{
  const std::string id = req.param("id");
  const std::string page = req.query_value("page", "1");

  res.json({
    "id", id,
    "page", page
  });
});
```

## Response object

`vix::Response` is the public response helper.

It wraps the native response object and provides convenient methods.

```cpp
res.status(200);
res.text("OK");
res.json({"ok", true});
res.redirect("/login");
res.file("public/index.html");
res.render("index.html", ctx);
```

The native response is later serialized by the session and written to the transport.

## Async I/O model

Vix Core uses `vix::async` for network I/O.

`vix::async` handles:

- TCP listening
- TCP accept
- TCP read
- TCP write
- timers
- cancellation
- coroutine tasks
- detached async tasks

The HTTP server owns an async context.

```cpp
vix::async::core::io_context
```

That context is used to run the server accept loop and session coroutines.

```text
io_context
  -> accept_loop
  -> handle_client
  -> session.run
  -> read/write/timer operations
```

## Runtime execution model

Vix Core uses `vix::runtime` through `RuntimeExecutor`.

`RuntimeExecutor` is the bridge between Core and the lower-level runtime.

```text
RuntimeExecutor
  -> Runtime
  -> Scheduler
  -> Worker
  -> Task
```

The runtime is designed for lightweight task execution.

It provides:

- worker threads
- task submission
- task metrics
- work scheduling
- work stealing
- task yielding
- runtime shutdown

## Why Core uses both async and runtime

`vix::async` and `vix::runtime` solve different problems.

```text
vix::async
  -> waits for I/O
  -> resumes coroutines
  -> handles timers and cancellation
  -> keeps network operations non-blocking

vix::runtime
  -> executes internal tasks
  -> schedules work across workers
  -> tracks execution metrics
  -> supports lightweight yielding
```

Core uses both because an HTTP server needs both models.

```text
network I/O
  -> vix::async

application work
  -> RuntimeExecutor / vix::runtime
```

This separation prevents the I/O model from becoming mixed with the application execution model.

## Thread model

A Vix Core application can involve several kinds of threads.

```text
main thread
  -> constructs App
  -> calls run or listen

server thread
  -> launched by App::listen
  -> runs HTTPServer::run

I/O threads
  -> run io_context
  -> process async network work

runtime workers
  -> owned by RuntimeExecutor
  -> execute runtime tasks
```

The simplified model is:

```text
App thread
  -> starts server

I/O threads
  -> accept, read, write

Runtime workers
  -> execute scheduled application work
```

## Startup flow

When an app starts, the flow is:

```text
App constructor
  -> create Config
  -> create RuntimeExecutor
  -> start executor
  -> create HTTPServer
  -> get Router from server
  -> install not-found handler
  -> install docs routes if enabled
  -> install access logs
  -> register /bench route

app.run(port)
  -> listen(port)
  -> wait()
  -> close()
```

With `listen(...)`, the server starts asynchronously.

```text
app.listen(port)
  -> set port
  -> install signal handlers
  -> start server thread
  -> HTTPServer::run
  -> start I/O threads
  -> start async server coroutine
  -> start accept loop
```

## Shutdown flow

The shutdown flow is designed to be idempotent.

```text
App::close
  -> mark stop requested
  -> run shutdown callback
  -> stop HTTP server async
  -> stop HTTP server blocking
  -> join server thread
  -> mark app stopped
```

The HTTP server shutdown flow is:

```text
HTTPServer::stop_async
  -> request stop
  -> close listener
  -> stop io_context
  -> wake monitor thread

HTTPServer::stop_blocking
  -> stop async
  -> join internal threads
```

The runtime executor shutdown flow is:

```text
RuntimeExecutor::stop
  -> stop accepting work
  -> stop runtime
  -> join runtime workers
```

## Static files and not-found fallback

Static files are integrated into the not-found path.

That means Core first tries normal routing.

```text
request path
  -> Router
  -> route found?
```

If no route matches, the app-level not-found handler can try static files.

```text
not found
  -> try static file mounts
  -> if found, send file
  -> otherwise send JSON 404
```

Example:

```cpp
app.static_dir("public", "/assets");
```

If `/assets/app.css` has no route, Core can still serve:

```text
public/app.css
```

## Template architecture

Templates are configured at the app level.

```cpp
app.templates("views");
```

This initializes:

```text
FileSystemLoader
  -> Template Engine
  -> TemplateView
  -> ResponseWrapper::render
```

Route handlers can then render views.

```cpp
app.get("/", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  vix::tmpl::Context ctx;
  ctx.set("title", "Home");

  res.render("index.html", ctx);
});
```

## Configuration architecture

The app owns a `Config`.

```cpp
vix::App app;

app.config().setServerPort(8080);
```

The configuration is passed into the HTTP server and sessions.

It controls:

- server port
- I/O thread count
- session timeout
- benchmark mode
- WAF settings
- TLS settings
- logging settings

## Heavy route architecture

Some routes can be marked as heavy.

```cpp
app.get_heavy("/reports", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.json({"status", "ready"});
});
```

Heavy routes are stored as route metadata.

```text
RouteOptions
  -> heavy = true

RouteNode
  -> heavy = true

RouteRecord
  -> heavy = true
```

This lets the router and runtime model identify costly routes.

## Internal separation

Core keeps these responsibilities separate:

| Layer             | Responsibility                                                |
| ----------------- | ------------------------------------------------------------- |
| `App`             | Developer-facing application facade.                          |
| `Router`          | Match method and path to handlers.                            |
| `RequestHandler`  | Adapt user handlers to the router interface.                  |
| `HTTPServer`      | Own I/O context, listener, accept loop, and server lifecycle. |
| `Session`         | Parse requests and write responses for one connection.        |
| `Transport`       | Abstract plain TCP and TLS.                                   |
| `RuntimeExecutor` | Bridge application work to `vix::runtime`.                    |
| `vix::async`      | Network I/O, timers, cancellation, coroutines.                |
| `vix::runtime`    | Lightweight internal task execution.                          |

## Minimal mental model

When writing an app, think like this:

```text
App registers routes.
HTTPServer accepts connections.
Session parses requests.
Router finds a handler.
RequestHandler runs user code.
Response is serialized back to the client.
```

When thinking about performance, think like this:

```text
vix::async keeps I/O moving.
vix::runtime executes scheduled work.
Core connects both into one HTTP application model.
```

## Next steps

Read the next pages:

- [Routing](./routing.md)
- [Handlers](./handlers.md)
- [Middleware](./middleware.md)
- [HTTP server](./http-server.md)
- [Sessions](./sessions.md)
- [Runtime executor](./runtime-executor.md)
- [Async and runtime](./async-and-runtime.md)
