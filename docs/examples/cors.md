# CORS Middleware Guide

## What is CORS?

CORS means **Cross-Origin Resource Sharing**.

Browsers block frontend applications from calling APIs hosted on a different origin unless the server explicitly allows it.

An origin is:
- protocol (http / https)
- domain
- port

Example:
```bash
- http://localhost:5173
- http://localhost:8080
```
These are different origins → CORS is required.

---

## Why CORS exists

Without CORS, any website could call your API using a logged-in user's browser.

CORS allows the server to say:

- Which origins are allowed
- Which HTTP methods are allowed
- Which headers are allowed
- Whether credentials are allowed

---

# 1) Basic CORS Example

This allows only `https://example.com` to call `/api`.

```cpp
#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>

using namespace vix;

int main()
{
  App app;

  app.use("/api", middleware::app::cors_dev({"https://example.com"}));

  app.get("/api", [](Request &, Response &res)
  {
    res.header("X-Request-Id", "req_123");
    res.json({ "ok", true });
  });

  app.run(8080);
}
```

### Test

```bash
curl -i http://localhost:8080/api -H "Origin: https://example.com"
```

Expected:
- 200 OK
- Access-Control-Allow-Origin header present

---

# 2) Strict CORS with Preflight (OPTIONS)

Browsers send a **preflight request** before certain requests (POST, PUT, custom headers).

This is an OPTIONS request.

You should define explicit OPTIONS routes so the middleware can respond correctly.

```cpp
#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>

using namespace vix;

int main()
{
  App app;

  app.use("/api", middleware::app::cors_dev({"https://example.com"}));

  app.options("/api", [](Request &, Response &res)
  {
    res.status(204).send();
  });

  app.get("/api", [](Request &, Response &res)
  {
    res.json({ "ok", true });
  });

  app.run(8080);
}
```

### Test allowed origin

```bash
curl -i -X OPTIONS http://localhost:8080/api   -H "Origin: https://example.com"   -H "Access-Control-Request-Method: POST"
```

Expected: 204 + CORS headers.

### Test blocked origin

```bash
curl -i -X OPTIONS http://localhost:8080/api   -H "Origin: https://evil.com"   -H "Access-Control-Request-Method: POST"
```

Expected: 403 Forbidden.

---

# 3) CORS in Production (Important)

In real applications:

- Apply CORS only on API routes (not public static routes)
- Combine with:
  - Security headers
  - CSRF protection
  - Authentication
- Never allow "*" with credentials

Correct order:

```cpp
app.use("/api", middleware::app::security_headers_dev());
app.use("/api", middleware::app::cors_dev({"https://example.com"}));
app.use("/api", middleware::app::csrf_dev());
```

Order matters.

---

# Common Beginner Mistakes

1) Forgetting OPTIONS route
2) Allowing "*" in production
3) Not understanding that CORS is enforced by browsers, not curl
4) Confusing CORS with authentication (they are different)

---

# Complete Working Example

Save as: `cors_app_full.cpp`

```cpp
#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>

using namespace vix;

int main()
{
  App app;

  app.use("/api", middleware::app::security_headers_dev());
  app.use("/api", middleware::app::cors_dev({"https://example.com"}));

  app.options("/api/data", [](Request &, Response &res)
  {
    res.status(204).send();
  });

  app.get("/api/data", [](Request &, Response &res)
  {
    res.json({
      "ok", true,
      "message", "CORS working"
    });
  });

  app.get("/", [](Request &, Response &res)
  {
    res.send("Public route");
  });

  app.run(8080);
}
```

Run:

```bash
vix run cors_app_full.cpp
```

---

# Summary

CORS controls which origins can call your API.

Use:
- `cors_dev()` for development
- Explicit OPTIONS routes
- Combine with CSRF and security headers
- Restrict allowed origins in production

CORS is a browser security feature, not an authentication system.

