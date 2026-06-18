# Routing

This page shows how routing works in Vix Core.

Use it when you want to register routes, use route parameters, group routes, handle `OPTIONS`, customize not-found behavior, and understand how the router matches requests.

## Public header

```cpp
#include <vix.hpp>
```

You can also include the router header directly:

```cpp
#include <vix/router/Router.hpp>
```

## What routing provides

Vix Core routing connects an HTTP method and path to a handler.

It provides:

- method-based routes
- static paths
- route parameters
- route groups
- automatic `OPTIONS` routes
- `HEAD` fallback to `GET`
- custom not-found handling
- route metadata
- heavy route classification

Most applications register routes through `vix::App`.

```cpp
vix::App app;

app.get("/", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.text("Hello from Vix");
});
```

## Basic route

```cpp
#include <vix.hpp>

int main()
{
  vix::App app;

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

Then open:

```text
http://localhost:8080
```

Expected response:

```text
Home
```

## HTTP methods

`App` provides helpers for common HTTP methods.

```cpp
app.get("/users", handler);
app.post("/users", handler);
app.put("/users/{id}", handler);
app.patch("/users/{id}", handler);
app.del("/users/{id}", handler);
app.head("/users", handler);
app.options("/users", handler);
```

Example:

```cpp
#include <vix.hpp>

int main()
{
  vix::App app;

  app.get("/users", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.json({
      "method", "GET",
      "route", "/users"
    });
  });

  app.post("/users", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.status(201).json({
      "method", "POST",
      "created", true
    });
  });

  app.run(8080);

  return 0;
}
```

## Route handlers

A route handler receives:

```cpp
vix::Request &req
vix::Response &res
```

Example:

```cpp
app.get("/status", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.json({
    "status", "ok"
  });
});
```

The handler reads from the request and writes to the response.

## Static paths

A static route matches an exact path.

```cpp
app.get("/about", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.text("About");
});
```

This matches:

```text
GET /about
```

It does not match:

```text
GET /about/team
```

## Route parameters

Use `{name}` to capture a path segment.

```cpp
app.get("/users/{id}", [](vix::Request &req, vix::Response &res)
{
  const std::string id = req.param("id");

  res.json({
    "id", id
  });
});
```

Request:

```text
GET /users/42
```

Example response:

```json
{
  "id": "42"
}
```

## Multiple parameters

A route can contain several parameters.

```cpp
app.get("/users/{user_id}/posts/{post_id}", [](vix::Request &req, vix::Response &res)
{
  res.json({
    "user_id", req.param("user_id"),
    "post_id", req.param("post_id")
  });
});
```

Request:

```text
GET /users/7/posts/42
```

Example response:

```json
{
  "user_id": "7",
  "post_id": "42"
}
```

## Query string

The router matches the path without the query string.

```text
GET /users/42?page=2
```

matches:

```cpp
app.get("/users/{id}", handler);
```

You can read query values from the request.

```cpp
app.get("/users/{id}", [](vix::Request &req, vix::Response &res)
{
  res.json({
    "id", req.param("id"),
    "page", req.query_value("page", "1")
  });
});
```

## Path normalization

Routes are normalized before they are stored.

These paths are treated cleanly:

```cpp
app.get("users", handler);
app.get("/users", handler);
app.get("/users/", handler);
```

The normalized route path is:

```text
/users
```

## Method normalization

HTTP methods are normalized to uppercase.

```cpp
app.get("/status", handler);
```

internally registers:

```text
GET /status
```

The router also normalizes request methods before matching.

## Route groups

Use `group(...)` to register many routes under the same prefix.

```cpp
#include <vix.hpp>

int main()
{
  vix::App app;

  app.group("/api", [](auto &api)
  {
    api.get("/status", [](vix::Request &req, vix::Response &res)
    {
      (void)req;

      res.json({"status", "ok"});
    });

    api.get("/version", [](vix::Request &req, vix::Response &res)
    {
      (void)req;

      res.json({"version", "1.0.0"});
    });
  });

  app.run(8080);

  return 0;
}
```

The routes become:

```text
GET /api/status
GET /api/version
```

## Nested route groups

Groups can be nested.

```cpp
app.group("/api", [](auto &api)
{
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

## Reusable group object

You can create a group object and reuse it.

```cpp
auto api = app.group("/api");

api.get("/status", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.json({"status", "ok"});
});

api.post("/users", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.status(201).json({"created", true});
});
```

## Group middleware

A group can register middleware for its prefix.

```cpp
app.group("/admin", [](auto &admin)
{
  admin.use([](vix::Request &req, vix::Response &res, vix::App::Next next)
  {
    const bool allowed = true;

    if (!allowed)
    {
      res.status(403).text("Forbidden");
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
```

The middleware applies to routes under:

```text
/admin
```

## Protect a route prefix

Use `protect(...)` to attach middleware to a route prefix.

```cpp
app.protect("/admin", [](vix::Request &req, vix::Response &res, vix::App::Next next)
{
  const std::string auth = req.header("Authorization");

  if (auth.empty())
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

## Protect an exact path

Use `protect_exact(...)` when the middleware should only run for one exact path.

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

## HEAD routes

You can register a `HEAD` route directly.

```cpp
app.head("/status", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.status(200).send();
});
```

If a `HEAD` route is not found, the router can fall back to the matching `GET` route.

```cpp
app.get("/status", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.json({"status", "ok"});
});
```

A `HEAD /status` request can reuse the `GET /status` handler, then finalize the response without a body.

## OPTIONS routes

You can register an `OPTIONS` route manually.

```cpp
app.options("/users", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.status(204).send();
});
```

When you register a normal route through `App`, Vix can also ensure an `OPTIONS` route exists for that path.

This helps clients and tooling perform preflight or capability checks.

## Automatic 404

If no route matches, Vix returns a default not-found response.

Example response shape:

```json
{
  "error": "Route not found",
  "hint": "Check path, method, or API version",
  "method": "GET",
  "path": "/missing"
}
```

The application-level not-found handler can also try static files before returning the default 404.

## Static files as fallback

Static files are integrated with routing through the not-found path.

```cpp
app.static_dir("public", "/assets");
```

If no explicit route matches:

```text
GET /assets/app.css
```

Vix can try to serve:

```text
public/app.css
```

If the file is not found and fallthrough is enabled, routing continues to the default not-found response.

## Heavy routes

Use heavy route helpers for expensive routes.

```cpp
app.get_heavy("/reports", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.json({
    "report", "ready"
  });
});
```

For POST:

```cpp
app.post_heavy("/reports", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.status(202).json({
    "accepted", true
  });
});
```

Heavy routes are stored with route metadata.

This lets Vix classify routes that may be CPU-heavy, database-heavy, or more expensive than normal request handlers.

## Route metadata

The router stores metadata for registered routes.

A route record contains:

```text
method
path
heavy
doc
```

This metadata is useful for:

- documentation
- OpenAPI generation
- runtime checks
- heavy route classification
- developer tooling

## Route documentation

Routes can carry documentation metadata through `RouteDoc`.

A route doc can contain:

- summary
- description
- tags
- request body metadata
- response metadata
- OpenAPI extensions

At the app level, most users do not need to construct `RouteDoc` directly.

It is used by tooling and documentation generation.

## Internal route tree

Internally, the router stores routes in a tree.

```text
Router
  -> root RouteNode
  -> children
  -> handler
  -> parameter nodes
```

A `RouteNode` can contain:

```text
children
handler
isParam
paramName
heavy
```

Static segments are stored by their exact name.

Parameter segments such as `{id}` are stored using a parameter child.

## Matching order

For each segment, the router tries:

```text
static child
parameter child
not found
```

Example route:

```text
GET /users/{id}
```

Request:

```text
GET /users/42
```

Simplified matching:

```text
GET/users/42
  -> GET
  -> users
  -> parameter id = 42
  -> handler
```

## Router API

Advanced code can access the router.

```cpp
auto router = app.router();
```

Most application code should register routes through `App`.

Direct router access is useful for:

- tooling
- integrations
- generated routes
- documentation systems
- custom frameworks on top of Vix

## Register directly on Router

Advanced usage:

```cpp
#include <vix.hpp>
#include <memory>

int main()
{
  vix::App app;

  auto router = app.router();

  auto handler = std::make_shared<vix::http::RequestHandler<decltype(
      [](vix::Request &req, vix::Response &res)
      {
        (void)req;
        res.text("direct");
      })>>(
      "/direct",
      [](vix::Request &req, vix::Response &res)
      {
        (void)req;
        res.text("direct");
      });

  router->add_route("GET", "/direct", handler);

  app.run(8080);

  return 0;
}
```

In normal code, prefer:

```cpp
app.get("/direct", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.text("direct");
});
```

## Check if a route exists

The router can check whether a route exists.

```cpp
if (app.router()->has_route("GET", "/status"))
{
  vix::print("route exists");
}
```

This is useful for tooling and route registration checks.

## List registered routes

The router exposes the list of registered route records.

```cpp
for (const auto &route : app.router()->routes())
{
  vix::print(route.method, route.path);
}
```

This can be used by documentation tools or diagnostics.

## Routing lifecycle

The route lifecycle is:

```text
App registers route
  -> middleware chain is collected
  -> user handler is wrapped
  -> RequestHandler is created
  -> Router stores route in tree
  -> Session dispatches Request
  -> Router matches method + path
  -> RequestHandler runs
  -> Response is finalized
```

## Complete example

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

  app.get("/users/{id}", [](vix::Request &req, vix::Response &res)
  {
    res.json({
      "id", req.param("id")
    });
  });

  app.group("/api", [](auto &api)
  {
    api.get("/status", [](vix::Request &req, vix::Response &res)
    {
      (void)req;

      res.json({
        "status", "ok"
      });
    });
  });

  app.get_heavy("/reports", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.json({
      "report", "ready"
    });
  });

  app.static_dir("public", "/assets");

  app.run(8080);

  return 0;
}
```

Registered routes:

```text
GET /.
GET /users/{id}
GET /api/status
GET /reports
```

Static fallback:

```text
/assets/*
```

## API summary

| API                                   | Purpose                         |
| ------------------------------------- | ------------------------------- |
| `app.get(path, handler)`              | Register a GET route.           |
| `app.post(path, handler)`             | Register a POST route.          |
| `app.put(path, handler)`              | Register a PUT route.           |
| `app.patch(path, handler)`            | Register a PATCH route.         |
| `app.del(path, handler)`              | Register a DELETE route.        |
| `app.head(path, handler)`             | Register a HEAD route.          |
| `app.options(path, handler)`          | Register an OPTIONS route.      |
| `app.get_heavy(path, handler)`        | Register a heavy GET route.     |
| `app.post_heavy(path, handler)`       | Register a heavy POST route.    |
| `app.group(prefix, fn)`               | Register routes under a prefix. |
| `app.use(middleware)`                 | Register global middleware.     |
| `app.use(prefix, middleware)`         | Register prefix middleware.     |
| `app.protect(prefix, middleware)`     | Protect a prefix.               |
| `app.protect_exact(path, middleware)` | Protect one exact path.         |
| `app.router()`                        | Access the shared router.       |
| `router->has_route(method, path)`     | Check if a route exists.        |
| `router->routes()`                    | Read registered route metadata. |

## Best practices

Keep route handlers small.

```cpp
app.get("/status", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.json({"status", "ok"});
});
```

Use route groups for APIs.

```cpp
app.group("/api", [](auto &api)
{
  api.get("/status", handler);
});
```

Use route parameters for resource identifiers.

```cpp
app.get("/users/{id}", handler);
```

Use middleware for cross-cutting logic.

```cpp
app.use("/admin", auth_middleware);
```

Use heavy routes for costly work.

```cpp
app.get_heavy("/reports", handler);
```

## Next steps

Read the next pages:

- [Handlers](./handlers.md)
- [Middleware](./middleware.md)
- [Request](./request.md)
- [Response](./response.md)
- [Static files](./static-files.md)
