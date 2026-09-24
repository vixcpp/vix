# Core

`vix::core` is the native application layer of Vix.

Use it when you want to build HTTP applications with routes, handlers, middleware, request and response objects, templates, static files, configuration, TLS, and server lifecycle management.

## Public header

```cpp
#include <vix.hpp>
```

You can also include the core header directly:

```cpp
#include <vix/core.hpp>
```

## What Core provides

Vix Core connects the main pieces needed to build a native Vix web application.

It includes:

- `vix::App` for application setup and server lifecycle
- HTTP routing with `GET`, `POST`, `PUT`, `PATCH`, `DELETE`, `HEAD`, and `OPTIONS`
- route parameters such as `/users/{id}`
- middleware and protected route prefixes
- native `Request` and `Response` objects
- JSON, text, file, redirect, and template responses
- static file mounting
- HTTP sessions and transports
- optional TLS support
- configuration loading
- an HTTP server built on `vix::async`
- a runtime executor built on `vix::runtime`

## Basic idea

Most Core applications start with an `App`.

```cpp
vix::App app;
```

Then routes are registered on the app.

```cpp
app.get("/", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.text("Hello from Vix");
});
```

Finally, the app is started.

```cpp
app.run(8080);
```

## Minimal HTTP server

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

Run:

```bash
vix run main.cpp
```

Then open:

```text
http://localhost:8080
```

Expected response:

```text
Hello from Vix
```

## JSON response

Use `res.json(...)` to send JSON.

```cpp
#include <vix.hpp>

int main()
{
  vix::App app;

  app.get("/api/status", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.json({
      "status", "ok",
      "server", "Vix.cpp"
    });
  });

  app.run(8080);

  return 0;
}
```

Example response:

```json
{
  "status": "ok",
  "server": "Vix.cpp"
}
```

## Request and response

Core exposes two common aliases for application code:

```cpp
vix::Request
vix::Response
```

`vix::Request` gives access to the incoming request.

```cpp
req.method();
req.path();
req.target();
req.body();
req.header("Content-Type");
req.query_value("page");
req.param("id");
```

`vix::Response` is a response helper built around the native Vix response object.

```cpp
res.status(200);
res.text("OK");
res.json({"ok", true});
res.redirect("/login");
res.file("public/index.html");
```

## Route parameters

Routes can contain named parameters.

```cpp
#include <vix.hpp>

int main()
{
  vix::App app;

  app.get("/users/{id}", [](vix::Request &req, vix::Response &res)
  {
    const std::string id = req.param("id");

    res.json({
      "user_id", id
    });
  });

  app.run(8080);

  return 0;
}
```

Request:

```text
GET /users/42
```

Example response:

```json
{
  "user_id": "42"
}
```

## Middleware

Middleware can run before a route handler.

```cpp
#include <vix.hpp>

int main()
{
  vix::App app;

  app.use([](vix::Request &req, vix::Response &res, vix::App::Next next)
  {
    (void)res;

    vix::print("request:", req.method(), req.path());

    next();
  });

  app.get("/", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.text("home");
  });

  app.run(8080);

  return 0;
}
```

Middleware can also be attached to a prefix.

```cpp
app.use("/admin", [](vix::Request &req, vix::Response &res, vix::App::Next next)
{
  (void)req;

  const bool allowed = true;

  if (!allowed)
  {
    res.status(403).json({"error", "forbidden"});
    return;
  }

  next();
});
```

## Route groups

Use groups to organize routes under a common prefix.

```cpp
#include <vix.hpp>

int main()
{
  vix::App app;

  app.group("/api", [](auto &api)
  {
    api.get("/status", [](vix::Request &req, vix::Response &res)
    {
      (void)req;

      res.json({"status", "ok"});
    });

    api.get("/version", [](vix::Request &req, vix::Response &res)
    {
      (void)req;

      res.json({"version", "1.0.0"});
    });
  });

  app.run(8080);

  return 0;
}
```

The routes become:

```text
GET /api/status
GET /api/version
```

## Static files

Use `static_dir` to mount a directory.

```cpp
#include <vix.hpp>

int main()
{
  vix::App app;

  app.static_dir("public", "/");

  app.run(8080);

  return 0;
}
```

Example mapping:

```text
GET /style.css
```

can serve:

```text
public/style.css
```

## Templates

Use `templates(...)` to configure a template directory.

```cpp
#include <vix.hpp>

int main()
{
  vix::App app;

  app.templates("views");

  app.get("/", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    vix::tmpl::Context ctx;
    ctx.set("title", "Home");

    res.render("index.html", ctx);
  });

  app.run(8080);

  return 0;
}
```

## Core architecture

Core is built as a set of layers.

```text
App
  -> Router
  -> HTTPServer
  -> Session
  -> Transport
  -> RuntimeExecutor
```

The simplified request flow is:

```text
TCP connection
  -> HTTPServer
  -> Session
  -> Transport
  -> Request parsing
  -> Router
  -> RequestHandler
  -> user handler
  -> Response
  -> Transport write
```

## Core and vix::async

Core uses `vix::async` for I/O work.

That includes:

- accepting TCP connections
- reading request bytes
- writing response bytes
- running session coroutines
- timers and cancellation
- spawning async tasks on the I/O context

The HTTP server owns a native async `io_context` and runs it on I/O threads.

## Core and vix::runtime

Core uses `vix::runtime` through `RuntimeExecutor`.

The executor is responsible for scheduling application work on runtime workers.

This keeps the model separated:

```text
vix::async
  -> network I/O, timers, coroutine scheduling

vix::runtime
  -> internal task execution, workers, scheduling, metrics

vix::core
  -> application API, HTTP server, router, sessions, handlers
```

## Heavy routes

Core can mark some routes as heavy.

```cpp
app.get_heavy("/reports", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.json({"status", "report generated"});
});
```

Heavy routes are stored with route metadata so the runtime and router can classify costly work separately when needed.

## Configuration

Core uses `vix::config::Config` for server configuration.

```cpp
vix::config::Config cfg;

cfg.setServerPort(8080);

vix::App app;
app.run(cfg);
```

Configuration controls server behavior such as:

- server port
- I/O thread count
- session timeout
- benchmark mode
- WAF settings
- TLS settings
- logging settings

## TLS

TLS is optional.

When TLS is disabled, Core accepts plain HTTP connections.

When TLS is enabled and configured, the accepted TCP stream is wrapped in a TLS transport before the HTTP session starts reading requests.

A TLS configuration contains:

```cpp
vix::server::TlsConfig tls;

tls.enabled = true;
tls.cert_file = "/etc/letsencrypt/live/example.com/fullchain.pem";
tls.key_file = "/etc/letsencrypt/live/example.com/privkey.pem";
```

## When to use Core

Use Core when you want to build:

- HTTP APIs
- JSON services
- small native web servers
- static websites
- template-rendered apps
- internal tools
- services that need native C++ performance
- applications that should run on top of the Vix runtime model

## Next steps

Read the next pages in this module:

- [App](./app.md)
- [Architecture](./architecture.md)
- [Routing](./routing.md)
- [Handlers](./handlers.md)
- [Middleware](./middleware.md)
- [Request](./request.md)
- [Response](./response.md)
- [HTTP server](./http-server.md)
- [Runtime executor](./runtime-executor.md)
- [API Reference](./api-reference.md)
