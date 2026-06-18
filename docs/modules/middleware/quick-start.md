# Quick Start

This page shows the fastest useful way to start with `vix::middleware`.

The goal is to build a small API that already behaves like a real backend:

```txt
security headers
CORS
rate limiting
body size limits
strict JSON parsing
typed request state
route-level protection
```

The example uses `vix::App`, because that is the normal application model for Vix backends.

## Create a small API

Create a file:

```txt
middleware_quick_start.cpp
```

Add this code:

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

  app.get("/api/health", [](Request &, Response &res)
  {
    res.json({
      "ok", true,
      "service", "vix"
    });
  });

  app.post("/api/users", [](Request &req, Response &res)
  {
    auto &body = req.state<middleware::parsers::JsonBody>();

    const std::string name = body.value.value("name", "");
    const std::string email = body.value.value("email", "");

    if (name.empty())
    {
      res.status(422).json({
        "ok", false,
        "error", "Missing name"
      });
      return;
    }

    if (email.empty())
    {
      res.status(422).json({
        "ok", false,
        "error", "Missing email"
      });
      return;
    }

    res.status(201).json({
      "ok", true,
      "user", {
        "name", name,
        "email", email
      }
    });
  });

  app.run(8080);
  return 0;
}
```

Run it:

```bash
vix run middleware_quick_start.cpp
```

The server listens on:

```txt
http://127.0.0.1:8080
```

## Test the health route

```bash
curl -i http://127.0.0.1:8080/api/health
```

Expected shape:

```txt
HTTP/1.1 200 OK
```

Body shape:

```json
{
  "ok": true,
  "service": "vix"
}
```

This route does not need a request body. It still receives the middleware installed on `/api`, such as security headers, CORS, and rate limiting.

## Send valid JSON

```bash
curl -i \
  -X POST http://127.0.0.1:8080/api/users \
  -H "Content-Type: application/json" \
  -d '{"name":"Ada","email":"ada@example.com"}'
```

Expected status:

```txt
201 Created
```

Expected body shape:

```json
{
  "ok": true,
  "user": {
    "name": "Ada",
    "email": "ada@example.com"
  }
}
```

The handler does not parse raw JSON manually.

The JSON parser middleware already parsed the body and stored it in typed request state:

```cpp
auto &body = req.state<vix::middleware::parsers::JsonBody>();
```

That is one of the main middleware patterns in Vix.

A middleware does the reusable work once. The handler reads the result.

## Send invalid JSON

```bash
curl -i \
  -X POST http://127.0.0.1:8080/api/users \
  -H "Content-Type: application/json" \
  -d '{"name":}'
```

Expected status:

```txt
400 Bad Request
```

The route handler is not called.

The JSON middleware stops the request before the handler because the body is invalid.

## Send the wrong content type

```bash
curl -i \
  -X POST http://127.0.0.1:8080/api/users \
  -H "Content-Type: text/plain" \
  -d '{"name":"Ada"}'
```

Expected status:

```txt
415 Unsupported Media Type
```

`json_strict_dev()` requires a JSON content type.

That means the handler can assume that a successful request has already passed the JSON parser.

## Send an empty body

```bash
curl -i \
  -X POST http://127.0.0.1:8080/api/users \
  -H "Content-Type: application/json" \
  -d ''
```

Expected status:

```txt
400 Bad Request
```

`json_strict_dev()` rejects an empty body.

Use strict parsing for routes where a body is required.

Use the more relaxed JSON parser when an empty body is acceptable.

## Send a body that is too large

The example limits write request bodies to `1024` bytes:

```cpp
app.use("/api", middleware::app::body_limit_write_dev(1024));
```

Try a larger body:

```bash
python3 - <<'PY' > /tmp/large.json
print('{"name":"' + 'a' * 2000 + '","email":"ada@example.com"}')
PY

curl -i \
  -X POST http://127.0.0.1:8080/api/users \
  -H "Content-Type: application/json" \
  --data-binary @/tmp/large.json
```

Expected status:

```txt
413 Payload Too Large
```

The body limit middleware rejects the request before JSON parsing.

This order matters.

```txt
body limit
  -> JSON parser
  -> handler
```

Rejecting oversized requests early avoids unnecessary parsing work.

## What happened in the request flow

For `POST /api/users`, the request flow is:

```txt
request
  -> security headers middleware
  -> CORS middleware
  -> rate limit middleware
  -> body limit middleware
  -> strict JSON parser
  -> route handler
  -> response
```

Some middleware works before the handler.

```txt
CORS
rate limit
body limit
JSON parser
```

Some middleware can work after the handler.

```txt
security headers
timing
compression
ETag
logging
metrics
```

A middleware continues the request by calling `next()`.

A middleware stops the request by sending a response and not calling `next()`.

That is the core middleware model.

## Add API key protection

Now protect an admin route.

Add this middleware before the route:

```cpp
app.use("/api/admin", middleware::app::api_key_dev("dev_key_123"));
```

Add the route:

```cpp
app.get("/api/admin/status", [](Request &req, Response &res)
{
  auto &key = req.state<middleware::auth::ApiKey>();

  res.json({
    "ok", true,
    "admin", true,
    "key_size", static_cast<long long>(key.value.size())
  });
});
```

Full protected section:

```cpp
app.use("/api/admin", middleware::app::api_key_dev("dev_key_123"));

app.get("/api/admin/status", [](Request &req, Response &res)
{
  auto &key = req.state<middleware::auth::ApiKey>();

  res.json({
    "ok", true,
    "admin", true,
    "key_size", static_cast<long long>(key.value.size())
  });
});
```

Test without a key:

```bash
curl -i http://127.0.0.1:8080/api/admin/status
```

Expected status:

```txt
401 Unauthorized
```

Test with the wrong key:

```bash
curl -i \
  http://127.0.0.1:8080/api/admin/status \
  -H "x-api-key: wrong"
```

Expected status:

```txt
403 Forbidden
```

Test with the valid key:

```bash
curl -i \
  http://127.0.0.1:8080/api/admin/status \
  -H "x-api-key: dev_key_123"
```

Expected status:

```txt
200 OK
```

When the key is valid, the middleware stores:

```cpp
vix::middleware::auth::ApiKey
```

The route can read it from request state.

Do not return real API keys in production responses. The example only shows how typed state works.

## Use prefixes intentionally

Middleware installed on a prefix applies to matching routes.

```cpp
app.use("/api", middleware::app::rate_limit_dev());
```

This applies to:

```txt
/api
/api/users
/api/admin/status
```

Middleware installed on a more specific prefix applies only there.

```cpp
app.use("/api/users", middleware::app::json_strict_dev(1024));
```

This applies to:

```txt
/api/users
/api/users/123
```

It does not apply to:

```txt
/api/health
/api/admin/status
```

Use broad middleware for general backend behavior.

Use narrow middleware for route-specific behavior.

## Recommended order

A practical order for many APIs is:

```cpp
app.use("/api", middleware::app::security_headers_dev());
app.use("/api", middleware::app::cors_dev());
app.use("/api", middleware::app::rate_limit_dev());
app.use("/api", middleware::app::body_limit_write_dev(1024 * 1024));

app.use("/api/private", middleware::app::api_key_dev("secret"));
app.use("/api/json", middleware::app::json_strict_dev(1024 * 1024));
```

The idea is:

```txt
security headers
  add safe browser response headers

CORS
  handle browser cross-origin rules

rate limit
  reject abusive clients early

body limit
  reject large bodies before parsing

authentication
  reject unauthorized callers

parser
  parse the body before the handler

handler
  run application logic
```

The exact order can change by application, but this shape is a good starting point.

## App presets vs low-level middleware

For normal `vix::App` applications, prefer App presets:

```cpp
middleware::app::cors_dev()
middleware::app::rate_limit_dev()
middleware::app::json_strict_dev()
middleware::app::api_key_dev("secret")
```

These return `vix::App::Middleware`, so they work directly with:

```cpp
app.use(...)
```

The lower-level middleware lives in namespaces such as:

```cpp
vix::middleware::security
vix::middleware::auth
vix::middleware::parsers
vix::middleware::performance
```

When you need lower-level control, adapt it:

```cpp
auto mw = vix::middleware::app::adapt_ctx(
  vix::middleware::parsers::json({
    .require_content_type = true,
    .allow_empty = false,
    .max_bytes = 4096,
    .store_in_state = true
  })
);

app.use("/api/custom-json", std::move(mw));
```

Use the App presets first.

Use lower-level middleware when the preset is not specific enough.

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

Use the shorter form when your project is on `v2.6.2` or newer.

## Static files are separate

Static file serving is handled by `vix::App`, not by the middleware module.

Use:

```cpp
app.static_dir("public", "/", "index.html");
```

for public files.

Middleware can still enhance static responses through a static response hook, for example compression, but static file routing itself belongs to Core.

Keep this mental model:

```txt
Core serves static files.
Middleware protects and enhances HTTP behavior.
```

## Next steps

Continue with:

```txt
App Integration
Core Concepts
Basics
Security
Authentication
Parsers
HTTP Cache
Performance
Observability
API Reference
```

The quick start shows the normal path.

The next pages explain how the pieces work and how to configure them.
