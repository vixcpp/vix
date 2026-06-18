# Observability

The `observability` group helps you understand what happens inside the middleware pipeline.

It is designed for backend visibility:

```txt id="m63frg"
trace request flow
measure request counts and durations
debug middleware execution
attach identifiers to responses
inspect failures
```

Observability does not change your business logic.

It gives you better visibility into how requests move through your backend.

The observability middleware lives under:

```cpp id="w608dy"
namespace vix::middleware::observability
```

Most applications start with `vix::App`.

Lower-level observability hooks are most useful with:

```cpp id="rjvvmt"
vix::middleware::HttpPipeline
```

## What observability provides

The observability group includes:

| Feature               | Purpose                                            |
| --------------------- | -------------------------------------------------- |
| `tracing_hooks()`     | Create trace and span identifiers around a request |
| `tracing_mw()`        | Install tracing as normal middleware               |
| `metrics_hooks()`     | Record request metrics through pipeline hooks      |
| `debug_trace_hooks()` | Emit simple begin, end, and error debug lines      |
| `debug_trace_mw()`    | Install debug trace as normal middleware           |
| `safe_method(...)`    | Return a safe method label                         |
| `safe_path(...)`      | Return a safe path label                           |

Observability can be used in two ways:

```txt id="k0q41s"
as normal middleware
  with app.use(...)

as pipeline hooks
  with HttpPipeline
```

Use normal middleware for App applications.

Use hooks for low-level pipelines, tests, and custom integrations.

## Why observability matters

A backend needs more than successful responses.

You also need to know:

```txt id="f9oz8x"
which route was called
how long it took
which request id was used
which trace id was generated
whether the request failed
which middleware stopped the request
how many requests were processed
```

Without observability, failures are harder to diagnose.

With observability, the pipeline becomes easier to inspect.

## Request id first

Observability is more useful when every request has an id.

Install request id early:

```cpp id="vuy3th"
app.use("/api", middleware::app::request_id_dev());
```

Then handlers, logs, errors, and responses can refer to the same request.

Example:

```cpp id="xop3bb"
#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

int main()
{
  App app;

  app.use("/api", middleware::app::request_id_dev());
  app.use("/api", middleware::app::timing_dev());

  app.get("/api/health", [](Request &req, Response &res)
  {
    auto *rid = req.try_state<middleware::basics::RequestId>();

    res.json({
      "ok", true,
      "request_id", rid ? rid->value : ""
    });
  });

  app.run(8080);
}
```

Request:

```bash id="ej08cl"
curl -i http://127.0.0.1:8080/api/health
```

Response headers may include:

```txt id="tetbhv"
x-request-id: ...
x-response-time: ...
server-timing: total;dur=...
```

This is the simplest observability layer.

## Timing

Timing is part of the basics group, but it belongs in the observability story.

It measures how long downstream work takes.

```cpp id="hn237s"
app.use("/api", middleware::app::timing_dev());
```

Typical response headers:

```txt id="iwwfkn"
x-response-time: 2ms
server-timing: total;dur=2
```

Use timing when you want a quick view of request cost.

For deeper metrics, use the observability hooks.

## Tracing

Tracing creates identifiers that let you connect work across a request.

The tracing context contains:

```txt id="so6ywn"
trace_id
span_id
parent_span_id
```

The middleware can:

```txt id="c3gdlw"
accept incoming trace ids
generate new trace ids
generate span ids
store TraceContext in request state
emit trace headers in the response
```

## Tracing as App middleware

Use `tracing_mw()` with `app::adapt_ctx(...)`.

```cpp id="c5s3kd"
#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

int main()
{
  App app;

  app.use("/api", middleware::app::adapt_ctx(
    middleware::observability::tracing_mw()
  ));

  app.get("/api/health", [](Request &req, Response &res)
  {
    auto *trace = req.try_state<middleware::observability::TraceContext>();

    res.json({
      "ok", true,
      "trace_id", trace ? trace->trace_id : "",
      "span_id", trace ? trace->span_id : ""
    });
  });

  app.run(8080);
}
```

Request:

```bash id="lxqikr"
curl -i http://127.0.0.1:8080/api/health
```

Expected response shape:

```json id="wjl8rf"
{
  "ok": true,
  "trace_id": "...",
  "span_id": "..."
}
```

The response can also include trace headers.

## Incoming trace headers

A client can send a trace id.

```bash id="iyyr0n"
curl -i \
  http://127.0.0.1:8080/api/health \
  -H "x-trace-id: 0123456789abcdef0123456789abcdef" \
  -H "x-span-id: 0123456789abcdef"
```

If the incoming values are accepted, the middleware can reuse the trace id and store the incoming span as the parent span.

This makes it possible to connect requests across services.

## Configure tracing

Use `TracingOptions` for exact behavior.

```cpp id="bdok9p"
vix::middleware::observability::TracingOptions opt;

opt.trace_header = "x-trace-id";
opt.span_header = "x-span-id";
opt.parent_span_header = "x-parent-span-id";

opt.accept_incoming_trace = true;
opt.accept_incoming_span = true;
opt.emit_response_headers = true;
opt.include_parent_in_response = false;

app.use("/api", vix::middleware::app::adapt_ctx(
  vix::middleware::observability::tracing_mw(opt)
));
```

Main options:

| Option                       | Purpose                                    |
| ---------------------------- | ------------------------------------------ |
| `trace_header`               | Header used for trace id                   |
| `span_header`                | Header used for span id                    |
| `parent_span_header`         | Header used for parent span id             |
| `accept_incoming_trace`      | Accept valid incoming trace ids            |
| `accept_incoming_span`       | Accept valid incoming span ids             |
| `emit_response_headers`      | Write trace headers to the response        |
| `include_parent_in_response` | Include parent span id in response headers |
| `enrich`                     | Custom callback to enrich `TraceContext`   |

## Enrich trace context

`TracingOptions::enrich` lets you customize the trace context.

```cpp id="s1mcv6"
vix::middleware::observability::TracingOptions opt;

opt.enrich = [](vix::middleware::Context &ctx,
                vix::middleware::observability::TraceContext &trace)
{
  (void)ctx;

  if (trace.trace_id.empty())
    return;

  // Add custom integration here later.
};

app.use("/api", vix::middleware::app::adapt_ctx(
  vix::middleware::observability::tracing_mw(opt)
));
```

Use this when you need to connect tracing with custom logging, tenants, services, or external observability tools.

## Debug trace

Debug trace emits simple lines around request execution.

It is useful when you are building or debugging middleware.

It can show:

```txt id="lphp41"
request begin
request end
error path
method
path
status
duration
```

Debug tracing can be used as hooks or as normal middleware.

## Debug trace as App middleware

```cpp id="fvjrjw"
#include <iostream>
#include <memory>
#include <string_view>

#include <vix.hpp>
#include <vix/middleware.hpp>

class StdoutDebugTrace final
  : public vix::middleware::observability::IDebugTraceSink
{
public:
  void log(std::string_view line) override
  {
    std::cout << line << "\n";
  }
};

int main()
{
  vix::App app;

  auto sink = std::make_shared<StdoutDebugTrace>();

  app.use("/api", vix::middleware::app::adapt_ctx(
    vix::middleware::observability::debug_trace_mw(sink)
  ));

  app.get("/api/health", [](vix::Request &, vix::Response &res)
  {
    res.json({
      "ok", true
    });
  });

  app.run(8080);
}
```

Request:

```bash id="nl4u0x"
curl -i http://127.0.0.1:8080/api/health
```

Example debug output:

```txt id="tze9r2"
[vix.debug] begin method=GET path=/api/health
[vix.debug] end method=GET path=/api/health status=200 ms=...
```

Use this for local debugging.

Avoid noisy debug traces in high-traffic production paths unless your sink is designed for that volume.

## Configure debug trace

Use `DebugTraceOptions`.

```cpp id="orv7q2"
vix::middleware::observability::DebugTraceOptions opt;

opt.include_method = true;
opt.include_path = true;
opt.include_status = true;
opt.include_duration_ms = true;
opt.include_trace_ids = true;
opt.prefix = "[vix.debug]";

app.use("/api", vix::middleware::app::adapt_ctx(
  vix::middleware::observability::debug_trace_mw(sink, opt)
));
```

Main options:

| Option                | Purpose                           |
| --------------------- | --------------------------------- |
| `include_method`      | Include request method            |
| `include_path`        | Include request path              |
| `include_status`      | Include response status           |
| `include_duration_ms` | Include duration                  |
| `include_trace_ids`   | Reserved for trace id integration |
| `prefix`              | Prefix added to debug lines       |

## Metrics

Metrics record request activity.

The metrics feature is designed around sinks and pipeline hooks.

A metrics sink can collect:

```txt id="eymjdb"
request count
status count
duration
method labels
path labels
```

Use metrics when you want to see how the application behaves over time.

Examples:

```txt id="xrsm0k"
how many requests reached this pipeline
how many returned 500
which route is slow
how many requests were rejected early
```

## In-memory metrics

For tests and local experiments, use an in-memory sink.

```cpp id="xxqau4"
auto metrics = std::make_shared<
  vix::middleware::observability::InMemoryMetrics
>();
```

Then attach it through hooks with `HttpPipeline`.

```cpp id="z2qvuw"
vix::middleware::HttpPipeline pipeline;

pipeline.set_hooks(
  vix::middleware::observability::metrics_hooks(metrics)
);
```

`HttpPipeline` is lower-level than `vix::App`.

Use it for tests and custom integrations.

## Observability hooks with HttpPipeline

`HttpPipeline` supports hooks:

```txt id="kxewb7"
on_begin
on_end
on_error
```

Observability features can use those hooks.

Example:

```cpp id="om1thi"
#include <memory>

#include <vix/http/Request.hpp>
#include <vix/http/Response.hpp>
#include <vix/http/ResponseWrapper.hpp>
#include <vix/middleware.hpp>

int main()
{
  auto metrics = std::make_shared<
    vix::middleware::observability::InMemoryMetrics
  >();

  auto debug = std::make_shared<
    vix::middleware::observability::InMemoryDebugTrace
  >();

  auto hooks = vix::middleware::merge_hooks(
    vix::middleware::observability::tracing_hooks(),
    vix::middleware::observability::metrics_hooks(metrics),
    vix::middleware::observability::debug_trace_hooks(debug)
  );

  vix::middleware::HttpPipeline pipeline;

  pipeline.set_hooks(std::move(hooks));

  pipeline.use(vix::middleware::basics::request_id());
  pipeline.use(vix::middleware::basics::timing());

  // Build req and res in your test or integration layer.
  // pipeline.run(req, res, final_handler);

  return 0;
}
```

This pattern is useful for tests and lower-level middleware validation.

Normal applications should start with `vix::App` middleware.

## Enable development observability

`HttpPipeline` can enable development observability helpers.

```cpp id="i5xriu"
vix::middleware::HttpPipeline pipeline;

pipeline.enable_dev_observability();
```

This can install tracing, metrics, and debug tracing when the environment is considered development.

Typical development environment values include:

```txt id="vqft4z"
VIX_ENV=dev
VIX_ENV=development
VIX_ENV=local
```

Use this for local inspection.

For production, wire explicit sinks and policies.

## Safe labels

Observability code often needs method and path labels.

The helpers are:

```cpp id="bcl2tx"
vix::middleware::observability::safe_method(req)
vix::middleware::observability::safe_path(req)
```

They return reasonable fallback values when method or path is empty.

This avoids emitting empty labels in logs, metrics, and traces.

Example:

```cpp id="rr1p1j"
const std::string method =
  vix::middleware::observability::safe_method(ctx.req());

const std::string path =
  vix::middleware::observability::safe_path(ctx.req());
```

## Observability and errors

When a middleware short-circuits the request, observability should still help you understand what happened.

Examples of short-circuiting middleware:

```txt id="m6a8e9"
rate limit
body limit
CORS preflight
CSRF
API key
JWT
RBAC
HTTP cache hit
parser errors
```

Use request ids and tracing early in the pipeline so error responses still carry identifiers.

A practical order is:

```cpp id="sahvko"
app.use("/api", middleware::app::request_id_dev());
app.use("/api", middleware::app::timing_dev());

app.use("/api", middleware::app::adapt_ctx(
  middleware::observability::tracing_mw()
));

app.use("/api", middleware::app::security_headers_dev());
app.use("/api", middleware::app::rate_limit_dev());
app.use("/api", middleware::app::body_limit_write_dev(1024 * 1024));
```

This gives early request identity and timing around the rest of the chain.

## Observability and logging

The basics group also provides `logger()`.

It can write a request summary after the handler has run.

Use it when you want middleware-level logs with a custom logger service.

For most apps, combine:

```txt id="nobn3g"
request_id
timing
tracing
debug_trace
logger
```

according to your needs.

For local debugging, debug trace can be enough.

For production, prefer structured logs, request ids, metrics, and controlled trace propagation.

## Complete App example

This example uses request id, timing, tracing, and debug trace as App middleware.

```cpp id="bsxa9e"
#include <iostream>
#include <memory>
#include <string_view>

#include <vix.hpp>
#include <vix/middleware.hpp>

class StdoutDebugTrace final
  : public vix::middleware::observability::IDebugTraceSink
{
public:
  void log(std::string_view line) override
  {
    std::cout << line << "\n";
  }
};

int main()
{
  vix::App app;

  auto debug = std::make_shared<StdoutDebugTrace>();

  app.use("/api", vix::middleware::app::request_id_dev());
  app.use("/api", vix::middleware::app::timing_dev());

  app.use("/api", vix::middleware::app::adapt_ctx(
    vix::middleware::observability::tracing_mw()
  ));

  app.use("/api", vix::middleware::app::adapt_ctx(
    vix::middleware::observability::debug_trace_mw(debug)
  ));

  app.get("/api/health", [](vix::Request &req, vix::Response &res)
  {
    auto *rid =
      req.try_state<vix::middleware::basics::RequestId>();

    auto *trace =
      req.try_state<vix::middleware::observability::TraceContext>();

    res.json({
      "ok", true,
      "request_id", rid ? rid->value : "",
      "trace_id", trace ? trace->trace_id : "",
      "span_id", trace ? trace->span_id : ""
    });
  });

  app.run(8080);
}
```

Run:

```bash id="bw4g2p"
vix run observability_app_demo.cpp
```

Request:

```bash id="vp33g7"
curl -i http://127.0.0.1:8080/api/health
```

You should see:

```txt id="g25kau"
request id in response state or headers
trace id and span id in response body
timing headers
debug trace lines in stdout
```

## Complete HttpPipeline example

This example shows the low-level pipeline model.

```cpp id="iygnqc"
#include <cassert>
#include <memory>

#include <vix/http/Request.hpp>
#include <vix/http/Response.hpp>
#include <vix/http/ResponseWrapper.hpp>
#include <vix/middleware.hpp>

static vix::http::Request make_request()
{
  vix::http::Request::HeaderMap headers;
  headers["Host"] = "localhost";

  return vix::http::Request(
    "GET",
    "/api/health",
    std::move(headers),
    ""
  );
}

int main()
{
  auto metrics = std::make_shared<
    vix::middleware::observability::InMemoryMetrics
  >();

  auto debug = std::make_shared<
    vix::middleware::observability::InMemoryDebugTrace
  >();

  auto hooks = vix::middleware::merge_hooks(
    vix::middleware::observability::tracing_hooks(),
    vix::middleware::observability::metrics_hooks(metrics),
    vix::middleware::observability::debug_trace_hooks(debug)
  );

  vix::middleware::HttpPipeline pipeline;

  pipeline.set_hooks(std::move(hooks));
  pipeline.use(vix::middleware::basics::request_id());
  pipeline.use(vix::middleware::basics::timing());

  auto req = make_request();

  vix::http::Response raw_res;
  vix::http::ResponseWrapper res(raw_res);

  pipeline.run(req, res, [](auto &, auto &out)
  {
    out.status(200).text("OK");
  });

  assert(raw_res.status() == 200);
  assert(raw_res.body() == "OK");

  return 0;
}
```

Use this style when testing middleware behavior without starting a server.

## App vs HttpPipeline

Use `vix::App` for normal applications.

```cpp id="atc8wy"
app.use("/api", middleware::app::request_id_dev());
app.use("/api", middleware::app::timing_dev());
app.use("/api", middleware::app::adapt_ctx(
  middleware::observability::tracing_mw()
));
```

Use `HttpPipeline` for lower-level control.

```cpp id="c8vw3q"
vix::middleware::HttpPipeline pipeline;

pipeline.set_hooks(...);
pipeline.use(...);
pipeline.run(req, res, final_handler);
```

The rule is simple:

```txt id="fi5jbm"
App
  production route model

HttpPipeline
  tests and custom integrations
```

## What to use first

For a normal backend, start with:

```cpp id="hr5bp3"
app.use("/api", middleware::app::request_id_dev());
app.use("/api", middleware::app::timing_dev());

app.use("/api", middleware::app::adapt_ctx(
  middleware::observability::tracing_mw()
));
```

Then add debug tracing when you need local inspection:

```cpp id="p4yrmi"
auto debug = std::make_shared<MyDebugTraceSink>();

app.use("/api", middleware::app::adapt_ctx(
  middleware::observability::debug_trace_mw(debug)
));
```

Use metrics hooks when you are building tests, diagnostics, or a deeper runtime integration.

## Summary

Use observability to make the backend visible.

Start with:

```txt id="s7ndep"
request ids
timing headers
trace ids
```

Add debug traces when diagnosing middleware flow.

Use metrics hooks when you need counters and request summaries.

Remember the separation:

```txt id="qfku2e"
request_id and timing are basics
tracing and debug trace are observability
metrics and hooks are lower-level pipeline tools
vix::App is the normal application path
HttpPipeline is for tests and custom integrations
```
