# Cookies

This example shows how to read and write HTTP cookies with Vix.

Use cookies when you need to store small client-side values such as:

```txt id="hx9w20"
theme preference
language preference
visitor id
CSRF token
small browser state
session id
```

For server-side session data, use the session middleware.

For authentication, prefer signed sessions, JWT, or another explicit auth strategy.

## What this example builds

The app exposes:

```txt id="htnhjq"
GET /cookie/set
GET /cookie/read
GET /cookie/delete
GET /theme/light
GET /theme/dark
GET /theme
```

It demonstrates:

```txt id="g9yf97"
setting a cookie
reading a cookie
deleting a cookie
using cookie options
reading cookies from Request
writing Set-Cookie to Response
```

## Header

Use:

```cpp id="bjv6bu"
#include <vix/middleware.hpp>
```

or include only the cookie helper:

```cpp id="ovwmnm"
#include <vix/middleware/http/cookies.hpp>
```

The cookie helpers live in:

```cpp id="v53vtc"
vix::middleware::cookies
```

## Project structure

Create:

```txt id="t2xx52"
cookies_demo/
└── cookies.cpp
```

Create the file:

```bash id="cphwcb"
mkdir cookies_demo
cd cookies_demo
touch cookies.cpp
```

## Source

Open:

```txt id="g7ywn2"
cookies.cpp
```

Add:

```cpp id="kt7zbd"
#include <string>

#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

static void install_middleware(App &app)
{
  app.use("/cookie", middleware::app::request_id_dev());
  app.use("/cookie", middleware::app::timing_dev());
  app.use("/cookie", middleware::app::security_headers_dev());

  app.use("/theme", middleware::app::request_id_dev());
  app.use("/theme", middleware::app::timing_dev());
  app.use("/theme", middleware::app::security_headers_dev());
}

static void set_cookie(
  Response &res,
  std::string name,
  std::string value,
  int max_age)
{
  middleware::cookies::Cookie cookie;

  cookie.name = std::move(name);
  cookie.value = std::move(value);
  cookie.path = "/";
  cookie.max_age = max_age;
  cookie.http_only = true;
  cookie.secure = false;
  cookie.same_site = "Lax";

  middleware::cookies::set(res, cookie);
}

static void delete_cookie(
  Response &res,
  std::string name)
{
  middleware::cookies::Cookie cookie;

  cookie.name = std::move(name);
  cookie.value = "";
  cookie.path = "/";
  cookie.max_age = 0;
  cookie.http_only = true;
  cookie.secure = false;
  cookie.same_site = "Lax";

  middleware::cookies::set(res, cookie);
}

static void register_cookie_routes(App &app)
{
  app.get("/", [](Request &, Response &res)
  {
    res.send(
      "Cookies example\n"
      "\n"
      "Try:\n"
      "  curl -i -c cookies.txt http://127.0.0.1:8080/cookie/set\n"
      "  curl -i -b cookies.txt http://127.0.0.1:8080/cookie/read\n"
      "  curl -i -b cookies.txt -c cookies.txt http://127.0.0.1:8080/cookie/delete\n"
    );
  });

  app.get("/cookie/set", [](Request &, Response &res)
  {
    set_cookie(
      res,
      "hello",
      "vix",
      3600
    );

    res.json({
      "ok", true,
      "message", "cookie set",
      "name", "hello"
    });
  });

  app.get("/cookie/read", [](Request &req, Response &res)
  {
    auto value =
      middleware::cookies::get(req, "hello");

    if (!value)
    {
      res.status(404).json({
        "ok", false,
        "error", "Cookie not found",
        "name", "hello"
      });
      return;
    }

    res.json({
      "ok", true,
      "name", "hello",
      "value", *value
    });
  });

  app.get("/cookie/delete", [](Request &, Response &res)
  {
    delete_cookie(res, "hello");

    res.json({
      "ok", true,
      "message", "cookie deleted",
      "name", "hello"
    });
  });
}

static void register_theme_routes(App &app)
{
  app.get("/theme/light", [](Request &, Response &res)
  {
    set_cookie(
      res,
      "theme",
      "light",
      60 * 60 * 24 * 30
    );

    res.json({
      "ok", true,
      "theme", "light"
    });
  });

  app.get("/theme/dark", [](Request &, Response &res)
  {
    set_cookie(
      res,
      "theme",
      "dark",
      60 * 60 * 24 * 30
    );

    res.json({
      "ok", true,
      "theme", "dark"
    });
  });

  app.get("/theme", [](Request &req, Response &res)
  {
    auto theme =
      middleware::cookies::get(req, "theme");

    res.json({
      "ok", true,
      "theme", theme ? *theme : "default"
    });
  });
}

int main()
{
  App app;

  install_middleware(app);
  register_cookie_routes(app);
  register_theme_routes(app);

  app.run(8080);
  return 0;
}
```

## Run it

Run:

```bash id="of7lpw"
vix run cookies.cpp
```

The server listens on:

```txt id="b2y646"
http://127.0.0.1:8080
```

## Set a cookie

Use `curl` with `-c` to save cookies into a local cookie jar:

```bash id="p018si"
curl -i \
  -c cookies.txt \
  http://127.0.0.1:8080/cookie/set
```

Expected status:

```txt id="rdoysb"
200 OK
```

Expected response header:

```txt id="vpy45r"
Set-Cookie: hello=vix; Path=/; Max-Age=3600; HttpOnly; SameSite=Lax
```

Expected body:

```json id="zpxc1r"
{
  "ok": true,
  "message": "cookie set",
  "name": "hello"
}
```

The cookie is stored by the client.

With `curl`, it is stored in:

```txt id="e6ssr0"
cookies.txt
```

## Read a cookie

Use `curl` with `-b` to send cookies from the cookie jar:

```bash id="b7xfcu"
curl -i \
  -b cookies.txt \
  http://127.0.0.1:8080/cookie/read
```

Expected body:

```json id="kq8lic"
{
  "ok": true,
  "name": "hello",
  "value": "vix"
}
```

The route reads the cookie with:

```cpp id="uchx4y"
auto value =
  middleware::cookies::get(req, "hello");
```

The return type is:

```cpp id="fn1epg"
std::optional<std::string>
```

So the handler must handle the missing-cookie case.

## Missing cookie

Call `/cookie/read` without sending cookies:

```bash id="vv795l"
curl -i http://127.0.0.1:8080/cookie/read
```

Expected status:

```txt id="gu1ewf"
404 Not Found
```

Expected body:

```json id="cznbfj"
{
  "ok": false,
  "error": "Cookie not found",
  "name": "hello"
}
```

## Delete a cookie

Use `-b` to send the cookie and `-c` to update the cookie jar:

```bash id="b85n7q"
curl -i \
  -b cookies.txt \
  -c cookies.txt \
  http://127.0.0.1:8080/cookie/delete
```

Expected header shape:

```txt id="iel65u"
Set-Cookie: hello=; Path=/; Max-Age=0; HttpOnly; SameSite=Lax
```

The important part is:

```txt id="n9lkzv"
Max-Age=0
```

That tells the browser to remove the cookie.

Try reading again:

```bash id="f3ibda"
curl -i \
  -b cookies.txt \
  http://127.0.0.1:8080/cookie/read
```

Expected status:

```txt id="fbsnyc"
404 Not Found
```

## Theme preference example

Set dark theme:

```bash id="dt3fwn"
curl -i \
  -c cookies.txt \
  http://127.0.0.1:8080/theme/dark
```

Read theme:

```bash id="njs19d"
curl -i \
  -b cookies.txt \
  http://127.0.0.1:8080/theme
```

Expected body:

```json id="dmy2yc"
{
  "ok": true,
  "theme": "dark"
}
```

Set light theme:

```bash id="w1qz39"
curl -i \
  -b cookies.txt \
  -c cookies.txt \
  http://127.0.0.1:8080/theme/light
```

Read again:

```bash id="i6x34d"
curl -i \
  -b cookies.txt \
  http://127.0.0.1:8080/theme
```

Expected body:

```json id="x5qzrv"
{
  "ok": true,
  "theme": "light"
}
```

This is a good cookie use case because the value is small and not sensitive.

## How to create a cookie

A cookie is represented by:

```cpp id="g15y8r"
middleware::cookies::Cookie cookie;
```

Example:

```cpp id="pqdyu8"
middleware::cookies::Cookie cookie;

cookie.name = "hello";
cookie.value = "vix";
cookie.path = "/";
cookie.max_age = 3600;
cookie.http_only = true;
cookie.secure = false;
cookie.same_site = "Lax";

middleware::cookies::set(res, cookie);
```

This writes a `Set-Cookie` response header.

## Cookie fields

The cookie struct contains:

```cpp id="qx98oy"
struct Cookie
{
  std::string name;
  std::string value;

  std::string path{"/"};
  std::string domain{};
  int max_age{-1};
  bool http_only{true};
  bool secure{false};
  std::string same_site{"Lax"};
};
```

Meaning:

```txt id="deiqu9"
name
  cookie name

value
  cookie value

path
  URL path where the cookie applies

domain
  optional domain

max_age
  lifetime in seconds, -1 means no Max-Age

http_only
  blocks JavaScript access when true

secure
  sends cookie only over HTTPS when true

same_site
  browser cross-site policy
```

## Read one cookie

Use:

```cpp id="zmou2r"
auto value =
  middleware::cookies::get(req, "theme");
```

Then:

```cpp id="krf3c8"
if (value)
{
  res.json({
    "theme", *value
  });
}
```

The cookie may be missing, so use `std::optional`.

## Parse all cookies

You can parse the full Cookie header:

```cpp id="kg27fu"
auto cookies =
  middleware::cookies::parse(req);
```

Example:

```cpp id="hx6qhu"
app.get("/cookie/all", [](Request &req, Response &res)
{
  auto all = middleware::cookies::parse(req);

  res.json({
    "ok", true,
    "count", static_cast<long long>(all.size())
  });
});
```

Use `get(...)` when you only need one cookie.

Use `parse(...)` when you need several values.

## Delete a cookie

To delete a cookie, set the same name and path with:

```txt id="mkscku"
Max-Age=0
```

Example:

```cpp id="trilf3"
middleware::cookies::Cookie cookie;

cookie.name = "theme";
cookie.value = "";
cookie.path = "/";
cookie.max_age = 0;
cookie.http_only = true;
cookie.secure = false;
cookie.same_site = "Lax";

middleware::cookies::set(res, cookie);
```

The name and path should match the cookie you want to remove.

## HttpOnly

Use:

```cpp id="vszfve"
cookie.http_only = true;
```

This adds:

```txt id="zpqr07"
HttpOnly
```

`HttpOnly` means browser JavaScript cannot read the cookie with `document.cookie`.

Use it for sensitive cookies such as:

```txt id="tq1uc1"
session ids
auth tokens
CSRF-related server cookies
```

For a harmless theme preference, `HttpOnly` can be false if JavaScript needs to read it.

For server-owned cookies, keep it true.

## Secure

Use:

```cpp id="j2nf91"
cookie.secure = true;
```

This adds:

```txt id="df6ph2"
Secure
```

A `Secure` cookie is sent only over HTTPS.

For local HTTP development, `secure = false` is common.

For production HTTPS apps, use:

```cpp id="rdm5m4"
cookie.secure = true;
```

especially for authentication and session cookies.

## SameSite

Use:

```cpp id="l6tbgp"
cookie.same_site = "Lax";
```

Common values are:

```txt id="clu188"
Lax
Strict
None
```

A practical default is:

```txt id="cr46yw"
Lax
```

Use `None` only when the cookie must be sent cross-site.

Modern browsers require `Secure` when `SameSite=None`.

So for cross-site production cookies:

```cpp id="vv28mu"
cookie.same_site = "None";
cookie.secure = true;
```

## Cookie path

Use:

```cpp id="lek49q"
cookie.path = "/";
```

That makes the cookie available to the whole site.

You can limit it:

```cpp id="oufxss"
cookie.path = "/admin";
```

Then the browser sends it only for matching paths.

Use a narrow path when a cookie belongs only to a specific area of the app.

## Cookie domain

Most apps can leave the domain empty:

```cpp id="pxyibf"
cookie.domain = "";
```

That makes the browser use the current host.

Set a domain only when you understand the browser cookie domain rules.

Example shape:

```cpp id="fy6j05"
cookie.domain = ".example.com";
```

Use this only when subdomains need to share a cookie.

## Important limitation

The current native response header map stores one value per header key.

That means repeated `Set-Cookie` headers may not be preserved by the current response API.

For simple examples, this is fine:

```cpp id="bzukw8"
middleware::cookies::set(res, cookie);
```

But if you need to set multiple cookies in the same response, make sure the response layer supports multiple `Set-Cookie` headers before relying on it.

This is important because HTTP cookies normally require separate `Set-Cookie` headers.

## Cookies vs sessions

Cookies store data on the client.

Sessions store data on the server and usually keep only a session id in the cookie.

Cookie example:

```txt id="xts6fq"
theme=dark
```

Session example:

```txt id="xm0dz0"
sid=abc123
```

The server uses `sid` to load data such as:

```txt id="sa8miu"
user id
cart
flash messages
login state
```

Use cookies for small client values.

Use sessions for server-managed state.

## Cookies vs JWT

A cookie is a transport/storage mechanism.

A JWT is a token format.

You can put a JWT in a cookie, but that is a separate authentication design choice.

Simple rule:

```txt id="we3gti"
cookie
  browser storage and transport

session
  server-side state using a cookie id

JWT
  signed token containing claims
```

## Security notes

Do not store sensitive data directly in plain cookies.

Avoid this:

```txt id="dd9x7k"
role=admin
user_id=123
balance=5000
```

The client can modify plain cookies.

For sensitive state, use:

```txt id="w6r45k"
server-side sessions
signed cookies
encrypted cookies
JWT with verification
database-backed state
```

A theme preference is fine in a plain cookie.

Authentication state should be protected.

## Production defaults

For session or auth-related cookies in production, prefer:

```txt id="dk0opc"
HttpOnly = true
Secure = true
SameSite = Lax or Strict
Path = /
short and intentional lifetime
```

For cross-site applications, such as frontend and API on different sites, cookie rules are stricter.

You may need:

```txt id="aes1hy"
SameSite=None
Secure=true
CORS with credentials
HTTPS
```

Design that carefully.

## Complete test flow

Run:

```bash id="qk3mug"
vix run cookies.cpp
```

Set cookie:

```bash id="ryvc0d"
curl -i \
  -c cookies.txt \
  http://127.0.0.1:8080/cookie/set
```

Read cookie:

```bash id="ewcq63"
curl -i \
  -b cookies.txt \
  http://127.0.0.1:8080/cookie/read
```

Delete cookie:

```bash id="p1v4sn"
curl -i \
  -b cookies.txt \
  -c cookies.txt \
  http://127.0.0.1:8080/cookie/delete
```

Read after delete:

```bash id="hgsphi"
curl -i \
  -b cookies.txt \
  http://127.0.0.1:8080/cookie/read
```

Theme example:

```bash id="w7clge"
curl -i -c cookies.txt http://127.0.0.1:8080/theme/dark
curl -i -b cookies.txt http://127.0.0.1:8080/theme
curl -i -b cookies.txt -c cookies.txt http://127.0.0.1:8080/theme/light
curl -i -b cookies.txt http://127.0.0.1:8080/theme
```

## Summary

Use cookies for small client-side values.

Set a cookie:

```cpp id="d058dr"
middleware::cookies::Cookie cookie;

cookie.name = "theme";
cookie.value = "dark";
cookie.path = "/";
cookie.max_age = 3600;
cookie.http_only = true;
cookie.secure = false;
cookie.same_site = "Lax";

middleware::cookies::set(res, cookie);
```

Read a cookie:

```cpp id="a9jzoh"
auto theme =
  middleware::cookies::get(req, "theme");
```

Delete a cookie:

```cpp id="p2s7ng"
cookie.name = "theme";
cookie.value = "";
cookie.path = "/";
cookie.max_age = 0;

middleware::cookies::set(res, cookie);
```

The mental model is:

```txt id="lidxkf"
Set-Cookie response header
  tells browser to store or delete a cookie

Cookie request header
  sends stored cookies back to the server

Vix cookie helpers
  build, parse, read, and write cookie values
```
