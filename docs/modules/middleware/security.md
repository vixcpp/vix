# Security

The `security` group protects HTTP routes before and after handlers run.

It covers browser security, cross-origin access, CSRF protection, IP filtering, and rate limiting.

Authentication answers:

```txt id="g4ohrd"
Who is making the request?
```

Security answers:

```txt id="e8v9df"
Should this HTTP request be allowed to reach this route?
Should this response include safer browser headers?
Should this client be slowed down or blocked?
```

The security middleware lives under:

```cpp id="ckqqih"
namespace vix::middleware::security
```

When using `vix::App`, prefer the App helpers:

```cpp id="rq8xoj"
namespace vix::middleware::app
```

## What security provides

The security group includes:

| Middleware     | Purpose                                                 |
| -------------- | ------------------------------------------------------- |
| `headers()`    | Add browser security headers to responses               |
| `cors()`       | Control cross-origin browser access                     |
| `csrf()`       | Protect unsafe methods with a cookie/header token check |
| `ip_filter()`  | Allow or deny requests based on client IP headers       |
| `rate_limit()` | Limit request frequency per client key                  |

For normal `vix::App` applications, use the App presets:

```cpp id="bss38n"
middleware::app::security_headers_dev()
middleware::app::cors_dev(...)
middleware::app::csrf_dev(...)
middleware::app::ip_filter_dev(...)
middleware::app::ip_filter_allow_deny_dev(...)
middleware::app::rate_limit_dev(...)
middleware::app::rate_limit_custom_dev(...)
```

## Basic security setup

A small API can start with:

```cpp id="y97irp"
#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

int main()
{
  App app;

  app.use("/api", middleware::app::security_headers_dev());
  app.use("/api", middleware::app::cors_dev({"https://example.com"}));
  app.use("/api", middleware::app::rate_limit_dev());

  app.get("/api/health", [](Request &, Response &res)
  {
    res.json({
      "ok", true
    });
  });

  app.run(8080);
}
```

This gives `/api` a basic HTTP security layer:

```txt id="uqehxe"
security headers
  make browser responses safer

CORS
  controls which browser origins can call the API

rate limit
  slows down abusive clients
```

## Recommended order

A practical order is:

```cpp id="thndml"
app.use("/api", middleware::app::security_headers_dev());
app.use("/api", middleware::app::cors_dev({"https://example.com"}));
app.use("/api", middleware::app::rate_limit_dev());
app.use("/api", middleware::app::body_limit_write_dev(1024 * 1024));
app.use("/api/private", middleware::app::api_key_dev("secret"));
app.use("/api/forms", middleware::app::csrf_dev());
```

The idea is:

```txt id="p6cqbm"
security headers
  can be added to most responses

CORS
  must handle browser preflight requests early

rate limit
  should reject abusive clients before expensive work

body limit
  should reject large bodies before parsers

authentication
  should protect private routes

CSRF
  should protect unsafe browser form/session routes
```

The exact order depends on the application.

The principle is stable:

```txt id="ibn5e9"
reject invalid requests early
keep route handlers focused
add response hardening consistently
```

## Security headers

`headers()` adds HTTP response headers that improve browser security.

The App preset is:

```cpp id="tw5tt3"
app.use("/api", middleware::app::security_headers_dev());
```

Example:

```cpp id="t9x58t"
#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

int main()
{
  App app;

  app.use("/api", middleware::app::security_headers_dev());

  app.get("/api/ping", [](Request &, Response &res)
  {
    res.json({
      "ok", true,
      "message", "headers applied"
    });
  });

  app.get("/", [](Request &, Response &res)
  {
    res.text("public route");
  });

  app.run(8080);
}
```

Test:

```bash id="xoj8o0"
curl -i http://127.0.0.1:8080/api/ping
```

Typical headers can include:

```txt id="k93yvi"
X-Content-Type-Options: nosniff
X-Frame-Options: DENY
Referrer-Policy: no-referrer
Permissions-Policy: ...
```

Security headers usually run after the handler because they modify the final response.

```txt id="c8yfu8"
request
  -> security headers middleware
  -> handler
  -> add headers
  -> response
```

## Configure security headers

Use the lower-level middleware when you need explicit options.

```cpp id="oivwrb"
vix::middleware::security::SecurityHeadersOptions opt;

opt.x_content_type_options = true;
opt.x_frame_options = true;
opt.referrer_policy = true;
opt.permissions_policy = true;
opt.hsts = false;
opt.content_security_policy = "default-src 'self'";

app.use("/api", vix::middleware::app::adapt_ctx(
  vix::middleware::security::headers(opt)
));
```

Main options:

| Option                    | Purpose                               |
| ------------------------- | ------------------------------------- |
| `x_content_type_options`  | Add `X-Content-Type-Options: nosniff` |
| `x_frame_options`         | Add `X-Frame-Options: DENY`           |
| `referrer_policy`         | Add `Referrer-Policy`                 |
| `permissions_policy`      | Add `Permissions-Policy`              |
| `hsts`                    | Add `Strict-Transport-Security`       |
| `content_security_policy` | Add a custom CSP value                |

Only enable HSTS when the application is served through HTTPS.

If Nginx or another proxy terminates TLS, make sure the deployment is truly HTTPS-only before enabling HSTS.

## CORS

CORS controls which browser origins can call your API.

It matters when a frontend is served from a different origin than the backend.

Example:

```txt id="z0acwq"
frontend
  https://example.com

backend
  http://127.0.0.1:8080
```

Install CORS on your API prefix:

```cpp id="uo8054"
app.use("/api", middleware::app::cors_dev({"https://example.com"}));
```

Example:

```cpp id="oaiu0c"
#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

int main()
{
  App app;

  app.use("/api", middleware::app::cors_dev({"https://example.com"}));

  app.get("/api", [](Request &, Response &res)
  {
    res.json({
      "ok", true
    });
  });

  app.run(8080);
}
```

Test:

```bash id="blzhmn"
curl -i \
  http://127.0.0.1:8080/api \
  -H "Origin: https://example.com"
```

Expected response headers include:

```txt id="tjxuzo"
Access-Control-Allow-Origin: https://example.com
Vary: Origin
```

## CORS preflight

Browsers send preflight requests for some cross-origin requests.

A preflight request uses:

```txt id="a51jts"
OPTIONS
Origin
Access-Control-Request-Method
Access-Control-Request-Headers
```

Example:

```bash id="aahdai"
curl -i \
  -X OPTIONS http://127.0.0.1:8080/api/update \
  -H "Origin: https://example.com" \
  -H "Access-Control-Request-Method: POST" \
  -H "Access-Control-Request-Headers: Content-Type, X-CSRF-Token"
```

Allowed origins should receive a successful preflight response.

Blocked origins should receive an error.

```bash id="sqc83x"
curl -i \
  -X OPTIONS http://127.0.0.1:8080/api/update \
  -H "Origin: https://evil.com" \
  -H "Access-Control-Request-Method: POST"
```

Expected blocked status:

```txt id="t5qfgd"
403 Forbidden
```

Common error code:

```txt id="dj23ix"
cors_forbidden
```

## Explicit OPTIONS routes

For browser APIs, it is often useful to define explicit `OPTIONS` routes for endpoints that need preflight.

```cpp id="jorsyr"
app.options("/api/update", [](Request &, Response &res)
{
  res.status(204).send();
});
```

With CORS installed on `/api`, the CORS middleware can handle the preflight behavior.

Example:

```cpp id="myx07w"
#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

int main()
{
  App app;

  app.use("/api", middleware::app::cors_dev({"https://example.com"}));

  app.options("/api/update", [](Request &, Response &res)
  {
    res.status(204).send();
  });

  app.post("/api/update", [](Request &, Response &res)
  {
    res.json({
      "ok", true
    });
  });

  app.run(8080);
}
```

Use explicit `OPTIONS` routes when you want predictable browser preflight behavior.

## Configure CORS

Use the lower-level middleware when you need explicit options.

```cpp id="p6g8sq"
vix::middleware::security::CorsOptions opt;

opt.allowed_origins = {"https://example.com"};
opt.allow_any_origin = false;
opt.allow_credentials = true;
opt.allow_methods = {"GET", "POST", "OPTIONS"};
opt.allow_headers = {"Content-Type", "Authorization", "X-CSRF-Token"};
opt.expose_headers = {"X-Request-Id"};
opt.max_age_seconds = 600;
opt.vary_origin = true;

app.use("/api", vix::middleware::app::adapt_ctx(
  vix::middleware::security::cors(opt)
));
```

Main options:

| Option              | Purpose                                |
| ------------------- | -------------------------------------- |
| `allowed_origins`   | List of accepted origins               |
| `allow_any_origin`  | Allow any origin when configured       |
| `allow_credentials` | Add `Access-Control-Allow-Credentials` |
| `allow_methods`     | Methods accepted during preflight      |
| `allow_headers`     | Headers accepted during preflight      |
| `expose_headers`    | Response headers visible to browsers   |
| `max_age_seconds`   | Browser preflight cache duration       |
| `vary_origin`       | Add `Vary: Origin`                     |

If credentials are enabled, avoid using `*` as the final `Access-Control-Allow-Origin` value.

## CSRF

`csrf()` protects unsafe HTTP methods using the double-submit cookie pattern.

The client must send the same token in:

```txt id="g2kaj6"
a cookie
a request header
```

Default names are usually:

```txt id="w7rxnm"
cookie: csrf_token
header: x-csrf-token
```

CSRF is most useful for browser-based applications that use cookies or sessions.

Example:

```cpp id="qwg2aj"
#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

int main()
{
  App app;

  app.use("/api", middleware::app::csrf_dev());

  app.get("/api/csrf", [](Request &, Response &res)
  {
    res.header("Set-Cookie", "csrf_token=abc; Path=/; SameSite=Lax");

    res.json({
      "csrf_token", "abc"
    });
  });

  app.post("/api/update", [](Request &, Response &res)
  {
    res.json({
      "ok", true,
      "message", "CSRF passed"
    });
  });

  app.run(8080);
}
```

Get the cookie:

```bash id="l0kgr6"
curl -i \
  -c cookies.txt \
  http://127.0.0.1:8080/api/csrf
```

Fail without the header:

```bash id="vj5c3l"
curl -i \
  -b cookies.txt \
  -X POST http://127.0.0.1:8080/api/update \
  -d "x=1"
```

Expected status:

```txt id="zux3lg"
403 Forbidden
```

Pass with matching cookie and header:

```bash id="jcznw4"
curl -i \
  -b cookies.txt \
  -X POST http://127.0.0.1:8080/api/update \
  -H "x-csrf-token: abc" \
  -d "x=1"
```

Expected status:

```txt id="u42mc6"
200 OK
```

Common error code:

```txt id="wgh118"
csrf_failed
```

## CSRF with CORS

When CORS and CSRF are used together, install CORS before CSRF.

```cpp id="onaxcn"
app.use("/api", middleware::app::cors_dev({"https://example.com"}));
app.use("/api", middleware::app::csrf_dev("csrf_token", "x-csrf-token", false));
```

CORS must be able to handle preflight requests.

CSRF should protect unsafe methods such as:

```txt id="pp60pt"
POST
PUT
PATCH
DELETE
```

Example:

```cpp id="wt7zq8"
#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

int main()
{
  App app;

  app.use("/api", middleware::app::security_headers_dev());
  app.use("/api", middleware::app::cors_dev({"https://example.com"}));
  app.use("/api", middleware::app::csrf_dev("csrf_token", "x-csrf-token", false));

  app.options("/api/update", [](Request &, Response &res)
  {
    res.status(204).send();
  });

  app.get("/api/csrf", [](Request &, Response &res)
  {
    res.header("Set-Cookie", "csrf_token=abc; Path=/; SameSite=Lax");

    res.json({
      "csrf_token", "abc"
    });
  });

  app.post("/api/update", [](Request &, Response &res)
  {
    res.json({
      "ok", true
    });
  });

  app.run(8080);
}
```

For cross-site cookies in browsers, production deployments may need cookie attributes such as:

```txt id="utjrst"
SameSite=None
Secure
```

Use those only when serving through HTTPS.

## Configure CSRF

Use the lower-level middleware when you need explicit options.

```cpp id="se6h67"
vix::middleware::security::CsrfOptions opt;

opt.cookie_name = "csrf_token";
opt.header_name = "x-csrf-token";
opt.protect_get = false;

app.use("/api", vix::middleware::app::adapt_ctx(
  vix::middleware::security::csrf(opt)
));
```

Main options:

| Option        | Purpose                                   |
| ------------- | ----------------------------------------- |
| `cookie_name` | Cookie that contains the CSRF token       |
| `header_name` | Header expected to contain the CSRF token |
| `protect_get` | Require CSRF on GET when set to true      |

Usually keep `protect_get` false.

GET routes should normally be safe and side-effect free.

## IP filter

`ip_filter()` allows or denies requests based on a client IP extracted from headers.

Common headers are:

```txt id="n7p3cs"
x-forwarded-for
x-real-ip
```

This is useful behind reverse proxies, internal APIs, admin endpoints, and private dashboards.

Example:

```cpp id="jowp2v"
#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

int main()
{
  App app;

  app.use("/api", middleware::app::ip_filter_allow_deny_dev(
    "x-forwarded-for",
    {"10.0.0.1", "127.0.0.1"},
    {"9.9.9.9"},
    true
  ));

  app.get("/", [](Request &, Response &res)
  {
    res.text("public route");
  });

  app.get("/api/hello", [](Request &req, Response &res)
  {
    res.json({
      "ok", true,
      "x_forwarded_for", req.header("x-forwarded-for")
    });
  });

  app.run(8080);
}
```

Allowed IP:

```bash id="t25jer"
curl -i \
  http://127.0.0.1:8080/api/hello \
  -H "X-Forwarded-For: 10.0.0.1"
```

Blocked by allow list:

```bash id="zhx0hu"
curl -i \
  http://127.0.0.1:8080/api/hello \
  -H "X-Forwarded-For: 1.2.3.4"
```

Explicitly denied:

```bash id="gbxb7o"
curl -i \
  http://127.0.0.1:8080/api/hello \
  -H "X-Forwarded-For: 9.9.9.9"
```

Expected blocked status:

```txt id="n4i4rc"
403 Forbidden
```

Common error codes:

```txt id="p8uw64"
ip_denied
ip_not_allowed
```

## Configure IP filter

Use the lower-level middleware when you need explicit control.

```cpp id="ci3lei"
vix::middleware::security::IpFilterOptions opt;

opt.header_name = "x-forwarded-for";
opt.allow = {"10.0.0.1", "127.0.0.1"};
opt.deny = {"9.9.9.9"};
opt.use_remote_addr_fallback = true;

app.use("/api", vix::middleware::app::adapt_ctx(
  vix::middleware::security::ip_filter(opt)
));
```

Main options:

| Option                     | Purpose                                    |
| -------------------------- | ------------------------------------------ |
| `allow`                    | If non-empty, only listed IPs are allowed  |
| `deny`                     | Denied IPs are rejected before allow rules |
| `header_name`              | Header used to extract the client IP       |
| `use_remote_addr_fallback` | Try fallback headers such as `x-real-ip`   |

Deny rules win before allow rules.

## Be careful with proxy headers

Headers such as `X-Forwarded-For` can be spoofed if clients connect directly to your server.

Use IP filtering with proxy headers only when:

```txt id="p4h7o0"
your app is behind a trusted proxy
the proxy overwrites client-provided forwarding headers
direct public access to the app port is blocked
```

If your app is exposed directly to the internet, do not blindly trust `X-Forwarded-For`.

## Rate limiting

`rate_limit()` limits how often a client can call a route.

It uses a token bucket model.

A client key is usually derived from a header such as:

```txt id="wz0vaa"
x-forwarded-for
x-real-ip
```

The App preset is:

```cpp id="ovltek"
app.use("/api", middleware::app::rate_limit_dev());
```

For demos, use a small capacity and no refill:

```cpp id="a22t05"
app.use("/api", middleware::app::rate_limit_custom_dev(
  5.0,
  0.0
));
```

Example:

```cpp id="u72la4"
#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

int main()
{
  App app;

  app.use("/api", middleware::app::rate_limit_custom_dev(
    5.0,
    0.0
  ));

  app.get("/", [](Request &, Response &res)
  {
    res.text("public route");
  });

  app.get("/api/ping", [](Request &req, Response &res)
  {
    res.json({
      "ok", true,
      "msg", "pong",
      "xff", req.header("x-forwarded-for")
    });
  });

  app.run(8080);
}
```

Test:

```bash id="t6zmxk"
for i in $(seq 1 6); do
  echo "---- $i"
  curl -i http://127.0.0.1:8080/api/ping
done
```

The sixth request can return:

```txt id="xjr2pc"
429 Too Many Requests
```

Common error code:

```txt id="o0yqrx"
rate_limited
```

Common response headers:

```txt id="dozxno"
X-RateLimit-Limit
X-RateLimit-Remaining
Retry-After
X-RateLimit-Reset
```

## Configure rate limit

Use the lower-level middleware when you need exact behavior.

```cpp id="tmxxki"
vix::middleware::security::RateLimitOptions opt;

opt.capacity = 60.0;
opt.refill_per_sec = 1.0;
opt.add_headers = true;
opt.key_header = "x-forwarded-for";

app.use("/api", vix::middleware::app::adapt_ctx(
  vix::middleware::security::rate_limit(opt)
));
```

Main options:

| Option           | Purpose                                       |
| ---------------- | --------------------------------------------- |
| `capacity`       | Maximum burst size                            |
| `refill_per_sec` | Tokens added per second                       |
| `add_headers`    | Add rate limit headers                        |
| `key_header`     | Header used to derive the default client key  |
| `key_fn`         | Custom function used to derive the client key |

Use `key_fn` when the key should come from something else, such as a tenant id, authenticated subject, or custom header.

```cpp id="ez0g8c"
vix::middleware::security::RateLimitOptions opt;

opt.capacity = 100.0;
opt.refill_per_sec = 10.0;
opt.key_fn = [](const vix::middleware::Request &req)
{
  const std::string tenant = req.header("x-tenant-id");

  return tenant.empty() ? "anonymous" : tenant;
};

app.use("/api", vix::middleware::app::adapt_ctx(
  vix::middleware::security::rate_limit(opt)
));
```

## Combine CORS, IP filter, and rate limit

A realistic API may combine multiple security layers.

```cpp id="ay25iz"
#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

int main()
{
  App app;

  app.use("/api", middleware::app::security_headers_dev());
  app.use("/api", middleware::app::cors_dev({"http://localhost:5173"}));

  app.use("/api", middleware::app::ip_filter_allow_deny_dev(
    "x-forwarded-for",
    {},
    {"1.2.3.4"},
    true
  ));

  app.use("/api", middleware::app::rate_limit_custom_dev(
    5.0,
    0.0,
    "x-forwarded-for"
  ));

  app.options("/api/ping", [](Request &, Response &res)
  {
    res.status(204).send();
  });

  app.get("/api/ping", [](Request &req, Response &res)
  {
    res.json({
      "ok", true,
      "ip", req.header("x-forwarded-for")
    });
  });

  app.run(8080);
}
```

Test allowed origin:

```bash id="rh56g3"
curl -i \
  http://127.0.0.1:8080/api/ping \
  -H "Origin: http://localhost:5173" \
  -H "X-Forwarded-For: 9.9.9.9"
```

Test denied IP:

```bash id="wodd96"
curl -i \
  http://127.0.0.1:8080/api/ping \
  -H "Origin: http://localhost:5173" \
  -H "X-Forwarded-For: 1.2.3.4"
```

Test rate limit:

```bash id="j2agq2"
for i in $(seq 1 6); do
  echo "---- $i"
  curl -i \
    http://127.0.0.1:8080/api/ping \
    -H "Origin: http://localhost:5173" \
    -H "X-Forwarded-For: 9.9.9.9"
done
```

This composition keeps route handlers simple.

The security layer decides whether the request should reach the route.

## Complete security example

```cpp id="acvb94"
#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

int main()
{
  App app;

  app.use("/api", middleware::app::security_headers_dev());
  app.use("/api", middleware::app::cors_dev({"https://example.com"}));
  app.use("/api", middleware::app::rate_limit_custom_dev(10.0, 1.0));
  app.use("/api/forms", middleware::app::csrf_dev("csrf_token", "x-csrf-token", false));

  app.options("/api/forms/update", [](Request &, Response &res)
  {
    res.status(204).send();
  });

  app.get("/api/health", [](Request &, Response &res)
  {
    res.json({
      "ok", true
    });
  });

  app.get("/api/forms/csrf", [](Request &, Response &res)
  {
    res.header("Set-Cookie", "csrf_token=abc; Path=/; SameSite=Lax");

    res.json({
      "csrf_token", "abc"
    });
  });

  app.post("/api/forms/update", [](Request &, Response &res)
  {
    res.json({
      "ok", true,
      "message", "updated"
    });
  });

  app.run(8080);
}
```

This gives the API:

```txt id="e6y604"
security headers
CORS
rate limiting
CSRF on form routes
explicit preflight route
```

Authentication can be added on top for private routes.

## Security vs authentication

Do not put everything in one category.

Security middleware protects the HTTP surface.

```txt id="pg6q6m"
CORS
CSRF
security headers
IP filter
rate limit
```

Authentication middleware identifies the caller.

```txt id="jmvmc3"
API key
JWT
RBAC
sessions
permissions
```

They work together.

Example:

```cpp id="ey9zgn"
app.use("/api", middleware::app::security_headers_dev());
app.use("/api", middleware::app::cors_dev());
app.use("/api", middleware::app::rate_limit_dev());

app.use("/api/admin", middleware::app::api_key_dev("secret"));
```

The security layer protects the HTTP surface.

The authentication layer protects private actions.

## Summary

Use the security group to protect routes before handlers do application work.

A good starting stack is:

```cpp id="awnvrm"
app.use("/api", middleware::app::security_headers_dev());
app.use("/api", middleware::app::cors_dev({"https://example.com"}));
app.use("/api", middleware::app::rate_limit_dev());
```

Add more specific protections when needed:

```cpp id="s0xyb1"
app.use("/api/forms", middleware::app::csrf_dev());
app.use("/api/internal", middleware::app::ip_filter_allow_deny_dev(
  "x-forwarded-for",
  {"10.0.0.1"},
  {},
  true
));
```

Remember the model:

```txt id="yyagja"
security middleware decides whether the request should reach the route
security headers make responses safer
authentication is documented separately
```
