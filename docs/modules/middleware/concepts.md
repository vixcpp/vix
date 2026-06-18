# Core Concepts

This page explains the model behind `vix::middleware`.

The quick start shows how to install middleware on `vix::App`.

This page explains what happens behind that API:

```txt id="mhksmt"
how middleware runs
what next() means
how a request can be stopped early
how middleware can modify a response
how typed request state works
how lower-level middleware integrates with vix::App
when to use HttpPipeline
why middleware order matters
```

The goal is to make middleware predictable.

Once the flow is clear, the module becomes simple to reason about.

## The simplest definition

A middleware is code that runs around a route handler.

It can:

```txt id="jnylx3"
inspect the request
modify the response
store data on the request
call the next middleware
stop the request early
```

A normal `vix::App` middleware has this shape:

```cpp id="ixrsk1"
app.use([](vix::Request &req, vix::Response &res, vix::App::Next next)
{
  (void)req;
  (void)res;

  next();
});
```

The important part is `next`.

Calling `next()` means:

```txt id="dg4jyy"
continue to the next middleware or route handler
```

Not calling `next()` means:

```txt id="o7s8tf"
stop the request here
```

This is the central rule.

## Request flow

When several middleware functions are installed, they run in order.

```cpp id="fouh5t"
app.use(middleware_a);
app.use(middleware_b);
app.use(middleware_c);
```

The request flow is:

```txt id="csfbx7"
request
  -> middleware_a
  -> middleware_b
  -> middleware_c
  -> route handler
  -> response
```

If every middleware calls `next()`, the request eventually reaches the route handler.

If one middleware does not call `next()`, the chain stops.

```txt id="01e015"
request
  -> middleware_a
  -> middleware_b
       stops here
  -> response
```

The route handler is not called.

This is how rate limits, API key checks, JWT checks, body limits, CORS preflight, CSRF, IP filtering, and cache hits work.

## Before middleware

Some middleware runs before the handler and decides whether the request can continue.

```cpp id="q3o0n4"
app.use("/admin", [](vix::Request &req, vix::Response &res, vix::App::Next next)
{
  if (req.header("x-api-key") != "secret")
  {
    res.status(401).json({
      "ok", false,
      "error", "unauthorized"
    });
    return;
  }

  next();
});
```

If the header is missing or invalid, the middleware sends a response and returns.

The handler is skipped.

This pattern is called short-circuiting.

## After middleware

Some middleware calls `next()` first, then modifies the response after the handler has run.

```cpp id="wosleo"
app.use([](vix::Request &req, vix::Response &res, vix::App::Next next)
{
  (void)req;

  next();

  res.header("X-Content-Type-Options", "nosniff");
});
```

This pattern is useful when the middleware needs to see the final response.

Examples:

```txt id="bbiccu"
security headers
timing
logging
compression
ETag
metrics
```

These middleware functions usually need the handler to produce a response first.

## Before and after middleware

A middleware can also do work before and after `next()`.

```cpp id="fv1zcv"
app.use([](vix::Request &req, vix::Response &res, vix::App::Next next)
{
  const auto path = req.path();

  next();

  res.header("X-Handled-Path", path);
});
```

The middleware reads the request before the handler, then updates the response after the handler.

This is the wrapping model.

```txt id="itlleb"
middleware begins
  -> next middleware
    -> route handler
  <- response returns
middleware ends
```

## Why order matters

Middleware order controls behavior.

This order is good:

```cpp id="m0v02i"
app.use("/api", middleware::app::body_limit_write_dev(1024 * 1024));
app.use("/api/users", middleware::app::json_strict_dev(4096));
```

The body limit runs before the JSON parser.

That means oversized bodies are rejected before parsing.

This order is usually worse:

```cpp id="bwqyh0"
app.use("/api/users", middleware::app::json_strict_dev(4096));
app.use("/api", middleware::app::body_limit_write_dev(1024 * 1024));
```

The parser can run before the global body limit.

A practical order is:

```txt id="i6eilc"
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

The exact order depends on the application.

The principle is stable:

```txt id="5kcad5"
reject bad requests early
parse only after size rules
authenticate before protected handlers
modify responses after handlers
observe the whole request flow
```

## Short-circuiting

Short-circuiting means a middleware sends a response and does not call `next()`.

```cpp id="hbe40u"
app.use("/api", [](vix::Request &req, vix::Response &res, vix::App::Next next)
{
  if (req.method() == "POST" && req.body().size() > 1024)
  {
    res.status(413).json({
      "ok", false,
      "error", "payload_too_large"
    });
    return;
  }

  next();
});
```

Short-circuiting is normal for:

```txt id="rr8bhx"
CORS preflight
body limit
rate limit
API key authentication
JWT authentication
RBAC authorization
CSRF protection
IP filtering
HTTP cache hits
parser errors
```

A middleware should short-circuit only when it has enough information to produce the response itself.

## App middleware and module middleware

Core already supports middleware through `vix::App`.

```cpp id="jec8vr"
app.use([](vix::Request &req, vix::Response &res, vix::App::Next next)
{
  (void)req;
  (void)res;

  next();
});
```

The `vix::middleware` module provides reusable middleware implementations.

```cpp id="bn5oau"
app.use(vix::middleware::app::cors_dev());
app.use(vix::middleware::app::security_headers_dev());
app.use(vix::middleware::app::rate_limit_dev());
```

The difference is:

```txt id="wbmzh0"
Core middleware
  custom function installed directly on vix::App

vix::middleware
  reusable middleware for common backend concerns
```

Use direct App middleware for one-off behavior.

Use `vix::middleware` when you need a reusable component such as CORS, rate limiting, request IDs, JSON parsing, sessions, JWT, HTTP cache, or compression.

## Context-based middleware

Inside the middleware module, the main lower-level type is:

```cpp id="hi8l83"
vix::middleware::MiddlewareFn
```

It has this shape:

```cpp id="dig03e"
using MiddlewareFn = std::function<void(Context &, Next)>;
```

A context-based middleware receives a `Context`.

```cpp id="pi8tw9"
vix::middleware::MiddlewareFn mw =
  [](vix::middleware::Context &ctx, vix::middleware::Next next)
  {
    ctx.res().header("X-Example", "yes");

    next();
  };
```

`Context` gives middleware a consistent place to access:

```txt id="g8bpsk"
request
response
services
typed state helpers
error helpers
```

The request and response are still the normal Vix HTTP objects.

The context just groups middleware-specific tools around them.

## Request and response access

A context-based middleware reads the request with:

```cpp id="eoi3cq"
ctx.req()
```

Example:

```cpp id="r7k9ls"
const std::string method = ctx.req().method();
const std::string path = ctx.req().path();
const std::string content_type = ctx.req().header("Content-Type");
```

It writes to the response with:

```cpp id="s7ljvq"
ctx.res()
```

Example:

```cpp id="l315h6"
ctx.res().status(200);
ctx.res().header("X-Example", "ok");
ctx.res().text("OK");
```

Most middleware either:

```txt id="c6sjqb"
reads the request before next()
writes the response after next()
does both
```

## Typed request state

Middleware often needs to attach data to the current request.

Examples:

```txt id="vccs8l"
request_id()
  stores RequestId

timing()
  stores Timing

json()
  stores JsonBody

form()
  stores FormBody

jwt()
  stores JwtClaims

api_key()
  stores ApiKey

rbac_context()
  stores Authz

session()
  stores Session
```

A handler can read typed state:

```cpp id="ibvw6z"
app.post("/api/echo", [](vix::Request &req, vix::Response &res)
{
  auto &body = req.state<vix::middleware::parsers::JsonBody>();

  res.json({
    "received", body.value.dump()
  });
});
```

The type is the key.

You do not need string keys.

You ask for the state by type.

## `state<T>()` and `try_state<T>()`

Use `state<T>()` when the value must exist.

```cpp id="5k92gn"
auto &body = req.state<vix::middleware::parsers::JsonBody>();
```

This is appropriate when the route is protected by the middleware that creates the state.

Example:

```cpp id="xs065f"
app.use("/api/users", vix::middleware::app::json_strict_dev());

app.post("/api/users", [](vix::Request &req, vix::Response &res)
{
  auto &body = req.state<vix::middleware::parsers::JsonBody>();

  res.json({
    "ok", true,
    "body", body.value.dump()
  });
});
```

Use `try_state<T>()` when the value may be missing.

```cpp id="6gh668"
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

This is safer when middleware is optional, conditional, or installed only on some routes.

## Why typed state matters

Typed state keeps handlers clean.

Without middleware, a handler may need to:

```txt id="yney5f"
read raw body
check content type
parse JSON
handle parse errors
validate auth headers
decode token
extract roles
load session
```

With middleware, reusable work happens before the handler.

The handler reads the typed result.

```cpp id="35tfo9"
auto &claims = req.state<vix::middleware::auth::JwtClaims>();
auto &body = req.state<vix::middleware::parsers::JsonBody>();
```

This keeps application logic focused.

## Services

`Context` also gives access to a services container.

```cpp id="m7ku5f"
ctx.services()
```

Services let middleware share objects without global variables.

Examples:

```txt id="425aij"
loggers
metrics sinks
permission resolvers
rate limiter state
custom application services
```

A middleware can look up a service:

```cpp id="a69i41"
auto resolver = ctx.services().get<MyService>();
```

If the service exists, the middleware can use it.

If it does not, the middleware can fall back, skip optional behavior, or return an error depending on its design.

Services are most useful in lower-level middleware and `HttpPipeline`.

Normal `vix::App` applications often use presets first and only use services for custom integrations.

## Normalized errors

Middleware should return predictable errors.

The module exposes a normalized error model through:

```cpp id="vso3gl"
vix::middleware::Error
vix::middleware::normalize(...)
ctx.send_error(...)
```

Example:

```cpp id="oa318l"
vix::middleware::Error err;

err.status = 401;
err.code = "unauthorized";
err.message = "Missing token";
err.details["hint"] = "Use Authorization header";

ctx.send_error(vix::middleware::normalize(std::move(err)));
```

A normalized error lets middleware produce consistent responses.

Common middleware errors include:

```txt id="z65eqs"
400 invalid_json
401 missing_api_key
401 missing_auth
403 forbidden
403 csrf_failed
411 length_required
413 payload_too_large
415 unsupported_media_type
429 rate_limited
500 internal_server_error
```

## App integration

Most reusable middleware is context-based.

To use it inside `vix::App`, adapt it:

```cpp id="zz7ag5"
app.use(vix::middleware::app::adapt_ctx(
  vix::middleware::basics::request_id()
));
```

This converts:

```txt id="fxlz2x"
MiddlewareFn
  -> vix::App::Middleware
```

For legacy HTTP middleware, use `adapt()`.

```cpp id="gqco55"
app.use(vix::middleware::app::adapt(my_http_middleware));
```

For normal applications, prefer App presets:

```cpp id="yxsp5a"
app.use("/api", vix::middleware::app::cors_dev());
app.use("/api", vix::middleware::app::rate_limit_dev());
app.use("/api/users", vix::middleware::app::json_strict_dev());
```

Presets are already adapted for `vix::App`.

## Prefix matching

`vix::App` can install middleware globally:

```cpp id="xkw6mi"
app.use(middleware);
```

Or on a prefix:

```cpp id="eemf4e"
app.use("/api", middleware);
```

A prefix middleware applies to matching routes.

```cpp id="reoup0"
app.use("/api", middleware::app::rate_limit_dev());
```

This applies to:

```txt id="6ct851"
/api
/api/users
/api/admin/status
```

It does not apply to:

```txt id="w1xik7"
/admin
/public
```

A more specific prefix limits the middleware to a smaller route group.

```cpp id="cimrzs"
app.use("/api/users", middleware::app::json_strict_dev());
```

This applies to:

```txt id="pv8cdi"
/api/users
/api/users/123
```

It does not apply to:

```txt id="0u5jvn"
/api/health
/api/admin/status
```

Use prefixes intentionally.

Broad middleware should go on broad prefixes.

Route-specific middleware should go on narrow prefixes.

## Exact path protection

Sometimes a middleware should apply to one exact route.

Use:

```cpp id="tis4d8"
vix::middleware::app::protect(...)
```

Example:

```cpp id="g7ymvx"
vix::middleware::app::protect(
  app,
  "/admin",
  vix::middleware::app::api_key_dev("secret")
);
```

This protects:

```txt id="v0d3yb"
/admin
```

It does not protect:

```txt id="7uzq8t"
/admin/settings
```

For route groups, use prefix protection instead.

```cpp id="1f16q9"
vix::middleware::app::protect_prefix(
  app,
  "/admin",
  vix::middleware::app::api_key_dev("secret")
);
```

## Chaining middleware

A chain combines several App middlewares into one.

```cpp id="99ajg0"
auto users_stack = vix::middleware::app::chain(
  vix::middleware::app::body_limit_write_dev(4096),
  vix::middleware::app::json_strict_dev(4096)
);

app.use("/api/users", std::move(users_stack));
```

The chain runs in order.

```txt id="2dsgvg"
body_limit_write_dev
  -> json_strict_dev
  -> handler
```

Use `chain()` when a route or route group has a clear middleware stack.

## Conditional middleware

Use `when()` when prefix matching is not enough.

```cpp id="b6qwbh"
auto only_post = vix::middleware::app::when(
  [](const vix::Request &req)
  {
    return req.method() == "POST";
  },
  vix::middleware::app::body_limit_write_dev(1024)
);

app.use("/api", std::move(only_post));
```

If the predicate returns false, the middleware is skipped.

Use `when()` for conditions such as:

```txt id="r3595z"
only POST requests
only a specific content type
only a specific header
custom path matching
custom tenant logic
```

## HttpPipeline

Most applications should use `vix::App`.

`HttpPipeline` exists for tests, examples, and lower-level integrations.

```cpp id="i7vv6f"
vix::middleware::HttpPipeline pipeline;

pipeline.use(vix::middleware::security::csrf());
pipeline.use(vix::middleware::parsers::json());

pipeline.run(req, res, [](auto &req, auto &res)
{
  res.ok().text("OK");
});
```

Use `HttpPipeline` when you want to run middleware without starting a server.

It is useful for:

```txt id="kqay7c"
unit tests
middleware tests
custom HTTP integration
manual pipeline composition
observability hooks
```

It is not the default application path.

The default path is still:

```txt id="esr7v0"
vix::App
  -> app.use(...)
  -> route handler
```

## Pipeline hooks

`HttpPipeline` supports hooks.

Common hook points are:

```txt id="qs8vok"
on_begin
on_end
on_error
```

Hooks are useful for observability.

Examples:

```txt id="1q4jpm"
tracing
metrics
debug trace
```

A pipeline can install hooks:

```cpp id="hd4wlr"
vix::middleware::HttpPipeline pipeline;

pipeline.set_hooks(
  vix::middleware::observability::tracing_hooks()
);
```

Hooks are lower-level than normal App middleware.

Use them when you need pipeline-level control.

## Static files are outside the middleware chain

Static files are served by `vix::App`.

```cpp id="ffhc9g"
app.static_dir("public", "/", "index.html");
```

That is a Core feature.

The middleware module can provide an optional static response hook, for example compression:

```cpp id="pqk9ru"
vix::App::set_static_response_hook(
  vix::middleware::performance::compressed_static_response_hook()
);
```

The distinction is important:

```txt id="wz8dlo"
app.use(...)
  route middleware

app.static_dir(...)
  static file serving

App::set_static_response_hook(...)
  optional response enhancement for static files
```

Do not think of static files as middleware.

Core serves them.

Middleware can enhance the response after Core writes it.

## A complete mental model

For a dynamic API route:

```txt id="vfcpq9"
request
  -> App prefix matching
  -> middleware chain
  -> route handler
  -> after-next middleware work
  -> response
```

For a blocked request:

```txt id="tssoii"
request
  -> middleware chain
  -> middleware sends error
  -> response
```

For a cached GET response:

```txt id="ha9k5s"
request
  -> HTTP cache middleware
  -> cache hit
  -> cached response
```

For a static file:

```txt id="4iv6yf"
request
  -> Core static file resolution
  -> file response
  -> optional static response hook
  -> response
```

This is the separation to remember:

```txt id="52ndwl"
Core owns the app and file serving.
Middleware owns reusable HTTP behavior.
App integration connects them.
```

## Summary

The middleware model is built on a few simple ideas:

```txt id="3v3ajf"
middleware wraps handlers
next() continues the request
not calling next() stops the request
middleware order matters
typed state carries parsed/authenticated data
App presets are the normal user-facing API
adapt_ctx() connects lower-level middleware to vix::App
HttpPipeline is for tests and custom integrations
static files belong to Core
```

Once these ideas are clear, the rest of the module is just a set of reusable backend components.
