# Production API Architecture with Groups

This guide shows a production-ready way to structure a Vix.cpp API using `group()` and `Group` middleware.
Goal: clean routing, consistent security, and predictable behavior under load.

## What Groups give you

A `Group` is a scoped router with:

- a base prefix (example: `/api/v1`)
- scoped middleware via `group.use(...)`
- nested groups (example: `/api/v1/admin`)
- optional `protect()` for a sub-scope (example: protect only `/secure` inside `/api`)

Think of `group()` as the API layout, and `use()` as the policy for that layout.

## The production layout (recommended)

Use this structure:

- `/health` public health check
- `/api/v1` public + authenticated APIs
- `/api/v1/internal` internal endpoints (tight IP allowlist)
- `/api/v1/admin` admin endpoints (JWT + RBAC)
- `/api/v1/uploads` uploads (multipart + strict body limit)
- `/docs` optional docs routes (if you ship offline docs)

Recommended versioning: `/api/v1` at the root, not in headers.
It keeps routing explicit and avoids client confusion.

## Middleware order (the rule)

Order matters. For a typical API scope, apply in this order:

1) Security headers
2) CORS (if browser clients exist)
3) Body limit (reject early)
4) IP filter (reject early)
5) Rate limit (protect resources)
6) Auth (API key or JWT)
7) Authorization (RBAC)
8) Business routes

Reason: reject bad requests as early as possible, before costly work.

## Keys and identity

Production key sources are usually:

- IP: via `X-Forwarded-For` set by your reverse proxy
- API key: via `x-api-key` header
- JWT: via `Authorization: Bearer <token>`
- Session: cookie based for browser apps

If you use `X-Forwarded-For`, do not trust client-provided values unless they come from a trusted proxy.
In local testing, you can set headers manually with curl. In production, your load balancer should set them.

## Pattern 1: API root group with shared hardening

```cpp
auto api = app.group("/api/v1");

api.use(middleware::app::security_headers_dev());   // baseline headers
api.use(middleware::app::cors_dev({"https://example.com"})); // if you serve browsers
api.use(middleware::app::body_limit_write_dev(64 * 1024));   // 64KB for writes
api.use(middleware::app::rate_limit_dev(120, std::chrono::minutes(1))); // 120/min
```

Notes:
- `body_limit_write_dev()` applies only to POST/PUT/PATCH in your presets.
- If you need strict chunked behavior, use `body_limit_dev(max, apply_to_get, allow_chunked)`.

## Pattern 2: Public vs secure sub-scopes inside the same group

Keep public routes inside the group, then protect a sub-prefix:

```cpp
api.get("/public", ...);

// Only secure scope needs API key
api.protect("/secure", middleware::app::api_key_dev("secret"));

api.get("/secure/me", ...);
```

This keeps your API surface obvious: `/public` stays public, `/secure/*` is protected.

## Pattern 3: Nested admin group with JWT + RBAC

For admin endpoints, do:

- JWT to authenticate
- RBAC to require role and optionally permissions

```cpp
api.group("/admin", [&](App::Group& admin){
  admin.use(middleware::app::jwt_auth("dev_secret"));
  admin.use(middleware::app::rbac_admin()); // requires role=admin

  admin.get("/dashboard", ...);
});
```

If you need granular permissions, chain `require_perm("x:y")` after RBAC context.

## Pattern 4: Internal endpoints (IP allowlist + optional API key)

Internal is for metrics, maintenance, control-plane calls.

Use IP allowlist first. Optionally add API key.

```cpp
api.group("/internal", [&](App::Group& in){
  in.use(middleware::app::ip_allowlist_dev("x-forwarded-for", {"10.0.0.1", "127.0.0.1"}));
  in.use(middleware::app::api_key_dev("internal_key"));

  in.get("/status", ...);
});
```

## Pattern 5: Upload scope (multipart + strict limits)

Uploads are where you must be strict:

- set body limits
- set multipart limit and upload dir
- consider rate limiting separately for uploads

```cpp
api.group("/uploads", [&](App::Group& up){
  up.use(middleware::app::body_limit_dev(5 * 1024 * 1024, false, false)); // 5MB, no chunked
  up.use(middleware::app::multipart_save_dev("uploads", 5 * 1024 * 1024));

  up.post("/image", ...);
});
```

## Pattern 6: Response caching for GET APIs

Cache only GET routes, under a prefix, with explicit bypass header.

```cpp
api.use(middleware::app::http_cache({
  .ttl_ms = 30'000,
  .allow_bypass = true,
  .bypass_header = "x-vix-cache",
  .bypass_value = "bypass",
  .vary_headers = {"accept-language"},
  .add_debug_header = true,
  .debug_header = "x-vix-cache-status",
}));
```

Use caching for read-heavy endpoints, not for user-specific endpoints unless you vary by user identity.

## Pattern 7: Compression

Compression is best applied globally, but keep a sane `min_size`.
It should also add `Vary: Accept-Encoding`.

```cpp
app.use(vix::middleware::app::adapt_ctx(
  vix::middleware::performance::compression({
    .min_size = 512,
    .add_vary = true,
    .enabled = true,
  })
));
```

## Pattern 8: Sessions vs JWT in production

Use JWT when:
- you have stateless API clients (mobile, CLI, services)
- you want horizontal scaling without shared session storage
- you want explicit RBAC claims in token

Use sessions when:
- you have browser apps
- you need CSRF protection + cookie-based auth
- you want easy server-side invalidation (rotate secret or store session ids)

Common production model:
- browser: session cookie + CSRF
- external API: JWT
- internal tools: API key + IP allowlist

## Observability checklist

At minimum:

- Set and return a request id (header: `x-request-id`)
- Log: method, path, status, latency, client key
- Add debug headers only in dev (cache status, rate limit remaining, etc.)

If your Logger supports request context, set it at the beginning of a request middleware.

## Complete example (copy-paste)

Save as: `production_groups_api.cpp`

```cpp
/**
 *
 *  @file production_groups_api.cpp
 *  @author Gaspard Kirira
 *
 *  Vix.cpp - Production API Architecture with Groups
 *
 */
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>
#include <vix/middleware/app/http_cache.hpp>

using namespace vix;

static void register_health(App& app)
{
  app.get("/health", [](Request&, Response& res){
    res.json({ "ok", true, "status", "up" });
  });
}

int main()
{
  App app;

  // Global compression (optional)
  // app.use(vix::middleware::app::adapt_ctx(
  //   vix::middleware::performance::compression({ .min_size = 512, .add_vary = true, .enabled = true })
  // ));

  register_health(app);

  // API root: /api/v1
  auto api = app.group("/api/v1");

  // Baseline hardening on all API responses
  api.use(middleware::app::security_headers_dev(false)); // HSTS off in dev
  api.use(middleware::app::cors_dev({
    "http://localhost:5173",
    "http://127.0.0.1:5173",
    "https://example.com"
  }));

  // Early rejection for write methods
  api.use(middleware::app::body_limit_write_dev(64 * 1024)); // 64KB for writes

  // Rate limiting per client key
  api.use(middleware::app::rate_limit_dev(120, std::chrono::minutes(1)));

  // Cache only GET endpoints under /api/v1/public-data
  api.group("/public-data", [&](App::Group& pub){
    pub.use(middleware::app::http_cache({
      .ttl_ms = 10'000,
      .allow_bypass = true,
      .bypass_header = "x-vix-cache",
      .bypass_value = "bypass",
      .vary_headers = {"accept-language"},
      .add_debug_header = true,
      .debug_header = "x-vix-cache-status",
    }));

    pub.get("/users", [](Request& req, Response& res){
      const std::string lang = req.has_header("accept-language") ? req.header("accept-language") : "none";
      res.json(vix::json::obj({
        "ok", true,
        "message", "users from origin",
        "accept_language", lang
      }));
    });
  });

  // Public API
  api.get("/public", [](Request&, Response& res){
    res.json({ "ok", true, "scope", "public" });
  });

  // Secure scope: API key
  api.protect("/secure", middleware::app::api_key_dev("secret"));

  api.get("/secure/whoami", [](Request& req, Response& res){
    auto &k = req.state<vix::middleware::auth::ApiKey>();
    res.json({ "ok", true, "scope", "secure", "api_key", k.value });
  });

  // Admin scope: JWT + RBAC(role=admin)
  api.group("/admin", [&](App::Group& admin){
    admin.use(middleware::app::jwt_auth("dev_secret"));
    admin.use(middleware::app::rbac_admin());

    admin.get("/dashboard", [](Request& req, Response& res){
      auto &authz = req.state<vix::middleware::auth::Authz>();
      res.json({ "ok", true, "scope", "admin", "sub", authz.subject });
    });
  });

  // Internal scope: IP allowlist + API key
  api.group("/internal", [&](App::Group& in){
    in.use(middleware::app::ip_allowlist_dev("x-forwarded-for", {"127.0.0.1", "10.0.0.1"}));
    in.use(middleware::app::api_key_dev("internal_key"));

    in.get("/status", [](Request&, Response& res){
      res.json({ "ok", true, "scope", "internal", "status", "green" });
    });
  });

  std::cout
    << "Running:\n"
    << "  http://localhost:8080/health\n"
    << "  http://localhost:8080/api/v1/public\n"
    << "  http://localhost:8080/api/v1/public-data/users\n"
    << "  http://localhost:8080/api/v1/secure/whoami\n"
    << "  http://localhost:8080/api/v1/admin/dashboard\n"
    << "  http://localhost:8080/api/v1/internal/status\n\n"
    << "API key:\n"
    << "  secret\n"
    << "Internal key:\n"
    << "  internal_key\n\n"
    << "Try:\n"
    << "  curl -i http://localhost:8080/health\n"
    << "  curl -i http://localhost:8080/api/v1/public\n"
    << "  curl -i http://localhost:8080/api/v1/public-data/users\n"
    << "  curl -i -H \"x-vix-cache: bypass\" http://localhost:8080/api/v1/public-data/users\n"
    << "  curl -i http://localhost:8080/api/v1/secure/whoami\n"
    << "  curl -i -H \"x-api-key: secret\" http://localhost:8080/api/v1/secure/whoami\n"
    << "  curl -i http://localhost:8080/api/v1/admin/dashboard\n"
    << "  curl -i -H \"x-api-key: internal_key\" -H \"X-Forwarded-For: 127.0.0.1\" http://localhost:8080/api/v1/internal/status\n";

  app.run(8080);
  return 0;
}
```

## Run

```bash
vix run production_groups_api.cpp
```

## Next step

If you want to push this to a real deployment, the next upgrade is:
- strict trust of proxy headers (trusted proxies list)
- real JWT exp verification
- CSRF + session mode for browser apps
- structured logs + request id middleware
- per-scope rate limits (uploads lower, public higher)

