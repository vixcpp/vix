# Hello App

This example is the smallest useful Vix HTTP application.

It shows the modern `vix::App` style:

```txt
create an App
register routes
write responses
run the server
test with curl
```

Use this example first before moving to JSON APIs, middleware, authentication, static files, or cache.

## Source

Create a file:

```txt
hello_app.cpp
```

Add this code:

```cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request &, Response &res)
  {
    res.text("Hello from Vix");
  });

  app.get("/health", [](Request &, Response &res)
  {
    res.json({
      "ok", true,
      "service", "hello-app"
    });
  });

  app.get("/hello", [](Request &req, Response &res)
  {
    const std::string name = req.query_value("name", "Vix");

    res.json({
      "message", "Hello, " + name
    });
  });

  app.run(8080);
  return 0;
}
```

## Run it

```bash
vix run hello_app.cpp
```

The server listens on:

```txt
http://127.0.0.1:8080
```

## Test the home route

```bash
curl -i http://127.0.0.1:8080/
```

Expected status:

```txt
HTTP/1.1 200 OK
```

Expected body:

```txt
Hello from Vix
```

This is the simplest route shape:

```cpp
app.get("/", [](Request &, Response &res)
{
  res.text("Hello from Vix");
});
```

The route receives a request and a response.

It writes text to the response.

## Test the health route

```bash
curl -i http://127.0.0.1:8080/health
```

Expected body shape:

```json
{
  "ok": true,
  "service": "hello-app"
}
```

The route writes JSON:

```cpp
app.get("/health", [](Request &, Response &res)
{
  res.json({
    "ok", true,
    "service", "hello-app"
  });
});
```

This is useful for simple health checks.

## Test a query parameter

```bash
curl -i "http://127.0.0.1:8080/hello?name=Ada"
```

Expected body shape:

```json
{
  "message": "Hello, Ada"
}
```

The route reads a query value:

```cpp
const std::string name = req.query_value("name", "Vix");
```

If the query parameter is missing, the default value is used.

```bash
curl -i http://127.0.0.1:8080/hello
```

Expected body shape:

```json
{
  "message": "Hello, Vix"
}
```

## What this example teaches

This example introduces the base shape of a Vix application.

```cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request &, Response &res)
  {
    res.text("Hello from Vix");
  });

  app.run(8080);
}
```

The important parts are:

| Part                 | Purpose                          |
| -------------------- | -------------------------------- |
| `#include <vix.hpp>` | Includes the main Vix API        |
| `App app;`           | Creates the HTTP application     |
| `app.get(...)`       | Registers a GET route            |
| `Request &`          | Gives access to request data     |
| `Response &`         | Writes the response              |
| `app.run(8080)`      | Starts the server on port `8080` |

## Route handlers

A route handler usually receives:

```cpp
Request &req
Response &res
```

Example:

```cpp
app.get("/hello", [](Request &req, Response &res)
{
  const std::string name = req.query_value("name", "Vix");

  res.json({
    "message", "Hello, " + name
  });
});
```

Use `Request` to read:

```txt
method
path
headers
query parameters
body
typed state
```

Use `Response` to write:

```txt
status
headers
text
JSON
files
```

## Text responses

Use `res.text(...)` for plain text:

```cpp
res.text("Hello from Vix");
```

You can also set a status first:

```cpp
res.status(200).text("OK");
```

## JSON responses

Use `res.json(...)` for JSON:

```cpp
res.json({
  "ok", true
});
```

For created resources:

```cpp
res.status(201).json({
  "ok", true,
  "created", true
});
```

For errors:

```cpp
res.status(404).json({
  "ok", false,
  "error", "Not found"
});
```

## Status codes

A response can set the HTTP status explicitly.

```cpp
app.get("/missing", [](Request &, Response &res)
{
  res.status(404).json({
    "ok", false,
    "error", "Not found"
  });
});
```

Test:

```bash
curl -i http://127.0.0.1:8080/missing
```

Expected status:

```txt
HTTP/1.1 404 Not Found
```

## Add another route

You can add more routes before `app.run(...)`.

```cpp
app.get("/version", [](Request &, Response &res)
{
  res.json({
    "name", "hello-app",
    "version", "1.0.0"
  });
});
```

Routes are registered during startup.

After that, `app.run(8080)` starts the server.

## Keep `main()` simple

For tiny examples, it is fine to register routes directly in `main()`.

For bigger applications, move route registration into a function.

```cpp
#include <vix.hpp>

using namespace vix;

static void register_routes(App &app)
{
  app.get("/", [](Request &, Response &res)
  {
    res.text("Hello from Vix");
  });

  app.get("/health", [](Request &, Response &res)
  {
    res.json({
      "ok", true
    });
  });
}

int main()
{
  App app;

  register_routes(app);

  app.run(8080);
  return 0;
}
```

This keeps the startup structure clear:

```txt
create app
register routes
run server
```

## Complete version with route function

```cpp
#include <vix.hpp>

using namespace vix;

static void register_routes(App &app)
{
  app.get("/", [](Request &, Response &res)
  {
    res.text("Hello from Vix");
  });

  app.get("/health", [](Request &, Response &res)
  {
    res.json({
      "ok", true,
      "service", "hello-app"
    });
  });

  app.get("/hello", [](Request &req, Response &res)
  {
    const std::string name = req.query_value("name", "Vix");

    res.json({
      "message", "Hello, " + name
    });
  });
}

int main()
{
  App app;

  register_routes(app);

  app.run(8080);
  return 0;
}
```

This is the version you should prefer when an example starts to grow.

## Next steps

Continue with:

1. [JSON API](./json-api)
2. [Middleware API](./middleware-api)
3. [API Key Auth](./auth-api-key)

`Hello App` teaches the base application shape.

The next examples add request parsing, middleware, and route protection.
