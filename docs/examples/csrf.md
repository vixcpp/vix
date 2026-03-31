# CSRF (Beginner Guide)

This guide explains CSRF protection in Vix.cpp with tiny examples you can copy paste.

CSRF means Cross Site Request Forgery. It matters when you use cookies for auth (sessions) because browsers attach cookies automatically to cross site requests.

If your API accepts a state changing request (POST, PUT, PATCH, DELETE) and the browser sends cookies automatically, an attacker can trick a user into sending requests from another site unless you protect it.

Vix.cpp CSRF middleware uses a simple model:

- Server sets a CSRF token in a cookie (for example `csrf_token=abc`)
- Client must echo the same token in a header (for example `x-csrf-token: abc`)
- If cookie and header do not match, the request is rejected

---

## 1) Minimal CSRF on `/api` prefix

This is the smallest server:

- `GET /api/csrf` sets a CSRF cookie and returns the token
- `POST /api/update` requires the header token to match the cookie token

```cpp
/**
 *
 *  @file csrf_strict_server.cpp - CSRF middleware example (Vix.cpp)
 *
 */
#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>

using namespace vix;

int main()
{
  App app;

  // Protect all /api routes with CSRF
  // Default cookie: csrf_token
  // Default header: x-csrf-token
  // Default protect_get: false
  app.use("/api", middleware::app::csrf_dev());

  // Issue token (cookie + JSON response for convenience)
  app.get("/api/csrf", [](Request &, Response &res)
  {
    res.header("Set-Cookie", "csrf_token=abc; Path=/; SameSite=Lax");
    res.json({"csrf_token", "abc"});
  });

  // Protected write route
  app.post("/api/update", [](Request &, Response &res)
  {
    res.json({"ok", true, "message", "CSRF passed"});
  });

  app.run(8080);
  return 0;
}
```

### Test with curl (cookie jar)

```bash
# 1) Get token (cookie)
curl -i -c cookies.txt http://localhost:8080/api/csrf

# 2) FAIL: missing header
curl -i -b cookies.txt -X POST http://localhost:8080/api/update -d "x=1"

# 3) FAIL: wrong token
curl -i -b cookies.txt -X POST http://localhost:8080/api/update \
  -H "x-csrf-token: wrong" -d "x=1"

# 4) OK: header token matches cookie token
curl -i -b cookies.txt -X POST http://localhost:8080/api/update \
  -H "x-csrf-token: abc" -d "x=1"
```

Expected:

- Missing token header -> 403 (or 401 depending on config)
- Wrong token -> 403
- Matching token -> 200

---

## 2) CSRF + CORS (browser realistic)

When you call an API from a browser on another origin, you usually need both:

- CORS to allow the origin
- CSRF to protect cookie based write requests

Important detail: browsers send OPTIONS preflight requests. If you want the CORS middleware to answer preflight correctly, define explicit OPTIONS routes for endpoints you call from the browser.

### Minimal pattern

```cpp
/**
 *
 *  @file security_cors_csrf_server.cpp - CORS + CSRF (Vix.cpp)
 *
 */
#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>

using namespace vix;

int main()
{
  App app;

  // Apply on /api prefix (order matters for a production style pipeline)
  app.use("/api", middleware::app::cors_dev({"https://example.com"}));
  app.use("/api", middleware::app::csrf_dev("csrf_token", "x-csrf-token", false));

  // Explicit OPTIONS routes for browser preflight
  app.options("/api/update", [](Request &, Response &res){ res.status(204).send(); });
  app.options("/api/csrf",   [](Request &, Response &res){ res.status(204).send(); });

  // Token endpoint
  app.get("/api/csrf", [](Request &, Response &res)
  {
    res.header("Set-Cookie", "csrf_token=abc; Path=/; SameSite=Lax");
    res.json({"csrf_token", "abc"});
  });

  // Protected write endpoint
  app.post("/api/update", [](Request &, Response &res)
  {
    res.json({"ok", true, "message", "CORS ok + CSRF ok"});
  });

  app.get("/", [](Request &, Response &res){ res.send("public"); });

  app.run(8080);
  return 0;
}
```

### Test preflight (curl)

```bash
# Allowed origin preflight should return 204 + CORS headers
curl -i -X OPTIONS http://localhost:8080/api/update \
  -H "Origin: https://example.com" \
  -H "Access-Control-Request-Method: POST" \
  -H "Access-Control-Request-Headers: Content-Type, X-CSRF-Token"

# Blocked origin preflight should return 403
curl -i -X OPTIONS http://localhost:8080/api/update \
  -H "Origin: https://evil.com" \
  -H "Access-Control-Request-Method: POST"
```

---

## 3) Strict mode (protect GET too)

Most APIs do not need CSRF on GET because GET should be read only. But if you expose a dangerous GET endpoint (bad design, but it happens), you can protect it too.

```cpp
// Strict CSRF preset also protects GET
app.use("/api", middleware::app::csrf_strict_dev("csrf_token", "x-csrf-token"));
```

Rule of thumb:

- protect_get = false is typical
- protect_get = true only if you have state changes on GET or you want extreme hardening

---

## Common beginner mistakes

1) Confusing CORS with CSRF
CORS controls which origins can read responses. CSRF controls whether a cookie based write request is allowed.

2) Missing OPTIONS routes for browser calls
If preflight is not handled properly, the browser will block your requests even if your API works with curl.

3) Cookie SameSite and Secure flags
- SameSite=Lax often blocks cookies in some cross site POST cases
- For true cross site cookies in modern browsers you need HTTPS and SameSite=None; Secure
- For local dev HTTP, SameSite=Lax is fine for curl demos

4) CSRF is not needed for Authorization header auth
If you only use `Authorization: Bearer <token>` and do not rely on cookies, CSRF is usually not required because the browser will not attach Authorization headers automatically.

---

## Production notes (practical)

- Use CSRF when you use Session cookies.
- Prefer short lived CSRF tokens or rotate them on login.
- Combine with security headers and rate limiting on `/api`.

Recommended order for `/api`:

1) security headers
2) CORS
3) auth (session or jwt)
4) CSRF (if cookie based)
5) rate limit
6) business routes

---

# Complete example (copy paste)

This single file is a realistic production demo:

- Security headers on all `/api` responses
- CORS for selected origins
- CSRF protection for write requests
- Explicit OPTIONS routes for browser preflight
- Two endpoints: `/api/csrf` and `/api/update`

Save as: `security_cors_csrf_headers_server.cpp`

```cpp
/**
 *
 *  @file security_cors_csrf_headers_server.cpp - CORS + CSRF + Security Headers (Vix.cpp)
 *
 */
#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>

using namespace vix;

static void register_options(App& app)
{
  auto options_noop = [](Request &, Response &res){ res.status(204).send(); };

  app.options("/api/update", options_noop);
  app.options("/api/csrf", options_noop);
}

static void register_routes(App& app)
{
  app.get("/api/csrf", [](Request &, Response &res)
  {
    // Local dev: SameSite=Lax is fine.
    // Cross site in browsers: use HTTPS + SameSite=None; Secure.
    res.header("Set-Cookie", "csrf_token=abc; Path=/; SameSite=Lax");
    res.header("X-Request-Id", "req_csrf_1");
    res.json({"csrf_token", "abc"});
  });

  app.post("/api/update", [](Request &, Response &res)
  {
    res.header("X-Request-Id", "req_update_1");
    res.json({"ok", true, "message", "CORS ok + CSRF ok + HEADERS ok"});
  });

  app.get("/", [](Request &, Response &res){ res.send("public route"); });
}

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
  app.use("/api", middleware::app::csrf_dev("csrf_token", "x-csrf-token", false));

  register_options(app);
  register_routes(app);

  app.run(8080);
  return 0;
}
```

### Run

```bash
vix run security_cors_csrf_headers_server.cpp
```

### Curl test

```bash
curl -i -c cookies.txt http://localhost:8080/api/csrf
curl -i -b cookies.txt -X POST http://localhost:8080/api/update -d "x=1"
curl -i -b cookies.txt -X POST http://localhost:8080/api/update -H "x-csrf-token: abc" -d "x=1"
```

