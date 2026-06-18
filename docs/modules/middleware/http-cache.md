# HTTP Cache

The HTTP cache middleware stores and replays dynamic `GET` responses.

It is useful when a route is expensive but returns the same response for the same request key during a short time window.

Examples:

```txt id="xj2w3h"
GET /api/users
GET /api/products?page=1
GET /api/categories
GET /api/public-feed
GET /api/search?q=phone
```

The HTTP cache middleware is different from static file caching.

```txt id="d0hv7h"
app.static_dir(...)
  serves public files and can add Cache-Control headers

middleware::app::http_cache(...)
  caches dynamic GET responses produced by route handlers
```

This page is about dynamic route responses.

## What it does

The HTTP cache middleware sits before your route handler.

On a cache miss:

```txt id="wl7vn9"
request
  -> HTTP cache middleware
  -> route handler runs
  -> response is stored
  -> response is sent
```

On a cache hit:

```txt id="u0ppe9"
request
  -> HTTP cache middleware
  -> cached response is replayed
  -> route handler is not called
```

That is the main value.

Expensive handlers can be skipped when the response is already cached.

## Basic example

```cpp id="z4pmfr"
#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

int main()
{
  App app;

  app.use("/api", middleware::app::http_cache({
    .ttl_ms = 30'000,
    .allow_bypass = true,
    .bypass_header = "x-vix-cache",
    .bypass_value = "bypass"
  }));

  app.get("/api/users", [](Request &, Response &res)
  {
    res.json({
      "ok", true,
      "source", "origin"
    });
  });

  app.get("/", [](Request &, Response &res)
  {
    res.text("home route is not cached");
  });

  app.run(8080);
}
```

Run:

```bash id="vh59z3"
vix run http_cache_demo.cpp
```

First request:

```bash id="bez8am"
curl -i http://127.0.0.1:8080/api/users
```

Expected cache status:

```txt id="w8ufqi"
x-vix-cache-status: miss
```

Second request:

```bash id="v2zmd8"
curl -i http://127.0.0.1:8080/api/users
```

Expected cache status:

```txt id="jqy5kx"
x-vix-cache-status: hit
```

Bypass the cache:

```bash id="t4d8ff"
curl -i \
  http://127.0.0.1:8080/api/users \
  -H "x-vix-cache: bypass"
```

Expected cache status:

```txt id="lg8tu8"
x-vix-cache-status: bypass
```

## Use it only for safe GET routes

The HTTP cache middleware is designed for `GET` responses.

Do not use it for routes that mutate state.

Good candidates:

```txt id="c0qr6h"
GET /api/products
GET /api/categories
GET /api/posts
GET /api/public-profile
GET /api/search
```

Bad candidates:

```txt id="ibf58d"
POST /api/orders
PUT /api/products/1
PATCH /api/profile
DELETE /api/items/1
GET routes that depend on hidden user/session state
```

A cache hit skips the route handler.

That is excellent for safe public data.

It is dangerous if the handler must always run.

## Cache key

The middleware builds a cache key from request data.

The key can include:

```txt id="ifhtxw"
HTTP method
path
query string
selected headers
```

This means different query strings can produce different cached responses.

Example:

```txt id="t0g8kb"
/api/products?page=1
/api/products?page=2
```

These should not share the same cached response.

Header-based variants can also be supported with `vary_headers`.

## Vary headers

Use `vary_headers` when a response changes depending on request headers.

Example:

```cpp id="i78j7f"
app.use("/api", middleware::app::http_cache({
  .ttl_ms = 30'000,
  .allow_bypass = true,
  .bypass_header = "x-vix-cache",
  .bypass_value = "bypass",
  .vary_headers = {"accept-language"}
}));
```

Route:

```cpp id="xx1xuk"
app.get("/api/users", [](Request &req, Response &res)
{
  const std::string lang =
    req.has_header("accept-language")
      ? req.header("accept-language")
      : "none";

  res.json({
    "ok", true,
    "source", "origin",
    "accept_language", lang
  });
});
```

Request in French:

```bash id="va3qpz"
curl -i \
  http://127.0.0.1:8080/api/users \
  -H "Accept-Language: fr"
```

Request in English:

```bash id="xb9mh9"
curl -i \
  http://127.0.0.1:8080/api/users \
  -H "Accept-Language: en"
```

These can produce separate cache entries.

Use `vary_headers` when the route response depends on headers such as:

```txt id="us4if5"
accept-language
accept
x-tenant-id
x-currency
```

Do not vary on headers that do not affect the response.

Unnecessary vary headers reduce cache reuse.

## Bypass header

The bypass header lets a client force the origin handler to run.

Default-style configuration:

```cpp id="toqtyh"
app.use("/api", middleware::app::http_cache({
  .ttl_ms = 30'000,
  .allow_bypass = true,
  .bypass_header = "x-vix-cache",
  .bypass_value = "bypass"
}));
```

Bypass request:

```bash id="s1vx4v"
curl -i \
  http://127.0.0.1:8080/api/users \
  -H "x-vix-cache: bypass"
```

This is useful for:

```txt id="x3admb"
debugging
manual refresh
admin tools
checking origin response
```

Disable bypass if clients should never be able to skip cache.

```cpp id="unvjoc"
app.use("/api", middleware::app::http_cache({
  .ttl_ms = 30'000,
  .allow_bypass = false
}));
```

## Cache status header

The middleware can write a cache status header.

Common values are:

```txt id="lprnzd"
miss
hit
bypass
```

Example:

```txt id="ip0q5b"
x-vix-cache-status: miss
```

A `miss` means the handler ran and the response may have been stored.

A `hit` means the response came from cache and the handler was skipped.

A `bypass` means the request asked to skip cache.

Use this header during development and diagnostics.

## TTL

`ttl_ms` controls how long a cached response can be reused.

```cpp id="jhhuz2"
app.use("/api", middleware::app::http_cache({
  .ttl_ms = 30'000
}));
```

This stores responses for about 30 seconds.

Use short TTLs for data that changes often.

Use longer TTLs for public data that changes rarely.

Examples:

| Data               | Suggested TTL                     |
| ------------------ | --------------------------------- |
| Health metadata    | Very short or no cache            |
| Public categories  | Longer                            |
| Product lists      | Short to medium                   |
| Search results     | Short                             |
| User-specific data | Avoid caching unless keyed safely |

The cache does not replace database correctness.

It only reduces repeated handler work for safe responses.

## Custom cache instance

You can inject a custom cache instance.

```cpp id="xv9vyp"
#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

int main()
{
  App app;

  auto cache = middleware::app::make_default_cache({
    .ttl_ms = 30'000
  });

  app.use("/api", middleware::app::http_cache_mw({
    .prefix = "/api",
    .only_get = true,
    .ttl_ms = 30'000,
    .allow_bypass = true,
    .bypass_header = "x-vix-cache",
    .bypass_value = "bypass",
    .vary_headers = {},
    .cache = cache,
    .add_debug_header = true,
    .debug_header = "x-vix-cache-status"
  }));

  app.get("/api/slow", [](Request &, Response &res)
  {
    res.text("slow response from origin");
  });

  app.run(8080);
}
```

Use a custom cache when:

```txt id="o3zi8d"
several middleware instances should share one cache
you want to control the cache policy explicitly
you want to inject a different store later
```

## Slow route example

This example makes cache behavior visible.

```cpp id="n8cfxu"
#include <chrono>
#include <thread>

#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

int main()
{
  App app;

  app.use("/api", middleware::app::http_cache({
    .ttl_ms = 30'000,
    .allow_bypass = true,
    .bypass_header = "x-vix-cache",
    .bypass_value = "bypass"
  }));

  app.get("/api/slow", [](Request &, Response &res)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    res.text("slow response from origin");
  });

  app.run(8080);
}
```

First request:

```bash id="vuz32h"
time curl -i http://127.0.0.1:8080/api/slow
```

Second request:

```bash id="l9yh47"
time curl -i http://127.0.0.1:8080/api/slow
```

The second request should avoid the artificial delay if it is served from cache.

Bypass:

```bash id="snkvjh"
time curl -i \
  http://127.0.0.1:8080/api/slow \
  -H "x-vix-cache: bypass"
```

The bypassed request should run the handler again.

## Configuration options

App-level HTTP cache configuration:

```cpp id="bhdzdc"
middleware::app::HttpCacheConfig{
  .prefix = "/api/",
  .only_get = true,
  .ttl_ms = 30'000,
  .allow_bypass = true,
  .bypass_header = "x-vix-cache",
  .bypass_value = "bypass",
  .vary_headers = {},
  .cache = nullptr,
  .add_debug_header = false,
  .debug_header = "x-vix-cache-status"
}
```

Main options:

| Option             | Purpose                                 |
| ------------------ | --------------------------------------- |
| `prefix`           | Prefix used by install helpers          |
| `only_get`         | Apply cache only to GET requests        |
| `ttl_ms`           | Cache lifetime in milliseconds          |
| `allow_bypass`     | Allow bypass header                     |
| `bypass_header`    | Header used to request bypass           |
| `bypass_value`     | Header value that triggers bypass       |
| `vary_headers`     | Headers included in the cache key       |
| `cache`            | Custom cache instance                   |
| `add_debug_header` | Add cache status header for diagnostics |
| `debug_header`     | Name of cache status header             |

For most applications, start with:

```cpp id="fehyvg"
app.use("/api", middleware::app::http_cache({
  .ttl_ms = 30'000
}));
```

Then add bypass and vary headers as needed.

## Lower-level HTTP cache middleware

The lower-level middleware is:

```cpp id="ek1r4w"
vix::middleware::http_cache(...)
```

It returns:

```cpp id="sq85zg"
vix::middleware::HttpMiddleware
```

The App helper adapts it for `vix::App`.

Use the lower-level API when building custom integrations.

Normal applications should use:

```cpp id="vteycj"
middleware::app::http_cache(...)
```

or:

```cpp id="qo555n"
middleware::app::http_cache_mw(...)
```

## Cache and status codes

By default, the middleware should cache successful `200` responses.

Avoid caching error responses unless you explicitly want that behavior.

A useful rule:

```txt id="vgf7to"
cache 200 OK
avoid caching 401, 403, 404, 422, 500
```

This prevents temporary failures from being replayed longer than intended.

## Cache and response bodies

Some responses should not be cached.

Avoid caching:

```txt id="sugmyu"
empty responses when the body matters
streaming responses
user-specific private responses
responses with volatile data
responses depending on cookies or sessions
```

Good cached responses are usually:

```txt id="wehpda"
public
safe
repeatable
not tied to one user session
valid for a short TTL
```

## Cache and authenticated routes

Be careful caching authenticated routes.

If a response depends on the authenticated user, the cache key must include something that separates users.

Examples:

```txt id="hhyme8"
Authorization
x-user-id
x-tenant-id
```

But caching by `Authorization` can create many cache entries and may be undesirable.

The safest starting rule is:

```txt id="x3g0j6"
cache public GET routes first
avoid private user-specific routes until the cache key is designed carefully
```

Good first targets:

```txt id="s2tizw"
public categories
public product lists
public blog posts
public docs metadata
```

Risky first targets:

```txt id="mcwqvx"
current user profile
admin dashboard
cart
orders
notifications
private messages
```

## HTTP cache vs ETag

HTTP cache middleware stores the response on the server side and can skip the handler on cache hits.

ETag helps the client revalidate a response.

```txt id="k6rzja"
HTTP cache
  server-side response reuse
  handler can be skipped

ETag
  client-side validation
  client may receive 304 Not Modified
```

They can be used together, but they solve different problems.

Use HTTP cache when the server should avoid repeated handler work.

Use ETag when clients should avoid downloading the same body again.

## HTTP cache vs static files

Static files are served by Core through:

```cpp id="mx0cnq"
app.static_dir(...)
```

Static files can receive `Cache-Control` headers from Core configuration.

HTTP cache middleware is for dynamic route responses:

```cpp id="avab1r"
app.get("/api/users", [](Request &, Response &res)
{
  res.json({
    "ok", true
  });
});
```

Keep this separation:

```txt id="hmx3t1"
Core static files
  public files, index.html, SPA fallback, Cache-Control

HTTP cache middleware
  dynamic GET route responses
```

Do not document static file serving as HTTP cache middleware.

## Complete example

```cpp id="d6hx80"
#include <chrono>
#include <thread>

#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

int main()
{
  App app;

  app.use("/api", middleware::app::security_headers_dev());
  app.use("/api", middleware::app::rate_limit_dev());

  app.use("/api", middleware::app::http_cache({
    .ttl_ms = 30'000,
    .allow_bypass = true,
    .bypass_header = "x-vix-cache",
    .bypass_value = "bypass",
    .vary_headers = {"accept-language"}
  }));

  app.get("/api/users", [](Request &req, Response &res)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    const std::string lang =
      req.has_header("accept-language")
        ? req.header("accept-language")
        : "none";

    res.json({
      "ok", true,
      "source", "origin",
      "accept_language", lang
    });
  });

  app.get("/", [](Request &, Response &res)
  {
    res.text("public home, not cached by /api middleware");
  });

  app.run(8080);
}
```

Run:

```bash id="klc4z9"
vix run http_cache_complete.cpp
```

First request:

```bash id="hpxpq4"
curl -i http://127.0.0.1:8080/api/users
```

Second request:

```bash id="xf4woa"
curl -i http://127.0.0.1:8080/api/users
```

Different language:

```bash id="pb44yx"
curl -i \
  http://127.0.0.1:8080/api/users \
  -H "Accept-Language: fr"
```

Bypass:

```bash id="taw9kv"
curl -i \
  http://127.0.0.1:8080/api/users \
  -H "x-vix-cache: bypass"
```

Expected behavior:

```txt id="p78t56"
first request
  cache miss, handler runs

second same request
  cache hit, handler skipped

different Accept-Language
  different cache key

bypass
  handler runs again
```

## Summary

Use HTTP cache middleware for dynamic `GET` routes that are safe to replay for a short time.

Good starting point:

```cpp id="fwcq48"
app.use("/api", middleware::app::http_cache({
  .ttl_ms = 30'000,
  .allow_bypass = true,
  .bypass_header = "x-vix-cache",
  .bypass_value = "bypass"
}));
```

Remember:

```txt id="vpn2wd"
cache public GET routes first
use vary_headers when headers change the response
avoid private user-specific routes until the cache key is designed carefully
static files belong to Core, not HTTP cache middleware
```
