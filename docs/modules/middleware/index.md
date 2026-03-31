# Middleware

Vix middleware is explicit.

A middleware is a function that runs before your route handler. It can:

-   read headers, path, query, body
-   set headers or status
-   stop the request early
-   call `next()` to continue

Vix supports two middleware styles:

1.  Context-based middleware (recommended)
2.  Legacy HTTP middleware (compatible)

You can attach middleware:

-   globally (all routes)
-   by prefix (only under `/api/`)
-   exactly on a single path

------------------------------------------------------------------------

## Context-based middleware (recommended)

Context middleware uses:

-   `vix::middleware::Context`
-   `vix::middleware::Next`
-   `adapt_ctx()` to install it on `App`

Minimal example: add a request id header and continue.

``` cpp
#include <vix.hpp>
#include <vix/middleware/app/adapter.hpp>

#include <chrono>
#include <cstdint>
#include <string>

using namespace vix;

static long long now_ms()
{
  using namespace std::chrono;
  return (long long)time_point_cast<milliseconds>(system_clock::now())
      .time_since_epoch().count();
}

int main()
{
  App app;

  // Global ctx middleware
  app.use(vix::middleware::app::adapt_ctx(
    [](vix::middleware::Context& ctx, vix::middleware::Next next)
    {
      const std::string rid =
        std::to_string(now_ms()) + "-" + std::to_string((std::uintptr_t)(&ctx));

      ctx.res().header("x-request-id", rid);
      next();
    }
  ));

  app.get("/", [](Request&, Response& res)
  {
    res.json({"ok", true, "message", "middleware ran"});
  });

  app.run(8080);
  return 0;
}
```

Test:

``` bash
curl -i http://127.0.0.1:8080/
```

You should see the response header `x-request-id`.

------------------------------------------------------------------------

## Legacy HTTP middleware

Legacy middleware uses:

-   `vix::Request`
-   `vix::Response`
-   `vix::middleware::Next`
-   `adapt()` to install it on `App`

Minimal example: require a header.

``` cpp
#include <vix.hpp>
#include <vix/middleware/app/adapter.hpp>

#include <string>

using namespace vix;

int main()
{
  App app;

  app.use(vix::middleware::app::adapt(
    [](Request& req, Response& res, vix::middleware::Next next)
    {
      const std::string v = req.header("x-demo");

      if (v != "1")
      {
        res.status(401).json({
          "ok", false,
          "error", "unauthorized",
          "hint", "Send header: x-demo: 1"
        });
        return;
      }

      next();
    }
  ));

  app.get("/ping", [](Request&, Response& res)
  {
    res.json({"ok", true, "pong", true});
  });

  app.run(8080);
  return 0;
}
```

Test:

``` bash
curl -i http://127.0.0.1:8080/ping
curl -i -H "x-demo: 1" http://127.0.0.1:8080/ping
```

------------------------------------------------------------------------

## Prefix middleware

You can attach middleware only for a prefix.

Example: everything under `/api/` requires `x-api-key: dev_key_123`.

``` cpp
#include <vix.hpp>

#include <string>

using namespace vix;

int main()
{
  App app;

  // Only runs for /api/*
  app.use("/api/", [](Request& req, Response& res, App::Next next)
  {
    const std::string key = req.header("x-api-key");

    if (key != "dev_key_123")
    {
      res.status(401).json({
        "ok", false,
        "error", "unauthorized",
        "hint", "Send header: x-api-key: dev_key_123"
      });
      return;
    }

    next();
  });

  app.get("/api/ping", [](Request&, Response& res)
  {
    res.json({"ok", true, "pong", true});
  });

  app.get("/", [](Request&, Response& res)
  {
    res.json({"ok", true, "public", true});
  });

  app.run(8080);
  return 0;
}
```

Test:

``` bash
curl -i http://127.0.0.1:8080/
curl -i http://127.0.0.1:8080/api/ping
curl -i -H "x-api-key: dev_key_123" http://127.0.0.1:8080/api/ping
```

------------------------------------------------------------------------

## Exact-path middleware

Use `protect_exact()` to run middleware only when the path matches
exactly.

Example: protect `/api/ping` only.

``` cpp
#include <vix.hpp>

#include <string>

using namespace vix;

int main()
{
  App app;

  app.protect_exact("/api/ping", [](Request& req, Response& res, App::Next next)
  {
    if (req.header("x-demo") != "1")
    {
      res.status(401).json({
        "ok", false,
        "error", "unauthorized",
        "hint", "Send header: x-demo: 1"
      });
      return;
    }
    next();
  });

  app.get("/api/ping", [](Request&, Response& res)
  {
    res.json({"ok", true, "pong", true});
  });

  app.get("/api/other", [](Request&, Response& res)
  {
    res.json({"ok", true, "note", "not protected"});
  });

  app.run(8080);
  return 0;
}
```

------------------------------------------------------------------------

## Chaining and early return

A middleware can stop the request early by sending a response and not
calling `next()`.

Example: two checks in a chain.

``` cpp
#include <vix.hpp>

#include <string>

using namespace vix;

int main()
{
  App app;

  app.use("/api/", [](Request& req, Response& res, App::Next next)
  {
    if (req.header("x-demo") != "1")
    {
      res.status(401).json({"ok", false, "error", "missing x-demo"});
      return;
    }
    next();
  });

  app.use("/api/", [](Request& req, Response& res, App::Next next)
  {
    if (req.header("x-role") != "admin")
    {
      res.status(403).json({"ok", false, "error", "admin required"});
      return;
    }
    next();
  });

  app.get("/api/stats", [](Request&, Response& res)
  {
    res.json({"ok", true, "stats", true});
  });

  app.run(8080);
  return 0;
}
```

Test:

``` bash
curl -i http://127.0.0.1:8080/api/stats
curl -i -H "x-demo: 1" http://127.0.0.1:8080/api/stats
curl -i -H "x-demo: 1" -H "x-role: admin" http://127.0.0.1:8080/api/stats
```

------------------------------------------------------------------------

## RequestState (store data across middleware and handlers)

Vix lets you store typed state on the request.

This is useful to pass data from middleware to handlers.

Minimal example: store an `AuthInfo` struct set by middleware.

``` cpp
#include <vix.hpp>

#include <string>

using namespace vix;

struct AuthInfo
{
  bool authed{false};
  std::string user;
  std::string role;
};

int main()
{
  App app;

  app.use("/api/", [](Request& req, Response& res, App::Next next)
  {
    AuthInfo a;
    a.user = req.header("x-user");
    a.role = req.header("x-role");

    if (!a.user.empty())
      a.authed = true;

    req.emplace_state<AuthInfo>(std::move(a));
    next();
  });

  app.get("/api/whoami", [](Request& req, Response& res)
  {
    if (!req.has_state_type<AuthInfo>())
    {
      res.status(500).json({"ok", false, "error", "state missing"});
      return;
    }

    const auto& a = req.state<AuthInfo>();

    if (!a.authed)
    {
      res.status(401).json({"ok", false, "error", "unauthorized"});
      return;
    }

    res.json({
      "ok", true,
      "user", a.user,
      "role", a.role
    });
  });

  app.run(8080);
  return 0;
}
```

Test:

``` bash
curl -i http://127.0.0.1:8080/api/whoami
curl -i -H "x-user: gaspard" -H "x-role: admin" http://127.0.0.1:8080/api/whoami
```

------------------------------------------------------------------------

## Notes

-   Middleware order matters: first installed runs first.
-   If a middleware sends a response and does not call `next()`, the
    chain stops.
-   Prefix matching uses normalized prefixes (for example `/api/`).
-   Keep middleware small and predictable.


