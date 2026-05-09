# JSON API

Build a small in-memory users API with structured JSON responses.

```txt
GET    /health       → health check
GET    /users        → list users
GET    /users/{id}   → get one user
POST   /users        → create user
DELETE /users/{id}   → delete user
```

## Create a workspace

```bash
mkdir -p ~/tmp/vix-examples/json-api
cd ~/tmp/vix-examples/json-api
touch main.cpp
```

## Full code

```cpp
#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include <vix.hpp>
using namespace vix;

struct User { std::int64_t id{}; std::string name; std::string role; };

static std::vector<User> make_seed_users()
{
  return {
    {1, "Alice", "admin"},
    {2, "Bob", "user"}
  };
}

static json::Json user_to_json(const User &u)
{
  return json::obj({
    {"id", u.id},
    {"name", u.name},
    {"role", u.role}
  });
}

static json::Json users_to_json(const std::vector<User> &users)
{
  json::Json items = json::Json::array();
  for (const auto &u : users)
    items.push_back(user_to_json(u));

  return items;
}

static void respond_error(Response &res, int status,
                           const std::string &code, const std::string &message)
{
  res.status(status).json({
    "ok", false,
    "error", code,
    "message", message
  });
}

static std::optional<std::int64_t> parse_id(const std::string &text)
{
  try{
    std::size_t pos = 0;
    const auto value = std::stoll(text, &pos);

    if (pos != text.size() || value <= 0)
      return std::nullopt;

    return value;
  }catch (...) {
    return std::nullopt;
  }
}

int main()
{
  App app;

  auto users = make_seed_users();

  app.get("/health", [](Request &, Response &res){
    res.json({
      "ok", true,
      "service", "json-api"
    });
  });

  app.get("/users", [&users](Request &, Response &res){
    res.json({
      "ok", true,
      "count", static_cast<int>(users.size()), "data", users_to_json(users)
    });
  });

  app.get("/users/{id}", [&users](Request &req, Response &res){

    const auto id = parse_id(req.param("id"));
    if (!id) {
      respond_error(res, 400, "invalid_id", "Invalid user id");
      return;
    }

    for (const auto &u : users){
      if (u.id == *id) {
        res.json({"ok", true, "data", user_to_json(u)});
        return;
      }
      respond_error(res, 404, "user_not_found", "User not found");
    }

  });

  app.post("/users", [&users](Request &req, Response &res){

    const auto &body = req.json();
    if (!body.is_object()) {
      respond_error(res, 400, "invalid_body", "Expected JSON object");
      return;
    }

    const std::string name = body.value("name", "");
    const std::string role = body.value("role", "user");

    if (name.empty()) {
      respond_error(res, 400, "validation_failed", "name is required");
      return;
    }

    const std::int64_t next_id = users.empty() ? 1 : users.back().id + 1;
    users.push_back({next_id, name, role.empty() ? "user" : role});

    res.status(201).json({
      "ok", true,
      "message", "user created",
      "data", user_to_json(users.back())
    });

  });

  app.del("/users/{id}", [&users](Request &req, Response &res){

    const auto id = parse_id(req.param("id"));
    if (!id) {
      respond_error(res, 400, "invalid_id", "Invalid user id");
      return;
    }

    const auto before = users.size();
    users.erase(std::remove_if(users.begin(), users.end(),[id](const User &u) {
      return u.id == *id;
    }), users.end());

    if (users.size() == before) {
      respond_error(res, 404, "user_not_found", "User not found");
      return;
    }

    res.json({
      "ok", true,
      "message", "user deleted",
      "id", *id
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
curl -i http://127.0.0.1:8080/health
curl -i http://127.0.0.1:8080/users
curl -i http://127.0.0.1:8080/users/1
curl -i http://127.0.0.1:8080/users/999
curl -i -X POST http://127.0.0.1:8080/users \
  -H "Content-Type: application/json" -d '{"name":"Charlie","role":"user"}'
curl -i -X DELETE http://127.0.0.1:8080/users/3
```

## Response shapes

```json
{ "ok": true, "count": 2, "data": [...] }
{ "ok": true, "data": { "id": 1, "name": "Alice", "role": "admin" } }
{ "ok": true, "message": "user created", "data": { "id": 3, ... } }
{ "ok": false, "error": "user_not_found", "message": "User not found" }
```

## HTTP status codes used

| Case | Status |
|------|--------|
| Read OK | 200 |
| Create OK | 201 |
| Invalid id or body | 400 |
| User not found | 404 |

## Add query parameters

```cpp
app.get("/users", [&users](Request &req, Response &res){
  const std::string page = req.query_value("page", "1");
  const std::string limit = req.query_value("limit", "20");

  res.json({
    "ok", true,
    "page", page,
    "limit", limit,
    "data", users_to_json(users)
  });
});
```

```bash
curl -i "http://127.0.0.1:8080/users?page=1&limit=10"
```

## Common mistakes

```cpp
// Wrong — returns 200 for errors
res.json({
  "ok", false,
  "error", "user_not_found"
});

// Correct
res.status(404).json({
  "ok", false,
  "error", "user_not_found"
});

// Wrong — no return after error
respond_error(res, 400, "...", "...");
res.status(201).json({"ok", true});

// Correct
respond_error(res, 400, "...", "...");
return;
```

## What you should remember

```txt
route → read params or JSON body → validate → build JSON → send Response
```

Use helpers: `user_to_json`, `users_to_json`, `respond_error`, `parse_id`. Keep shapes consistent.

The core idea: **don't just return JSON — return predictable JSON.**

Next: [Middleware](/examples/middleware)
