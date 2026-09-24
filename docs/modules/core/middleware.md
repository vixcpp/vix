# Middleware

This page shows how middleware works in Vix Core.

Use it when you want to run logic before route handlers, protect routes, attach middleware to prefixes, share request-scoped data, or stop a request early.

## Public header

```cpp
#include <vix.hpp>
```

You can also include the app header directly:

```cpp
#include <vix/app/App.hpp>
```

## What middleware provides

Middleware is code that runs before the final route handler.

It can be used for:

- logging
- authentication
- authorization
- request validation
- CORS
- rate limiting
- request state injection
- route protection
- early error responses
- shared logic across many routes

Middleware receives:

```cpp
vix::Request &req
vix::Response &res
vix::App::Next next
```

Call `next()` to continue.

Do not call `next()` if the middleware already sent a response.

## Basic middleware

```cpp
#include <vix.hpp>

int main()
{
  vix::App app;

  app.use([](vix::Request &req, vix::Response &res, vix::App::Next next)
  {
    (void)res;

    vix::print("request:", req.method(), req.path());

    next();
  });

  app.get("/", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.text("Home");
  });

  app.run(8080);

  return 0;
}
```

Run:

```bash
vix run main.cpp
```

Request:

```text
GET /
```

Example response:

```text
Home
```

Console output:

```text
request: GET /
```

## Middleware signature

The middleware signature is:

```cpp
void(vix::Request &req, vix::Response &res, vix::App::Next next)
```

Example:

```cpp
app.use([](vix::Request &req, vix::Response &res, vix::App::Next next)
{
  (void)req;
  (void)res;

  next();
});
```

`req` is the incoming request.

`res` is the outgoing response helper.

`next` continues to the next middleware or final route handler.

## Continue to the next middleware

Call `next()` when the request should continue.

```cpp
app.use([](vix::Request &req, vix::Response &res, vix::App::Next next)
{
  (void)req;
  (void)res;

  next();
});
```

The flow is:

```text
middleware
  -> next
  -> route handler
```

## Stop early

A middleware can stop the request by writing a response and not calling `next()`.

```cpp
app.use([](vix::Request &req, vix::Response &res, vix::App::Next next)
{
  if (!req.has_header("Authorization"))
  {
    res.status(401).json({
      "error", "missing authorization header"
    });

    return;
  }

  next();
});
```

The flow is:

```text
middleware
  -> response
  -> stop
```

The route handler does not run.

## Global middleware

Use `app.use(middleware)` to register middleware for all routes.

```cpp
app.use([](vix::Request &req, vix::Response &res, vix::App::Next next)
{
  vix::print(req.method(), req.path());

  next();
});
```

This middleware applies to every route.

```text
/
 /api/status
 /admin/dashboard
```

## Prefix middleware

Use `app.use(prefix, middleware)` to register middleware for a route prefix.

```cpp
app.use("/admin", [](vix::Request &req, vix::Response &res, vix::App::Next next)
{
  const std::string token = req.header("Authorization");

  if (token.empty())
  {
    res.status(401).json({
      "error", "missing authorization header"
    });
    return;
  }

  next();
});
```

This applies to:

```text
/admin
/admin/users
/admin/settings
```

It does not apply to:

```text
/
 /api
 /users
```

## Prefix matching

A prefix middleware matches the exact prefix or a path under that prefix.

For prefix:

```text
/admin
```

Matched paths:

```text
/admin
/admin/users
/admin/settings
```

Not matched:

```text
/adminx
/api/admin
```

This prevents accidental matches with unrelated paths.

## Protect a prefix

Use `protect(...)` when the middleware is used for route protection.

```cpp
app.protect("/admin", [](vix::Request &req, vix::Response &res, vix::App::Next next)
{
  const bool allowed = req.has_header("Authorization");

  if (!allowed)
  {
    res.status(401).json({
      "error", "unauthorized"
    });
    return;
  }

  next();
});
```

`protect(...)` is a clearer alias for prefix middleware.

## Protect an exact path

Use `protect_exact(...)` when only one path should be protected.

```cpp
app.protect_exact("/admin/settings", [](vix::Request &req, vix::Response &res, vix::App::Next next)
{
  (void)req;

  const bool allowed = true;

  if (!allowed)
  {
    res.status(403).text("Forbidden");
    return;
  }

  next();
});
```

This applies only to:

```text
/admin/settings
```

It does not apply to:

```text
/admin
/admin/settings/profile
```

## Middleware order

Middleware runs in registration order.

```cpp
app.use([](vix::Request &req, vix::Response &res, vix::App::Next next)
{
  (void)req;
  (void)res;

  vix::print("first");

  next();
});

app.use([](vix::Request &req, vix::Response &res, vix::App::Next next)
{
  (void)req;
  (void)res;

  vix::print("second");

  next();
});
```

For a matched route, the flow is:

```text
first middleware
  -> second middleware
  -> route handler
```

## Global before prefix middleware

Vix collects global middleware first, then matching prefix middleware.

```cpp
app.use(global_middleware);
app.use("/admin", admin_middleware);
```

For:

```text
GET /admin/dashboard
```

The flow is:

```text
global_middleware
  -> admin_middleware
  -> route handler
```

## Middleware and handlers

Middleware wraps the final route handler.

```cpp
app.use([](vix::Request &req, vix::Response &res, vix::App::Next next)
{
  vix::print("before");

  next();

  vix::print("after");
});

app.get("/", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.text("Home");
});
```

The flow is:

```text
before
  -> route handler
after
```

This lets middleware run logic before and after the handler.

## Logging middleware

```cpp
#include <vix.hpp>

int main()
{
  vix::App app;

  app.use([](vix::Request &req, vix::Response &res, vix::App::Next next)
  {
    (void)res;

    vix::print(req.method(), req.path());

    next();
  });

  app.get("/", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.text("Home");
  });

  app.run(8080);

  return 0;
}
```

## Authentication middleware

```cpp
#include <vix.hpp>

int main()
{
  vix::App app;

  app.protect("/api/private", [](vix::Request &req, vix::Response &res, vix::App::Next next)
  {
    const std::string token = req.header("Authorization");

    if (token.empty())
    {
      res.status(401).json({
        "error", "missing authorization header"
      });
      return;
    }

    next();
  });

  app.get("/api/private/me", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.json({
      "id", "42",
      "name", "Ada"
    });
  });

  app.run(8080);

  return 0;
}
```

Request without authorization:

```text
GET /api/private/me
```

Example response:

```json
{
  "error": "missing authorization header"
}
```

## Request validation middleware

```cpp
app.use("/api", [](vix::Request &req, vix::Response &res, vix::App::Next next)
{
  const std::string content_type = req.header("Content-Type");

  if (req.method() == "POST" && content_type.empty())
  {
    res.status(400).json({
      "error", "missing content type"
    });
    return;
  }

  next();
});
```

## CORS middleware

```cpp
app.use([](vix::Request &req, vix::Response &res, vix::App::Next next)
{
  res.header("Access-Control-Allow-Origin", "*");
  res.header("Access-Control-Allow-Methods", "GET, POST, PUT, PATCH, DELETE, OPTIONS");
  res.header("Access-Control-Allow-Headers", "Content-Type, Authorization");

  if (req.method() == "OPTIONS")
  {
    res.status(204).send();
    return;
  }

  next();
});
```

## Request state

Middleware can attach request-scoped data to `RequestState`.

```cpp
struct CurrentUser
{
  std::string id;
};

app.use([](vix::Request &req, vix::Response &res, vix::App::Next next)
{
  (void)res;

  req.state().set(CurrentUser{"42"});

  next();
});
```

A handler can read the value later.

```cpp
app.get("/me", [](vix::Request &req, vix::Response &res)
{
  const auto &user = req.state().get<CurrentUser>();

  res.json({
    "id", user.id
  });
});
```

## Optional request state

Use `try_get` when the value may not exist.

```cpp
app.get("/me", [](vix::Request &req, vix::Response &res)
{
  auto *user = req.state().try_get<CurrentUser>();

  if (!user)
  {
    res.status(401).json({
      "error", "unauthorized"
    });
    return;
  }

  res.json({
    "id", user->id
  });
});
```

## Middleware with route groups

Groups can register middleware for their prefix.

```cpp
#include <vix.hpp>

int main()
{
  vix::App app;

  app.group("/admin", [](auto &admin)
  {
    admin.use([](vix::Request &req, vix::Response &res, vix::App::Next next)
    {
      const bool allowed = req.has_header("Authorization");

      if (!allowed)
      {
        res.status(401).json({
          "error", "unauthorized"
        });
        return;
      }

      next();
    });

    admin.get("/dashboard", [](vix::Request &req, vix::Response &res)
    {
      (void)req;

      res.text("Admin dashboard");
    });
  });

  app.run(8080);

  return 0;
}
```

The middleware applies to:

```text
/admin/dashboard
```

## Nested group middleware

```cpp
app.group("/api", [](auto &api)
{
  api.use([](vix::Request &req, vix::Response &res, vix::App::Next next)
  {
    res.header("X-API", "Vix");
    next();
  });

  api.group("/v1", [](auto &v1)
  {
    v1.get("/status", [](vix::Request &req, vix::Response &res)
    {
      (void)req;

      res.json({
        "status", "ok"
      });
    });
  });
});
```

The route becomes:

```text
GET /api/v1/status
```

The middleware is registered for:

```text
/api
```

So it applies to the nested route.

## Stop before the handler

Middleware can prevent the final route handler from running.

```cpp
app.use("/locked", [](vix::Request &req, vix::Response &res, vix::App::Next next)
{
  (void)req;
  (void)next;

  res.status(423).json({
    "error", "locked"
  });
});

app.get("/locked/page", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.text("this handler will not run");
});
```

## Continue conditionally

```cpp
app.use("/api", [](vix::Request &req, vix::Response &res, vix::App::Next next)
{
  if (req.header("X-Disabled") == "1")
  {
    res.status(403).json({
      "error", "disabled"
    });
    return;
  }

  next();
});
```

## Middleware and response headers

Middleware can add headers to every response.

```cpp
app.use([](vix::Request &req, vix::Response &res, vix::App::Next next)
{
  (void)req;

  res.header("X-Powered-By", "Vix.cpp");

  next();
});
```

## Middleware and timing

Middleware can measure request duration.

```cpp
#include <chrono>

app.use([](vix::Request &req, vix::Response &res, vix::App::Next next)
{
  (void)req;

  const auto start = std::chrono::steady_clock::now();

  next();

  const auto end = std::chrono::steady_clock::now();
  const auto ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

  res.header("X-Response-Time-Ms", std::to_string(ms));
});
```

## Middleware chain model

The middleware chain is recursive.

```text
middleware[0]
  -> next
    -> middleware[1]
      -> next
        -> middleware[2]
          -> next
            -> final handler
```

A middleware decides whether the chain continues.

```text
call next
  -> continue

do not call next
  -> stop
```

## When middleware is collected

When a route is registered, Vix collects middleware that applies to the route path.

```text
App::add_route
  -> collect matching middleware
  -> wrap final handler
  -> register handler in Router
```

This means middleware should normally be registered before the routes that need it.

Recommended:

```cpp
app.use("/admin", admin_middleware);
app.get("/admin/dashboard", dashboard_handler);
```

Avoid:

```cpp
app.get("/admin/dashboard", dashboard_handler);
app.use("/admin", admin_middleware);
```

## Middleware with static files

Static files are served through the application not-found fallback.

Middleware registered for a prefix can still be useful when static files are mounted under that prefix.

```cpp
app.use("/assets", [](vix::Request &req, vix::Response &res, vix::App::Next next)
{
  res.header("X-Static", "true");

  next();
});

app.static_dir("public/assets", "/assets");
```

## Complete example

```cpp
#include <vix.hpp>

#include <chrono>
#include <string>

struct CurrentUser
{
  std::string id;
};

int main()
{
  vix::App app;

  app.use([](vix::Request &req, vix::Response &res, vix::App::Next next)
  {
    const auto start = std::chrono::steady_clock::now();

    next();

    const auto end = std::chrono::steady_clock::now();
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    res.header("X-Response-Time-Ms", std::to_string(ms));

    vix::print(req.method(), req.path(), ms);
  });

  app.protect("/api/private", [](vix::Request &req, vix::Response &res, vix::App::Next next)
  {
    const std::string token = req.header("Authorization");

    if (token.empty())
    {
      res.status(401).json({
        "error", "missing authorization header"
      });
      return;
    }

    req.state().set(CurrentUser{"42"});

    next();
  });

  app.get("/", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.text("Home");
  });

  app.get("/api/private/me", [](vix::Request &req, vix::Response &res)
  {
    const auto &user = req.state().get<CurrentUser>();

    res.json({
      "id", user.id
    });
  });

  app.run(8080);

  return 0;
}
```

## API summary

| API                                      | Purpose                                           |
| ---------------------------------------- | ------------------------------------------------- |
| `app.use(middleware)`                    | Register global middleware.                       |
| `app.use(prefix, middleware)`            | Register prefix middleware.                       |
| `app.protect(prefix, middleware)`        | Protect a route prefix.                           |
| `app.protect_exact(path, middleware)`    | Protect one exact path.                           |
| `Group::use(middleware)`                 | Register middleware for a group prefix.           |
| `Group::protect(prefix, middleware)`     | Protect a group sub-prefix.                       |
| `Group::protect_exact(path, middleware)` | Protect one exact group path.                     |
| `vix::App::Next`                         | Continue to the next middleware or final handler. |
| `req.state().set<T>(value)`              | Store request-scoped data.                        |
| `req.state().get<T>()`                   | Read required request-scoped data.                |
| `req.state().try_get<T>()`               | Read optional request-scoped data.                |

## Best practices

Register middleware before the routes that need it.

```cpp
app.use("/admin", auth_middleware);
app.get("/admin/dashboard", dashboard_handler);
```

Keep middleware focused on one responsibility.

```cpp
logging_middleware
auth_middleware
cors_middleware
```

Always call `next()` only when the request should continue.

```cpp
if (!allowed)
{
  res.status(403).json({"error", "forbidden"});
  return;
}

next();
```

Use request state to pass data from middleware to handlers.

```cpp
req.state().set(CurrentUser{"42"});
```

Use prefix middleware for sections of your app.

```cpp
app.use("/api", api_middleware);
app.use("/admin", admin_middleware);
```

## Next steps

Read the next pages:

- [Request](./request.md)
- [Response](./response.md)
- [Handlers](./handlers.md)
- [Routing](./routing.md)
- [Static files](./static-files.md)
