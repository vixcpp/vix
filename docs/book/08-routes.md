# Routes

Routes are the heart of a Vix HTTP application.
They connect an HTTP request to C++ code:

```txt
GET /users/42  →  app.get("/users/{id}", handler);
```

## Route anatomy

```cpp
app.get("/", [](Request &req, Response &res){
  res.text("Hello");
});
```

| Part | Purpose |
|------|---------|
| `app.get` | HTTP method |
| `"/"` | Path pattern |
| Lambda | Handler |
| `Request &req` | Incoming request |
| `Response &res` | Outgoing response |

## HTTP methods

```cpp
app.get("/users", list_handler);          // read
app.post("/users", create_handler);       // create
app.put("/users/{id}", replace_handler);  // replace
app.patch("/users/{id}", update_handler); // partial update
app.del("/users/{id}", delete_handler);   // delete
```

## Path parameters

```cpp
app.get("/users/{id}", [](Request &req, Response &res){
  const std::string id = req.param("id");
  res.json({"id", id});
});
```

Matches: `/users/1`, `/users/42`, `/users/abc`

### Multiple path parameters

```cpp
app.get("/posts/{year}/{slug}", [](Request &req, Response &res){
  res.json({"year", req.param("year"), "slug", req.param("slug")});
});
```

## Query parameters

Query params are NOT part of the route pattern — they come after `?`:

```cpp
app.get("/users", [](Request &req, Response &res){
  const std::string page = req.query_value("page", "1");
  const std::string limit = req.query_value("limit", "20");
  res.json({"page", page, "limit", limit});
});
```

```bash
curl -i "http://127.0.0.1:8080/users?page=2&limit=10"
```

## Path params vs query params

```txt
Path params  → identify the resource:  /users/42
Query params → modify how it's read:   /users?page=2&limit=10
```

## Error routes

```cpp
app.get("/users/{id}", [](Request &req, Response &res){
  const std::string id = req.param("id");

  if (id == "0"){
    es.status(404).json({"ok", false, "error", "user not found"});
    return;    // always return after error
  }

  res.json({"ok", true, "id", id});
});
```

## Organizing routes by feature

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

  app.run(8080);

  return 0;
}
```

## Route order matters

Routes are matched in registration order.

Specific routes should be registered before generic fallback routes.

```cpp
// Correct
app.get("/users/search", search_handler);
app.get("/users/{id}", user_handler);
app.get("/*", fallback_handler);

// Wrong — wildcard catches everything
app.get("/*", fallback_handler);
app.get("/users/{id}", user_handler);

// In the wrong version, the wildcard route can catch requests before /users/{id} gets a chance to run.
```

## Wildcard routes

A wildcard route matches many paths and is useful as a fallback.

```cpp
app.get("/*", [](Request &req, Response &res){
  res.json({"path", req.path()});
});
```

Used for:
  - static file fallback
  - SPA fallback
  - custom 404 behavior

Used for: SPA fallback, static file fallback, custom 404.

## Static file fallback

```cpp
#include <vix.hpp>
using namespace vix;

int main()
{
  App app;

  app.get("/*", [](Request &req, Response &res){
    std::string path = "public" + req.path();

    res.header("Cache-Control", "public, max-age=86400");
    res.file(path);

  });

  app.run(8080);
}
```

With this structure:

```
project/
├── main.cpp
└── public/
    └── index.html
```

A request to:

```txt
/index.html
```
serves:
```txt
public/index.html
```

Run:
```bash
vix run main.cpp
curl http://localhost:8080/index.html
```

## SPA fallback

```cpp
app.get("/api/users", users_handler);
app.static_dir("public");

app.get("/*", [](Request &, Response &res){
  res.file("public/index.html");
});
```

## Split routes into files

**Header (`src/routes/PublicRoutes.hpp`):**

```cpp
#pragma once
#include <vix.hpp>
void public_routes(vix::App &app);
```

**Source (`src/routes/PublicRoutes.cpp`):**

```cpp
#include "PublicRoutes.hpp"

void public_routes(vix::App &app)
{
  app.get("/", [](vix::Request &, vix::Response &res){
    res.json({"message", "Hello"});
  });
}
```

Update `CMakeLists.txt`:

```cmake
add_executable(app src/main.cpp src/routes/PublicRoutes.cpp)
```

## Complete example

```cpp
#include <vix.hpp>
using namespace vix;

static void public_routes(App &app)
{
  app.get("/", [](Request &, Response &res){
    res.json({"message", "Vix routes example"});
  });

  app.get("/health", [](Request &, Response &res){
    res.json({"ok", true, "service", "routes-example"});
  });
}

static void user_routes(App &app)
{
  app.get("/users", [](Request &req, Response &res){
    res.json({
      "ok", true,
      "page", req.query_value("page", "1"),
      "items", vix::json::array({"Alice", "Bob"})
    });
  });

  app.get("/users/{id}", [](Request &req, Response &res){
    const std::string id = req.param("id");

    if (id == "0") {
      res.status(404).json({
        "ok", false, "error",
        "not found"
      });
      return;
    }

    res.json({
      "ok", true,
      "user", vix::json::o("id", id),
      "name", "Ada"
    });

  });

  app.post("/users", [](Request &req, Response &res){
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

  app.run(8080);

  return 0;
}
```

## Common mistakes

### Missing slash

```cpp
// Wrong
app.get("health", handler);

// Correct
app.get("/health", handler);
```

### Confusing path params and query params

```cpp
// Path: /users/{id}  →  req.param("id")
// Query: /users?page=2  →  req.query_value("page", "1")
```

### Forgetting to return after error

```cpp
// Always return after sending an error
if (bad) {
  res.status(400).json(...);
  return;
}
```

### Wildcard route too early

Specific routes must be registered before wildcard routes.

## What you should remember

A route connects: HTTP method + path → C++ handler.
Use path params for resource identity, query params for options, grouped functions as the app grows.
The core idea: routes are the public shape of your application — keep them clear, predictable, and organized.

## Next chapter

[Next: Request and Response](/book/09-request-response)
