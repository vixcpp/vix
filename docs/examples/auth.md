# Auth and Middleware (Minimal Patterns)

This page shows minimal auth and middleware patterns in Vix.cpp.

Each section is: - one concept - one minimal `main()` - a quick curl
test

------------------------------------------------------------------------

## 1) API key middleware (protect one route)

A public route plus a secure route that requires `x-api-key`.

``` cpp
#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>
using namespace vix;

int main()
{
  App app;

  app.get("/public", [](Request&, Response& res){
    res.json({ "ok", true, "scope", "public" });
  });

  // Install API key middleware only on this prefix
  middleware::app::install(app, "/secure/", middleware::app::api_key_dev("dev_key_123"));

  app.get("/secure/whoami", [](Request&, Response& res){
    res.json({ "ok", true, "scope", "secure", "message", "API key accepted" });
  });

  app.run(8080);
  return 0;
}
```

Try:

``` bash
curl -i http://127.0.0.1:8080/public
curl -i http://127.0.0.1:8080/secure/whoami
curl -i -H "x-api-key: dev_key_123" http://127.0.0.1:8080/secure/whoami
```

------------------------------------------------------------------------

## 2) Prefix protection (protect all /api routes)

Everything under `/api/` is protected.

``` cpp
#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>
using namespace vix;

int main()
{
  App app;

  middleware::app::install(app, "/api/", middleware::app::api_key_dev("dev_key_123"));

  app.get("/api/ping", [](Request&, Response& res){
    res.json({ "ok", true, "pong", true });
  });

  app.get("/api/users", [](Request&, Response& res){
    res.json({ "ok", true, "data", json::array({ "u1", "u2" }) });
  });

  app.run(8080);
  return 0;
}
```

Try:

``` bash
curl -i http://127.0.0.1:8080/api/ping
curl -i -H "x-api-key: dev_key_123" http://127.0.0.1:8080/api/ping
```

------------------------------------------------------------------------

## 3) Custom middleware (context style + RequestState)

This shows how to store data in RequestState and read it in the handler.

``` cpp
#include <vix.hpp>
#include <vix/middleware/app/adapter.hpp>
#include <chrono>
#include <string>
using namespace vix;

struct RequestId{
  std::string value;
};

static long long now_ms(){
  using namespace std::chrono;
  return (long long)time_point_cast<milliseconds>(system_clock::now()).time_since_epoch().count();
}

static vix::middleware::MiddlewareFn mw_request_id(){
  return [](vix::middleware::Context& ctx, vix::middleware::Next next)
  {
    RequestId rid;
    rid.value = std::to_string(now_ms());

    ctx.req().emplace_state<RequestId>(rid);
    ctx.res().header("x-request-id", rid.value);

    next();
  };
}

int main()
{
  App app;

  // Adapt context middleware into app middleware
  app.use(vix::middleware::app::adapt_ctx(mw_request_id()));

  app.get("/who", [](Request& req, Response& res){
    const auto rid = req.state<RequestId>().value;
    res.json({ "ok", true, "request_id", rid });
  });

  app.run(8080);
  return 0;
}
```

Try:

``` bash
curl -i http://127.0.0.1:8080/who
```

------------------------------------------------------------------------

## 4) Role gating (fake auth + admin only)

This is a minimal RBAC-style gate using headers for the demo.

``` cpp
#include <vix.hpp>
#include <vix/middleware/app/adapter.hpp>
#include <string>

using namespace vix;
namespace J = vix::json;

struct AuthInfo{
  bool authed{false};
  std::string subject;
  std::string role;
};

static vix::middleware::MiddlewareFn mw_fake_auth(){
  return [](vix::middleware::Context& ctx, vix::middleware::Next next)
  {
    AuthInfo a;
    const std::string user = ctx.req().header("x-user");
    const std::string role = ctx.req().header("x-role");

    if (!user.empty())
    {
      a.authed = true;
      a.subject = user;
      a.role = role.empty() ? "user" : role;
    }

    ctx.req().emplace_state<AuthInfo>(a);
    next();
  };
}

static vix::middleware::MiddlewareFn mw_require_admin(){
  return [](vix::middleware::Context& ctx, vix::middleware::Next next){
    if (!ctx.req().has_state_type<AuthInfo>() || !ctx.req().state<AuthInfo>().authed)
    {
      ctx.res().status(401).json(J::obj({ "ok", false, "error", "unauthorized" }));
      return;
    }

    if (ctx.req().state<AuthInfo>().role != "admin")
    {
      ctx.res().status(403).json(J::obj({ "ok", false, "error", "forbidden", "hint", "admin required" }));
      return;
    }

    next();
  };
}

int main()
{
  App app;

  app.use(vix::middleware::app::adapt_ctx(mw_fake_auth()));

  // Install admin guard only under /admin/
  vix::middleware::app::install(app, "/admin/", vix::middleware::app::adapt_ctx(mw_require_admin()));

  app.get("/admin/stats", [](Request& req, Response& res)
  {
    const auto& a = req.state<AuthInfo>();
    res.json({ "ok", true, "admin", true, "subject", a.subject });
  });

  app.run(8080);
  return 0;
}
```

Try:

``` bash
curl -i http://127.0.0.1:8080/admin/stats
curl -i -H "x-user: gaspard" http://127.0.0.1:8080/admin/stats
curl -i -H "x-user: gaspard" -H "x-role: admin" http://127.0.0.1:8080/admin/stats
```

------------------------------------------------------------------------

## 5) Legacy HttpMiddleware style (adapt)

If you have an older middleware signature `(Request, Response, next)`
you can adapt it.

``` cpp
#include <vix.hpp>
#include <vix/middleware/app/adapter.hpp>
#include <string>

using namespace vix;
namespace J = vix::json;

static vix::middleware::HttpMiddleware require_header(std::string header, std::string expected)
{
  return [header = std::move(header), expected = std::move(expected)](Request& req, Response& res, vix::middleware::Next next)
  {
    const std::string got = req.header(header);
    if (got != expected)
    {
      res.status(401).json(J::obj({
        "ok", false,
        "error", "unauthorized",
        "required_header", header
      }));
      return;
    }
    next();
  };
}

int main()
{
  App app;

  vix::middleware::app::install_exact(app, "/api/ping", vix::middleware::app::adapt(require_header("x-demo", "1")));

  app.get("/api/ping", [](Request&, Response& res){
    res.json({ "ok", true, "pong", true });
  });

  app.run(8080);
  return 0;
}
```

Try:

``` bash
curl -i http://127.0.0.1:8080/api/ping
curl -i -H "x-demo: 1" http://127.0.0.1:8080/api/ping
```

------------------------------------------------------------------------

## 6) Chaining middleware

Apply multiple middlewares on the same prefix.

``` cpp
#include <vix.hpp>
#include <vix/middleware/app/adapter.hpp>
#include <vix/middleware/app/presets.hpp>

using namespace vix;

static vix::middleware::MiddlewareFn mw_mark()
{
  return [](vix::middleware::Context& ctx, vix::middleware::Next next)
  {
    ctx.res().header("x-mw", "on");
    next();
  };
}

int main()
{
  App app;

  // Chain: api key then a custom marker middleware
  vix::middleware::app::install(
    app,
    "/secure/",
    vix::middleware::app::chain(
      vix::middleware::app::api_key_dev("dev_key_123"),
      vix::middleware::app::adapt_ctx(mw_mark())
    )
  );

  app.get("/secure/hello", [](Request&, Response& res)
  {
    res.json({ "ok", true, "message", "Hello secure" });
  });

  app.run(8080);
  return 0;
}
```

Try:

``` bash
curl -i -H "x-api-key: dev_key_123" http://127.0.0.1:8080/secure/hello
```

------------------------------------------------------------------------

## What this teaches

-   Prefix install: protect a group of routes
-   Exact install: protect one route
-   Context middleware: state and headers
-   Legacy middleware adaptation
-   Basic RBAC-style gating
-   Middleware chaining
