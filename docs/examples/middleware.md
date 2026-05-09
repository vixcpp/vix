# Middleware

Add shared logic around routes: logging, headers, CORS, rate limiting, route protection.

```txt
request → middleware → route handler → response
```

## What you will build

```txt
GET /            → public route
GET /api/health  → API route with headers
GET /admin/stats → protected admin route
```

## Create a workspace

```bash
mkdir -p ~/tmp/vix-examples/middleware
cd ~/tmp/vix-examples/middleware
touch main.cpp
```

## Full code

```cpp
#include <iostream>
#include <string>
#include <vix.hpp>
using namespace vix;

static void respond_error(Response &res, int status,
                           const std::string &code, const std::string &message)
{
  res.status(status).json({"ok", false, "error", code, "message", message});
}

int main()
{
  App app;

  // Request logging — global
  app.use([](Request &req, Response &, App::Next next){
    std::cout << "[request] " << req.method() << " " << req.path() << "\n";
    next();
  });

  // Security headers — global
  app.use([](Request &, Response &res, App::Next next){
    res.header("X-Powered-By", "Vix.cpp"); res.header("X-Content-Type-Options", "nosniff");
    next();
  });

  // API prefix headers
  app.use("/api", [](Request &, Response &res, App::Next next){
    res.header("X-API", "true");
    next();
  });

  // Admin protection
  app.protect("/admin", [](Request &req, Response &res, App::Next next){
    if (req.header("x-admin-token") != "secret"){
      respond_error(res, 401, "unauthorized", "Missing or invalid admin token");
      return;
    }
    next();
  });

  app.get("/", [](Request &, Response &res) {
    res.text("Public page");
  });

  app.get("/api/health", [](Request &, Response &res){
    res.json({
      "ok", true,
      "service", "middleware-example"
    });
  });

  app.get("/admin/stats", [](Request &, Response &res){
    res.json({
      "ok", true,
      "message", "private admin stats",
      "users", 42
    });
  });

  app.run(8080);

  return 0;
}
```

## Run and test

```bash
vix run main.cpp
```

```bash
curl -i http://127.0.0.1:8080/
curl -i http://127.0.0.1:8080/api/health          # includes X-API: true
curl -i http://127.0.0.1:8080/admin/stats         # 401
curl -i http://127.0.0.1:8080/admin/stats -H "x-admin-token: secret"  # 200
```

## Key middleware patterns

```cpp
// Global middleware
app.use([](Request &, Response &res, App::Next next){
  res.header("X-App", "Vix");
  next();
});

// Prefix middleware
app.use("/api", [](Request &, Response &res, App::Next next){
  res.header("X-API", "true");
  next();
});

// Route protection (prefix)
app.protect("/admin", middleware);

// Route protection (exact path)
app.protect_exact("/admin/hook", middleware);
```

## Add CORS

```cpp
#include <vix/middleware/app/presets.hpp>

app.use(vix::middleware::app::cors_dev({
    "http://localhost:5173",
    "http://127.0.0.1:5173"
}));
```

## Add rate limiting

```cpp
#include <vix/middleware/app/presets.hpp>

app.use(vix::middleware::app::rate_limit({.max_requests = 60, .window_seconds = 60}));

// Stricter for auth routes
vix::middleware::app::use_on_prefix(app, "/auth",
    vix::middleware::app::rate_limit({.max_requests = 5, .window_seconds = 60}));
```

Test:

```bash
for i in $(seq 1 8); do
  curl -s -o /dev/null -w "%{http_code}\n" http://127.0.0.1:8080/api/health
done
```

## Recommended middleware order

```txt
CORS → rate limit → request logging → security headers → body limits → auth → routes
```

## Common mistakes

```cpp
// Forgetting next()
// Wrong
app.use([](Request &, Response &res, App::Next next) {
  res.header("X-App", "Vix");
});

// Correct
app.use([](Request &, Response &res, App::Next next) {
  res.header("X-App", "Vix");
  next();
});

// Calling next() after error
// Wrong — still calls next()
if (!authorized) {
  respond_error(res, 401, "...", "...");
}
next();

// Correct
if (!authorized) {
  respond_error(res, 401, "...", "...");
  return;
}
next();

// Wrong CORS origin

// That's the API, not the frontend
cors_dev({"http://localhost:8080"});

// Correct
cors_dev({"http://localhost:5173"});
```

## What you should remember

Call `next()` when the request should continue. Return without `next()` when you've already sent the response.

The core idea: **middleware keeps repeated route behavior in one place.**

Next: [Authentication](/examples/auth)
