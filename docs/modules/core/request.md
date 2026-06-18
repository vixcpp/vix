# Request

This page shows how to use `vix::Request` in Vix Core.

Use it when you want to read the HTTP method, path, query parameters, route parameters, headers, body, JSON data, or request-scoped state.

## Public header

```cpp
#include <vix.hpp>
```

You can also include the request header directly:

```cpp
#include <vix/http/Request.hpp>
```

## What Request provides

`vix::Request` represents an incoming HTTP request.

It gives access to:

- HTTP method
- full target
- path without query string
- raw query string
- parsed query parameters
- route parameters
- request headers
- request body
- parsed JSON body
- request-scoped state

Most handlers receive a request by reference.

```cpp
app.get("/", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.text("Hello");
});
```

## Basic usage

```cpp
#include <vix.hpp>

int main()
{
  vix::App app;

  app.get("/debug", [](vix::Request &req, vix::Response &res)
  {
    res.json({
      "method", req.method(),
      "path", req.path(),
      "target", req.target()
    });
  });

  app.run(8080);

  return 0;
}
```

Request:

```text
GET /debug?page=1
```

Example response:

```json
{
  "method": "GET",
  "path": "/debug",
  "target": "/debug?page=1"
}
```

## HTTP method

Use `method()` to read the request method.

```cpp
app.get("/method", [](vix::Request &req, vix::Response &res)
{
  res.json({
    "method", req.method()
  });
});
```

Example:

```text
GET
POST
PUT
PATCH
DELETE
HEAD
OPTIONS
```

## Target

Use `target()` to read the full request target.

The target includes the path and query string.

```cpp
app.get("/debug", [](vix::Request &req, vix::Response &res)
{
  res.json({
    "target", req.target()
  });
});
```

Request:

```text
GET /debug?page=2
```

Example response:

```json
{
  "target": "/debug?page=2"
}
```

## Path

Use `path()` to read the path without the query string.

```cpp
app.get("/debug", [](vix::Request &req, vix::Response &res)
{
  res.json({
    "path", req.path()
  });
});
```

Request:

```text
GET /debug?page=2
```

Example response:

```json
{
  "path": "/debug"
}
```

## Raw query string

Use `query_string()` to read the raw query string without `?`.

```cpp
app.get("/search", [](vix::Request &req, vix::Response &res)
{
  res.json({
    "query_string", req.query_string()
  });
});
```

Request:

```text
GET /search?q=vix&page=2
```

Example response:

```json
{
  "query_string": "q=vix&page=2"
}
```

## Query parameters

Use `query_value(...)` to read one query parameter.

```cpp
app.get("/search", [](vix::Request &req, vix::Response &res)
{
  const std::string q = req.query_value("q", "");
  const std::string page = req.query_value("page", "1");

  res.json({
    "q", q,
    "page", page
  });
});
```

Request:

```text
GET /search?q=vix&page=2
```

Example response:

```json
{
  "q": "vix",
  "page": "2"
}
```

## Query fallback

The second argument to `query_value(...)` is the fallback value.

```cpp
const std::string page = req.query_value("page", "1");
```

If the query parameter is missing, the fallback is returned.

```text
GET /search?q=vix
```

Result:

```text
page = 1
```

## Check query parameter

Use `has_query(...)`.

```cpp
app.get("/search", [](vix::Request &req, vix::Response &res)
{
  if (!req.has_query("q"))
  {
    res.status(400).json({
      "error", "missing query parameter q"
    });
    return;
  }

  res.json({
    "q", req.query_value("q")
  });
});
```

## Read all query parameters

Use `query()` to access the parsed query map.

```cpp
app.get("/search", [](vix::Request &req, vix::Response &res)
{
  const auto &query = req.query();

  res.json({
    "count", query.size()
  });
});
```

## Route parameters

Use `param(...)` to read a route parameter.

```cpp
app.get("/users/{id}", [](vix::Request &req, vix::Response &res)
{
  res.json({
    "id", req.param("id")
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

## Route parameter fallback

Use the second argument to provide a fallback.

```cpp
const std::string id = req.param("id", "unknown");
```

If the parameter does not exist, the fallback is returned.

## Check route parameter

Use `has_param(...)`.

```cpp
app.get("/users/{id}", [](vix::Request &req, vix::Response &res)
{
  if (!req.has_param("id"))
  {
    res.status(400).json({
      "error", "missing id"
    });
    return;
  }

  res.json({
    "id", req.param("id")
  });
});
```

## Read all route parameters

Use `params()`.

```cpp
app.get("/users/{user_id}/posts/{post_id}", [](vix::Request &req, vix::Response &res)
{
  const auto &params = req.params();

  res.json({
    "count", params.size(),
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
  "count": 2,
  "user_id": "7",
  "post_id": "42"
}
```

## Headers

Use `header(...)` to read a header value.

```cpp
app.get("/agent", [](vix::Request &req, vix::Response &res)
{
  res.json({
    "user_agent", req.header("User-Agent")
  });
});
```

If the header is missing, an empty string is returned.

## Check header

Use `has_header(...)`.

```cpp
app.get("/auth", [](vix::Request &req, vix::Response &res)
{
  if (!req.has_header("Authorization"))
  {
    res.status(401).json({
      "error", "missing authorization header"
    });
    return;
  }

  res.json({
    "authorized", true
  });
});
```

## Read all headers

Use `headers()`.

```cpp
app.get("/headers", [](vix::Request &req, vix::Response &res)
{
  const auto &headers = req.headers();

  res.json({
    "count", headers.size()
  });
});
```

## Body

Use `body()` to read the raw request body.

```cpp
app.post("/echo", [](vix::Request &req, vix::Response &res)
{
  res.text(req.body());
});
```

Request body:

```text
hello from Vix
```

Response:

```text
hello from Vix
```

## JSON body

Use `json()` to parse the body as JSON.

```cpp
app.post("/json", [](vix::Request &req, vix::Response &res)
{
  const auto &body = req.json();

  res.json({
    "received", body
  });
});
```

Request body:

```json
{
  "name": "Ada"
}
```

Example response:

```json
{
  "received": {
    "name": "Ada"
  }
}
```

## Convert JSON body

Use `json_as<T>()` to convert the parsed JSON body to a type when supported.

```cpp
auto value = req.json_as<MyType>();
```

Example shape:

```cpp
app.post("/users", [](vix::Request &req, vix::Response &res)
{
  auto user = req.json_as<User>();

  res.status(201).json({
    "created", true
  });
});
```

The conversion depends on the JSON type support available for `T`.

## Request state

`RequestState` is request-scoped storage.

Use it when middleware needs to pass data to handlers.

```cpp
struct CurrentUser
{
  std::string id;
};
```

Middleware can store a value.

```cpp
app.use([](vix::Request &req, vix::Response &res, vix::App::Next next)
{
  (void)res;

  req.state().set(CurrentUser{"42"});

  next();
});
```

A handler can read it later.

```cpp
app.get("/me", [](vix::Request &req, vix::Response &res)
{
  const auto &user = req.state().get<CurrentUser>();

  res.json({
    "id", user.id
  });
});
```

## Check request state

Use `has_state_type<T>()`.

```cpp
if (req.has_state_type<CurrentUser>())
{
  const auto &user = req.state().get<CurrentUser>();
}
```

## Optional request state

Use `try_get<T>()` when a value may be missing.

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

## Store request state in middleware

```cpp
#include <vix.hpp>

#include <string>

struct CurrentUser
{
  std::string id;
};

int main()
{
  vix::App app;

  app.use("/api/private", [](vix::Request &req, vix::Response &res, vix::App::Next next)
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

## Modify request fields

Most application code reads the request, but the request object also exposes setters.

Set the method:

```cpp
req.set_method("GET");
```

Set the target and recompute path/query caches:

```cpp
req.set_target("/users?page=1");
```

Set the body:

```cpp
req.set_body("new body");
```

Set a header:

```cpp
req.set_header("X-App", "Vix");
```

Remove a header:

```cpp
req.remove_header("X-App");
```

These APIs are mostly useful for middleware, testing, adapters, or advanced integrations.

## Set route parameters

Use `set_params(...)` for advanced code that builds or adapts requests.

```cpp
vix::Request::ParamMap params;
params["id"] = "42";

req.set_params(std::move(params));
```

Normal route handlers should use `req.param(...)` instead.

## Copy request with parameters

Use `with_params(...)` to return a copy of a request with new route parameters.

```cpp
auto next_req = req.with_params({
  "id", "42"
});
```

This is mostly used internally by the request handler adapter.

## Request lifecycle

A request is usually created by the HTTP session.

```text
Transport read
  -> HTTP parser
  -> ParsedRequestHead
  -> Request
  -> Router
  -> RequestHandler
  -> user handler
```

Application code normally receives it here:

```cpp
app.get("/path", [](vix::Request &req, vix::Response &res)
{
  // use req here
});
```

## Request and routing

The router uses:

```cpp
req.method()
req.target()
req.path()
```

Route parameters are added before the final user handler runs.

```cpp
req.param("id");
```

Query parameters are parsed from the target.

```cpp
req.query_value("page");
```

## Request and middleware

Middleware receives the same request object as the handler.

```cpp
app.use([](vix::Request &req, vix::Response &res, vix::App::Next next)
{
  (void)res;

  vix::print(req.method(), req.path());

  next();
});
```

Middleware can read request data, validate it, or attach state.

## Complete example

```cpp
#include <vix.hpp>
#include <string>

struct CurrentUser
{
  std::string id;
};

int main()
{
  vix::App app;

  app.use("/api", [](vix::Request &req, vix::Response &res, vix::App::Next next)
  {
    res.header("X-API", "Vix");

    next();
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

  app.get("/api/users/{id}", [](vix::Request &req, vix::Response &res)
  {
    res.json({
      "id", req.param("id"),
      "page", req.query_value("page", "1"),
      "method", req.method(),
      "path", req.path()
    });
  });

  app.post("/api/echo", [](vix::Request &req, vix::Response &res)
  {
    res.json({
      "body", req.body()
    });
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

| API                           | Purpose                                       |
| ----------------------------- | --------------------------------------------- |
| `method()`                    | Return the HTTP method.                       |
| `set_method(method)`          | Set the HTTP method.                          |
| `target()`                    | Return the full request target.               |
| `set_target(target)`          | Set the target and recompute path/query data. |
| `path()`                      | Return the path without query string.         |
| `query_string()`              | Return the raw query string.                  |
| `query()`                     | Return parsed query parameters.               |
| `has_query(name)`             | Check whether a query parameter exists.       |
| `query_value(name, fallback)` | Return a query parameter value.               |
| `params()`                    | Return route parameters.                      |
| `set_params(params)`          | Replace route parameters.                     |
| `has_param(name)`             | Check whether a route parameter exists.       |
| `param(name, fallback)`       | Return a route parameter value.               |
| `headers()`                   | Return all headers.                           |
| `set_headers(headers)`        | Replace all headers.                          |
| `header(name)`                | Return one header value.                      |
| `has_header(name)`            | Check whether a header exists.                |
| `set_header(name, value)`     | Set one header.                               |
| `remove_header(name)`         | Remove one header.                            |
| `body()`                      | Return the request body.                      |
| `set_body(body)`              | Replace the request body.                     |
| `json()`                      | Parse and return the body as JSON.            |
| `json_as<T>()`                | Convert the JSON body to type `T`.            |
| `has_state()`                 | Check whether state storage exists.           |
| `state()`                     | Access request-scoped state.                  |
| `has_state_type<T>()`         | Check whether state contains `T`.             |
| `with_params(params)`         | Return a request copy with new parameters.    |

## Best practices

Use `path()` when matching or logging the route path.

```cpp
vix::print(req.path());
```

Use `target()` when you need the original path plus query string.

```cpp
vix::print(req.target());
```

Use fallbacks for optional query parameters.

```cpp
const std::string page = req.query_value("page", "1");
```

Use route parameters for resource identifiers.

```cpp
const std::string id = req.param("id");
```

Use middleware and `RequestState` for authenticated user data.

```cpp
req.state().set(CurrentUser{"42"});
```

Validate input before using it.

```cpp
if (!req.has_query("q"))
{
  res.status(400).json({"error", "missing q"});
  return;
}
```

## Next steps

Read the next pages:

- [Response](./response.md)
- [Handlers](./handlers.md)
- [Middleware](./middleware.md)
- [Routing](./routing.md)
- [Sessions](./sessions.md)
