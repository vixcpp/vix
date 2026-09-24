# Middleware

`vix::middleware` is the backend layer for `vix::App`.

It gives a Vix HTTP application the reusable behavior that real backends need around routes: request limits, CORS, security headers, authentication, sessions, JSON parsing, form parsing, multipart uploads, response caching, compression, ETags, tracing, metrics, and debug tools.

Core owns the application model.

```txt
vix::App
  owns routes
  owns handlers
  owns request and response objects
  owns static files
  owns templates
  owns server lifecycle
```

The middleware module owns reusable HTTP behavior around those routes.

```txt
vix::middleware
  protects requests
  parses request bodies
  authenticates users
  stores typed request state
  modifies responses
  caches GET responses
  adds observability
  adapts reusable middleware to vix::App
```

This separation keeps the framework simple.

Core stays responsible for running the application. Middleware stays responsible for the reusable backend concerns that should not be rewritten in every project.

## What problem it solves

A route handler should focus on the application action.

```cpp
app.post("/api/users", [](vix::Request &req, vix::Response &res)
{
  // create user
});
```

But a real backend route usually needs more than the action itself.

Before the handler runs, the application may need to:

```txt
reject bodies that are too large
reject invalid content types
parse JSON
check CORS
check rate limits
authenticate the caller
verify permissions
load a session
```

After the handler runs, the application may need to:

```txt
add security headers
add timing headers
compress the response
generate an ETag
store a cache entry
record metrics
emit trace identifiers
```

`vix::middleware` provides these behaviors as composable pieces.

The handler stays small. The route behavior stays explicit.

## Basic example

Most applications use middleware through `vix::App`.

```cpp
#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

int main()
{
  App app;

  app.use("/api", middleware::app::security_headers_dev());
  app.use("/api", middleware::app::cors_dev({"https://example.com"}));
  app.use("/api", middleware::app::rate_limit_dev(60));
  app.use("/api", middleware::app::body_limit_write_dev(1024));
  app.use("/api/users", middleware::app::json_strict_dev(1024));

  app.post("/api/users", [](Request &req, Response &res)
  {
    auto &body = req.state<middleware::parsers::JsonBody>();

    const std::string name = body.value.value("name", "");

    if (name.empty())
    {
      res.status(422).json({
        "ok", false,
        "error", "Missing name"
      });
      return;
    }

    res.status(201).json({
      "ok", true,
      "name", name
    });
  });

  app.run(8080);
}
```

This route is not just a raw handler.

It is protected by security headers, CORS, rate limiting, body size limits, and a strict JSON parser before the user creation logic runs.

That is the purpose of the middleware module.

## Public header

For Vix.cpp `v2.6.2` and newer, use:

```cpp
#include <vix.hpp>
#include <vix/middleware.hpp>
```

`<vix.hpp>` exposes the main Vix application APIs.

`<vix/middleware.hpp>` exposes the middleware module, including App integration helpers.

For Vix.cpp `v2.6.0` and `v2.6.1`, some App integration headers may need to be included explicitly:

```cpp
#include <vix.hpp>
#include <vix/middleware.hpp>

#include <vix/middleware/app/adapter.hpp>
#include <vix/middleware/app/app_middleware.hpp>
#include <vix/middleware/app/http_cache.hpp>
#include <vix/middleware/app/presets.hpp>
```

Use the shorter include form when your project uses `v2.6.2` or newer.

## The normal application path

The normal path is:

```txt
vix::App
  -> app.use(...)
  -> middleware::app presets
  -> route handler
```

Example:

```cpp
app.use("/api", vix::middleware::app::cors_dev());
app.use("/api", vix::middleware::app::rate_limit_dev());
app.use("/api", vix::middleware::app::json_dev());

app.post("/api/echo", [](vix::Request &req, vix::Response &res)
{
  auto &body = req.state<vix::middleware::parsers::JsonBody>();

  res.json({
    "ok", true,
    "received", body.value.dump()
  });
});
```

The `app` namespace exists because many middleware functions are written in a lower-level context-based model. App helpers adapt them into the normal `vix::App` middleware model.

In normal backend applications, start with:

```cpp
vix::middleware::app::...
```

Then use lower-level middleware only when you need more control.

## What the module provides

The module is organized by backend responsibility.

| Area            | Purpose                                                                |
| --------------- | ---------------------------------------------------------------------- |
| `app`           | App adapters, presets, prefix installation, chaining, route protection |
| `basics`        | Body limits, request ids, timing, recovery, logging                    |
| `security`      | CORS, CSRF, security headers, IP filtering, rate limiting              |
| `auth`          | API keys, JWT, RBAC, permissions, sessions                             |
| `parsers`       | JSON, URL-encoded forms, multipart metadata, multipart file uploads    |
| `http`          | HTTP helpers such as cookies                                           |
| `http_cache`    | GET response caching through `vix::cache`                              |
| `performance`   | Compression, ETags, static response compression hook                   |
| `observability` | Tracing, metrics, debug traces                                         |
| `pipeline`      | Low-level middleware pipeline for tests and custom integrations        |

This makes the module broad enough to build serious HTTP backends, while still keeping each feature small and explicit.

## Request flow

Middleware runs around route handlers.

```txt
request
  -> middleware
  -> middleware
  -> middleware
  -> route handler
  -> response
```

Every middleware receives a continuation function called `next`.

Calling `next()` means the request continues.

Not calling `next()` means the middleware stops the request and sends the response itself.

This is how middleware can block invalid requests.

```cpp
app.use("/admin", [](vix::Request &req, vix::Response &res, vix::App::Next next)
{
  if (req.header("x-api-key") != "secret")
  {
    res.status(401).json({
      "ok", false,
      "error", "missing_or_invalid_api_key"
    });
    return;
  }

  next();
});
```

This same idea is used by API key authentication, JWT authentication, RBAC, CSRF, IP filtering, rate limiting, body limits, and HTTP cache hits.

## Middleware can run before or after the handler

Some middleware runs before the handler.

```txt
body limit
JSON parser
API key
JWT
CSRF
rate limit
IP filter
```

These middleware functions decide whether the request is allowed to continue.

Other middleware runs after the handler.

```txt
security headers
timing
logging
compression
ETag
metrics
```

These middleware functions usually call `next()` first, then inspect or modify the response.

Example:

```cpp
app.use([](vix::Request &req, vix::Response &res, vix::App::Next next)
{
  (void)req;

  next();

  res.header("X-Content-Type-Options", "nosniff");
});
```

This is why order matters.

A middleware that runs after `next()` wraps everything that comes after it.

## Typed request state

Middleware can attach typed data to the current request.

For example, the JSON parser stores:

```cpp
vix::middleware::parsers::JsonBody
```

JWT authentication stores:

```cpp
vix::middleware::auth::JwtClaims
```

API key authentication stores:

```cpp
vix::middleware::auth::ApiKey
```

Sessions store:

```cpp
vix::middleware::auth::Session
```

A handler can read the typed state:

```cpp
app.post("/api/echo", [](vix::Request &req, vix::Response &res)
{
  auto &body = req.state<vix::middleware::parsers::JsonBody>();

  res.json({
    "ok", true,
    "received", body.value.dump()
  });
});
```

Use `state<T>()` when the middleware is required for the route.

Use `try_state<T>()` when the state may be missing.

```cpp
auto *session = req.try_state<vix::middleware::auth::Session>();

if (!session)
{
  res.status(500).json({
    "ok", false,
    "error", "session_missing"
  });
  return;
}
```

Typed state is one of the main reasons middleware stays clean. A parser, auth layer, or session layer can do the work once, then the handler reads the result without re-parsing or re-validating the same input.

## App integration

The `vix::middleware::app` namespace contains the helpers used with `vix::App`.

Common helpers include:

```cpp
vix::middleware::app::adapt_ctx(...)
vix::middleware::app::adapt(...)
vix::middleware::app::when(...)
vix::middleware::app::protect(...)
vix::middleware::app::protect_prefix(...)
vix::middleware::app::install(...)
vix::middleware::app::install_exact(...)
vix::middleware::app::chain(...)
```

Most simple applications use presets:

```cpp
app.use("/api", vix::middleware::app::cors_dev());
app.use("/api", vix::middleware::app::rate_limit_dev());
app.use("/api", vix::middleware::app::json_dev());
```

When you need custom behavior, use the lower-level middleware and adapt it:

```cpp
auto mw = vix::middleware::app::adapt_ctx(
  vix::middleware::security::rate_limit({
    .capacity = 100.0,
    .refill_per_sec = 10.0
  })
);

app.use("/api", std::move(mw));
```

Use `adapt_ctx()` for context-based middleware.

Use `adapt()` for legacy HTTP middleware that already works with `Request`, `Response`, and `Next`.

## Route protection

Middleware can be installed globally:

```cpp
app.use(vix::middleware::app::security_headers_dev());
```

It can be installed on a prefix:

```cpp
app.use("/api", vix::middleware::app::cors_dev());
```

It can protect one exact path:

```cpp
vix::middleware::app::protect(
  app,
  "/admin",
  vix::middleware::app::api_key_dev("secret")
);
```

It can protect a whole prefix:

```cpp
vix::middleware::app::protect_prefix(
  app,
  "/admin",
  vix::middleware::app::api_key_dev("secret")
);
```

Use exact protection for one route.

Use prefix protection for route groups such as `/api`, `/admin`, `/dashboard`, or `/internal`.

## A practical backend stack

A small backend can start with this shape:

```cpp
#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

int main()
{
  App app;

  app.use("/api", middleware::app::security_headers_dev());
  app.use("/api", middleware::app::cors_dev({"https://example.com"}));
  app.use("/api", middleware::app::rate_limit_dev(120));
  app.use("/api", middleware::app::body_limit_write_dev(1024 * 1024));

  app.use("/api/auth", middleware::app::json_strict_dev(4096));
  app.use("/api/admin", middleware::app::api_key_dev("dev_key"));

  app.get("/api/health", [](Request &, Response &res)
  {
    res.json({
      "ok", true
    });
  });

  app.post("/api/auth/login", [](Request &req, Response &res)
  {
    auto &body = req.state<middleware::parsers::JsonBody>();

    const std::string email = body.value.value("email", "");
    const std::string password = body.value.value("password", "");

    if (email.empty() || password.empty())
    {
      res.status(422).json({
        "ok", false,
        "error", "Missing credentials"
      });
      return;
    }

    res.json({
      "ok", true
    });
  });

  app.get("/api/admin/status", [](Request &, Response &res)
  {
    res.json({
      "ok", true,
      "admin", true
    });
  });

  app.run(8080);
}
```

This is the main value of the middleware module: a backend can stay readable while still having real request handling behavior.

## Static files belong to Core

Static file serving is not a middleware feature.

Use `vix::App::static_dir(...)` for public files:

```cpp
app.static_dir(
  "public",
  "/",
  "index.html",
  true,
  "public, max-age=3600",
  true,
  false
);
```

Core handles:

```txt
public directory
mount path
index file
Cache-Control
fallthrough
SPA fallback
file response
```

The middleware module can attach an optional hook after a static file response has been written.

That hook can compress eligible static responses.

```cpp
vix::App::set_static_response_hook(
  vix::middleware::performance::compressed_static_response_hook()
);
```

So the rule is simple:

```txt
Core serves static files.
Middleware can enhance static responses.
Bootstrap wires configuration.
```

For generated applications, these values often come from configuration:

```dotenv
PUBLIC_PATH=public
PUBLIC_MOUNT=/
PUBLIC_INDEX=index.html
PUBLIC_CACHE_CONTROL=public, max-age=3600
PUBLIC_SPA_FALLBACK=false
PUBLIC_COMPRESSION=false
PUBLIC_COMPRESSION_MIN_SIZE=1024
```

The middleware documentation only covers the optional enhancement. The Core documentation covers static file serving itself.

## HTTP cache is for dynamic GET responses

Do not confuse static file cache headers with the HTTP cache middleware.

`app.static_dir(...)` can add `Cache-Control` headers to public files.

The HTTP cache middleware is different. It caches dynamic `GET` responses produced by routes.

```cpp
app.use("/api", vix::middleware::app::http_cache({
  .ttl_ms = 30'000,
  .allow_bypass = true,
  .bypass_header = "x-vix-cache",
  .bypass_value = "bypass"
}));
```

Then a route such as this can be cached:

```cpp
app.get("/api/users", [](vix::Request &, vix::Response &res)
{
  res.json({
    "ok", true,
    "source", "origin"
  });
});
```

On a cache hit, the handler is not called.

On a cache miss, the handler runs and the response may be stored.

## Low-level pipeline

Most applications should use `vix::App`.

The lower-level `HttpPipeline` exists for tests, custom integrations, and situations where you want to run middleware without a full server.

```cpp
vix::middleware::HttpPipeline pipeline;

pipeline.use(vix::middleware::security::csrf());
pipeline.use(vix::middleware::parsers::json());

pipeline.run(req, res, [](auto &req, auto &res)
{
  res.ok().text("OK");
});
```

Use `HttpPipeline` when you need full control over the middleware chain.

Use `vix::App` for normal applications.

## Recommended reading order

Read the middleware documentation in this order:

1. [Quick Start](./quick-start)
2. [App Integration](./app-integration)
3. [Core Concepts](./concepts)
4. [Basics](./basics)
5. [Security](./security)
6. [Authentication](./authentication)
7. [Parsers](./parsers)
8. [HTTP Cache](./http-cache)
9. [Performance](./performance)
10. [Observability](./observability)
11. [API Reference](./api-reference)

The guide pages explain how to build with the module.

The API reference exists for lookup once you already understand the model.
