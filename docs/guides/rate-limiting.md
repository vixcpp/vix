# Rate limiting

This guide shows how to add rate limiting to a Vix API.

Rate limiting protects your application from repeated requests, abuse, accidental loops, and brute-force attempts.

## What is rate limiting?

```txt
Allow 60 requests per minute per IP address.
```

If the client sends too many requests, the API returns `429 Too Many Requests`.

## Setup

```bash
vix new rate-limit-api
cd rate-limit-api
```

```cpp
#include <vix.hpp>
#include <vix/middleware.hpp>
```

## Simple global rate limit

```cpp
app.use(vix::middleware::app::rate_limit({
    .max_requests = 60,
    .window_seconds = 60
}));
```

## Stricter login rate limit

```cpp
// Global limit
app.use(vix::middleware::app::rate_limit({.max_requests = 60, .window_seconds = 60}));

// Stricter limit for /auth
vix::middleware::app::use_on_prefix(
    app,
    "/auth",
    vix::middleware::app::rate_limit({.max_requests = 5, .window_seconds = 60}));
```

## Custom rate limit options

```cpp
vix::middleware::security::RateLimitOptions options;
options.max_requests = 60;
options.window_seconds = 60;
options.key = "ip";   // or "user", "api_key"

app.use(vix::middleware::app::adapt_ctx(
    vix::middleware::security::rate_limit(std::move(options))));
```

## Where to apply rate limiting

| Endpoint                 | Suggested limit              |
|--------------------------|------------------------------|
| `GET /health`            | High limit or no limit.      |
| `GET /api/*`             | Normal application limit.    |
| `POST /auth/login`       | Strict authentication limit. |
| `POST /auth/register`    | Strict registration limit.   |
| `POST /password/reset`   | Very strict recovery limit.  |

## Test rate limiting

```bash
for i in $(seq 1 80); do
  curl -s -o /dev/null -w "%{http_code}\n" http://127.0.0.1:8080/api/data
done
```

After the limit is reached, responses should become `429`.

## Rate limit response headers

```text
X-RateLimit-Limit: 60
X-RateLimit-Remaining: 42
X-RateLimit-Reset: 1710000000
Retry-After: 30
```

## Recommended middleware order

```txt
CORS → rate limit → security headers → body limit → authentication → routes
```

## Rate limiting behind Nginx

Nginx forwards the real client IP via headers:

```nginx
proxy_set_header X-Real-IP $remote_addr;
proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
```

Only trust forwarded headers from your own reverse proxy.

## Common mistakes

### Not limiting login

Without a rate limit, attackers can retry many passwords quickly.

### Returning 403 instead of 429

```cpp
res.status(429).json({
  "ok", false,
  "error", "rate_limit_exceeded"
});
```

### Making local limits too high during testing

For testing:

```cpp
.max_requests = 3,
.window_seconds = 30
```

## What to use next

- [SQLite API guide](/guides/sqlite-api)
- [MySQL API guide](/guides/mysql-api)
- [Production Nginx + systemd guide](/guides/production-nginx-systemd)
