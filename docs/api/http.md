# HTTP API

The HTTP API is the core application API of Vix. It gives you the main building blocks for web applications and APIs.

```txt
App — Request — Response — Routes — Static files — Server lifecycle
```

## Public header

```cpp
#include <vix.hpp>
```

## Basic example

```cpp
#include <vix.hpp>
using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request &, Response &res) {
    res.text("Hello from Vix");
  });

  app.run(8080);

  return 0;
}
```

## App

`App` is the main application object. It owns the router, middleware chain, server lifecycle, static files, and runtime configuration.

```cpp
App app;
```

### Server lifecycle

```cpp
app.run(8080);              // start and block

app.listen(8080, []() {     // start async with callback
  std::cout << "listening\n";
});

app.wait();                 // block until server stops
app.close();                // stop the server
```

## Routes

A route connects an HTTP method and path to a C++ handler.

```cpp
app.get("/users/{id}", [](Request &req, Response &res){
  const std::string id = req.param("id");
  res.json({"id", id});
});
```

### HTTP methods

```cpp
app.get("/users", list_handler);
app.post("/users", create_handler);
app.put("/users/{id}", replace_handler);
app.patch("/users/{id}", update_handler);
app.del("/users/{id}", delete_handler);
app.head("/health", head_handler);
app.options("/users", options_handler);
```

### Path parameters

```cpp
// Matches /users/1, /users/42, /users/abc
app.get("/users/{id}", [](Request &req, Response &res){
  res.json({"id", req.param("id")});
});

// Multiple parameters
app.get("/posts/{year}/{slug}", [](Request &req, Response &res){
  res.json({
    "year", req.param("year"),
    "slug", req.param("slug")
  });
});
```

### Query parameters

```cpp
// /users?page=2&limit=10
app.get("/users", [](Request &req, Response &res){
  const std::string page = req.query_value("page", "1");
  const std::string limit = req.query_value("limit", "20");

  res.json({
    "page", page,
    "limit", limit
  });
});
```

Use path parameters to identify a resource (`/users/42`), query parameters to modify how resources are read (`/users?page=2`).

## Request

| Method | Purpose |
|--------|---------|
| `req.param("id")` | Read a path parameter |
| `req.param("id", "0")` | Read with fallback |
| `req.query_value("page", "1")` | Read a query parameter with fallback |
| `req.query()` | Read all query parameters |
| `req.header("Authorization")` | Read a request header |
| `req.body()` | Read the raw request body |
| `req.json()` | Read the parsed JSON body |
| `req.path()` | Read the request path |
| `req.method()` | Read the HTTP method |

### Read JSON body

```cpp
app.post("/users", [](Request &req, Response &res){

  const auto &body = req.json();
  if (!body.is_object())
  {
    res.status(400).json({
      "ok", false,
      "error", "expected JSON object"
    });
    return;
  }

  const std::string name = body.value("name", "");
  if (name.empty())
  {
    res.status(400).json({
      "ok", false,
      "error", "name is required"
    });
    return;
  }

  res.status(201).json({
    "ok", true,
    "name", name
  });

});
```

## Response

| Method | Purpose |
|--------|---------|
| `res.text("Hello")` | Send plain text |
| `res.send("Hello")` | Send a basic response body |
| `res.json({...})` | Send JSON |
| `res.status(201)` | Set HTTP status (chainable) |
| `res.header("X-Foo", "bar")` | Set a response header |
| `res.file("public/index.html")` | Send a file |

```cpp
res.status(201).json({
  "ok", true,
  "message", "created"
});
res.header("X-Powered-By", "Vix.cpp");
res.file("public/index.html");
```

## Status codes

| Status | Meaning |
|--------|---------|
| 200 | OK |
| 201 | Created |
| 204 | No Content |
| 400 | Bad Request |
| 401 | Unauthorized |
| 403 | Forbidden |
| 404 | Not Found |
| 409 | Conflict |
| 429 | Too Many Requests |
| 500 | Internal Server Error |

## Error helper

```cpp
static void respond_error(Response &res, int status,
                           const std::string &code,
                           const std::string &message)
{
  res.status(status).json({
    "ok", false,
    "error", code,
    "message", message
  });
}

// Always return after sending an error
if (id == "0") {
  respond_error(res, 404, "user_not_found", "User not found");
  return;
}
```

## Good response shapes

```json
{ "ok": true, "data": {} }
{ "ok": true, "count": 2, "data": [] }
{ "ok": false, "error": "validation_failed", "message": "name is required" }
```

## Route order

Register specific routes before generic ones:

```cpp
// Correct
app.get("/users/search", search_handler);
app.get("/users/{id}", user_handler);
app.get("/*", fallback_handler);

// Wrong — wildcard catches everything first
app.get("/*", fallback_handler);
app.get("/users/{id}", user_handler);
```

## Wildcard and static files

```cpp
// Manual static file fallback
app.get("/*", [](Request &req, Response &res){
  res.header("Cache-Control", "public, max-age=86400");
  res.file("public" + req.path());
});

// Simple static directory
app.static_dir("public");
```

## Route groups

```cpp
app.group("/api", [](auto &api){
  api.get("/health", [](Request &, Response &res) {
    res.json({"ok", true});
  });

  api.get("/users", [](Request &, Response &res) {
    res.json({"ok", true});
  });

  api.group("/admin", [](auto &admin){
    admin.get("/stats", [](Request &, Response &res){
      res.json({"ok", true});
    });
  });
});
// Routes: GET /api/health, GET /api/users, GET /api/admin/stats
```

## Organizing routes

```cpp
static void register_public_routes(App &app)
{
  app.get("/", [](Request &, Response &res) {
    res.json({"message", "Hello"});
  });

  app.get("/health", [](Request &, Response &res) {
    res.json({"ok", true});
  });
}

int main()
{
  App app;

  register_public_routes(app);

  app.run(8080);

  return 0;
}
```

## Complete example

```cpp
#include <vix.hpp>
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

static void register_user_routes(App &app)
{
  app.get("/users", [](Request &req, Response &res){
    res.json({
        "ok", true,
        "page", req.query_value("page", "1"),
        "data", vix::json::array({"Alice", "Bob"})
    });
  });

  app.get("/users/{id}", [](Request &req, Response &res){
    const std::string id = req.param("id");
    if (id == "0") {
      respond_error(res, 404, "user_not_found", "User not found");
      return;
    }

    res.json({
      "ok", true,
      "data", vix::json::o("id", id),
      "name", "Ada"
    });
  });

  app.post("/users", [](Request &req, Response &res){
    const auto &body = req.json();
    if (!body.is_object()) {
      respond_error(res, 400, "invalid_body", "Expected JSON object");
      return;
    }

    const std::string name = body.value("name", "");
    if (name.empty()) {
      respond_error(res, 400, "validation_failed", "name is required");
      return;
    }

    res.status(201).json({
      "ok", true,
      "message", "user created"
    });
  });

  app.del("/users/{id}", [](Request &req, Response &res){
    const std::string id = req.param("id");
    if (id == "0") {
      respond_error(res, 404, "user_not_found", "User not found");
      return;
    }

    res.json({
      "ok", true,
      "message", "user deleted",
      "id", id
    });
  });
}

int main()
{
  App app;

  app.get("/health", [](Request &, Response &res) {
    res.json({"ok", true});
  });

  register_user_routes(app);

  app.run(8080);

  return 0;
}
```

## Test

```bash
curl -i http://127.0.0.1:8080/health
curl -i "http://127.0.0.1:8080/users?page=2"
curl -i http://127.0.0.1:8080/users/1
curl -i -X POST http://127.0.0.1:8080/users \
  -H "Content-Type: application/json" -d '{"name":"Charlie"}'
curl -i -X DELETE http://127.0.0.1:8080/users/1
```

## Common mistakes

### Missing leading slash

```cpp
app.get("health", handler);   // Wrong
app.get("/health", handler);  // Correct
```

### Forgetting to return after an error

```cpp
// Wrong — continues after error
respond_error(res, 400, "...", "...");
res.status(201).json({"ok", true});

// Correct
respond_error(res, 400, "...", "...");
return;
```

### Using -- for runtime arguments

```bash
vix run main.cpp -- --port 8080   # Wrong — compiler flags
vix run main.cpp --run --port 8080  # Correct — runtime args
```

## What you should remember

The HTTP API is built around three types: `App`, `Request`, `Response`.

```txt
client → Request → route handler → Response
```

The most common starting point: `#include <vix.hpp>`

Next: [JSON API](/api/json)
