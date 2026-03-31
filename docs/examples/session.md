# Session Middleware Guide (Beginner)

## 1. What is a Session?

A session allows the server to store user-specific data across multiple
HTTP requests.

HTTP is stateless. That means every request is independent. Sessions
solve this by attaching a session ID to the client (usually via a
cookie).

The server then:

-   Reads the session ID from the cookie
-   Loads session data
-   Allows you to read/write values
-   Sends the updated session back (signed)

------------------------------------------------------------------------

## 2. Basic Example

``` cpp
#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>
using namespace vix;

int main()
{
  App app;

  app.use(middleware::app::adapt_ctx(
      middleware::auth::session({.secret = "dev"})));

  app.get("/session", [](Request &req, Response &res)
  {
    auto &s = req.state<middleware::auth::Session>();

    int n = s.get("n") ? std::stoi(*s.get("n")) : 0;
    s.set("n", std::to_string(++n));

    res.text("n=" + std::to_string(n));
  });

  app.run(8080);
}
```

Run:
```bash
    vix run session_app.cpp
```
Test:
```bash
    curl -i http://localhost:8080/session
    curl -i http://localhost:8080/session
    curl -i http://localhost:8080/session
```
You will see:
```bash
    n=1
    n=2
    n=3
```
The value persists because it is stored in the session.

------------------------------------------------------------------------

## 3. Using the Preset (Recommended)

Instead of manually adapting, use:

``` cpp
app.use(middleware::app::session_dev("dev_secret"));
```

Default behavior:

-   Cookie name: sid
-   Signed session cookie
-   HttpOnly enabled
-   SameSite=Lax
-   TTL: 7 days

------------------------------------------------------------------------

## 4. How Session Works Internally

1.  Client sends request.
2.  Middleware checks cookie (sid).
3.  If missing, creates new session (auto_create=true).
4.  Session data is signed with secret.
5.  Updated session is written back to cookie.

Security note: Always use a strong secret in production.

------------------------------------------------------------------------

## 5. Strict Production Mode

``` cpp
app.use(middleware::app::session_strict("super_strong_secret"));
```

This enables:

-   Secure=true
-   HttpOnly=true
-   SameSite=None
-   HTTPS required

------------------------------------------------------------------------

## 6. Reading and Writing Values

``` cpp
auto &s = req.state<middleware::auth::Session>();

s.set("user_id", "42");

auto uid = s.get("user_id");
if (uid)
{
    res.text("User = " + *uid);
}
```

------------------------------------------------------------------------

## 7. Full Example (Complete App)

``` cpp
#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>

using namespace vix;

int main()
{
  App app;

  // Install session middleware
  app.use(middleware::app::session_dev("dev_secret"));

  app.get("/", [](Request &, Response &res)
  {
    res.text("Visit /login, /me, /logout");
  });

  app.get("/login", [](Request &req, Response &res)
  {
    auto &s = req.state<middleware::auth::Session>();
    s.set("user", "alice");
    res.text("Logged in as alice");
  });

  app.get("/me", [](Request &req, Response &res)
  {
    auto &s = req.state<middleware::auth::Session>();
    auto user = s.get("user");

    if (!user)
    {
      res.status(401).text("Not logged in");
      return;
    }

    res.text("Current user: " + *user);
  });

  app.get("/logout", [](Request &req, Response &res)
  {
    auto &s = req.state<middleware::auth::Session>();
    s.clear();
    res.text("Logged out");
  });

  app.run(8080);
}
```

Test:
```bash
    curl -i http://localhost:8080/login
    curl -i http://localhost:8080/me
    curl -i http://localhost:8080/logout
    curl -i http://localhost:8080/me
```
