# Handlers

This page shows how handlers work in Vix Core.

Use it when you want to write route handlers, read requests, send responses, use route parameters, return text or JSON, and understand how Vix adapts user handlers internally.

## Public header

```cpp
#include <vix.hpp>
```

You can also include the handler headers directly:

```cpp
#include <vix/http/IRequestHandler.hpp>
#include <vix/http/RequestHandler.hpp>
```

## What handlers provide

A handler is the function that runs when a route matches a request.

In most Vix applications, handlers are registered through `vix::App`.

```cpp
app.get("/", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.text("Hello from Vix");
});
```

Handlers provide access to:

- the incoming request
- route parameters
- query parameters
- headers
- request body
- JSON body
- request state
- the outgoing response
- text responses
- JSON responses
- redirects
- files
- templates

## Basic handler

A basic handler receives:

```cpp
vix::Request &req
vix::Response &res
```

Example:

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

Expected response:

```text
Home
```

## Handler signature

The public handler signature is:

```cpp
void(vix::Request &req, vix::Response &res)
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

`vix::Request` is the incoming request object.

`vix::Response` is the public response helper.

## Read request data

Use `req` to read request data.

```cpp
app.get("/debug", [](vix::Request &req, vix::Response &res)
{
  res.json({
    "method", req.method(),
    "path", req.path(),
    "target", req.target()
  });
});
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

## Read route parameters

Use `req.param(...)` to read route parameters.

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

## Read multiple route parameters

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

## Read query parameters

Use `req.query_value(...)` to read query parameters.

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

## Check query parameter existence

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

## Read headers

Use `req.header(...)` to read a header.

```cpp
app.get("/agent", [](vix::Request &req, vix::Response &res)
{
  res.json({
    "user_agent", req.header("User-Agent")
  });
});
```

Use `has_header(...)` to check whether a header exists.

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

  res.json({"authorized", true});
});
```

## Read request body

Use `req.body()` to read the request body.

```cpp
app.post("/echo", [](vix::Request &req, vix::Response &res)
{
  res.text(req.body());
});
```

Request body:

```text
hello
```

Response:

```text
hello
```

## Read JSON body

Use `req.json()` when the body contains JSON.

```cpp
app.post("/users", [](vix::Request &req, vix::Response &res)
{
  const auto &body = req.json();

  res.status(201).json({
    "received", body
  });
});
```

You can also convert JSON to a type when supported.

```cpp
auto user = req.json_as<User>();
```

## Send text

Use `res.text(...)`.

```cpp
app.get("/", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.text("Hello");
});
```

## Send JSON

Use `res.json(...)`.

```cpp
app.get("/api/status", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.json({
    "status", "ok",
    "server", "Vix.cpp"
  });
});
```

## Set status code

Use `res.status(...)`.

```cpp
app.post("/users", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.status(201).json({
    "created", true
  });
});
```

## Send an error

```cpp
app.get("/admin", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  const bool allowed = false;

  if (!allowed)
  {
    res.status(403).json({
      "error", "forbidden"
    });
    return;
  }

  res.text("admin");
});
```

## Redirect

Use `res.redirect(...)`.

```cpp
app.get("/old", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.redirect("/new");
});
```

With explicit status:

```cpp
app.get("/old", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.redirect(301, "/new");
});
```

## Send a file

Use `res.file(...)`.

```cpp
app.get("/download", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.file("public/file.txt");
});
```

## Render a template

First configure templates:

```cpp
app.templates("views");
```

Then render from a handler:

```cpp
app.get("/", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  vix::tmpl::Context ctx;
  ctx.set("title", "Home");

  res.render("index.html", ctx);
});
```

## Empty response

Use `send()` when you want to send an empty response.

```cpp
app.get("/empty", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.status(204).send();
});
```

## Send status

Use `sendStatus(...)` to set a status and send a default body when appropriate.

```cpp
app.get("/missing", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.sendStatus(404);
});
```

## Set headers

Use `res.header(...)`.

```cpp
app.get("/custom", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.header("X-App", "Vix");
  res.text("custom header");
});
```

Use `res.type(...)` to set the content type.

```cpp
app.get("/plain", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.type("text/plain; charset=utf-8");
  res.send("plain text");
});
```

## Auto-send return values

A handler can return a sendable value.

```cpp
app.get("/hello", [](vix::Request &req, vix::Response &res)
{
  (void)req;
  (void)res;

  return "Hello from Vix";
});
```

If the response body was not already written, Vix can send the returned value automatically.

Example with JSON-like data:

```cpp
app.get("/status", [](vix::Request &req, vix::Response &res)
{
  (void)req;
  (void)res;

  return vix::json::Json{
    "status", "ok"
  };
});
```

## Return status and payload

Handlers can also return a status and payload pair when supported.

```cpp
app.get("/created", [](vix::Request &req, vix::Response &res)
{
  (void)req;
  (void)res;

  return std::pair{201, "created"};
});
```

Or a tuple:

```cpp
app.get("/accepted", [](vix::Request &req, vix::Response &res)
{
  (void)req;
  (void)res;

  return std::tuple{202, "accepted"};
});
```

## Stop early

A handler can stop early by returning after writing a response.

```cpp
app.get("/private", [](vix::Request &req, vix::Response &res)
{
  if (!req.has_header("Authorization"))
  {
    res.status(401).json({
      "error", "unauthorized"
    });
    return;
  }

  res.text("private content");
});
```

## Handler with middleware

Handlers often receive requests after middleware has already run.

```cpp
app.use([](vix::Request &req, vix::Response &res, vix::App::Next next)
{
  (void)res;

  vix::print("before handler:", req.path());

  next();
});

app.get("/", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.text("home");
});
```

The flow is:

```text
middleware
  -> handler
  -> response
```

## Request state

`RequestState` can store request-scoped data.

This is useful for middleware sharing data with handlers.

```cpp
struct CurrentUser
{
  std::string id;
};

app.use([](vix::Request &req, vix::Response &res, vix::App::Next next)
{
  (void)res;

  req.state().set(CurrentUser{"42"});

  next();
});

app.get("/me", [](vix::Request &req, vix::Response &res)
{
  const auto &user = req.state().get<CurrentUser>();

  res.json({
    "id", user.id
  });
});
```

Use `try_get` when the value may be absent.

```cpp
if (auto *user = req.state().try_get<CurrentUser>())
{
  res.json({"id", user->id});
  return;
}

res.status(401).json({"error", "unauthorized"});
```

## Handler errors

If a handler throws, Vix catches the exception inside the request handler adapter and sends an error response.

```cpp
app.get("/fail", [](vix::Request &req, vix::Response &res)
{
  (void)req;
  (void)res;

  throw std::runtime_error("something failed");
});
```

In development, error responses can include more useful debugging information.

In production, handlers should return explicit error responses when possible.

## Internal handler model

Public handlers are adapted into a common internal interface.

```text
user handler
  -> RequestHandler<Handler>
  -> IRequestHandler
  -> Router
```

The internal interface is asynchronous:

```cpp
virtual vix::async::core::task<void> handle_request(
    const Request &req,
    Response &res) = 0;
```

This lets the router await handlers through a uniform `task<void>` interface.

## Handler adaptation

When a route is registered through `App`, Vix wraps the user handler.

```text
App::add_route
  -> collect middleware
  -> create final handler
  -> create RequestHandler
  -> router.add_route
```

The wrapper is responsible for:

- running middleware
- calling the final user handler
- auto-sending return values when needed
- finalizing the response
- adapting the handler to `IRequestHandler`

## Route parameter extraction

Route parameters are extracted from the route pattern.

```cpp
app.get("/posts/{id}", handler);
```

For:

```text
GET /posts/42
```

The handler can read:

```cpp
req.param("id");
```

Internally, Vix compares the route pattern with the request path and fills the request parameter map.

## Valid handler shape

The most common handler shape is:

```cpp
[](vix::Request &req, vix::Response &res)
{
  res.text("OK");
}
```

This is the recommended style for application code.

## Complete example

```cpp
#include <vix.hpp>

struct CurrentUser
{
  std::string id;
};

int main()
{
  vix::App app;

  app.use([](vix::Request &req, vix::Response &res, vix::App::Next next)
  {
    (void)res;

    req.state().set(CurrentUser{"42"});

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

  app.post("/echo", [](vix::Request &req, vix::Response &res)
  {
    res.json({
      "body", req.body()
    });
  });

  app.get("/me", [](vix::Request &req, vix::Response &res)
  {
    const auto &user = req.state().get<CurrentUser>();

    res.json({
      "id", user.id
    });
  });

  app.get("/old", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.redirect("/new");
  });

  app.run(8080);

  return 0;
}
```

## API summary

| API                               | Purpose                               |
| --------------------------------- | ------------------------------------- |
| `req.method()`                    | Return the HTTP method.               |
| `req.path()`                      | Return the path without query string. |
| `req.target()`                    | Return the full request target.       |
| `req.body()`                      | Return the request body.              |
| `req.param(name)`                 | Return a route parameter.             |
| `req.has_param(name)`             | Check if a route parameter exists.    |
| `req.query()`                     | Return parsed query parameters.       |
| `req.query_value(name, fallback)` | Return one query parameter.           |
| `req.has_query(name)`             | Check if a query parameter exists.    |
| `req.header(name)`                | Return a request header.              |
| `req.has_header(name)`            | Check if a header exists.             |
| `req.json()`                      | Parse body as JSON.                   |
| `req.json_as<T>()`                | Convert JSON body to type `T`.        |
| `req.state()`                     | Access request-scoped state.          |
| `res.status(code)`                | Set HTTP status.                      |
| `res.text(data)`                  | Send plain text.                      |
| `res.json(value)`                 | Send JSON.                            |
| `res.redirect(url)`               | Send redirect.                        |
| `res.file(path)`                  | Send file.                            |
| `res.render(name, context)`       | Render a template.                    |
| `res.header(name, value)`         | Set a header.                         |
| `res.type(mime)`                  | Set content type.                     |
| `res.send()`                      | Send an empty or current response.    |
| `res.sendStatus(code)`            | Send a status response.               |

## Best practices

Keep handlers small.

```cpp
app.get("/status", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.json({"status", "ok"});
});
```

Use middleware for repeated logic.

```cpp
app.use("/admin", auth_middleware);
```

Use route parameters for resource identifiers.

```cpp
app.get("/users/{id}", handler);
```

Return explicit error responses.

```cpp
res.status(400).json({"error", "bad request"});
```

Avoid long blocking work directly inside handlers. Use the runtime/executor model for expensive work when needed.

## Next steps

Read the next pages:

- [Middleware](./middleware.md)
- [Request](./request.md)
- [Response](./response.md)
- [Routing](./routing.md)
- [Runtime executor](./runtime-executor.md)
