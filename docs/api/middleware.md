# Middleware API

Middleware is code that runs around your routes.

```txt
request → middleware → route handler → response
```

## Public header

```cpp
#include <vix.hpp>
#include <vix/middleware.hpp>
```

## Middleware shape

```cpp
app.use([](Request &req, Response &res, App::Next next){
  res.header("X-Powered-By", "Vix.cpp");
  next();  // continue to next middleware or route
});
```

Call `next()` to continue. Do not call `next()` if you've already sent the final response.

## Middleware order

```cpp
app.use(cors_middleware);       // 1st
app.use(rate_limit_middleware); // 2nd
app.use(auth_middleware);       // 3rd
register_routes(app);           // always configure middleware before routes
```

Recommended order: CORS → rate limit → security headers → body limit → authentication → routes.

## Global middleware

```cpp
app.use([](Request &, Response &res, App::Next next){
  res.header("X-App", "Vix");
  next();
});
```

## Prefix middleware

```cpp
app.use("/api", [](Request &, Response &res, App::Next next){
  res.header("X-API", "true");
  next();
});
```

Only applies to routes under `/api`.

## Route protection

```cpp
app.use("/admin", [](Request &req, Response &res, App::Next next){
  if (req.header("x-admin-token") != "secret")
  {
    res.status(401).json({
      "ok", false,
      "error", "unauthorized"
    });
    return;  // do not call next()
  }
  next();
});

// Convenience APIs
app.protect("/admin", middleware);               // prefix
app.protect_exact("/admin/hook", middleware);    // exact path
```

## CORS middleware

```cpp
// Development
app.use(vix::middleware::app::cors_dev({
    "http://localhost:5173",
    "http://127.0.0.1:5173"
}));

// Production with explicit options
#include <vix/middleware.hpp>

vix::middleware::security::CorsOptions options;
options.allowed_origins = {"https://app.example.com"};
options.allow_any_origin = false;
options.allow_credentials = true;
options.allow_methods = {"GET", "POST", "PUT", "PATCH", "DELETE", "OPTIONS"};
options.allow_headers = {"Content-Type", "Authorization", "Accept"};
options.expose_headers = {"X-Request-Id"};

app.use(vix::middleware::app::adapt_ctx(
    vix::middleware::security::cors(std::move(options))));
```

CORS is not authentication — it is a browser rule about which origins can call your API.

## Rate limiting middleware

```cpp
// Global limit
app.use(vix::middleware::app::rate_limit({
    .max_requests = 60,
    .window_seconds = 60
}));

// Stricter limit for auth routes
vix::middleware::app::use_on_prefix(
    app, "/auth",
    vix::middleware::app::rate_limit({
        .max_requests = 5,
        .window_seconds = 60
    }));
```

When exceeded, returns `429 Too Many Requests`.

## Static files middleware

```cpp
// Simple
app.static_dir("public");

// Advanced
#include <filesystem>
app.use(vix::middleware::app::adapt_ctx(
    vix::middleware::performance::static_files(
        std::filesystem::path{"public"},
        {
            .mount = "/",
            .index_file = "index.html",
            .add_cache_control = true,
            .cache_control = "public, max-age=3600",
            .fallthrough = true,
        })));
```

## Request logging middleware

```cpp
app.use([](Request &req, Response &, App::Next next){
  std::cout << "[request] " << req.method() << " " << req.path() << "\n";
  next();
});
```

## Security headers middleware

```cpp
app.use([](Request &, Response &res, App::Next next){
  res.header("X-Content-Type-Options", "nosniff");
  res.header("X-Frame-Options", "DENY");
  res.header("Referrer-Policy", "no-referrer");
  next();
});
```

## Middleware with groups

```cpp
auto api = app.group("/api");

api.use([](Request &, Response &res, App::Next next){
  res.header("X-API", "true"); next();
});

api.get("/health", [](Request &, Response &res) {
  res.json({"ok", true});
});
```

## Complete example

```cpp
#include <iostream>
#include <vix.hpp>
#include <vix/middleware.hpp>
using namespace vix;

static void respond_error(Response &res, int status,
                           const std::string &code, const std::string &message)
{
  res.status(status).json({
    "ok", false,
    "error", code,
    "message", message
  });
}

static void configure_middlewares(App &app)
{
  app.use(vix::middleware::app::cors_dev({
      "http://localhost:5173", "http://127.0.0.1:5173"
  }));

  app.use(vix::middleware::app::rate_limit({.max_requests = 60, .window_seconds = 60}));

  app.use([](Request &req, Response &, App::Next next){
    std::cout << req.method() << " " << req.path() << "\n"; next();
  });

  app.use([](Request &, Response &res, App::Next next){
    res.header("X-Powered-By", "Vix.cpp"); next();
  });

  app.protect("/admin", [](Request &req, Response &res, App::Next next){
    if (req.header("x-admin-token") != "secret"){
      respond_error(res, 401, "unauthorized", "Invalid admin token");
      return;
    }
    next();
  });
}

static void register_routes(App &app)
{
  app.get("/api/health", [](Request &, Response &res) {
    res.json({"ok", true});
  });

  app.get("/admin/stats", [](Request &, Response &res){
    res.json({
      "ok", true,
      "message", "private admin stats"
    });
  });
}

int main()
{
  App app;

  configure_middlewares(app);
  register_routes(app);

  app.run(8080);

  return 0;
}
```

## When to use middleware

Use middleware when a behavior applies to many routes: headers, auth checks, rate limiting, logging, CORS, static files, route group protection.

Avoid middleware when logic belongs to one route only — keep it in the route or service layer.

## Common mistakes

### Forgetting next()

```cpp
// Wrong — request is silently dropped
app.use([](Request &, Response &res, App::Next next) {
  res.header("X-App", "Vix");
});

// Correct
app.use([](Request &, Response &res, App::Next next) {
  res.header("X-App", "Vix");
  next();
});
```

### Calling next() after an error

```cpp
// Wrong
if (!authorized) {
  res.status(401).json({"ok", false});
}
next();

// Correct
if (!authorized) {
  res.status(401).json({"ok", false});
  return;
}
next();
```

### Wrong CORS origin

```cpp
// Wrong — that's the API, not the frontend
app.use(vix::middleware::app::cors_dev({"http://localhost:8080"}));

// Correct
app.use(vix::middleware::app::cors_dev({"http://localhost:5173"}));
```

## What you should remember

```txt
request → middleware → route handler → response
```

Call `next()` only when the request should continue. Use middleware for shared behavior: CORS, rate limiting, security headers, authentication, logging, static files.

Next: [Config API](/api/config)
