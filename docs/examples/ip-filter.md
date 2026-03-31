# IP Filter (Beginner Guide)

Welcome 👋\
This page explains how to protect routes using the **IP filter
middleware** in Vix.cpp.

With an IP filter you can:

-   allow only trusted IPs (allowlist)
-   block known bad IPs (denylist)
-   protect `/api/*` while keeping public routes open

## What is an IP filter?

An IP filter checks the client IP address **before** your route handler
runs.

If the IP is not allowed, the middleware stops the request early and
returns:

-   **403 Forbidden**

## Where does the server get the client IP?

In real production deployments you usually have a reverse proxy (Nginx,
cloud load balancer).\
That proxy sends the client IP through headers such as:

-   `X-Forwarded-For` (most common)
-   `X-Real-IP`

When `X-Forwarded-For` contains multiple values like:

    client, proxy1, proxy2

Vix uses the **first IP** (the real client).

# 1) Minimal IP filter on `/api/*`

This example:

-   keeps `/` public
-   protects `/api/hello`
-   allows only `10.0.0.1` and `127.0.0.1`
-   explicitly denies `9.9.9.9` (deny wins)

``` cpp
#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>

using namespace vix;

int main()
{
  App app;

  // Apply on /api/*
  app.use("/api", middleware::app::ip_filter_allow_deny_dev(
                      "x-forwarded-for",
                      {"10.0.0.1", "127.0.0.1"}, // allow
                      {"9.9.9.9"},               // deny (priority)
                      true                       // fallback to x-real-ip, etc.
                      ));

  app.get("/", [](Request &, Response &res) {
    res.send("public route");
  });

  app.get("/api/hello", [](Request &req, Response &res) {
    res.json({
      "ok", true,
      "message", "Hello from /api/hello",
      "x_forwarded_for", req.header("x-forwarded-for"),
      "x_real_ip", req.header("x-real-ip")
    });
  });

  app.run(8080);
}
```

# 2) Test with curl

Start:

``` bash
vix run ip_filter_server.cpp
```

Public route (no middleware):

``` bash
curl -i http://localhost:8080/
```

Allowed IP:

``` bash
curl -i http://localhost:8080/api/hello -H "X-Forwarded-For: 10.0.0.1"
```

Not allowed (not in allow list):

``` bash
curl -i http://localhost:8080/api/hello -H "X-Forwarded-For: 1.2.3.4"
```

Denied explicitly (deny wins):

``` bash
curl -i http://localhost:8080/api/hello -H "X-Forwarded-For: 9.9.9.9"
```

X-Forwarded-For chain:

``` bash
curl -i http://localhost:8080/api/hello -H "X-Forwarded-For: 10.0.0.1, 127.0.0.1"
```

# 3) Common beginner mistakes

## Mistake 1: trusting X-Forwarded-For directly on the internet

If your server is directly exposed (no proxy), attackers can forge
headers.

Production rule:

-   only trust `X-Forwarded-For` if it comes from a trusted proxy
-   otherwise use remote address (socket IP) or `X-Real-IP` set by your
    proxy

## Mistake 2: forgetting that allowlist blocks everything else

If you configure an allow list, any IP not in it is blocked.

## Mistake 3: wrong header name

Your middleware reads the header you give it.

If you configured:

-   `"x-forwarded-for"`

Then your requests must send:

-   `X-Forwarded-For: ...`

Header names are case-insensitive in HTTP, but spelling must match.

# 4) When to use allow, deny, or both

-   **Allow only**: admin dashboards, internal APIs, webhooks from known
    providers
-   **Deny only**: block a list of abusive IPs
-   **Allow + deny**: allow trusted range but always block specific
    offenders (deny priority)

# Summary

-   Install IP filter on a prefix: `app.use("/api", ...)`
-   Use allowlist for strict protection
-   Use denylist for explicit blocks
-   Prefer running behind a proxy that sets `X-Forwarded-For` safely
