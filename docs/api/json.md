# JSON API

The JSON API is used to build and parse structured data in Vix applications.

```txt
API responses — request body parsing — objects — arrays — errors — sync payloads
```

## Public headers

```cpp
#include <vix.hpp>         // for HTTP apps (includes JSON)
#include <vix/json.hpp>    // for JSON-only code
```

Common namespace: `vix::json`. With `using namespace vix`: `json::Json`, `json::obj(...)`, `json::array(...)`, `json::kv(...)`.

## Basic JSON response

```cpp
app.get("/health", [](Request &, Response &res){
  res.json({
    "ok", true,
    "service", "json-api"
  });
});
```

## The main JSON type

`vix::json::Json` represents: object, array, string, number, boolean, null.

```cpp
// Object
vix::json::Json data = vix::json::obj({
    {"ok", true},
    {"message", "Hello from Vix"}
});

// Array
vix::json::Json names = vix::json::array({"Alice", "Bob", "Charlie"});

// Dynamic array
vix::json::Json items = vix::json::Json::array();

items.push_back(vix::json::obj({
  {"id", 1},
  {"name", "Alice"}
}));

items.push_back(vix::json::obj({
  {"id", 2},
  {"name", "Bob"}
}));
```

## Good response shapes

```json
{ "ok": true, "data": {} }
{ "ok": true, "count": 2, "data": [] }
{ "ok": false, "error": "validation_failed", "message": "name is required" }
```

## res.json(...)

```cpp
res.json({
  "ok", true,
  "message", "Hello"
});

res.status(201).json({
  "ok", true,
  "message", "created"
});

res.status(400).json({
  "ok", false,
  "error", "bad_request"
});
```

## Build JSON with helpers

```cpp
struct User {
  std::int64_t id{};
  std::string name;
  std::string role;
};

static vix::json::Json user_to_json(const User &u)
{
  return vix::json::obj({
    {"id", u.id},
    {"name", u.name},
    {"role", u.role}
  });
}

static vix::json::Json users_to_json(const std::vector<User> &users)
{
  vix::json::Json items = vix::json::Json::array();
  for (const auto &u : users)
    items.push_back(user_to_json(u));
  return items;
}

// Usage in route
res.json({
  "ok", true,
  "count", static_cast<int>(users.size()),
  "data", users_to_json(users)
});
```

## Read JSON request body

```cpp
app.post("/users", [](Request &req, Response &res){
  const auto &body = req.json();

  // Always check shape first
  if (!body.is_object())
  {
    res.status(400).json({
      "ok", false,
      "error", "expected JSON object"
    });
    return;
  }

  // Read fields with fallback
  const std::string name = body.value("name", "");
  const std::string role = body.value("role", "user");

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
    "name", name,
    "role", role
  });
});
```

## Error helper

```cpp
static void respond_error(Response &res, int status,
                           const std::string &code, const std::string &message)
{
  res.status(status).json({
    "ok", false,
    "error", code,
    "message", message
  });
}
```

## Stable error codes

```txt
invalid_body, validation_failed, not_found, unauthorized, forbidden, conflict, internal_error
```

## JSON and validation

```cpp
const std::string email = body.value("email", "");

auto result = vix::validation::validate("email", email)
                  .required().email().result();
```

JSON answers: "what did the client send?" — Validation answers: "is this data valid?"

## JSON and database

```cpp
static json::Json user_row_to_json(const vix::db::Row &row)
{
  return json::obj({
    {"id", row.getInt64(0)},
    {"name", row.getString(1)},
    {"role", row.getString(2)}
  });
}
```

Flow: `database row → C++ object → json::Json → res.json(...)`

## JSON and sync

```cpp
// Payloads stored as JSON strings must be replayable
Operation op;
op.kind = "message.send";
op.payload = R"({"id":"msg_1","text":"hello","created_at":1710000000000})";
```

## Complete example

```cpp
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include <vix.hpp>
using namespace vix;

struct User { std::int64_t id{}; std::string name; std::string role; };

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
  try {
    return std::stoll(text);
  } catch (...) {
    return std::nullopt;
  }
}

int main()
{
  App app;
  std::vector<User> users{
    {1, "Alice", "admin"},
    {2, "Bob", "user"}
  };

  app.get("/health", [](Request &, Response &res) {
    res.json({"ok", true});
  });

  app.get("/users", [&users](Request &, Response &res){
    res.json({
      "ok", true,
      "count", static_cast<int>(users.size()),
      "data", users_to_json(users)
    });
  });

  app.get("/users/{id}", [&users](Request &req, Response &res){
    const auto id = parse_id(req.param("id"));
    if (!id) { respond_error(res, 400, "invalid_id", "Invalid user id"); return; }
    for (const auto &u : users)
      if (u.id == *id) {
        res.json({
          "ok", true,
          "data", user_to_json(u)
        });
        return;
      }
      respond_error(res, 404, "user_not_found", "User not found");
  });

  app.post("/users", [&users](Request &req, Response &res){
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

    const std::int64_t next_id = users.empty() ? 1 : users.back().id + 1;
    users.push_back({next_id, name, body.value("role", "user")});

    res.status(201).json({
      "ok", true,
      "message", "user created"
    });
  });

  app.run(8080);
  return 0;
}
```

## Common mistakes

### Returning different shapes everywhere

```cpp
// Bad — inconsistent
{ "success": true }
{ "status": "ok", "items": [] }

// Good — predictable
{ "ok": true, "data": {} }
```

### Reading fields without checking shape

```cpp
// Wrong
const std::string name = body.value("name", "");

// Correct
if (!body.is_object()) {
  respond_error(res, 400, "invalid_body", "Expected JSON object");
  return;
}
const std::string name = body.value("name", "");
```

### Building JSON strings manually

```cpp
// Wrong
std::string body = "{\"ok\":true,\"name\":\"" + name + "\"}";

// Correct
res.json({
  "ok", true,
  "name", name
});
```

## What you should remember

```cpp
// success
res.json({
  "ok", true,
  "data", {}
});

// error
res.status(400).json({
  "ok", false,
  "error", "code"
});

const auto &body = req.json();
if (!body.is_object()) {
  /* error */
  return;
}

const std::string name = body.value("name", "");
```

The core idea: JSON is the public data shape of your API — keep it predictable.

Next: [Middleware API](/api/middleware)
