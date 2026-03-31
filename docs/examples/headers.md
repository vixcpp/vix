# Security headers

This guide shows how to add security headers in Vix.cpp, and how to combine them with CORS and CSRF safely.

What you get from the preset:

- X-Content-Type-Options: nosniff
- X-Frame-Options: DENY (or equivalent)
- Referrer-Policy
- Permissions-Policy
- Optional Strict-Transport-Security (HSTS) when enabled

Notes:

- Security headers are cheap and should usually be enabled for API responses.
- Order matters when stacking middleware.
- If you need cross-site cookies in browsers, you must use HTTPS and SameSite=None; Secure.

---

## 1) Minimal security headers on /api

This is the smallest pattern: apply headers only under a prefix.

```cpp
/**
 *
 *  @file security_headers_server.cpp - Security headers middleware example (Vix.cpp)
 *
 */
// Run:
//   vix run security_headers_server.cpp
//
// Tests:
//   curl -i http://localhost:8080/api/ping
//   curl -i http://localhost:8080/

#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>

using namespace vix;

int main()
{
  App app;

  // Apply security headers only on /api
  app.use("/api", middleware::app::security_headers_dev()); // HSTS is OFF by default

  app.get("/api/ping", [](Request &, Response &res)
  {
    res.json({"ok", true, "message", "headers applied"});
  });

  // Public route (no forced headers)
  app.get("/", [](Request &, Response &res)
  {
    res.send("public route");
  });

  app.run(8080);
  return 0;
}
```

What to look for:

```bash
curl -i http://localhost:8080/api/ping
```

You should see the security headers in the response.

---

## 2) Realistic stack: Security headers + CORS + CSRF (same prefix)

This is the common API pattern:

1) Security headers first (so they also apply to errors)
2) CORS second (so preflight and browser rules work)
3) CSRF third (protect write operations)

```cpp
/**
 *
 *  @file security_cors_csrf_headers_server.cpp - CORS + CSRF + Security Headers (Vix.cpp)
 *
 */
// Run:
//   vix run security_cors_csrf_headers_server.cpp
//
// Quick tests are listed below.

#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>

using namespace vix;

int main()
{
  App app;

  // Apply on ALL /api/*
  // Order matters: headers first, then CORS, then CSRF.
  app.use("/api", middleware::app::security_headers_dev()); // HSTS off by default

  app.use("/api", middleware::app::cors_dev({
    "http://localhost:5173",
    "http://0.0.0.0:5173",
    "https://example.com"
  }));

  // CSRF expects: cookie "csrf_token" and header "x-csrf-token" by default
  app.use("/api", middleware::app::csrf_dev("csrf_token", "x-csrf-token", false));

  // Explicit OPTIONS routes (lets CORS middleware answer preflight)
  app.options("/api/update", [](Request &, Response &res){ res.status(204).send(); });
  app.options("/api/csrf",   [](Request &, Response &res){ res.status(204).send(); });

  // Routes
  app.get("/api/csrf", [](Request &, Response &res)
  {
    // For cross-origin cookie in browsers:
    // - Use HTTPS
    // - SameSite=None; Secure
    //
    // For local dev over HTTP:
    // - SameSite=Lax is fine, but cookie might not be sent cross-site.
    res.header("Set-Cookie", "csrf_token=abc; Path=/; SameSite=Lax");
    res.header("X-Request-Id", "req_csrf_1");
    res.json({"csrf_token", "abc"});
  });

  app.post("/api/update", [](Request &, Response &res)
  {
    res.header("X-Request-Id", "req_update_1");
    res.json({"ok", true, "message", "CORS + CSRF + HEADERS"});
  });

  app.get("/", [](Request &, Response &res)
  {
    res.send("public route");
  });

  app.run(8080);
  return 0;
}
```

### Terminal tests (curl)

Preflight allowed (204 + CORS headers):

```bash
curl -i -X OPTIONS http://localhost:8080/api/update \
  -H "Origin: https://example.com" \
  -H "Access-Control-Request-Method: POST" \
  -H "Access-Control-Request-Headers: Content-Type, X-CSRF-Token"
```

Preflight blocked (403):

```bash
curl -i -X OPTIONS http://localhost:8080/api/update \
  -H "Origin: https://evil.com" \
  -H "Access-Control-Request-Method: POST"
```

Get CSRF cookie (sets csrf_token=abc):

```bash
curl -i -c cookies.txt http://localhost:8080/api/csrf \
  -H "Origin: https://example.com"
```

Fail: missing CSRF header:

```bash
curl -i -b cookies.txt -X POST http://localhost:8080/api/update \
  -H "Origin: https://example.com" \
  -d "x=1"
```

Fail: wrong token:

```bash
curl -i -b cookies.txt -X POST http://localhost:8080/api/update \
  -H "Origin: https://example.com" \
  -H "X-CSRF-Token: wrong" \
  -d "x=1"
```

OK: correct token:

```bash
curl -i -b cookies.txt -X POST http://localhost:8080/api/update \
  -H "Origin: https://example.com" \
  -H "X-CSRF-Token: abc" \
  -d "x=1"
```

---

## Common mistakes

### 1) Wrong order

If you put CSRF before CORS, browser preflight can fail in confusing ways.

Recommended order on the same prefix:

Security headers -> CORS -> CSRF -> Auth -> Rate limit -> Handlers

### 2) Cross-site cookies on HTTP

If your frontend is on another origin, browsers often require:

- SameSite=None
- Secure
- HTTPS

If you stay on HTTP locally, cookie behavior can differ from production.

### 3) Forgetting OPTIONS routes

Browsers send OPTIONS preflight requests. If you do not handle OPTIONS correctly,
your CORS middleware might not return the expected headers.

---

## Full copy-paste example (recommended)

This is the single-file version you can run immediately.

Save as headers_stack_server.cpp:

```cpp
#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>

using namespace vix;

static void register_routes(App &app)
{
  app.options("/api/csrf",   [](Request &, Response &res){ res.status(204).send(); });
  app.options("/api/update", [](Request &, Response &res){ res.status(204).send(); });

  app.get("/api/csrf", [](Request &, Response &res)
  {
    res.header("Set-Cookie", "csrf_token=abc; Path=/; SameSite=Lax");
    res.json({"csrf_token", "abc"});
  });

  app.post("/api/update", [](Request &, Response &res)
  {
    res.json({"ok", true, "message", "protected update"});
  });

  app.get("/", [](Request &, Response &res)
  {
    res.send("public route");
  });
}

int main()
{
  App app;

  // Stack (order matters)
  app.use("/api", middleware::app::security_headers_dev());
  app.use("/api", middleware::app::cors_dev({"https://example.com"}));
  app.use("/api", middleware::app::csrf_dev("csrf_token", "x-csrf-token", false));

  register_routes(app);

  app.run(8080);
  return 0;
}
```

Run:

```bash
vix run headers_stack_server.cpp
```

