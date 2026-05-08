# Authentication

Protect routes with API keys, sessions, cookies, and JWT.

```txt
GET /                 → public
GET /api/profile      → requires x-api-key
GET /api/admin/stats  → requires x-api-key + x-role: admin
```

## Create a workspace

```bash
mkdir -p ~/tmp/vix-examples/authentication
cd ~/tmp/vix-examples/authentication
touch main.cpp
```

## Full code

```cpp
#include <iostream>
#include <string>
#include <vix.hpp>
#include <vix/middleware/app/adapter.hpp>
#include <vix/middleware/app/presets.hpp>
#include <vix/middleware/app/http_cache.hpp>
#include <vix/middleware/http/cookies.hpp>
#include <vix/middleware/auth/session.hpp>
using namespace vix;

static void respond_error(Response &res, int status,
                           const std::string &code, const std::string &message)
{
  res.status(status).json({"ok", false, "error", code, "message", message});
}

static void install_middlewares(App &app)
{
  app.use([](Request &req, Response &, App::Next next){
    std::cout << "[request] " << req.method() << " " << req.path() << "\n";
    next();
  });

  app.use([](Request &, Response &res, App::Next next){
    res.header("X-Powered-By", "Vix.cpp");
    next();
  });

  app.protect("/api/profile", [](Request &req, Response &res, App::Next next){
    if (req.header("x-api-key") != "dev_key_123"){
      respond_error(res, 401, "unauthorized", "Missing or invalid API key");
      return;
    }
    next();
  });

  app.protect("/api/admin", [](Request &req, Response &res, App::Next next){
    if (req.header("x-api-key") != "dev_key_123"){
      respond_error(res, 401, "unauthorized", "Missing or invalid API key");
      return;
    }

    if (req.header("x-role") != "admin"){
      respond_error(res, 403, "forbidden", "Admin role required");
      return;
    }
   next();
  });
}

static void register_routes(App &app)
{
  app.get("/", [](Request &, Response &res){
    res.json({
      "ok", true,
      "message", "Public route"
    });
  });

  app.get("/api/profile", [](Request &, Response &res){
    res.json({
      "ok", true,
      "message", "Private profile",
      "user", vix::json::kv({
                {"id", 1},
                {"name", "Ada"}
              })
    });
  });

  app.get("/api/admin/stats", [](Request &, Response &res){
    res.json({
      "ok", true,
      "message", "Admin stats",
      "users", 42
    });
  });
}

int main()
{
  App app;

  install_middlewares(app);
  register_routes(app);

  app.run(8080);

  return 0;
}
```

## Run and test

```bash
vix run main.cpp
```

```bash
curl -i http://127.0.0.1:8080/                                    # 200
curl -i http://127.0.0.1:8080/api/profile                         # 401
curl -i http://127.0.0.1:8080/api/profile -H "x-api-key: dev_key_123"  # 200
curl -i http://127.0.0.1:8080/api/admin/stats -H "x-api-key: dev_key_123"  # 403
curl -i http://127.0.0.1:8080/api/admin/stats \
  -H "x-api-key: dev_key_123" -H "x-role: admin"                 # 200
```

## 401 vs 403

| Status | Meaning | Example |
|--------|---------|---------|
| 401 Unauthorized | Not authenticated | Missing or invalid key/token |
| 403 Forbidden | Authenticated but not allowed | Valid key, wrong role |

## API key middleware preset

```cpp
app.use("/api/secure", vix::middleware::app::api_key_dev("dev_key_123"));
```

## Sessions

```cpp
#include <vix/middleware/auth/session.hpp>

app.use("/api/session", vix::middleware::app::session_dev("dev_session_secret", "sid"));

app.get("/api/session/whoami", [](Request &req, Response &res){
  auto *session = req.try_state<vix::middleware::auth::Session>();
  if (!session) {
    res.status(500).json({
      "ok", false,
      "error", "session_missing"
    });
    return;
  }

  auto name = session->get("name").value_or("guest");

  res.json({
    "ok", true,
    "sid", session->id,
    "name", name
  });
});

app.post("/api/session/setname/{name}", [](Request &req, Response &res){
  auto *session = req.try_state<vix::middleware::auth::Session>();
  if (!session) {
    res.status(500).json({"ok", false});
    return;
  }

  session->set("name", req.param("name", "guest"));

  res.json({
    "ok", true,
    "updated", true
  });
});
```

```bash
curl -i -c jar.txt http://127.0.0.1:8080/api/session/whoami
curl -i -b jar.txt -X POST http://127.0.0.1:8080/api/session/setname/gaspard
curl -i -b jar.txt http://127.0.0.1:8080/api/session/whoami
```

## Session options

```cpp
vix::middleware::auth::SessionOptions options;
options.secret = "dev_session_secret";
options.cookie_name = "sid";
options.secure = false;      // true in production (HTTPS)
options.http_only = true;
options.same_site = "Lax";
options.auto_create = true;
options.ttl = std::chrono::hours(24 * 7);
```

## JWT

```cpp
app.use("/api/private", vix::middleware::app::jwt_dev("dev_jwt_secret", false));
// Second arg: enforce expiration (true in production)
```

## Recommended production shape

```txt
request → CORS → rate limit → security headers → authentication → authorization → route
```

| Route | Auth | Authorization |
|-------|------|--------------|
| `GET /` | none | public |
| `GET /api/profile` | required | current user |
| `GET /api/admin/stats` | required | admin only |
| `POST /p2p/connect` | required | admin only |

## Common mistakes

### Calling next() after auth error

```cpp
// Wrong
if (api_key != "dev_key_123") {
  respond_error(res, 401, "...", "...");
}
next();

// Correct
if (api_key != "dev_key_123") {
  respond_error(res, 401, "...", "...");
  return;
}
next();
```

### Trusting role headers in production

In this example `x-role: admin` comes from the client. In production, role must come from a verified JWT, server-side session, or database lookup — never trust user-supplied role.

### Forgetting HTTPS for cookies

```cpp
options.secure = true;    // production
options.http_only = true;
options.same_site = "Lax";
```

## What you should remember

```txt
invalid credentials → send 401 and return
valid credentials, wrong role → send 403 and return
allowed → call next()
```

The core idea: **authentication decides who the user is — authorization decides what they can do.**

Next: [Database](/examples/database)
