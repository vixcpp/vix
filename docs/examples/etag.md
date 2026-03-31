# ETag Middleware Guide

## What is an ETag?

ETag stands for **Entity Tag**.

It is an HTTP response header used for **cache validation**.

Instead of re-downloading a resource every time, the client can say:

> "I already have version X. Has it changed?"

If not changed → server replies:

    304 Not Modified

No body is sent.\
This saves bandwidth and improves performance.

## How ETag Works

1.  Server sends response with:

```{=html}
<!-- -->
```
    ETag: "abc123"

2.  Client stores it.

3.  On next request, client sends:

```{=html}
<!-- -->
```
    If-None-Match: "abc123"

4.  Server compares:

-   If same → `304 Not Modified`
-   If different → `200 OK` with new body + new ETag

# Minimal Example

Save as: `etag_app_simple.cpp`

``` cpp
#include <iostream>
#include <string>
#include <vix.hpp>

#include <vix/middleware/app/adapter.hpp>
#include <vix/middleware/performance/etag.hpp>

using namespace vix;

static void print_help()
{
  std::cout
      << "Vix ETag example running:\n"
      << "  http://localhost:8080/x\n\n"
      << "Try:\n"
      << "  curl -i http://localhost:8080/x\n"
      << "  curl -i -H 'If-None-Match: <etag>' http://localhost:8080/x\n"
      << "  curl -I http://localhost:8080/x\n";
}

int main()
{
  App app;

  // Install ETag middleware globally
  auto mw = vix::middleware::app::adapt_ctx(
      vix::middleware::performance::etag({
          .weak = true,
          .add_cache_control_if_missing = false,
          .min_body_size = 1
      }));

  app.use(std::move(mw));

  app.get("/x", [](Request &, Response &res)
  {
      res.text("Hello ETag world");
  });

  app.head("/x", [](Request &, Response &res)
  {
      res.status(200);
  });

  print_help();
  app.run(8080);
  return 0;
}
```

## Run

``` bash
vix run etag_app_simple.cpp
```

## Test Step by Step

### 1) First request

``` bash
curl -i http://localhost:8080/x
```

Response will contain:

    ETag: W/"..."

Copy that value.

### 2) Send If-None-Match

``` bash
curl -i -H 'If-None-Match: W/"...your_etag_here..."' http://localhost:8080/x
```

If unchanged →

    304 Not Modified

No body returned.

### 3) HEAD request

``` bash
curl -I http://localhost:8080/x
```

HEAD returns headers only.\
ETag is still calculated.

# Middleware Options Explained

``` cpp
vix::middleware::performance::etag({
    .weak = true,
    .add_cache_control_if_missing = false,
    .min_body_size = 1
});
```

### weak

-   `true` → `W/"hash"`
-   `false` → `"hash"` (strong)

Weak ETags are recommended for dynamic APIs.

### add_cache_control_if_missing

If true and response has no Cache-Control header, middleware can inject
a default one.

### min_body_size

Only compute ETag if body \>= this size.

Helps skip tiny responses.

# When to Use ETag

Use ETag for:

-   JSON APIs
-   Static content
-   CDN friendly responses
-   Any idempotent GET endpoint

Avoid for:

-   Non-deterministic responses
-   Streaming endpoints

# Production Pattern

Typical production setup:

    Compression
    ETag
    Cache-Control

Example:

``` cpp
app.use(adapt_ctx(compression(...)));
app.use(adapt_ctx(etag({...})));
```

Order matters.

# Common Mistakes

1.  Forgetting GET route (HEAD alone is not enough)
2.  Sending different body order (JSON fields shuffled)
3.  Mixing weak/strong inconsistently
4.  Not handling 304 correctly on client side

# Summary

ETag gives you:

-   Conditional requests
-   Bandwidth savings
-   Faster responses
-   Cleaner caching logic

Minimal, powerful, and production-ready.

