# App Integration

The `app` integration layer connects `vix::middleware` to `vix::App`.

Most Vix backends are built with `vix::App`, routes, handlers, groups, and `app.use(...)`.

The middleware module contains reusable middleware written in two styles:

```txt
App middleware
  works directly with vix::App

Context-based middleware
  works with vix::middleware::Context and vix::middleware::Next

Legacy HTTP middleware
  works with Request, Response and Next
```

The `vix::middleware::app` namespace makes these styles usable inside the normal `vix::App` model.

## The normal path

For normal application code, use:

```cpp
#include <vix.hpp>
#include <vix/middleware.hpp>
```

Then install middleware with `app.use(...)`.

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

  app.get("/api/health", [](Request &, Response &res)
  {
    res.json({
      "ok", true
    });
  });

  app.run(8080);
}
```

This is the normal integration model:

```txt
vix::App
  -> app.use(...)
  -> middleware::app preset
  -> route handler
```

Start with the `middleware::app` helpers when building a backend.

Use the lower-level namespaces only when you need custom behavior.

## Why App integration exists

Core already has its own middleware signature.

```cpp
app.use([](vix::Request &req, vix::Response &res, vix::App::Next next)
{
  (void)req;
  (void)res;

  next();
});
```

This is the middleware shape expected by `vix::App`.

The middleware module also has a lower-level context-based shape:

```cpp
vix::middleware::MiddlewareFn
```

A `MiddlewareFn` receives:

```cpp
vix::middleware::Context &
vix::middleware::Next
```

The App integration layer converts reusable middleware into the shape that `vix::App` expects.

```txt
vix::middleware::MiddlewareFn
  -> vix::App::Middleware
```

That is the role of `adapt_ctx()`.

There is also a legacy HTTP middleware shape:

```cpp
vix::middleware::HttpMiddleware
```

A `HttpMiddleware` receives:

```cpp
vix::Request &
vix::Response &
vix::middleware::Next
```

That is converted with `adapt()`.

## App helpers

The App integration helpers live in:

```cpp
namespace vix::middleware::app
```

Common helpers:

| Helper                    | Purpose                                                    |
| ------------------------- | ---------------------------------------------------------- |
| `adapt_ctx(...)`          | Convert context-based middleware to `vix::App::Middleware` |
| `adapt(...)`              | Convert legacy HTTP middleware to `vix::App::Middleware`   |
| `when(...)`               | Run an App middleware only when a predicate matches        |
| `protect(...)`            | Install middleware on one exact path                       |
| `protect_prefix(...)`     | Install middleware on a route prefix                       |
| `install(...)`            | Install middleware with `app.use(prefix, mw)`              |
| `install_exact(...)`      | Install middleware for one exact path                      |
| `chain(...)`              | Combine several App middleware functions in order          |
| `http_cache(...)`         | Build App middleware for HTTP cache                        |
| `install_http_cache(...)` | Install HTTP cache on the configured prefix                |

These helpers are glue code.

They do not replace `vix::App`. They make reusable middleware fit naturally into `vix::App`.

## Use presets first

Most applications should start with presets.

```cpp
app.use("/api", vix::middleware::app::cors_dev());
app.use("/api", vix::middleware::app::rate_limit_dev());
app.use("/api", vix::middleware::app::json_dev());
```

Presets already return `vix::App::Middleware`.

That means they work directly with:

```cpp
app.use(...)
```

Examples:

```cpp
app.use("/api", middleware::app::security_headers_dev());
app.use("/api", middleware::app::cors_dev({"https://example.com"}));
app.use("/api", middleware::app::rate_limit_dev(120));
app.use("/api", middleware::app::body_limit_write_dev(1024 * 1024));
app.use("/api/users", middleware::app::json_strict_dev(4096));
app.use("/api/admin", middleware::app::api_key_dev("secret"));
```

This is the simplest way to build a backend stack.

## Prefix installation

`app.use(prefix, middleware)` installs middleware for routes that match the prefix.

```cpp
app.use("/api", middleware::app::cors_dev());
```

This applies to:

```txt
/api
/api/users
/api/admin/status
```

It does not apply to:

```txt
/
/admin
/public
```

Use broad prefixes for shared behavior.

```cpp
app.use("/api", middleware::app::security_headers_dev());
app.use("/api", middleware::app::rate_limit_dev());
```

Use narrower prefixes for route-specific behavior.

```cpp
app.use("/api/users", middleware::app::json_strict_dev(4096));
app.use("/api/admin", middleware::app::api_key_dev("secret"));
```

This keeps route behavior explicit.

## `install`

`install()` is a small wrapper around `app.use(prefix, middleware)`.

```cpp
vix::middleware::app::install(
  app,
  "/api",
  vix::middleware::app::rate_limit_dev()
);
```

This is useful when you organize middleware registration inside functions.

```cpp
static void install_api_middlewares(vix::App &app)
{
  using namespace vix::middleware::app;

  install(app, "/api", security_headers_dev());
  install(app, "/api", cors_dev({"https://example.com"}));
  install(app, "/api", rate_limit_dev());
}
```

Then your `main()` stays clean.

```cpp
int main()
{
  vix::App app;

  install_api_middlewares(app);
  register_routes(app);

  app.run(8080);
}
```

## `protect`

`protect()` installs middleware for one exact path.

```cpp
vix::middleware::app::protect(
  app,
  "/admin",
  vix::middleware::app::api_key_dev("secret")
);
```

This applies to:

```txt
/admin
```

It does not apply to:

```txt
/admin/users
/admin/settings
```

Use `protect()` when one exact route needs middleware.

## `protect_prefix`

`protect_prefix()` installs middleware for a whole route prefix.

```cpp
vix::middleware::app::protect_prefix(
  app,
  "/admin",
  vix::middleware::app::api_key_dev("secret")
);
```

This applies to:

```txt
/admin
/admin/users
/admin/settings
```

Use `protect_prefix()` for route groups such as:

```txt
/api
/admin
/internal
/dashboard
```

Example:

```cpp
#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

int main()
{
  App app;

  middleware::app::protect_prefix(
    app,
    "/admin",
    middleware::app::api_key_dev("secret")
  );

  app.get("/admin/status", [](Request &req, Response &res)
  {
    auto &key = req.state<middleware::auth::ApiKey>();

    res.json({
      "ok", true,
      "admin", true,
      "key_size", static_cast<long long>(key.value.size())
    });
  });

  app.run(8080);
}
```

Request:

```bash
curl -i \
  http://127.0.0.1:8080/admin/status \
  -H "x-api-key: secret"
```

If the key is missing or invalid, the handler is not called.

## `install_exact`

`install_exact()` installs middleware for one exact path.

```cpp
vix::middleware::app::install_exact(
  app,
  "/api/ping",
  vix::middleware::app::api_key_dev("secret")
);
```

This is useful when you are building a middleware registry and want explicit installation functions.

```cpp
static void install_middlewares(vix::App &app)
{
  using namespace vix::middleware::app;

  install_exact(app, "/api/ping", api_key_dev("secret"));
}
```

For normal route groups, prefer prefix installation.

For one route, use `protect()` or `install_exact()`.

## `when`

`when()` applies an App middleware only when a predicate returns true.

```cpp
auto only_post = vix::middleware::app::when(
  [](const vix::Request &req)
  {
    return req.method() == "POST";
  },
  vix::middleware::app::body_limit_write_dev(1024)
);

app.use("/api", std::move(only_post));
```

If the predicate returns false, the middleware is skipped and the request continues.

Use `when()` when prefix matching is not enough.

Examples:

```txt
only apply to POST requests
only apply when a header exists
only apply to one content type
only apply to paths matching custom logic
```

## `chain`

`chain()` combines several App middlewares into one middleware.

```cpp
auto admin_stack = vix::middleware::app::chain(
  vix::middleware::app::api_key_dev("secret"),
  vix::middleware::app::rate_limit_dev()
);

app.use("/admin", std::move(admin_stack));
```

The middleware functions run in order.

```txt
api_key_dev
  -> rate_limit_dev
  -> handler
```

A chain is useful when a route group has a known stack.

Example with JSON route protection:

```cpp
auto users_stack = vix::middleware::app::chain(
  vix::middleware::app::body_limit_write_dev(4096),
  vix::middleware::app::json_strict_dev(4096)
);

app.use("/api/users", std::move(users_stack));

app.post("/api/users", [](vix::Request &req, vix::Response &res)
{
  auto &body = req.state<vix::middleware::parsers::JsonBody>();

  res.status(201).json({
    "ok", true,
    "body", body.value.dump()
  });
});
```

Use `chain()` to make route-specific middleware stacks readable.

## `adapt_ctx`

`adapt_ctx()` converts context-based middleware into App middleware.

Use it when the middleware returns:

```cpp
vix::middleware::MiddlewareFn
```

Example:

```cpp
auto app_mw = vix::middleware::app::adapt_ctx(
  vix::middleware::basics::request_id()
);

app.use(app_mw);
```

A context-based middleware can access:

```txt
ctx.req()
ctx.res()
ctx.services()
ctx.set_state(...)
ctx.try_state<T>()
ctx.send_error(...)
```

Example:

```cpp
#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

int main()
{
  App app;

  app.use(middleware::app::adapt_ctx(
    middleware::basics::request_id()
  ));

  app.get("/", [](Request &req, Response &res)
  {
    auto *rid = req.try_state<middleware::basics::RequestId>();

    res.json({
      "request_id", rid ? rid->value : ""
    });
  });

  app.run(8080);
}
```

Most users do not need to call `adapt_ctx()` directly for common behavior, because presets already do that.

Use `adapt_ctx()` when you want custom options from the lower-level middleware.

## Configure lower-level middleware with `adapt_ctx`

Presets are convenient. Lower-level options give you control.

```cpp
vix::middleware::parsers::JsonParserOptions opt;

opt.require_content_type = true;
opt.allow_empty = false;
opt.max_bytes = 4096;
opt.store_in_state = true;

app.use("/api/json", vix::middleware::app::adapt_ctx(
  vix::middleware::parsers::json(opt)
));
```

Now the parser is configured manually but still installed through `vix::App`.

Same pattern for security middleware:

```cpp
vix::middleware::security::RateLimitOptions opt;

opt.capacity = 100.0;
opt.refill_per_sec = 5.0;
opt.key_header = "x-forwarded-for";

app.use("/api", vix::middleware::app::adapt_ctx(
  vix::middleware::security::rate_limit(opt)
));
```

Use this pattern when presets are not specific enough.

## `adapt`

`adapt()` converts legacy HTTP middleware into App middleware.

Use it when the middleware already works with:

```cpp
vix::Request &
vix::Response &
vix::middleware::Next
```

Example:

```cpp
vix::middleware::HttpMiddleware require_header =
  [](vix::Request &req, vix::Response &res, vix::middleware::Next next)
  {
    if (req.header("x-demo") != "1")
    {
      res.status(401).json({
        "ok", false,
        "error", "missing_x_demo"
      });
      return;
    }

    next();
  };

app.use("/api", vix::middleware::app::adapt(require_header));
```

Use `adapt()` for legacy HTTP middleware.

Use `adapt_ctx()` for context-based middleware.

## Choosing the right helper

Use this rule:

| Situation                                 | Use                                     |
| ----------------------------------------- | --------------------------------------- |
| You want a built-in middleware preset     | `middleware::app::*_dev(...)`           |
| You have `MiddlewareFn`                   | `adapt_ctx(...)`                        |
| You have `HttpMiddleware`                 | `adapt(...)`                            |
| You want middleware on a prefix           | `app.use(prefix, mw)` or `install(...)` |
| You want middleware on one exact path     | `protect(...)` or `install_exact(...)`  |
| You want custom request matching          | `when(...)`                             |
| You want several middlewares as one stack | `chain(...)`                            |

Start simple.

Use presets first.

Use adapters when you need lower-level control.

## Recommended backend structure

For a real backend, do not put all middleware registration inside `main()`.

Use small functions.

```cpp
#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

static void install_global_middlewares(App &app)
{
  app.use("/api", middleware::app::security_headers_dev());
  app.use("/api", middleware::app::cors_dev({"https://example.com"}));
  app.use("/api", middleware::app::rate_limit_dev());
}

static void install_route_middlewares(App &app)
{
  app.use("/api/users", middleware::app::body_limit_write_dev(4096));
  app.use("/api/users", middleware::app::json_strict_dev(4096));

  app.use("/api/admin", middleware::app::api_key_dev("secret"));
}

static void register_routes(App &app)
{
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

  app.get("/api/admin/status", [](Request &, Response &res)
  {
    res.json({
      "ok", true,
      "admin", true
    });
  });
}

int main()
{
  App app;

  install_global_middlewares(app);
  install_route_middlewares(app);
  register_routes(app);

  app.run(8080);
}
```

This keeps the application readable.

```txt
main()
  creates the app
  installs middleware
  registers routes
  runs the server
```

Each function has one job.

## Middleware order

Order matters.

A practical order is:

```txt
recovery
request id
timing
security headers
CORS
rate limit
body limit
authentication
parser
handler
compression
ETag
logging
metrics
```

The exact order depends on your application, but the idea is stable:

```txt
reject bad requests early
parse only after size and content rules
authenticate before protected handlers
modify responses after handlers
observe the complete request flow
```

Example:

```cpp
app.use("/api", middleware::app::security_headers_dev());
app.use("/api", middleware::app::cors_dev());
app.use("/api", middleware::app::rate_limit_dev());
app.use("/api", middleware::app::body_limit_write_dev(1024 * 1024));

app.use("/api/admin", middleware::app::api_key_dev("secret"));
app.use("/api/users", middleware::app::json_strict_dev(4096));
```

If a middleware sends a response and does not call `next()`, the rest of the chain is skipped.

That is how rate limit, auth, body limits, CORS preflight, CSRF, and parsers stop invalid requests.

## App groups

`vix::App` also supports groups.

A group prefixes routes and middleware with a common path.

```cpp
app.group("/api", [](vix::App::Group api)
{
  api.use(vix::middleware::app::security_headers_dev());
  api.use(vix::middleware::app::cors_dev());

  api.get("/health", [](vix::Request &, vix::Response &res)
  {
    res.json({
      "ok", true
    });
  });
});
```

This keeps related routes together.

Use groups when you want the code structure to match the route structure.

## Static files are not App middleware

Static files are configured through `vix::App`.

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

This belongs to Core.

The middleware module can add an optional static response hook, for example compression:

```cpp
vix::App::set_static_response_hook(
  vix::middleware::performance::compressed_static_response_hook()
);
```

The difference is important:

```txt
app.static_dir(...)
  serves public files

App::set_static_response_hook(...)
  can enhance static file responses

app.use(...)
  installs route middleware
```

Do not document static file serving as a middleware feature.

Middleware can enhance static responses, but Core serves the files.

## HTTP cache integration

HTTP cache has an App helper because it is commonly installed on a route prefix.

```cpp
app.use("/api", vix::middleware::app::http_cache({
  .ttl_ms = 30'000,
  .allow_bypass = true,
  .bypass_header = "x-vix-cache",
  .bypass_value = "bypass"
}));
```

This caches dynamic `GET` route responses.

It is different from static file `Cache-Control`.

```txt
app.static_dir(...)
  cache headers for public files

middleware::app::http_cache(...)
  stores and replays dynamic GET route responses
```

A cache hit can stop the request before the route handler runs.

A cache miss calls the handler and may store the response.

## Version note

For Vix.cpp `v2.6.2` and newer, this is enough:

```cpp
#include <vix.hpp>
#include <vix/middleware.hpp>
```

For Vix.cpp `v2.6.0` and `v2.6.1`, App integration headers may need to be included explicitly:

```cpp
#include <vix.hpp>
#include <vix/middleware.hpp>

#include <vix/middleware/app/adapter.hpp>
#include <vix/middleware/app/app_middleware.hpp>
#include <vix/middleware/app/http_cache.hpp>
#include <vix/middleware/app/presets.hpp>
```

Use the shorter include form when your project is on `v2.6.2` or newer.

## Summary

`vix::middleware::app` is the bridge between reusable middleware and `vix::App`.

Use it to:

```txt
install middleware on prefixes
protect exact routes
protect route groups
chain middleware stacks
adapt lower-level middleware
connect HTTP cache to App routes
keep backend setup readable
```

The most common path is:

```cpp
app.use("/api", middleware::app::security_headers_dev());
app.use("/api", middleware::app::cors_dev());
app.use("/api", middleware::app::rate_limit_dev());
app.use("/api/users", middleware::app::json_strict_dev());
```

Start with presets.

Use adapters when you need custom options.

Keep Core and Middleware separate:

```txt
Core owns the app.
Middleware owns reusable HTTP behavior.
```
