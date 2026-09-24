# Basics

The `basics` group contains the middleware that should exist near the foundation of most Vix HTTP backends.

These middleware functions do not implement business logic. They make the request pipeline safer, easier to debug, and more predictable.

They help with:

```txt id="z0ozhb"
recovering from exceptions
assigning request ids
measuring request time
rejecting oversized bodies
logging completed requests
```

The basics group lives under:

```cpp id="r9to3c"
namespace vix::middleware::basics
```

When using `vix::App`, prefer the App helpers:

```cpp id="oi1t4z"
namespace vix::middleware::app
```

## What basics provides

The basics group includes:

| Middleware     | Purpose                                                 |
| -------------- | ------------------------------------------------------- |
| `recovery()`   | Catch exceptions and return a normalized `500` response |
| `request_id()` | Create or reuse a request id                            |
| `timing()`     | Measure request duration                                |
| `body_limit()` | Reject request bodies that are too large                |
| `logger()`     | Write one summary line after request handling           |

For normal `vix::App` applications, use the App presets:

```cpp id="qoy8v4"
middleware::app::recovery_dev()
middleware::app::request_id_dev()
middleware::app::timing_dev()
middleware::app::body_limit_dev()
middleware::app::body_limit_write_dev(...)
```

## Basic backend setup

A small backend can start with:

```cpp id="k1wyni"
#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

int main()
{
  App app;

  app.use("/api", middleware::app::recovery_dev());
  app.use("/api", middleware::app::request_id_dev());
  app.use("/api", middleware::app::timing_dev());
  app.use("/api", middleware::app::body_limit_write_dev(1024 * 1024));

  app.get("/api/health", [](Request &, Response &res)
  {
    res.json({
      "ok", true
    });
  });

  app.post("/api/echo", [](Request &req, Response &res)
  {
    res.json({
      "ok", true,
      "bytes", static_cast<long long>(req.body().size())
    });
  });

  app.run(8080);
}
```

This gives the `/api` routes a basic request foundation.

```txt id="x6d1ot"
recovery
  catches failures

request_id
  gives the request an id

timing
  measures request duration

body_limit
  rejects large write bodies
```

## Recommended order

A practical order is:

```cpp id="w6pkaj"
app.use("/api", middleware::app::recovery_dev());
app.use("/api", middleware::app::request_id_dev());
app.use("/api", middleware::app::timing_dev());
app.use("/api", middleware::app::body_limit_write_dev(1024 * 1024));
```

The idea is:

```txt id="ofbzq3"
recovery wraps the request
request_id makes the request traceable
timing measures downstream work
body_limit rejects oversized write bodies early
```

After basics, you can add security, authentication, parsers, cache, performance, and observability.

Example:

```cpp id="yoidiu"
app.use("/api", middleware::app::recovery_dev());
app.use("/api", middleware::app::request_id_dev());
app.use("/api", middleware::app::timing_dev());
app.use("/api", middleware::app::body_limit_write_dev(1024 * 1024));

app.use("/api", middleware::app::security_headers_dev());
app.use("/api", middleware::app::cors_dev());
app.use("/api", middleware::app::rate_limit_dev());

app.use("/api/users", middleware::app::json_strict_dev(4096));
```

## Recovery

`recovery()` catches exceptions thrown by downstream middleware or handlers and turns them into a normalized `500` response.

Without recovery, an exception can escape the request pipeline.

With recovery, the server can respond cleanly.

```cpp id="l03j18"
app.use("/api", middleware::app::recovery_dev());

app.get("/api/fail", [](Request &, Response &)
{
  throw std::runtime_error("database unavailable");
});
```

Request:

```bash id="cnsd2n"
curl -i http://127.0.0.1:8080/api/fail
```

Expected status:

```txt id="0uny7l"
500 Internal Server Error
```

Recovery is usually installed early so it wraps the rest of the chain.

```txt id="3gwfwk"
recovery
  -> request id
  -> timing
  -> security
  -> auth
  -> parser
  -> handler
```

## Configure recovery

Use the lower-level middleware when you need explicit options.

```cpp id="l5wbrm"
vix::middleware::basics::RecoveryOptions opt;

opt.include_exception_message = false;
opt.code = "internal_server_error";
opt.message = "Internal Server Error";

app.use("/api", vix::middleware::app::adapt_ctx(
  vix::middleware::basics::recovery(opt)
));
```

In development, you may include the exception message.

```cpp id="8eelv7"
opt.include_exception_message = true;
```

In production, keep it false unless you intentionally want exception details in responses.

## Request id

`request_id()` gives each request a stable identifier.

The default header is:

```txt id="l9fbfu"
x-request-id
```

The middleware can:

```txt id="khna7q"
accept a valid incoming request id
generate one when missing
store it in request state
write it back to the response header
```

Use it when you want to connect a client request, logs, metrics, and errors.

```cpp id="qai8u2"
app.use("/api", middleware::app::request_id_dev());

app.get("/api/health", [](Request &req, Response &res)
{
  auto *rid = req.try_state<middleware::basics::RequestId>();

  res.json({
    "ok", true,
    "request_id", rid ? rid->value : ""
  });
});
```

Request:

```bash id="n4p23k"
curl -i http://127.0.0.1:8080/api/health
```

Response headers can include:

```txt id="sh6jws"
x-request-id: ...
```

Response body shape:

```json id="u84bc8"
{
  "ok": true,
  "request_id": "..."
}
```

## Incoming request ids

A client can send an id:

```bash id="ypsnvt"
curl -i \
  http://127.0.0.1:8080/api/health \
  -H "x-request-id: req_demo_1234"
```

If the id is valid, the middleware can reuse it.

Invalid ids are ignored and a new one can be generated.

Accepted ids are intentionally limited to reasonable characters and size.

This prevents unsafe or very large values from being copied into logs and response headers.

## Configure request id

Use `RequestIdOptions` for lower-level control.

```cpp id="kwp86w"
vix::middleware::basics::RequestIdOptions opt;

opt.header_name = "x-request-id";
opt.accept_incoming = true;
opt.generate_if_missing = true;
opt.always_set_response_header = true;

app.use("/api", vix::middleware::app::adapt_ctx(
  vix::middleware::basics::request_id(opt)
));
```

Main options:

| Option                       | Purpose                                      |
| ---------------------------- | -------------------------------------------- |
| `header_name`                | Header used to read and write the request id |
| `accept_incoming`            | Accept a valid incoming request id           |
| `generate_if_missing`        | Generate a request id when none is present   |
| `always_set_response_header` | Write the request id to the response         |

## Timing

`timing()` measures how long downstream middleware and handlers take to run.

It can write response headers such as:

```txt id="kis1ra"
x-response-time: 2ms
server-timing: total;dur=2
```

It can also store the result in typed request state:

```cpp id="qjdnr6"
vix::middleware::basics::Timing
```

Example:

```cpp id="s3gn76"
app.use("/api", middleware::app::timing_dev());

app.get("/api/work", [](Request &, Response &res)
{
  res.json({
    "ok", true
  });
});
```

Request:

```bash id="j2blhw"
curl -i http://127.0.0.1:8080/api/work
```

Response headers can include:

```txt id="gv7a8r"
x-response-time: 1ms
server-timing: total;dur=1
```

## Read Timing state

A handler can read timing state when available.

```cpp id="z8y1bi"
app.get("/api/debug", [](Request &req, Response &res)
{
  auto *timing = req.try_state<middleware::basics::Timing>();

  res.json({
    "duration_ms", timing ? timing->total_ms : 0
  });
});
```

In many cases, timing is more useful as a response header or as input for logging and metrics.

## Configure timing

Use `TimingOptions` for lower-level control.

```cpp id="n9jmkz"
vix::middleware::basics::TimingOptions opt;

opt.set_x_response_time = true;
opt.set_server_timing = true;
opt.store_in_state = true;
opt.x_response_time_header = "x-response-time";
opt.server_timing_header = "server-timing";
opt.server_timing_metric = "total";

app.use("/api", vix::middleware::app::adapt_ctx(
  vix::middleware::basics::timing(opt)
));
```

Main options:

| Option                   | Purpose                            |
| ------------------------ | ---------------------------------- |
| `set_x_response_time`    | Write `x-response-time`            |
| `set_server_timing`      | Write `server-timing`              |
| `store_in_state`         | Store `Timing` in request state    |
| `x_response_time_header` | Header name for response time      |
| `server_timing_header`   | Header name for server timing      |
| `server_timing_metric`   | Metric name inside `Server-Timing` |

## Body limit

`body_limit()` rejects request bodies that are too large.

It protects the application before parsers and handlers do more work.

This is especially important before:

```txt id="kz3c3w"
JSON parsing
form parsing
multipart parsing
file upload handling
authentication endpoints
public POST routes
```

Use a body limit before parsers.

```cpp id="vvvhf5"
app.use("/api", middleware::app::body_limit_write_dev(1024));
app.use("/api/users", middleware::app::json_strict_dev(1024));
```

Request with a small body:

```bash id="pvdij0"
curl -i \
  -X POST http://127.0.0.1:8080/api/echo \
  -H "Content-Type: text/plain" \
  --data "hello"
```

Expected status:

```txt id="sl4hfk"
200 OK
```

Request with a large body:

```bash id="3tpozy"
python3 - <<'PY' > /tmp/large.txt
print("x" * 2048)
PY

curl -i \
  -X POST http://127.0.0.1:8080/api/echo \
  -H "Content-Type: text/plain" \
  --data-binary @/tmp/large.txt
```

Expected status:

```txt id="lbwx6g"
413 Payload Too Large
```

The handler is not called.

## Body limit for write methods

Most APIs should limit write methods.

```cpp id="x2svzv"
app.use("/api", middleware::app::body_limit_write_dev(1024 * 1024));
```

This targets methods such as:

```txt id="lws4b5"
POST
PUT
PATCH
```

It avoids applying body rules to normal `GET` routes unless you explicitly want that.

## Strict body limit

Some applications require `Content-Length`.

A strict body limit can reject requests without a length when chunked bodies are not allowed.

The lower-level option is:

```cpp id="cep9rg"
vix::middleware::basics::BodyLimitOptions opt;

opt.max_bytes = 1024;
opt.apply_to_get = false;
opt.allow_chunked = false;
```

Install it with `adapt_ctx()`:

```cpp id="f8rarw"
app.use("/api/upload", vix::middleware::app::adapt_ctx(
  vix::middleware::basics::body_limit(opt)
));
```

If the request has no `Content-Length` and chunked bodies are not allowed, the middleware can return:

```txt id="jwxw4m"
411 Length Required
```

## Conditional body limit

For advanced cases, `BodyLimitOptions` supports `should_apply`.

```cpp id="sir48u"
vix::middleware::basics::BodyLimitOptions opt;

opt.max_bytes = 16;
opt.should_apply = [](const vix::middleware::Context &ctx)
{
  const auto &req = ctx.req();

  return req.method() == "POST" ||
         req.method() == "PUT" ||
         req.method() == "PATCH";
};

app.use("/api", vix::middleware::app::adapt_ctx(
  vix::middleware::basics::body_limit(opt)
));
```

Use `should_apply` when you need custom method, route, header, or content-type rules.

For normal applications, use the App preset first.

## Configure body limit

Main options:

| Option          | Purpose                                              |
| --------------- | ---------------------------------------------------- |
| `max_bytes`     | Maximum allowed request body size                    |
| `apply_to_get`  | Whether the middleware applies to `GET`              |
| `allow_chunked` | Whether requests without `Content-Length` may pass   |
| `should_apply`  | Custom predicate to decide whether the limit applies |

Common errors:

| Status | Code                | Meaning                                    |
| ------ | ------------------- | ------------------------------------------ |
| `413`  | `payload_too_large` | Body or `Content-Length` exceeds the limit |
| `411`  | `length_required`   | Length is required but missing             |

## Logger

`logger()` writes one summary line after downstream middleware and the route handler run.

It can log:

```txt id="r39i0g"
method
path
status
duration
request id
user agent
forwarded client ip
```

The logger middleware uses an `ILogger` service from the middleware services container.

That makes it useful for custom integrations where you provide your own logging sink.

For most applications, Vix Core already has server logging. Use `logger()` when you want middleware-level request logging with a custom service.

## Logger interface

A logger service implements:

```cpp id="oi0e33"
struct ILogger
{
  virtual ~ILogger() = default;

  virtual void info(std::string_view msg) = 0;
  virtual void warn(std::string_view msg) = 0;
  virtual void error(std::string_view msg) = 0;
};
```

The middleware can output either text or JSON-style lines depending on options.

```cpp id="vjtvgp"
vix::middleware::basics::LoggerOptions opt;

opt.format = vix::middleware::basics::LogFormat::Text;
opt.log_request_id = true;
opt.log_timing = true;
opt.level_from_status = true;
```

When `level_from_status` is true:

```txt id="f2q0o8"
status >= 500 -> error
status >= 400 -> warn
otherwise     -> info
```

## Use basics with lower-level middleware

App presets are the simplest path.

```cpp id="l4vcb4"
app.use("/api", middleware::app::request_id_dev());
app.use("/api", middleware::app::timing_dev());
app.use("/api", middleware::app::body_limit_write_dev(1024));
```

Use lower-level middleware when you need exact options.

```cpp id="mr8vna"
app.use("/api", vix::middleware::app::adapt_ctx(
  vix::middleware::basics::request_id({
    .header_name = "x-request-id",
    .accept_incoming = true,
    .generate_if_missing = true,
    .always_set_response_header = true
  })
));
```

This pattern works for all context-based middleware:

```txt id="h60fum"
create options
build lower-level middleware
adapt with app::adapt_ctx(...)
install with app.use(...)
```

## Complete example

```cpp id="i95euw"
#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

int main()
{
  App app;

  app.use("/api", middleware::app::recovery_dev());
  app.use("/api", middleware::app::request_id_dev());
  app.use("/api", middleware::app::timing_dev());
  app.use("/api", middleware::app::body_limit_write_dev(1024));

  app.get("/api/health", [](Request &req, Response &res)
  {
    auto *rid = req.try_state<middleware::basics::RequestId>();

    res.json({
      "ok", true,
      "request_id", rid ? rid->value : ""
    });
  });

  app.post("/api/echo", [](Request &req, Response &res)
  {
    res.json({
      "ok", true,
      "bytes", static_cast<long long>(req.body().size())
    });
  });

  app.get("/api/fail", [](Request &, Response &)
  {
    throw std::runtime_error("demo failure");
  });

  app.run(8080);
}
```

Run it:

```bash id="v6s4d9"
vix run basics_demo.cpp
```

Test health:

```bash id="lddzbu"
curl -i http://127.0.0.1:8080/api/health
```

Test body limit:

```bash id="cswxzw"
python3 - <<'PY' > /tmp/large.txt
print("x" * 2048)
PY

curl -i \
  -X POST http://127.0.0.1:8080/api/echo \
  --data-binary @/tmp/large.txt
```

Test recovery:

```bash id="02l66s"
curl -i http://127.0.0.1:8080/api/fail
```

Expected behavior:

```txt id="u01rap"
health returns 200
large body returns 413
failure returns 500
responses include request/timing headers when enabled
```

## Summary

The basics group gives your backend its first layer of reliability.

Use it to:

```txt id="wzahja"
catch unexpected failures
make requests traceable
measure response time
reject oversized bodies early
prepare the pipeline for security, auth, parsers, and performance middleware
```

A good default stack is:

```cpp id="oh2o76"
app.use("/api", middleware::app::recovery_dev());
app.use("/api", middleware::app::request_id_dev());
app.use("/api", middleware::app::timing_dev());
app.use("/api", middleware::app::body_limit_write_dev(1024 * 1024));
```

Then add the rest of your backend behavior on top.
