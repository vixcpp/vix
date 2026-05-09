# JSON API

Now you will build a complete JSON API.
JSON APIs are one of the most common things you will build with Vix.

```txt
client sends JSON → Vix reads Request → route validates input → route returns JSON Response
```

## Routes to build

```txt
GET  /
GET  /health
GET  /api/users
GET  /api/users/{id}
POST /api/users
```

## Recommended response shape

```json
{ "ok": true, "data": {} }
{ "ok": true, "count": 2, "data": [] }
{ "ok": false, "error": "message" }
```

## User struct

```cpp
struct User
{
  std::int64_t id{};
  std::string name;
  std::string role;
};

static std::vector<User> make_seed_users()
{
  return { {1, "Alice", "admin"}, {2, "Bob", "user"} };
}
```

> Data is in-memory for now. The database chapter will replace this with SQLite or MySQL.

## JSON helpers

```cpp
static json::Json user_to_json(const User &user)
{
  return json::kv({
      {"id", json::Json(user.id)},
      {"name", json::Json(user.name)},
      {"role", json::Json(user.role)},
  });
}

static json::Json users_to_json(const std::vector<User> &users)
{
  json::Json items = json::Json::array();
  for (const auto &user : users)
    items.push_back(user_to_json(user));

  return items;
}

static void respond_error(Response &res, int status, const std::string &message)
{
  res.status(status).json(json::kv({
      {"ok", json::Json(false)},
      {"error", json::Json(message)},
  }));
}

static std::optional<User> find_user_by_id(const std::vector<User> &users, std::int64_t id)
{
  for (const auto &user : users)
    if (user.id == id)
      return user;

  return std::nullopt;
}

static std::optional<std::int64_t> parse_id(const std::string &text)
{
  try {
    return std::stoll(text);
  }
  catch (...) {
    return std::nullopt;
  }
}
```

## GET /api/users

```cpp
app.get("/api/users", [&users](Request &, Response &res){
  const auto data = users_to_json(users);
  res.json(json::kv({
      {"ok", json::Json(true)},
      {"count", json::Json(static_cast<int>(users.size()))},
      {"data", data}
  }));
});
```

```bash
curl -i http://127.0.0.1:8080/api/users
```

## GET /api/users/{id}

```cpp
app.get("/api/users/{id}", [&users](Request &req, Response &res){

  const auto id = parse_id(req.param("id"));
  if (!id) {
    respond_error(res, 400, "invalid user id");
    return;
  }

  const auto user = find_user_by_id(users, *id);
  if (!user) {
    respond_error(res, 404, "user not found");
    return;
  }

  res.json(json::kv({
      {"ok", json::Json(true)},
      {"data", user_to_json(*user)}
  }));

});
```

```bash
curl -i http://127.0.0.1:8080/api/users/1
curl -i http://127.0.0.1:8080/api/users/999   # 404
curl -i http://127.0.0.1:8080/api/users/abc   # 400
```

## POST /api/users

```cpp
app.post("/api/users", [&users](Request &req, Response &res){

  const auto &body = req.json();
  if (!body.is_object())
  {
    respond_error(res, 400, "expected JSON object body");
    return;
  }

  const std::string name = body.value("name", "");
  const std::string role = body.value("role", "user");

  if (name.empty()) {
    respond_error(res, 400, "field 'name' is required");
    return;
  }

  const std::int64_t next_id = users.empty() ? 1 : users.back().id + 1;
  User user{next_id, name, role.empty() ? "user" : role};
  users.push_back(user);

  res.status(201).json(json::kv({
      {"ok", json::Json(true)},
      {"message", json::Json("user created")},
      {"data", user_to_json(user)}
  }));

});
```

```bash
curl -i -X POST http://127.0.0.1:8080/api/users \
  -H "Content-Type: application/json" \
  -d '{"name":"Charlie","role":"user"}'
```

## Complete example

```cpp
#include <vix.hpp>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

using namespace vix;

struct User {
  std::int64_t id{};
  std::string name;
  std::string role;
};

static std::vector<User> make_seed_users(){
  return {
    {1, "Alice", "admin"},
    {2, "Bob", "user"}
  };
}

static json::Json user_to_json(const User &u){
  return json::kv({
    {"id", json::Json(u.id)},
    {"name", json::Json(u.name)},
    {"role", json::Json(u.role)}
  });
}

static json::Json users_to_json(const std::vector<User> &users)
{
  json::Json items = json::Json::array();
  for (const auto &u : users)
    items.push_back(user_to_json(u));

  return items;
}

static void respond_error(Response &res, int status, const std::string &msg){
  res.status(status).json(json::kv({
    {"ok", json::Json(false)},
    {"error", json::Json(msg)}
  }));
}

static std::optional<User> find_user_by_id(const std::vector<User> &users, std::int64_t id)
{
  for (const auto &u : users)
    if (u.id == id)
      return u;

  return std::nullopt;
}

static std::optional<std::int64_t> parse_id(const std::string &text)
{
  try {
    return std::stoll(text);
  } catch (...) {
    return std::nullopt;
  }
}

static void public_routes(App &app)
{
  app.get("/", [](Request &, Response &res){
    res.json(json::kv({
      {"message", json::Json("Vix JSON API")}
    }));
  });

  app.get("/health", [](Request &, Response &res){
    res.json(json::kv({
      {"ok", json::Json(true)},
      {"service", json::Json("json-api")}
    }));
  });

}

static void user_routes(App &app, std::vector<User> &users)
{
  app.get("/api/users", [&users](Request &, Response &res){
    res.json(json::kv({
        {"ok", json::Json(true)},
        {"count", json::Json(static_cast<int>(users.size()))},
        {"data", users_to_json(users)}
    }));
  });

  app.get("/api/users/{id}", [&users](Request &req, Response &res){
    const auto id = parse_id(req.param("id"));

    if (!id) {
      respond_error(res, 400, "invalid user id");
      return;
    }

    const auto user = find_user_by_id(users, *id);
    if (!user) {
      respond_error(res, 404, "user not found");
      return;
    }

    res.json(json::kv({
      {"ok", json::Json(true)},
      {"data", user_to_json(*user)}
    }));
  });

  app.post("/api/users", [&users](Request &req, Response &res){
    const auto &body = req.json();

    if (!body.is_object()) {
      respond_error(res, 400, "expected JSON object body");
      return;
    }

    const std::string name = body.value("name", "");
    const std::string role = body.value("role", "user");
    if (name.empty()) {
      respond_error(res, 400, "field 'name' is required");
      return;
    }

    const std::int64_t next_id = users.empty() ? 1 : users.back().id + 1;
    User user{next_id, name, role.empty() ? "user" : role};
    users.push_back(user);

    res.status(201).json(json::kv({
        {"ok", json::Json(true)},
        {"message", json::Json("user created")},
        {"data", user_to_json(user)}
    }));

  });
}

int main()
{
  std::vector<User> users = make_seed_users();

  App app;

  public_routes(app);
  ruser_routes(app, users);

  app.run(8080);

  return 0;
}
```

## Test the complete API

```bash
curl -i http://127.0.0.1:8080/health
curl -i http://127.0.0.1:8080/api/users
curl -i http://127.0.0.1:8080/api/users/1
curl -i http://127.0.0.1:8080/api/users/999
curl -i http://127.0.0.1:8080/api/users/abc
curl -i -X POST http://127.0.0.1:8080/api/users \
  -H "Content-Type: application/json" \
  -d '{"name":"Charlie","role":"user"}'
curl -i -X POST http://127.0.0.1:8080/api/users \
  -H "Content-Type: application/json" \
  -d '{}'
```

## Status codes for JSON APIs

| Status | Meaning                           |
|--------|-----------------------------------|
| `200`  | OK, request succeeded.            |
| `201`  | Created, resource added.          |
| `400`  | Bad Request, invalid input.       |
| `401`  | Unauthorized, auth required.      |
| `403`  | Forbidden, access denied.         |
| `404`  | Not Found, resource missing.      |
| `409`  | Conflict, state mismatch.         |
| `429`  | Too Many Requests, rate limited.  |
| `500`  | Internal Server Error.            |

## Route flow for JSON APIs

```txt
read request → parse params or body → validate input → run logic → format JSON → send response
```

## Preparing for the next chapters

- **Database:** the in-memory vector will be replaced by SQLite or MySQL.
- **Middleware:** CORS, rate limiting, and authentication will wrap the routes.
- **Validation:** manual checks will become declarative with `vix::validation`.

## Common mistakes

### Forgetting `Content-Type` with curl

```bash
curl -i -X POST http://127.0.0.1:8080/api/users \
  -H "Content-Type: application/json" \
  -d '{"name":"Ada"}'
```

### Trusting body shape

```cpp
if (!body.is_object()) {
  respond_error(res, 400, "expected JSON object body");
  return;
}
```

### Forgetting to return after error

```cpp
if (name.empty()) {
  respond_error(res, 400, "field 'name' is required");
  return;
}
```

### Returning inconsistent errors

Use one helper: `respond_error(res, 400, "message")`.

## What you should remember

A JSON API route follows: Request → validate → logic → JSON Response.
Use `res.json(...)` for responses, `req.json()` for JSON bodies, helpers for consistent errors and JSON formatting.
The core idea: JSON APIs become simple when request parsing, validation, logic, and response formatting stay separate.

## Next chapter

[Next: Middleware](/book/11-middleware)
