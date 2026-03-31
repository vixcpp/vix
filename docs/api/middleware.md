# Middleware API

This page documents the middleware system in Vix.

Vix supports two middleware styles:

1)  Context-based middleware (recommended)
2)  Legacy HTTP middleware (Request/Response based)

Both can be adapted and chained.

------------------------------------------------------------------------

# Headers

Context middleware:

``` cpp
#include <vix/middleware/Middleware.hpp>
```

Legacy middleware:

``` cpp
#include <vix/http/HttpMiddleware.hpp>
```

------------------------------------------------------------------------

# 1) Context Middleware (Recommended)

Type:

``` cpp
vix::middleware::MiddlewareFn
```

Signature:

``` cpp
(Context& ctx, Next next)
```

------------------------------------------------------------------------

## Minimal Example

``` cpp
#include <vix.hpp>
#include <vix/middleware/Middleware.hpp>

using namespace vix;

int main()
{
  App app;

  vix::middleware::MiddlewareFn mw =
    [](vix::middleware::Context& ctx, vix::middleware::Next next)
    {
      // pre logic
      next();
      // post logic
    };

  app.use(mw);

  app.get("/", [](Request&, Response& res)
  {
    res.send("Hello");
  });

  app.run(8080);
}
```

------------------------------------------------------------------------

# Registering Middleware

## Global

``` cpp
app.use(mw);
```

Applies to all routes.

------------------------------------------------------------------------

## Prefix

``` cpp
app.use("/api/", mw);
```

Applies only to routes under `/api/`.

------------------------------------------------------------------------

## Exact Path

``` cpp
app.use_exact("/ping", mw);
```

Applies only to the exact path.

------------------------------------------------------------------------

# Chaining Middleware

Multiple middleware can be chained:

``` cpp
auto chained = vix::middleware::chain(mw1, mw2, mw3);

app.use(chained);
```

Execution order:

-   mw1 pre
-   mw2 pre
-   mw3 pre
-   handler
-   mw3 post
-   mw2 post
-   mw1 post

------------------------------------------------------------------------

# Request State Storage

Context allows storing typed state per request.

``` cpp
ctx.emplace_state<int>(42);

int& value = ctx.state<int>();
```

Safe access:

``` cpp
if (auto* v = ctx.try_state<int>())
{
  // state exists
}
```

------------------------------------------------------------------------

# 2) Legacy HTTP Middleware

Type:

``` cpp
vix::HttpMiddleware
```

Signature:

``` cpp
(Request&, Response&, Next)
```

Example:

``` cpp
#include <vix.hpp>
#include <vix/http/HttpMiddleware.hpp>

using namespace vix;

int main()
{
  App app;

  HttpMiddleware mw =
    [](Request& req, Response& res, auto next)
    {
      (void)req;
      (void)res;
      next();
    };

  app.use(mw);

  app.get("/", [](Request&, Response& res)
  {
    res.send("Hello");
  });

  app.run(8080);
}
```

------------------------------------------------------------------------

# Adapters

Adapt legacy to context:

``` cpp
auto adapted = vix::middleware::adapt(mw);
app.use(adapted);
```

Adapt context to legacy:

``` cpp
auto adapted = vix::middleware::adapt_ctx(ctx_mw);
app.use(adapted);
```

------------------------------------------------------------------------

# Behavior Notes

-   Middleware executes in registration order.
-   If `next()` is not called, the chain stops.
-   Middleware can modify request state before handler execution.
-   Middleware should not block for long operations.

The middleware system is explicit and composable by design.

