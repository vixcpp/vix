# Middleware

In the previous chapter, you built a JSON API.
Now you will learn middleware.

Middleware is code that runs around your routes.
It can inspect the request before the route handler runs, and modify the response before it is sent.

```txt
request → middleware → route handler → response
```

## Why middleware exists

Without middleware, shared logic like CORS, rate limiting, authentication, and security headers must be repeated in every route. Middleware lets you write common behavior once.

## Public headers

```cpp
#include <vix.hpp>
#include <vix/middleware.hpp>
```

## Middleware order

Order matters. Configure middleware before registering routes:

```cpp
int main()
{
  App app;

  configure_middlewares(app);
  register_routes(app);

  app.run(8080);
  return 0;
}
```

A common order:

```txt
CORS → rate limit → security headers → body limit → authentication → routes
```

## CORS middleware

```cpp
app.use(vix::middleware::app::cors_dev({
    "http://localhost:5173",
    "http://127.0.0.1:5173"
}));
```

CORS is not authentication. It is a browser rule that answers: which browser origins are allowed to call this API?

For production, allow only your real frontend domain. Do not use open CORS unless the API is intentionally public.

## Rate limiting middleware

```cpp
app.use(vix::middleware::app::rate_limit({
    .max_requests = 60,
    .window_seconds = 60
}));
```

Use rate limiting to protect public APIs, login endpoints, and small VPS deployments. When the limit is exceeded, the API returns `429 Too Many Requests`.

### Test rate limiting

```cpp
app.use(vix::middleware::app::rate_limit({
    .max_requests = 5,
    .window_seconds = 30
}));
```

```bash
for i in $(seq 1 10); do
  curl -s -o /dev/null -w "%{http_code}\n" http://127.0.0.1:8080/api/data
done
```

## Stricter middleware for auth routes

```cpp
vix::middleware::app::use_on_prefix(
    app,
    "/auth",
    vix::middleware::app::rate_limit({
        .max_requests = 5,
        .window_seconds = 60
    }));
```

## Static files middleware

```cpp
#include <vix/middleware/app/adapter.hpp>
#include <vix/middleware/performance/static_files.hpp>

app.use(vix::middleware::app::adapt_ctx(
  vix::middleware::performance::static_files(
      std::filesystem::path{"public"},
      {
          .mount = "/",
          .index_file = "index.html",
          .add_cache_control = true,
          .cache_control = "public, max-age=3600",
          .fallthrough = true,
      }
  )
));
```

## Manual auth check in a route

```cpp
app.get("/api/private", [](Request &req, Response &res){

  const std::string auth = req.header("Authorization");
  if (auth != "Bearer dev-token")
  {
    res.status(401).json({
      "ok", false,
      "error", "unauthorized"
    });

    return;
  }

  res.json({
    "ok", true,
    "message", "private data"
  });

});
```

## Complete example

```cpp
#include <vix.hpp>
#include <vix/middleware.hpp>
using namespace vix;

static void respond_error(Response &res, int status, const std::string &message)
{
  res.status(status).json({"ok", false, "error", message});
}

static void configure_middlewares(App &app)
{
  app.use(vix::middleware::app::cors_dev({
      "http://localhost:5173",
      "http://127.0.0.1:5173"
  }));

  app.use(vix::middleware::app::rate_limit({
      .max_requests = 60,
      .window_seconds = 60
  }));

  vix::middleware::app::use_on_prefix(
      app,
      "/auth",
      vix::middleware::app::rate_limit({
          .max_requests = 5,
          .window_seconds = 60
      })
  );
}

static void public_routes(App &app)
{
  app.get("/", [](Request &, Response &res){
    res.json({"message", "Vix middleware example"});
  });

  app.get("/health", [](Request &, Response &res){
    res.json({
      "ok", true,
      "service", "middleware-example"
    });
  });
}

static void api_routes(App &app)
{
  app.get("/api/data", [](Request &, Response &res){
    res.json({
      "ok", true,
      "data", vix::json::o("name", "Vix.cpp"),
      "type", "runtime"
    });
  });

  app.get("/api/private", [](Request &req, Response &res){
    if (req.header("Authorization") != "Bearer dev-token"){
      respond_error(res, 401, "unauthorized");
      return;
    }
    res.json({"ok", true, "message", "private data"});
  });
}

static void auth_routes(App &app)
{
  app.post("/auth/login", [](Request &req, Response &res)
           {
             const auto &body = req.json();
             if (!body.is_object()) { respond_error(res, 400, "expected JSON object body"); return; }
             const std::string email = body.value("email", "");
             const std::string password = body.value("password", "");
             if (email != "ada@example.com" || password != "password123")
             {
               respond_error(res, 401, "invalid credentials");
               return;
             }
             res.json({"ok", true, "token", "dev-token"});
           });
}

int main()
{
  App app;

  configure_middlewares(app);
  public_routes(app);
  api_routes(app);
  auth_routes(app);

  app.run(8080);
  return 0;
}
```

## Test

```bash
curl -i http://127.0.0.1:8080/health
curl -i http://127.0.0.1:8080/api/data
curl -i http://127.0.0.1:8080/api/private
curl -i http://127.0.0.1:8080/api/private -H "Authorization: Bearer dev-token"
curl -i -X POST http://127.0.0.1:8080/auth/login \
  -H "Content-Type: application/json" \
  -d '{"email":"ada@example.com","password":"password123"}'

# Test CORS
curl -i http://127.0.0.1:8080/api/data -H "Origin: http://localhost:5173"
```

## Middleware and route responsibility

| Middleware       | Example route action          |
|------------------|-------------------------------|
| CORS             | Allows browser API calls.     |
| Rate limiting    | Protects public endpoints.    |
| Authentication   | Guards dashboard routes.      |
| Request IDs      | Tracks each request in logs.  |
| Logging          | Records request activity.     |
| Body limits      | Rejects oversized requests.   |
## Common mistakes

### Registering middleware after routes

```cpp
// Wrong
register_routes(app);
configure_middlewares(app);

// Correct
configure_middlewares(app);
register_routes(app);
```

### Making CORS too open in production

Development CORS can allow localhost.
Production should allow your real frontend only.

### Using one rate limit for everything

Login needs a stricter limit than normal API routes.

### Returning 403 instead of 429 for rate limits

Rate limit failures should return `429 Too Many Requests`.

## What you should remember

Middleware wraps route handlers. Use it for shared behavior:
CORS, rate limiting, authentication, logging, security, body limits, static files.

```cpp
int main()
{
  App app;

  configure_middlewares(app);
  register_routes(app);

  app.run(8080);
}
```

The core idea:
middleware keeps route handlers clean by moving shared request behavior into one reusable layer.

## Next chapter

[Next: Validation](/book/12-validation)
