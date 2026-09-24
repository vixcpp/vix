# Examples

This section contains practical Vix examples built around the current `vix::App` model.

The goal is not to show every feature at once.

The goal is to give you small, modern, runnable examples that make sense with the current Vix API.

Each example focuses on one real backend need:

```txt
start an HTTP app
build a JSON API
protect routes with middleware
serve static files
cache dynamic GET responses
prepare a production-style bootstrap
```

## What these examples are

These examples are written for the modern Vix application style:

```cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request &, Response &res)
  {
    res.text("Hello from Vix");
  });

  app.run(8080);
}
```

When middleware is needed, examples use:

```cpp
#include <vix/middleware.hpp>
```

For Vix.cpp `v2.6.2` and newer, this is enough:

```cpp
#include <vix.hpp>
#include <vix/middleware.hpp>
```

For Vix.cpp `v2.6.0` and `v2.6.1`, some App middleware integration headers may need to be included explicitly:

```cpp
#include <vix.hpp>
#include <vix/middleware.hpp>

#include <vix/middleware/app/adapter.hpp>
#include <vix/middleware/app/app_middleware.hpp>
#include <vix/middleware/app/http_cache.hpp>
#include <vix/middleware/app/presets.hpp>
```

Use the shorter include form when your project is on `v2.6.2` or newer.

## What these examples are not

These examples are not old Vix v1 snippets.

They are not historical demos.

They are not random API fragments.

They should be:

```txt
current
small
runnable
easy to copy
easy to test with curl
aligned with vix::App
clear about what feature they demonstrate
```

If an example depends on an unstable API, it should not be added here yet.

A wrong example is worse than no example.

## Recommended reading order

Start here:

1. [Hello App](./hello-app)
2. [JSON API](./json-api)
3. [Middleware API](./middleware-api)
4. [API Key Auth](./auth-api-key)
5. [Static Site](./static-site)
6. [HTTP Cache](./http-cache)
7. [Production Bootstrap](./production-bootstrap)

This order moves from simple to realistic.

```txt
Hello App
  minimal server

JSON API
  request body parsing and validation

Middleware API
  security, CORS, rate limit, body limit, JSON parser

API Key Auth
  protected route

Static Site
  public files, index, SPA fallback, Cache-Control

HTTP Cache
  dynamic GET response cache

Production Bootstrap
  config-driven App setup
```

## Run an example

Most examples can be run with:

```bash
vix run examples/<file>.cpp
```

For example:

```bash
vix run examples/hello_app.cpp
```

Then test with `curl`:

```bash
curl -i http://127.0.0.1:8080/
```

Some documentation pages show the full source inline.

You can copy the code into a local `.cpp` file and run it with `vix run`.

## Example: minimal App

```cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request &, Response &res)
  {
    res.text("Hello from Vix");
  });

  app.get("/health", [](Request &, Response &res)
  {
    res.json({
      "ok", true
    });
  });

  app.run(8080);
}
```

Run:

```bash
vix run hello_app.cpp
```

Test:

```bash
curl -i http://127.0.0.1:8080/
curl -i http://127.0.0.1:8080/health
```

This is the base shape used by the rest of the examples.

## Example: JSON API

```cpp
#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

int main()
{
  App app;

  app.use("/api/users", middleware::app::json_strict_dev(4096));

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

Test:

```bash
curl -i \
  -X POST http://127.0.0.1:8080/api/users \
  -H "Content-Type: application/json" \
  -d '{"name":"Ada"}'
```

This introduces the main middleware pattern:

```txt
middleware parses reusable input
handler reads typed request state
handler focuses on application logic
```

## Example: middleware stack

```cpp
#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

int main()
{
  App app;

  app.use("/api", middleware::app::security_headers_dev());
  app.use("/api", middleware::app::cors_dev({"https://example.com"}));
  app.use("/api", middleware::app::rate_limit_dev());
  app.use("/api", middleware::app::body_limit_write_dev(1024 * 1024));

  app.use("/api/users", middleware::app::json_strict_dev(4096));

  app.get("/api/health", [](Request &, Response &res)
  {
    res.json({
      "ok", true
    });
  });

  app.post("/api/users", [](Request &req, Response &res)
  {
    auto &body = req.state<middleware::parsers::JsonBody>();

    res.status(201).json({
      "ok", true,
      "body", body.value.dump()
    });
  });

  app.run(8080);
}
```

This is the kind of backend shape Vix middleware is designed for:

```txt
security headers
CORS
rate limit
body limit
strict JSON parser
route handler
```

## Static files belong to App

Static file serving is a Core App feature.

Use:

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

Do not treat static files as middleware.

Middleware can enhance static responses, for example with a compression hook, but Core serves the files.

The mental model is:

```txt
vix::App serves static files
vix::middleware enhances HTTP behavior
bootstrap code wires configuration
```

## HTTP cache is for dynamic GET routes

The HTTP cache example is about dynamic route responses.

```cpp
app.use("/api", middleware::app::http_cache({
  .ttl_ms = 30'000,
  .allow_bypass = true,
  .bypass_header = "x-vix-cache",
  .bypass_value = "bypass"
}));
```

This is different from static file `Cache-Control`.

```txt
app.static_dir(...)
  controls public file serving and static cache headers

middleware::app::http_cache(...)
  stores and replays dynamic GET route responses
```

## Production-style examples

The `Production Bootstrap` example shows a more realistic structure.

It is useful when your app needs:

```txt
.env configuration
server settings
public path
static file configuration
optional static compression
middleware registry
route registry
AppBootstrap class
```

This is closer to generated projects and production backends.

The goal is to keep `main()` small and move startup wiring into a bootstrap layer.

## Add new examples carefully

Before adding a new example, check that it is:

```txt
based on the current Vix API
small enough to understand quickly
runnable with vix run or clearly marked otherwise
tested with curl or a simple command
focused on one clear idea
not duplicating another example
```

Avoid examples that depend on uncertain APIs.

Good example names are specific:

```txt
hello-app
json-api
middleware-api
auth-api-key
static-site
http-cache
production-bootstrap
```

Avoid vague names:

```txt
auth
cache
database
middleware
production-app
```

Specific names make the docs easier to navigate.

## Summary

Use these examples to learn Vix by building real pieces:

```txt
start an app
return JSON
parse request bodies
protect routes
serve files
cache GET responses
structure production startup
```

The examples should stay modern, small, and aligned with the current `vix::App` API.

If an old example no longer matches the current API, remove it or rewrite it.

Correct examples build trust.
