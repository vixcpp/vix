# Routes

Routes define the public HTTP surface of a Vix.cpp application.
A route connects an HTTP method and a path pattern to C++ code:

```txt
GET /users/42 -> app.get("/users/{id}", handler)
```

In a backend application, routes are not just small examples. They describe how clients interact with the application. They should be clear, predictable, and easy to organize as the project grows.

## Route anatomy

A minimal route looks like this:

```cpp
app.get("/", [](Request &req, Response &res) {
  res.text("Hello");
});
```

| Part            | Purpose                                      |
| --------------- | -------------------------------------------- |
| `app.get`       | Registers a route for the HTTP `GET` method. |
| `"/"`           | Defines the path pattern.                    |
| Lambda          | Handles the request.                         |
| `Request &req`  | Represents the incoming request.             |
| `Response &res` | Builds and sends the response.               |

The handler receives the request and writes the response.

## HTTP methods

Use the HTTP method that matches the intent of the operation.

```cpp
app.get("/users", list_handler);
app.post("/users", create_handler);
app.put("/users/{id}", replace_handler);
app.patch("/users/{id}", update_handler);
app.del("/users/{id}", delete_handler);
```

A common API convention is:

| Method   | Typical use                             |
| -------- | --------------------------------------- |
| `GET`    | Read data.                              |
| `POST`   | Create a resource or perform an action. |
| `PUT`    | Replace a resource.                     |
| `PATCH`  | Update part of a resource.              |
| `DELETE` | Remove a resource.                      |

Use consistent method semantics. It makes APIs easier to test, document, and maintain.

## Path parameters

Path parameters identify values inside the URL path.

```cpp
app.get("/users/{id}", [](Request &req, Response &res) {
  const std::string id = req.param("id");

  res.json({
    "ok", true,
    "id", id
  });
});
```

This route matches:

```txt
/users/1
/users/42
/users/abc
```

The value is read with:

```cpp
req.param("id")
```

## Multiple path parameters

A route can contain more than one path parameter.

```cpp
app.get("/posts/{year}/{slug}", [](Request &req, Response &res) {
  const std::string year = req.param("year");
  const std::string slug = req.param("slug");

  res.json({
    "ok", true,
    "year", year,
    "slug", slug
  });
});
```

This route matches paths such as:

```txt
/posts/2026/vix-release-notes
```

Path parameters should describe the identity of the resource being accessed.

## Query parameters

Query parameters are not part of the route pattern. They appear after `?`.

```cpp
app.get("/users", [](Request &req, Response &res) {
  const std::string page = req.query_value("page", "1");
  const std::string limit = req.query_value("limit", "20");

  res.json({
    "ok", true,
    "page", page,
    "limit", limit
  });
});
```

Request:

```bash
curl -i "http://127.0.0.1:8080/users?page=2&limit=10"
```

Here:

```cpp
req.query_value("page", "1")
```

returns the `page` query parameter. If `page` is missing, it returns `"1"`.

## Path parameters vs query parameters

Use path parameters for resource identity.

Use query parameters for options, filters, sorting, pagination, and optional modifiers.

```txt
Path parameter:
  /users/42

Query parameters:
  /users?page=2&limit=10
  /users?sort=name
  /users?active=true
```

A good rule is:

```txt
Path params identify what you want.
Query params describe how you want to read it.
```

## Error responses

When a route sends an error response, return immediately.

```cpp
app.get("/users/{id}", [](Request &req, Response &res) {
  const std::string id = req.param("id");

  if (id == "0") {
    res.status(404).json({
      "ok", false,
      "error", "user_not_found",
      "message", "User was not found"
    });

    return;
  }

  res.json({
    "ok", true,
    "id", id
  });
});
```

The `return` matters. Without it, the handler may continue and try to send another response.

## Response shape

A backend should use predictable response shapes.

Success:

```json
{
  "ok": true,
  "data": {}
}
```

List:

```json
{
  "ok": true,
  "count": 2,
  "data": []
}
```

Error:

```json
{
  "ok": false,
  "error": "validation_failed",
  "message": "email is required"
}
```

This consistency helps clients, tests, logs, generated documentation, and language models that read public examples from the documentation.

## Organizing routes by feature

Small examples can keep routes in `main.cpp`.

Real applications should group routes by feature.

```cpp
static void public_routes(App &app) { /* / and /health */ }
static void user_routes(App &app) { /* /users */ }
static void auth_routes(App &app) { /* /auth/login */ }
static void admin_routes(App &app) { /* /admin */ }

int main()
{
  App app;

  public_routes(app);
  user_routes(app);
  auth_routes(app);
  admin_routes(app);

  app.run();

  return 0;
}
```

This keeps `main()` readable and makes the API easier to maintain.

## Route order matters

Routes are matched in registration order.

Register specific routes before generic routes.

```cpp
app.get("/users/search", search_handler);
app.get("/users/{id}", user_handler);
app.get("/*", fallback_handler);
```

Avoid this order:

```cpp
app.get("/*", fallback_handler);
app.get("/users/{id}", user_handler);
```

A wildcard route registered too early can catch requests before more specific routes get a chance to run.

## Wildcard routes

A wildcard route matches many paths.

```cpp
app.get("/*", [](Request &req, Response &res) {
  res.json({
    "ok", true,
    "path", req.path()
  });
});
```

Wildcard routes are useful for:

- custom 404 behavior
- static file fallback
- SPA fallback

Use them carefully and register them after specific routes.

## Static file fallback

A simple static file fallback can serve files from `public/`.

```cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/*", [](Request &req, Response &res) {
    const std::string path = "public" + req.path();

    res.header("Cache-Control", "public, max-age=86400");
    res.file(path);
  });

  app.run();

  return 0;
}
```

Project structure:

```txt
project/
├── main.cpp
└── public/
    └── index.html
```

Request:

```txt
/index.html
```

Serves:

```txt
public/index.html
```

Run:

```bash
vix run main.cpp
curl http://localhost:8080/index.html
```

For production applications, validate paths carefully and avoid exposing files that should not be public.

## SPA fallback

A single-page application often needs API routes first, then a frontend fallback.

```cpp
app.get("/api/users", users_handler);

app.static_dir("public");

app.get("/*", [](Request &, Response &res) {
  res.file("public/index.html");
});
```

The order matters:

```txt
API routes first
static files second
SPA fallback last
```

This prevents frontend fallback behavior from hiding API routes.

## Split routes into files

As the application grows, move route groups into separate files.

Header:

```cpp
#pragma once

#include <vix.hpp>

void public_routes(vix::App &app);
```

Source:

```cpp
#include "PublicRoutes.hpp"

void public_routes(vix::App &app)
{
  app.get("/", [](vix::Request &, vix::Response &res) {
    res.json({
      "ok", true,
      "message", "Hello"
    });
  });
}
```

With CMake, include the new source file in the target:

```cmake
add_executable(app
  src/main.cpp
  src/routes/PublicRoutes.cpp
)
```

With `vix.app`, add the file to `sources`:

```txt
sources = [
  src/main.cpp,
  src/routes/PublicRoutes.cpp,
]
```

## Complete example

```cpp
#include <vix.hpp>

using namespace vix;

static void public_routes(App &app)
{
  app.get("/", [](Request &, Response &res) {
    res.json({
      "ok", true,
      "message", "Vix routes example"
    });
  });

  app.get("/health", [](Request &, Response &res) {
    res.json({
      "ok", true,
      "service", "routes-example"
    });
  });
}

static void user_routes(App &app)
{
  app.get("/users", [](Request &req, Response &res) {
    res.json({
      "ok", true,
      "page", req.query_value("page", "1"),
      "items", vix::json::array({"Alice", "Bob"})
    });
  });

  app.get("/users/{id}", [](Request &req, Response &res) {
    const std::string id = req.param("id");

    if (id == "0") {
      res.status(404).json({
        "ok", false,
        "error", "user_not_found",
        "message", "User was not found"
      });

      return;
    }

    res.json({
      "ok", true,
      "user", vix::json::o("id", id, "name", "Ada")
    });
  });

  app.post("/users", [](Request &req, Response &res) {
    res.status(201).json({
      "ok", true,
      "body", req.json()
    });
  });
}

int main()
{
  App app;

  public_routes(app);
  user_routes(app);

  app.run();

  return 0;
}
```

Run it:

```bash
vix run main.cpp
```

Test it:

```bash
curl -i http://127.0.0.1:8080/
curl -i http://127.0.0.1:8080/health
curl -i "http://127.0.0.1:8080/users?page=2"
curl -i http://127.0.0.1:8080/users/42
```

## Common mistakes

### Missing leading slash

Wrong:

```cpp
app.get("health", handler);
```

Correct:

```cpp
app.get("/health", handler);
```

### Confusing path parameters and query parameters

Path parameter:

```txt
/users/{id}
```

Read it with:

```cpp
req.param("id")
```

Query parameter:

```txt
/users?page=2
```

Read it with:

```cpp
req.query_value("page", "1")
```

### Forgetting to return after an error

```cpp
if (bad) {
  res.status(400).json({
    "ok", false,
    "error", "bad_request"
  });

  return;
}
```

Always return after sending an error response.

### Registering a wildcard route too early

Specific routes should come first.

```cpp
app.get("/users/search", search_handler);
app.get("/users/{id}", user_handler);
app.get("/*", fallback_handler);
```

Register wildcard routes last.

## What you should remember

A route connects an HTTP method and a path pattern to a C++ handler.

Use path parameters for resource identity.

Use query parameters for optional read behavior such as pagination, sorting, and filters.

Group routes by feature as the application grows.

Register specific routes before wildcard routes.

Keep route behavior predictable because routes define the public API shape of the application.

## Next chapter

[Next: Request and Response](/book/05-request-response)
