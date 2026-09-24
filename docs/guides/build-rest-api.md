# Build a REST API

This guide shows how to build a small REST API with Vix.

You will create:

- `GET /`
- `GET /health`
- `GET /users`
- `GET /users/{id}`
- `POST /users`

## Requirements

```bash
vix --version
vix new rest-api
cd rest-api
```

## Minimal server

Create or replace `src/main.cpp`:

```cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request &, Response &res){
    res.json({
        "message", "Hello from Vix",
        "framework", "Vix.cpp"
    });
  });

  app.run(8080);
  return 0;
}
```

```bash
vix dev
curl -i http://127.0.0.1:8080/
```

## Basic Vix HTTP structure

| Part            | Purpose                                 |
| --------------- | --------------------------------------- |
| `App app;`      | Creates the HTTP application instance.  |
| `app.get(...)`  | Registers a `GET` route handler.        |
| `Request &req`  | Provides access to request data.        |
| `Response &res` | Provides methods for sending responses. |
| `res.json(...)` | Sends a JSON response.                  |
| `app.run(8080)` | Starts the server on port `8080`.       |

## Create a health route

```cpp
app.get("/health", [](Request &, Response &res){
  res.json({
      "ok", true,
      "service", "rest-api"
  });
});
```

## Add an in-memory users list

```cpp
std::vector<json::Json> users = {
    { {"id", 1}, {"name", "Alice"}, {"role", "admin"} },
    { {"id", 2}, {"name", "Bob"}, {"role", "user"}}
};
```

## Add GET /users

```cpp
app.get("/users", [&users](Request &, Response &res){
  res.json({
      "ok", true,
      "count", users.size(),
      "data", users
  });
});
```

## Add GET /users/{id}

```cpp
app.get("/users/{id}", [&users](Request &req, Response &res){

  const std::string id = req.param("id");
  for (const auto &user : users)
  {
    if (std::to_string(user.value("id", 0)) == id)
    {
      res.json({
        "ok", true,
        "data", user
      });

      return;
    }
  }

  res.status(404).json({
      "ok", false,
      "error", "user not found",
      "id", id
  });

});
```

## Add query params

```cpp
app.get("/users", [&users](Request &req, Response &res){

  const std::string page = req.query_value("page", "1");
  const std::string limit = req.query_value("limit", "10");

  res.json({
      "ok", true,
      "page", page,
      "limit", limit,
      "count", users.size(),
      "data", users
  });

});
```

Useful request APIs:

| API                            | Purpose                                |
| ------------------------------ | -------------------------------------- |
| `req.param("id")`              | Reads a route path parameter.          |
| `req.param("id", "0")`         | Reads a route parameter with fallback. |
| `req.query_value("page", "1")` | Reads a query parameter with fallback. |
| `req.query()`                  | Reads all query parameters.            |
| `req.body()`                   | Reads the raw request body.            |
| `req.json()`                   | Reads the parsed JSON body.            |
| `req.header("Authorization")`  | Reads a request header value.          |

## Add POST /users

```cpp
app.post("/users", [&users](Request &req, Response &res){

  const auto &body = req.json();
  if (!body.is_object())
  {
    res.status(400).json({
      "ok", false,
      "error", "expected JSON object body"
    });

    return;
  }

  const std::string name = body.value("name", "");
  const std::string role = body.value("role", "user");
  if (name.empty())
  {
    res.status(400).json({
      "ok", false,
      "error", "field 'name' is required"
    });

    return;
  }

  const int new_id = users.empty() ? 1 : users.back().value("id", 0) + 1;

  json::Json user = {
    {"id", new_id},
    {"name", name},
    {"role", role}
  };

  users.push_back(user);

  res.status(201).json({
    "ok", true,
    "message", "user created",
    "data", user
  });

});
```

```bash
curl -i -X POST http://127.0.0.1:8080/users \
  -H "Content-Type: application/json" \
  -d '{"name":"Charlie","role":"user"}'
```

## Final version

Replace `src/main.cpp` with this complete, organized version:

```cpp
#include <vix.hpp>
#include <string>
#include <vector>

using namespace vix;

struct AppState
{
  std::vector<json::Json> users;
};

static AppState create_state()
{
  AppState state;

  state.users = {
    { {"id", 1}, {"name", "Alice"}, {"role", "admin"} },
    { {"id", 2}, {"name", "Bob"}, {"role", "user"} }
  };

  return state;
}

static void register_public_routes(App &app)
{
  app.get("/", [](Request &, Response &res){
    res.json({
      "message", "Vix REST API",
      "framework", "Vix.cpp"
    });
  });

  app.get("/health", [](Request &, Response &res){
    res.json({
      "ok", true,
      "service", "rest-api"
    });
  });
}

static void register_user_routes(App &app, AppState &state)
{
  app.get("/users", [&state](Request &req, Response &res){
    const std::string page = req.query_value("page", "1");
    const std::string limit = req.query_value("limit", "10");

    res.json({
      "ok", true,
      "page", page,
      "limit", limit,
      "count", state.users.size(),
      "data", state.users
    });
  });

  app.get("/users/{id}", [&state](Request &req, Response &res){

    const std::string id = req.param("id");
    for (const auto &user : state.users)
    {
      if (std::to_string(user.value("id", 0)) == id)
      {
        res.json({
          "ok", true,
          "data", user
        });

        return;
      }
    }

    res.status(404).json({
      "ok", false,
      "error", "user not found",
      "id", id
    });

  });

  app.post("/users", [&state](Request &req, Response &res){

    const auto &body = req.json();
    if (!body.is_object())
    {
      res.status(400).json({
        "ok", false,
        "error", "expected JSON object body"
      });

      return;
    }

    const std::string name = body.value("name", "");
    const std::string role = body.value("role", "user");
    if (name.empty())
    {
      res.status(400).json({
        "ok", false,
        "error", "field 'name' is required"
      });

      return;
    }

    const int new_id = state.users.empty() ? 1 : state.users.back().value("id", 0) + 1;

    json::Json user = {
      {"id", new_id},
      {"name", name},
      {"role", role}
    };

    state.users.push_back(user);

    res.status(201).json({
      "ok", true,
      "message", "user created",
      "data", user
    });

  });
}

int main()
{
  App app;

  AppState state = create_state();
  register_public_routes(app);
  register_user_routes(app, state);

  app.run(8080);

  return 0;
}
```

## Test the API

```bash
curl -i http://127.0.0.1:8080/
curl -i http://127.0.0.1:8080/health
curl -i http://127.0.0.1:8080/users
curl -i "http://127.0.0.1:8080/users?page=1&limit=10"
curl -i http://127.0.0.1:8080/users/1
curl -i -X POST http://127.0.0.1:8080/users \
  -H "Content-Type: application/json" \
  -d '{"name":"Charlie","role":"user"}'
```

## Response helpers

```cpp
res.json({"ok", true, "message", "Hello"});   // JSON
res.text("Hello Vix");                         // Text
res.send("Created");                           // Generic
res.status(201).json({"ok", true});            // With status
res.file("public/index.html");                 // File
```

## Status codes

| Status | Meaning                | Example                        |
| ------ | ---------------------- | ------------------------------ |
| `200`  | OK.                    | Successful `GET` request.      |
| `201`  | Created.               | Successful `POST` request.     |
| `400`  | Bad Request.           | Invalid input was provided.    |
| `401`  | Unauthorized.          | Authentication is missing.     |
| `403`  | Forbidden.             | Access is not allowed.         |
| `404`  | Not Found.             | Requested resource is missing. |
| `500`  | Internal Server Error. | Server-side failure occurred.  |

## Recommended JSON shape

```json
{ "ok": true, "data": {} }
{ "ok": false, "error": "message" }
{ "ok": true, "count": 2, "data": [] }
```

## Common mistakes

### Forgetting to return after sending an error

```cpp
// Wrong
if (name.empty()) {
  res.status(400).json({
    "error", "name is required"
  });
}

res.json({"ok", true});

// Correct
if (name.empty()) {
  res.status(400).json({
    "error", "name is required"
  });
  return;
}

res.json({"ok", true});
```

### Passing runtime args incorrectly

```bash
vix run main.cpp --run --port 8080   # correct
vix run main.cpp -- --port 8080      # wrong: -- is for compiler flags
```

### Expecting in-memory data to persist

The `users` vector is in memory.
If the server restarts, created users are lost.
For persistence, use SQLite or MySQL.

## What to use next

- [SQLite API guide](/guides/database/sqlite) — for persistence
- [Validation guide](/guides/validation) — for validating input
- [Authentication guide](/guides/authentication) — for auth
