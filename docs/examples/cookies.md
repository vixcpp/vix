# Cookies Guide

Cookies are small key/value strings stored by the browser and sent back on future requests.

They are useful for:

- sessions (login)
- user preferences
- CSRF tokens
- small state across requests

In HTTP, the server sends a cookie using the header:

Set-Cookie: name=value; options...

The browser then sends it back using:

Cookie: name=value; other=value

---

## Minimal Example: Set a Cookie

This endpoint sets a cookie named `hello` with value `vix`.

```cpp
#include <vix.hpp>
#include <vix/middleware/http/cookies.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/cookie", [](Request&, Response& res)
  {
    vix::middleware::cookies::Cookie c;
    c.name = "hello";
    c.value = "vix";
    c.max_age = 3600; // 1 hour

    vix::middleware::cookies::set(res, c);

    res.text("cookie set");
  });

  app.run(8080);
}
```

---

## Run

```bash
vix run cookie_app_simple.cpp
```

---

## Test With curl

### 1) See the Set-Cookie header

```bash
curl -i http://localhost:8080/cookie
```

You should see something like:

Set-Cookie: hello=vix; Path=/; Max-Age=3600; HttpOnly; SameSite=Lax

---

### 2) Send the cookie back manually

```bash
curl -i http://localhost:8080/echo-cookie -H "Cookie: hello=vix"
```

(We add this route in the complete example below.)

---

### 3) Let curl store cookies automatically

```bash
# Save cookies into jar.txt
curl -i -c jar.txt http://localhost:8080/cookie

# Reuse cookies from jar.txt
curl -i -b jar.txt http://localhost:8080/echo-cookie
```

---

## Most Important Cookie Options

### Path
Controls which URLs receive the cookie.
Default is `/`.

### Max-Age
How long the cookie lives in seconds.
- `max_age = 3600` => 1 hour
- `max_age = -1` => omit Max-Age (session cookie)

### HttpOnly
If true, JavaScript cannot read the cookie.
This is safer for session cookies.

### Secure
If true, cookie is only sent over HTTPS.
Use this in production.

### SameSite
Helps protect against CSRF.

Common values:

- Lax (default, good for most apps)
- Strict (more locked down)
- None (required for cross-site cookies, but must use Secure=true)

---

# Complete Example (set + read)

Save as: cookie_app_simple.cpp

```cpp
#include <vix.hpp>
#include <vix/middleware/http/cookies.hpp>

using namespace vix;

static void register_routes(App& app)
{
  // 1) Set a cookie
  app.get("/cookie", [](Request&, Response& res)
  {
    vix::middleware::cookies::Cookie c;
    c.name = "hello";
    c.value = "vix";
    c.max_age = 3600;
    c.http_only = true;
    c.secure = false;
    c.same_site = "Lax";

    vix::middleware::cookies::set(res, c);
    res.text("cookie set");
  });

  // 2) Read a cookie from the request
  app.get("/echo-cookie", [](Request& req, Response& res)
  {
    auto v = vix::middleware::cookies::get(req, "hello");

    res.json({
      "ok", true,
      "cookie_hello", v ? *v : "",
      "has_cookie", (bool)v
    });
  });
}

int main()
{
  App app;
  register_routes(app);
  app.run(8080);
  return 0;
}
```

---

## Quick Demo Commands

```bash
vix run cookie_app_simple.cpp
```

```bash
curl -i -c jar.txt http://localhost:8080/cookie
curl -i -b jar.txt http://localhost:8080/echo-cookie
```

---

## Summary

- Use `cookies::set(res, cookie)` to send Set-Cookie
- Use `cookies::get(req, "name")` to read Cookie
- In production, enable Secure=true + HTTPS for session cookies

