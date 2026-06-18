# JSON API

This example shows how to build a small JSON API with Vix.

It covers the most common JSON patterns:

```txt id="m4gz51"
return JSON directly with res.json(...)
build JSON with vix::json::o(...) and vix::json::a(...)
parse JSON automatically with middleware::app::json_strict_dev(...)
read parsed JSON from JsonBody
validate required fields with get_opt(...)
use defaults with get_or(...)
parse manually with try_loads(...) when you do not want middleware
```

Vix gives you more than one way to work with JSON because different routes need different levels of control.

## What this example builds

The API exposes:

```txt id="s0lqup"
GET  /api/health
GET  /api/users
GET  /api/users/{id}
POST /api/users
POST /api/echo-manual
```

The example demonstrates two JSON request-body styles:

```txt id="e54crd"
middleware parsing
  app.use("/api/users", middleware::app::json_strict_dev(...))

manual parsing
  vix::json::try_loads(req.body())
```

Use middleware parsing for normal API routes.

Use manual parsing when a route needs full control over parsing behavior.

## Source

Create a file:

```txt id="a05s15"
json_api.cpp
```

Add this code:

```cpp id="sfudja"
#include <string>
#include <vector>

#include <vix.hpp>
#include <vix/middleware.hpp>
#include <vix/json.hpp>

using namespace vix;

struct User
{
  int id;
  std::string name;
  std::string email;
  bool active;
};

static std::vector<User> users{
  {1, "Ada", "ada@example.com", true},
  {2, "Linus", "linus@example.com", true}
};

static vix::json::Json user_to_json(const User &user)
{
  using namespace vix::json;

  return o(
    "id", user.id,
    "name", user.name,
    "email", user.email,
    "active", user.active
  );
}

static vix::json::Json users_to_json()
{
  using namespace vix::json;

  Json items = arr();

  for (const auto &user : users)
  {
    items.push_back(user_to_json(user));
  }

  return items;
}

static const User *find_user(int id)
{
  for (const auto &user : users)
  {
    if (user.id == id)
      return &user;
  }

  return nullptr;
}

static void register_routes(App &app)
{
  app.get("/api/health", [](Request &, Response &res)
  {
    res.json({
      "ok", true,
      "service", "json-api"
    });
  });

  app.get("/api/users", [](Request &, Response &res)
  {
    using namespace vix::json;

    Json payload = o(
      "ok", true,
      "users", users_to_json()
    );

    res.json(payload);
  });

  app.get("/api/users/{id}", [](Request &req, Response &res)
  {
    using namespace vix::json;

    const int id = std::stoi(req.param("id"));

    const User *user = find_user(id);

    if (!user)
    {
      res.status(404).json({
        "ok", false,
        "error", "User not found"
      });
      return;
    }

    res.json(o(
      "ok", true,
      "user", user_to_json(*user)
    ));
  });

  app.post("/api/users", [](Request &req, Response &res)
  {
    using namespace vix::json;

    auto &body = req.state<middleware::parsers::JsonBody>();

    auto name = get_opt<std::string>(body.value, "name");
    auto email = get_opt<std::string>(body.value, "email");
    const bool active = get_or<bool>(body.value, "active", true);

    if (!name || name->empty())
    {
      res.status(422).json({
        "ok", false,
        "error", "Missing required field",
        "field", "name"
      });
      return;
    }

    if (!email || email->empty())
    {
      res.status(422).json({
        "ok", false,
        "error", "Missing required field",
        "field", "email"
      });
      return;
    }

    const int next_id = users.empty() ? 1 : users.back().id + 1;

    users.push_back(User{
      next_id,
      *name,
      *email,
      active
    });

    res.status(201).json(o(
      "ok", true,
      "user", user_to_json(users.back())
    ));
  });

  app.post("/api/echo-manual", [](Request &req, Response &res)
  {
    using namespace vix::json;

    auto body = try_loads(req.body());

    if (!body)
    {
      res.status(400).json({
        "ok", false,
        "error", "Invalid JSON"
      });
      return;
    }

    res.json(o(
      "ok", true,
      "body", *body
    ));
  });
}

int main()
{
  App app;

  app.use("/api", middleware::app::security_headers_dev());
  app.use("/api", middleware::app::body_limit_write_dev(1024 * 1024));

  app.use("/api/users", middleware::app::json_strict_dev(
    4096,
    false,
    true
  ));

  register_routes(app);

  app.run(8080);
  return 0;
}
```

## Run it

```bash id="mrm1ex"
vix run json_api.cpp
```

The server listens on:

```txt id="h44xcj"
http://127.0.0.1:8080
```

## Test the health route

```bash id="spc27w"
curl -i http://127.0.0.1:8080/api/health
```

Expected body shape:

```json id="oju3gt"
{
  "ok": true,
  "service": "json-api"
}
```

This route uses the simplest JSON response style:

```cpp id="oo2bkf"
res.json({
  "ok", true,
  "service", "json-api"
});
```

Use this style for small responses.

## Test the users list

```bash id="f3udnz"
curl -i http://127.0.0.1:8080/api/users
```

Expected body shape:

```json id="wzzce8"
{
  "ok": true,
  "users": [
    {
      "id": 1,
      "name": "Ada",
      "email": "ada@example.com",
      "active": true
    },
    {
      "id": 2,
      "name": "Linus",
      "email": "linus@example.com",
      "active": true
    }
  ]
}
```

This route builds a payload first:

```cpp id="vxjr32"
using namespace vix::json;

Json payload = o(
  "ok", true,
  "users", users_to_json()
);

res.json(payload);
```

Use this style when the JSON response is larger or reused by helper functions.

## Test one user

```bash id="wqbyyy"
curl -i http://127.0.0.1:8080/api/users/1
```

Expected body shape:

```json id="xoc7my"
{
  "ok": true,
  "user": {
    "id": 1,
    "name": "Ada",
    "email": "ada@example.com",
    "active": true
  }
}
```

Request a missing user:

```bash id="f54jjq"
curl -i http://127.0.0.1:8080/api/users/999
```

Expected status:

```txt id="yh3b3m"
404 Not Found
```

Expected body shape:

```json id="ssxrsh"
{
  "ok": false,
  "error": "User not found"
}
```

## Create a user

```bash id="i7d6tg"
curl -i \
  -X POST http://127.0.0.1:8080/api/users \
  -H "Content-Type: application/json" \
  -d '{"name":"Grace","email":"grace@example.com","active":true}'
```

Expected status:

```txt id="r5oolx"
201 Created
```

Expected body shape:

```json id="l0f0pk"
{
  "ok": true,
  "user": {
    "id": 3,
    "name": "Grace",
    "email": "grace@example.com",
    "active": true
  }
}
```

The route reads parsed JSON from request state:

```cpp id="y4ri4h"
auto &body = req.state<middleware::parsers::JsonBody>();
```

That state exists because the route prefix installed the JSON parser middleware:

```cpp id="pud34l"
app.use("/api/users", middleware::app::json_strict_dev(
  4096,
  false,
  true
));
```

The route then validates required fields:

```cpp id="z6nij3"
auto name = get_opt<std::string>(body.value, "name");
auto email = get_opt<std::string>(body.value, "email");
```

And reads an optional field with a default:

```cpp id="u6lkj8"
const bool active = get_or<bool>(body.value, "active", true);
```

## Test validation

Missing `name`:

```bash id="zn6lug"
curl -i \
  -X POST http://127.0.0.1:8080/api/users \
  -H "Content-Type: application/json" \
  -d '{"email":"missing-name@example.com"}'
```

Expected status:

```txt id="bgcayx"
422 Unprocessable Entity
```

Expected body shape:

```json id="t2dm69"
{
  "ok": false,
  "error": "Missing required field",
  "field": "name"
}
```

Missing `email`:

```bash id="dez9sf"
curl -i \
  -X POST http://127.0.0.1:8080/api/users \
  -H "Content-Type: application/json" \
  -d '{"name":"Grace"}'
```

Expected status:

```txt id="ccn895"
422 Unprocessable Entity
```

Expected body shape:

```json id="d7xxsr"
{
  "ok": false,
  "error": "Missing required field",
  "field": "email"
}
```

Invalid JSON:

```bash id="og00iz"
curl -i \
  -X POST http://127.0.0.1:8080/api/users \
  -H "Content-Type: application/json" \
  -d '{"name":}'
```

Expected status:

```txt id="dg9sfy"
400 Bad Request
```

Wrong content type:

```bash id="mcd2co"
curl -i \
  -X POST http://127.0.0.1:8080/api/users \
  -H "Content-Type: text/plain" \
  -d '{"name":"Grace","email":"grace@example.com"}'
```

Expected status:

```txt id="wvf4t6"
415 Unsupported Media Type
```

The JSON parser middleware stops invalid requests before the handler runs.

## Manual JSON parsing route

The route below does not use `JsonBody`.

It parses the body manually:

```cpp id="k4j96w"
auto body = try_loads(req.body());

if (!body)
{
  res.status(400).json({
    "ok", false,
    "error", "Invalid JSON"
  });
  return;
}
```

Test it:

```bash id="vms7va"
curl -i \
  -X POST http://127.0.0.1:8080/api/echo-manual \
  -H "Content-Type: application/json" \
  -d '{"message":"Hello"}'
```

Expected body shape:

```json id="jdrqhu"
{
  "ok": true,
  "body": {
    "message": "Hello"
  }
}
```

Invalid JSON:

```bash id="lp93ja"
curl -i \
  -X POST http://127.0.0.1:8080/api/echo-manual \
  -H "Content-Type: application/json" \
  -d 'not-json'
```

Expected status:

```txt id="swfm51"
400 Bad Request
```

Use manual parsing when:

```txt id="qmxjw5"
the route needs special parsing behavior
you want custom error messages
you do not want middleware on that route
you parse optional or mixed body formats
```

For normal JSON APIs, prefer parser middleware.

## The different JSON styles

Vix gives you several JSON styles.

They are not competitors.

They are for different cases.

## 1. Direct `res.json({...})`

Use this for small responses.

```cpp id="h126k6"
res.json({
  "ok", true,
  "service", "api"
});
```

Good for:

```txt id="oyazv6"
health checks
simple errors
small success payloads
quick examples
```

## 2. Build JSON with `o()` and `a()`

Use `vix::json::o()` and `vix::json::a()` when the response is bigger or nested.

```cpp id="vs2txu"
using namespace vix::json;

Json payload = o(
  "ok", true,
  "user", o(
    "id", 1,
    "name", "Ada"
  ),
  "roles", a("admin", "editor")
);

res.json(payload);
```

Good for:

```txt id="u5xe4g"
nested responses
response helper functions
test fixtures
generated metadata
configuration JSON
readable C++ JSON construction
```

## 3. Build progressively with `obj()` and `arr()`

Use `obj()` and `arr()` when fields are added in steps.

```cpp id="ccnz71"
using namespace vix::json;

Json payload = obj();

payload["ok"] = true;
payload["count"] = users.size();

Json items = arr();

for (const auto &user : users)
{
  items.push_back(user_to_json(user));
}

payload["users"] = items;

res.json(payload);
```

Good for:

```txt id="qdsuqb"
loops
conditional fields
computed fields
response building across multiple steps
```

## 4. Parse with middleware

Use parser middleware for normal request bodies.

```cpp id="yu59uo"
app.use("/api/users", middleware::app::json_strict_dev(4096));
```

Then read:

```cpp id="a5ei2g"
auto &body = req.state<middleware::parsers::JsonBody>();
```

Good for:

```txt id="vwtkbc"
public API routes
POST routes
PUT routes
PATCH routes
routes where invalid JSON should stop before handler logic
```

## 5. Parse manually with `try_loads()`

Use manual parsing when you need full control.

```cpp id="tegz2x"
using namespace vix::json;

auto body = try_loads(req.body());

if (!body)
{
  res.status(400).json({
    "ok", false,
    "error", "Invalid JSON"
  });
  return;
}
```

Good for:

```txt id="ighyym"
custom parse errors
optional JSON body
mixed content routes
small isolated examples
manual validation flows
```

## 6. Read fields safely

Use `get_opt<T>()` when a required field must be validated.

```cpp id="ln5k2f"
auto name = get_opt<std::string>(body, "name");

if (!name || name->empty())
{
  res.status(422).json({
    "ok", false,
    "error", "Missing required field",
    "field", "name"
  });
  return;
}
```

Use `get_or<T>()` when an optional field has a default value.

```cpp id="oeq8oa"
const int page = get_or<int>(body, "page", 1);
const int limit = get_or<int>(body, "limit", 20);
```

Good for:

```txt id="v6xfi4"
validation
pagination defaults
optional filters
safe field access from external input
```

## 7. Request JSON cache

Some Vix request APIs can expose JSON-oriented helpers such as:

```cpp id="hm3pbe"
const vix::json::Json &data = req.json();
```

and:

```cpp id="fw8ei0"
auto value = req.json_as<MyType>();
```

Use this only when the route or middleware design already guarantees that the body is valid JSON, or when exception-based behavior is intentional.

For public endpoints, prefer one of these two patterns:

```cpp id="r5m5mu"
middleware::app::json_strict_dev(...)
```

or:

```cpp id="oxrs9w"
vix::json::try_loads(req.body())
```

They make bad client input easier to handle cleanly.

## Which JSON style should I use?

Use this rule:

| Need                                    | Best style                                |
| --------------------------------------- | ----------------------------------------- |
| Small response                          | `res.json({...})`                         |
| Nested response                         | `vix::json::o()` and `vix::json::a()`     |
| Conditional response                    | `vix::json::obj()` and `vix::json::arr()` |
| Normal JSON request body                | `middleware::app::json_strict_dev(...)`   |
| Custom parsing behavior                 | `vix::json::try_loads(req.body())`        |
| Required field validation               | `get_opt<T>()`                            |
| Optional field defaults                 | `get_or<T>()`                             |
| Internal already-validated request body | `req.json()` or `req.json_as<T>()`        |

## Middleware parsing vs manual parsing

Use middleware parsing for clean APIs:

```cpp id="tijrxt"
app.use("/api/users", middleware::app::json_strict_dev(4096));

app.post("/api/users", [](Request &req, Response &res)
{
  auto &body = req.state<middleware::parsers::JsonBody>();

  // business validation here
});
```

Use manual parsing for route-level control:

```cpp id="x7tx9l"
app.post("/api/echo", [](Request &req, Response &res)
{
  auto body = vix::json::try_loads(req.body());

  if (!body)
  {
    res.status(400).json({
      "ok", false,
      "error", "Invalid JSON"
    });
    return;
  }

  res.json({
    "ok", true
  });
});
```

The recommended default for backend APIs is middleware parsing.

## Complete test flow

Run the server:

```bash id="jww642"
vix run json_api.cpp
```

Health:

```bash id="axky60"
curl -i http://127.0.0.1:8080/api/health
```

List users:

```bash id="shnqwp"
curl -i http://127.0.0.1:8080/api/users
```

Get one user:

```bash id="cncrj2"
curl -i http://127.0.0.1:8080/api/users/1
```

Create user:

```bash id="oohvf0"
curl -i \
  -X POST http://127.0.0.1:8080/api/users \
  -H "Content-Type: application/json" \
  -d '{"name":"Grace","email":"grace@example.com"}'
```

Invalid body:

```bash id="gok8p8"
curl -i \
  -X POST http://127.0.0.1:8080/api/users \
  -H "Content-Type: application/json" \
  -d '{"name":}'
```

Manual echo:

```bash id="ixg1oo"
curl -i \
  -X POST http://127.0.0.1:8080/api/echo-manual \
  -H "Content-Type: application/json" \
  -d '{"message":"Hello"}'
```

## Summary

A Vix JSON API usually follows this shape:

```txt id="u9ccqq"
middleware validates and parses the request body
handler reads JsonBody
handler validates business fields
handler builds response JSON
handler sends res.json(...)
```

Use direct JSON for small responses:

```cpp id="b3e20d"
res.json({
  "ok", true
});
```

Use builders for structured responses:

```cpp id="t8670z"
using namespace vix::json;

res.json(o(
  "ok", true,
  "items", a("one", "two")
));
```

Use parser middleware for normal request bodies:

```cpp id="wec5ab"
app.use("/api/users", middleware::app::json_strict_dev(4096));
```

Use manual parsing when the route needs custom control:

```cpp id="d1fj05"
auto body = vix::json::try_loads(req.body());
```

The recommended default is:

```txt id="iwl83t"
json_strict_dev for request bodies
get_opt for required fields
get_or for optional fields
o/a for structured responses
res.json for sending
```
