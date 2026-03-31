# Body Limit (Beginner Guide)

Welcome 👋\
This page explains how to use the **body limit middleware** in Vix.cpp.

Body limits protect your server from:

-   very large requests (accidental or malicious)
-   memory pressure (huge JSON uploads)
-   slow uploads (DoS patterns)
-   endpoints that should never accept big bodies

When the request body is too large, Vix returns:

-   **413 Payload Too Large**

## What does "body limit" mean?

It means: **do not accept requests bigger than N bytes**.

Example: if you set `max_bytes = 32`:

-   0..32 bytes is allowed
-   33+ bytes is rejected with 413

## 1) Minimal example

This server:

-   keeps `/` public
-   limits `/api/*` requests to **32 bytes**
-   lets GET pass (default behavior)
-   demonstrates a strict route that rejects chunked uploads

``` cpp
#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>

using namespace vix;

static void register_routes(App &app)
{
  app.get("/", [](Request &, Response &res)
          { res.send("body_limit example: /api/ping, /api/echo, /api/strict"); });

  app.get("/api/ping", [](Request &, Response &res)
          { res.json({"ok", true, "msg", "pong"}); });

  app.post("/api/echo", [](Request &req, Response &res)
           { res.json({"ok", true,
                       "bytes", static_cast<long long>(req.body().size()),
                       "content_type", req.header("content-type")}); });

  app.post("/api/strict", [](Request &req, Response &res)
           { res.json({"ok", true,
                       "msg", "strict accepted",
                       "bytes", static_cast<long long>(req.body().size())}); });
}

int main()
{
  App app;

  // /api: max 32 bytes (demo), chunked allowed
  app.use("/api", middleware::app::body_limit_dev(
                      32,    // max_bytes
                      false, // apply_to_get
                      true   // allow_chunked
                      ));

  // /api/strict: max 32 bytes, chunked NOT allowed
  app.use("/api/strict", middleware::app::body_limit_dev(
                             32,    // max_bytes
                             false, // apply_to_get
                             false  // allow_chunked (strict)
                             ));

  register_routes(app);

  app.run(8080);
  return 0;
}
```

## 2) Test with curl

Run:

``` bash
vix run body_limit_app.cpp
```

### Small body (OK)

``` bash
curl -i -X POST http://localhost:8080/api/echo   -H "Content-Type: text/plain"   --data "hello"
```

### Large body (413 Payload Too Large)

``` bash
python3 - <<'PY'
import requests
print(requests.post("http://localhost:8080/api/echo", data="x"*64).status_code)
PY
```

### GET is ignored by default

``` bash
curl -i http://localhost:8080/api/ping
```

### Strict mode: reject chunked bodies

This simulates a request with `Transfer-Encoding: chunked` (no
Content-Length).\
If `allow_chunked=false`, the server returns **411 Length Required**.

``` bash
curl -i -X POST http://localhost:8080/api/strict   -H "Transfer-Encoding: chunked"   -H "Content-Type: text/plain"   --data "hello"
```

## 3) A safer production preset: only limit write methods

Usually, you only want body limits on:

-   POST
-   PUT
-   PATCH

Vix provides an alias for that:

``` cpp
app.use("/", middleware::app::body_limit_write_dev(16));
```

Meaning:

-   GET is not limited
-   only write methods are limited to 16 bytes

This is perfect for:

-   login endpoints
-   webhook endpoints
-   small JSON APIs
-   protecting uploads unless you explicitly allow them

## 4) Conditional body limit with `should_apply` (advanced but useful)

Sometimes you want to limit only some paths.

Example idea:

-   allow unlimited `/health` and `/api/ping`
-   limit `/upload` and `/api/*`

You can do it using the `should_apply` callback in `body_limit_dev()`.

``` cpp
app.use("/", middleware::app::body_limit_dev(
  16,      // max_bytes
  false,   // apply_to_get
  true,    // allow_chunked
  [](const vix::middleware::Context& ctx){
    const auto m = ctx.req().method();
    if (m != "POST" && m != "PUT" && m != "PATCH")
      return false;

    const auto p = ctx.req().path();
    return (p == "/upload" || p.rfind("/api/", 0) == 0);
  }
));
```

If you're a beginner, you can skip this section and use
`body_limit_write_dev()`.

## Common beginner mistakes

1)  Setting the limit too low\
    If you limit to 32 bytes but your JSON request is 200 bytes,
    everything fails.

2)  Forgetting chunked uploads exist\
    Some clients stream data. If you want strict enforcement, set
    `allow_chunked=false`.

3)  Applying to GET by accident\
    Usually GET has no body. Keep `apply_to_get=false` unless you really
    need it.

## Summary

-   `body_limit_dev(max_bytes, apply_to_get, allow_chunked)` is the main
    preset
-   For most apps, use `body_limit_write_dev(max_bytes)`
-   Too large body -\> 413
-   Strict mode with `allow_chunked=false` can return 411

