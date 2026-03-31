# Auth and Middleware (Minimal Patterns)

This page shows minimal auth and middleware patterns in Vix.cpp.

Rule of this doc:
- one concept
- one minimal `main()`
- a quick curl test

## 1) API key middleware (protect one route)

A public route plus a secure route that requires `x-api-key`.

```cpp
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

```bash
curl -i http://127.0.0.1:8080/public
curl -i http://127.0.0.1:8080/secure/whoami
curl -i -H "x-api-key: dev_key_123" http://127.0.0.1:8080/secure/whoami
```

## 2) Prefix protection (protect all /api routes)

Everything under `/api/` is protected.

```cpp
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

```bash
curl -i http://127.0.0.1:8080/api/ping
curl -i -H "x-api-key: dev_key_123" http://127.0.0.1:8080/api/ping
```

## 3) Custom middleware (context style + RequestState)

Store data into request state and read it in the handler.

```cpp
#include <vix.hpp>
#include <vix/middleware/app/adapter.hpp>
#include <chrono>
#include <string>
using namespace vix;

struct RequestId { std::string value; };

static long long now_ms()
{
  using namespace std::chrono;
  return (long long)time_point_cast<milliseconds>(system_clock::now())
      .time_since_epoch().count();
}

static vix::middleware::MiddlewareFn mw_request_id()
{
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

  app.use(vix::middleware::app::adapt_ctx(mw_request_id()));

  app.get("/who", [](Request& req, Response& res){
    res.json({ "ok", true, "request_id", req.state<RequestId>().value });
  });

  app.run(8080);
  return 0;
}
```

Try:

```bash
curl -i http://127.0.0.1:8080/who
```

## 4) Role gating (fake auth + admin only)

Minimal RBAC style gate using headers for the demo.

```cpp
#include <vix.hpp>
#include <vix/middleware/app/adapter.hpp>
#include <string>

using namespace vix;
namespace J = vix::json;

struct AuthInfo
{
  bool authed{false};
  std::string subject;
  std::string role;
};

static vix::middleware::MiddlewareFn mw_fake_auth()
{
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

static vix::middleware::MiddlewareFn mw_require_admin()
{
  return [](vix::middleware::Context& ctx, vix::middleware::Next next)
  {
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

```bash
curl -i http://127.0.0.1:8080/admin/stats
curl -i -H "x-user: gaspard" http://127.0.0.1:8080/admin/stats
curl -i -H "x-user: gaspard" -H "x-role: admin" http://127.0.0.1:8080/admin/stats
```

## 5) Legacy HttpMiddleware style (adapt)

If you have an older middleware signature `(Request, Response, next)` you can adapt it.

```cpp
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

  vix::middleware::app::install_exact(
    app,
    "/api/ping",
    vix::middleware::app::adapt(require_header("x-demo", "1"))
  );

  app.get("/api/ping", [](Request&, Response& res){
    res.json({ "ok", true, "pong", true });
  });

  app.run(8080);
  return 0;
}
```

Try:

```bash
curl -i http://127.0.0.1:8080/api/ping
curl -i -H "x-demo: 1" http://127.0.0.1:8080/api/ping
```

## 6) Chaining middleware

Apply multiple middlewares on the same prefix.

```cpp
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

  vix::middleware::app::install(
    app,
    "/secure/",
    vix::middleware::app::chain(
      vix::middleware::app::api_key_dev("dev_key_123"),
      vix::middleware::app::adapt_ctx(mw_mark())
    )
  );

  app.get("/secure/hello", [](Request&, Response& res){
    res.json({ "ok", true, "message", "Hello secure" });
  });

  app.run(8080);
  return 0;
}
```

Try:

```bash
curl -i -H "x-api-key: dev_key_123" http://127.0.0.1:8080/secure/hello
```

## What this teaches

- Prefix install: protect a group of routes
- Exact install: protect one route
- Context middleware: state and headers
- Legacy middleware adaptation
- Basic RBAC style gating
- Middleware chaining

---

# RBAC (Roles + Permissions) using JWT

## What is RBAC

RBAC means Role Based Access Control.

You check:
- roles (admin, user, editor)
- permissions (products:write, orders:read)

In Vix.cpp:
- JWT extracts claims
- RBAC builds `Authz`
- `require_role()` and `require_perm()` enforce rules

## Request flow

1. Client sends JWT: `Authorization: Bearer <token>`
2. JWT middleware validates signature
3. RBAC builds an `Authz` context
4. Role and permission middlewares run
5. Handler executes

## Minimal RBAC pattern

```cpp
App app;

vix::middleware::auth::JwtOptions jwt_opt{};
jwt_opt.secret = "dev_secret";
jwt_opt.verify_exp = false;

vix::middleware::auth::RbacOptions rbac_opt{};
rbac_opt.require_auth = true;
rbac_opt.use_resolver = false;

auto jwt_mw  = vix::middleware::app::adapt_ctx(vix::middleware::auth::jwt(jwt_opt));
auto ctx_mw  = vix::middleware::app::adapt_ctx(vix::middleware::auth::rbac_context(rbac_opt));
auto role_mw = vix::middleware::app::adapt_ctx(vix::middleware::auth::require_role("admin"));
auto perm_mw = vix::middleware::app::adapt_ctx(vix::middleware::auth::require_perm("products:write"));

app.use(vix::middleware::app::when(
  [](const Request& r){ return r.path() == "/admin"; }, std::move(jwt_mw)));
app.use(vix::middleware::app::when(
  [](const Request& r){ return r.path() == "/admin"; }, std::move(ctx_mw)));
app.use(vix::middleware::app::when(
  [](const Request& r){ return r.path() == "/admin"; }, std::move(role_mw)));
app.use(vix::middleware::app::when(
  [](const Request& r){ return r.path() == "/admin"; }, std::move(perm_mw)));
```

## Reading `Authz` in a handler

```cpp
app.get("/admin", [](Request& req, Response& res){
  auto& authz = req.state<vix::middleware::auth::Authz>();
  res.json({
    "ok", true,
    "sub", authz.subject,
    "has_admin", authz.has_role("admin"),
    "has_products_write", authz.has_perm("products:write")
  });
});
```

## Common statuses

- 401: missing token, invalid token, invalid signature
- 403: authenticated but missing required role or permission

---

# Rate limiting (minimal)

Rate limiting protects your API from brute force, spam, and bursts.

The model is a token bucket:
- capacity: max burst
- refill_per_sec: tokens per second
- empty bucket returns 429

## Minimal limiter on /api

```cpp
#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>
using namespace vix;

int main()
{
  App app;

  app.use("/api", middleware::app::rate_limit_custom_dev(5.0, 0.0));

  app.get("/api/ping", [](Request& req, Response& res){
    res.json({ "ok", true, "msg", "pong", "xff", req.header("x-forwarded-for") });
  });

  app.run(8080);
}
```

Try:

```bash
for i in $(seq 1 6); do
  echo "---- $i"
  curl -i http://localhost:8080/api/ping
done
```

---

# CSRF (cookie + header)

CSRF is relevant mainly for browser sessions using cookies.

Vix default:
- cookie: `csrf_token`
- header: `x-csrf-token`

## Minimal CSRF on /api

```cpp
#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>
using namespace vix;

int main()
{
  App app;

  app.use("/api", middleware::app::csrf_dev());

  app.get("/api/csrf", [](Request&, Response& res){
    res.header("Set-Cookie", "csrf_token=abc; Path=/; SameSite=Lax");
    res.json({ "csrf_token", "abc" });
  });

  app.post("/api/update", [](Request&, Response& res){
    res.json({ "ok", true, "message", "CSRF passed" });
  });

  app.run(8080);
}
```

Try:

```bash
curl -i -c cookies.txt http://localhost:8080/api/csrf
curl -i -b cookies.txt -X POST http://localhost:8080/api/update -d "x=1"
curl -i -b cookies.txt -X POST http://localhost:8080/api/update -H "x-csrf-token: wrong" -d "x=1"
curl -i -b cookies.txt -X POST http://localhost:8080/api/update -H "x-csrf-token: abc" -d "x=1"
```

---

# Complete example (Session + RBAC + Rate limit)

This is a single file that shows:
- cookie sessions for browser style auth
- RBAC protected admin API using JWT
- rate limit on /api

Save as `security_complete.cpp`.

```cpp
#include <chrono>
#include <iostream>
#include <string>

#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>

#include <vix/middleware/app/adapter.hpp>
#include <vix/middleware/auth/jwt.hpp>
#include <vix/middleware/auth/rbac.hpp>

using namespace vix;

// Example admin token (HS256, secret=dev_secret)
// payload: {"sub":"user123","roles":["admin"],"perms":["products:write"]}
static const std::string TOKEN_OK =
  "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
  "eyJzdWIiOiJ1c2VyMTIzIiwicm9sZXMiOlsiYWRtaW4iXSwicGVybXMiOlsicHJvZHVjdHM6d3JpdGUiXX0."
  "w1y3nA2F1kq0oJ0x8wWc5wQx8zF4h2d6V7mYp0jYk3Q";

static void install_session(App& app)
{
  app.use(middleware::app::session_dev(
    "dev_session_secret",
    "sid",
    std::chrono::hours(24 * 7),
    false,
    "Lax",
    true,
    "/",
    true
  ));
}

static void install_api_security(App& app)
{
  // Rate limit all /api traffic
  app.use("/api", middleware::app::rate_limit_dev(60, std::chrono::minutes(1)));

  // JWT + RBAC only for /api/admin
  vix::middleware::auth::JwtOptions jwt_opt{};
  jwt_opt.secret = "dev_secret";
  jwt_opt.verify_exp = false;

  vix::middleware::auth::RbacOptions rbac_opt{};
  rbac_opt.require_auth = true;
  rbac_opt.use_resolver = false;

  auto jwt_mw  = vix::middleware::app::adapt_ctx(vix::middleware::auth::jwt(jwt_opt));
  auto ctx_mw  = vix::middleware::app::adapt_ctx(vix::middleware::auth::rbac_context(rbac_opt));
  auto role_mw = vix::middleware::app::adapt_ctx(vix::middleware::auth::require_role("admin"));
  auto perm_mw = vix::middleware::app::adapt_ctx(vix::middleware::auth::require_perm("products:write"));

  app.use(vix::middleware::app::when(
    [](const Request& r){ return r.path().rfind("/api/admin", 0) == 0; },
    std::move(jwt_mw)
  ));
  app.use(vix::middleware::app::when(
    [](const Request& r){ return r.path().rfind("/api/admin", 0) == 0; },
    std::move(ctx_mw)
  ));
  app.use(vix::middleware::app::when(
    [](const Request& r){ return r.path().rfind("/api/admin", 0) == 0; },
    std::move(role_mw)
  ));
  app.use(vix::middleware::app::when(
    [](const Request& r){ return r.path().rfind("/api/admin", 0) == 0; },
    std::move(perm_mw)
  ));
}

static void install_routes(App& app)
{
  app.get("/", [](Request&, Response& res){
    res.send(
      "Vix security complete example:\n"
      "  GET /session        increments a counter stored in a signed cookie\n"
      "  GET /api/ping        rate limited\n"
      "  GET /api/admin/stats requires JWT + role admin + perm products:write\n"
    );
  });

  app.get("/session", [](Request& req, Response& res){
    auto& s = req.state<middleware::auth::Session>();
    int n = s.get("n") ? std::stoi(*s.get("n")) : 0;
    s.set("n", std::to_string(++n));
    res.text("n=" + std::to_string(n));
  });

  app.get("/api/ping", [](Request&, Response& res){
    res.json({ "ok", true, "pong", true });
  });

  app.get("/api/admin/stats", [](Request& req, Response& res){
    auto& authz = req.state<vix::middleware::auth::Authz>();
    res.json({
      "ok", true,
      "sub", authz.subject,
      "is_admin", authz.has_role("admin"),
      "can_write_products", authz.has_perm("products:write")
    });
  });
}

int main()
{
  App app;

  install_session(app);
  install_api_security(app);
  install_routes(app);

  std::cout
    << "Running:\n"
    << "  http://localhost:8080/\n"
    << "  http://localhost:8080/session\n"
    << "  http://localhost:8080/api/ping\n"
    << "  http://localhost:8080/api/admin/stats\n\n"
    << "Admin token:\n  " << TOKEN_OK << "\n\n"
    << "Try:\n"
    << "  curl -i http://localhost:8080/session\n"
    << "  curl -i http://localhost:8080/api/ping\n"
    << "  curl -i http://localhost:8080/api/admin/stats\n"
    << "  curl -i -H \"Authorization: Bearer " << TOKEN_OK << "\" http://localhost:8080/api/admin/stats\n";

  app.run(8080);
  return 0;
}
```

Run:

```bash
vix run security_complete.cpp
```


