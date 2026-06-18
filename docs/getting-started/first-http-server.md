# Your First HTTP Server

This page shows how to build a small HTTP server with Vix.cpp.

You will create four routes:

```txt
GET /
GET /health
GET /hello/{name}
GET /users/{id}
```

The goal is to understand the basic Vix HTTP model:

```txt
App -> route -> Request -> Response -> app.run()
```

A Vix HTTP server is still a native C++ application. Vix.cpp provides the runtime API and the development workflow around it.

## Start from your project

Use the project created in the previous page:

```bash
cd ~/tmp/hello
```

Open the main source file:

```txt
src/main.cpp
```

## Configure the server port

Open `.env` and make sure it contains:

```dotenv
SERVER_PORT=8080
VIX_LOG_LEVEL=info
VIX_LOG_FORMAT=kv
```

The port belongs in configuration, not in the application logic.

This lets the same source code run on different ports in local development, testing, staging, or production.

## Create a minimal server

Replace `src/main.cpp` with:

```cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request &, Response &res) {
    res.text("Hello from Vix.cpp");
  });

  app.run();

  return 0;
}
```

Start the application in development mode:

```bash
vix dev
```

Open another terminal and test the route:

```bash
curl -i http://127.0.0.1:8080/
```

Expected response body:

```txt
Hello from Vix.cpp
```

## What this code does

The application starts with:

```cpp
App app;
```

This creates the Vix application object.

The route is registered with:

```cpp
app.get("/", [](Request &, Response &res) {
  res.text("Hello from Vix.cpp");
});
```

This means: when the server receives a `GET /` request, send a plain text response.

The server starts with:

```cpp
app.run();
```

Because the port is configured in `.env`, the source code does not need to hardcode it.

## Core concepts

| Part                   | Purpose                                                         |
| ---------------------- | --------------------------------------------------------------- |
| `#include <vix.hpp>`   | Imports the main Vix API.                                       |
| `using namespace vix;` | Lets the example use `App`, `Request`, and `Response` directly. |
| `App app;`             | Creates the HTTP application.                                   |
| `app.get(...)`         | Registers a GET route.                                          |
| `Request &req`         | Represents the incoming request.                                |
| `Response &res`        | Builds and sends the response.                                  |
| `app.run()`            | Starts the server using the runtime configuration.              |

This is the core shape of a Vix HTTP application.

## Return JSON

Most backend services return JSON rather than plain text.

Replace the `/` route with:

```cpp
app.get("/", [](Request &, Response &res) {
  res.json({
    "message", "Hello from Vix.cpp",
    "framework", "Vix.cpp"
  });
});
```

Test it:

```bash
curl -i http://127.0.0.1:8080/
```

Expected response shape:

```json
{
  "message": "Hello from Vix.cpp",
  "framework": "Vix.cpp"
}
```

The exact formatting may differ, but the response should contain the same data.

## Add a health route

A health route is useful for checking whether the server is running.

Add this before `app.run()`:

```cpp
app.get("/health", [](Request &, Response &res) {
  res.json({
    "ok", true,
    "service", "hello"
  });
});
```

Test it:

```bash
curl -i http://127.0.0.1:8080/health
```

Expected response shape:

```json
{
  "ok": true,
  "service": "hello"
}
```

Health routes are commonly used by deployment systems, reverse proxies, monitoring tools, and local development scripts.

## Add a path parameter

Path parameters let a route read values from the URL.

Add this route before `app.run()`:

```cpp
app.get("/hello/{name}", [](Request &req, Response &res) {
  const std::string name = req.param("name");

  res.json({
    "greeting", "Hello " + name,
    "powered_by", "Vix.cpp"
  });
});
```

Test it:

```bash
curl -i http://127.0.0.1:8080/hello/Gaspard
```

Expected response shape:

```json
{
  "greeting": "Hello Gaspard",
  "powered_by": "Vix.cpp"
}
```

Here:

```cpp
req.param("name")
```

reads the `{name}` segment from:

```txt
/hello/{name}
```

So `/hello/Gaspard` gives `"Gaspard"`.

## Add query parameters

Query parameters come after `?` in the URL.

Add this route before `app.run()`:

```cpp
app.get("/users/{id}", [](Request &req, Response &res) {
  const std::string id = req.param("id");
  const std::string page = req.query_value("page", "1");
  const std::string limit = req.query_value("limit", "10");

  res.json({
    "ok", true,
    "id", id,
    "page", page,
    "limit", limit
  });
});
```

Test it:

```bash
curl -i "http://127.0.0.1:8080/users/42?page=2&limit=20"
```

Expected response shape:

```json
{
  "ok": true,
  "id": "42",
  "page": "2",
  "limit": "20"
}
```

Here:

```cpp
req.param("id")
```

reads the route parameter.

```cpp
req.query_value("page", "1")
```

reads the `page` query parameter and returns `"1"` if it is missing.

## Response helpers

Vix provides response helpers for common HTTP responses:

```cpp
res.send("Hello world");
res.text("Hello Vix");
res.json({"ok", true});
res.status(201).json({"ok", true});
res.header("Cache-Control", "no-cache");
res.file("public/index.html");
```

Use them according to the response you want to send:

| Method                      | Use when                           |
| --------------------------- | ---------------------------------- |
| `res.send(...)`             | You want a generic response.       |
| `res.text(...)`             | You want plain text.               |
| `res.json(...)`             | You want JSON.                     |
| `res.status(...).json(...)` | You want to set the HTTP status.   |
| `res.header(...)`           | You want to set a response header. |
| `res.file(...)`             | You want to send a file.           |

## Complete example

Your full `src/main.cpp` can now look like this:

```cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request &, Response &res) {
    res.json({
      "message", "Hello from Vix.cpp",
      "framework", "Vix.cpp"
    });
  });

  app.get("/health", [](Request &, Response &res) {
    res.json({
      "ok", true,
      "service", "hello"
    });
  });

  app.get("/hello/{name}", [](Request &req, Response &res) {
    const std::string name = req.param("name");

    res.json({
      "greeting", "Hello " + name,
      "powered_by", "Vix.cpp"
    });
  });

  app.get("/users/{id}", [](Request &req, Response &res) {
    const std::string id = req.param("id");
    const std::string page = req.query_value("page", "1");
    const std::string limit = req.query_value("limit", "10");

    res.json({
      "ok", true,
      "id", id,
      "page", page,
      "limit", limit
    });
  });

  app.run();

  return 0;
}
```

Run it:

```bash
vix dev
```

Test all routes:

```bash
curl -i http://127.0.0.1:8080/
curl -i http://127.0.0.1:8080/health
curl -i http://127.0.0.1:8080/hello/Gaspard
curl -i "http://127.0.0.1:8080/users/42?page=2&limit=20"
```

## Organize routes with functions

As the application grows, avoid placing every route directly inside `main()`.

You can group related routes in functions:

```cpp
#include <vix.hpp>

using namespace vix;

static void register_public_routes(App &app)
{
  app.get("/", [](Request &, Response &res) {
    res.json({
      "message", "Hello from Vix.cpp",
      "framework", "Vix.cpp"
    });
  });

  app.get("/health", [](Request &, Response &res) {
    res.json({
      "ok", true,
      "service", "hello"
    });
  });
}

static void register_user_routes(App &app)
{
  app.get("/users/{id}", [](Request &req, Response &res) {
    const std::string id = req.param("id");
    const std::string page = req.query_value("page", "1");

    res.json({
      "ok", true,
      "id", id,
      "page", page
    });
  });
}

int main()
{
  App app;

  register_public_routes(app);
  register_user_routes(app);

  app.run();

  return 0;
}
```

This keeps `main()` small and makes the application easier to maintain.

For larger backends, use the backend template instead of growing a single file indefinitely:

```bash
vix new api --template backend
```

## Common mistakes

### Forgetting to start the server

Routes do nothing until the application starts:

```cpp
app.run();
```

### Hardcoding the port

Avoid this in normal projects:

```cpp
app.run(8080);
```

Prefer:

```cpp
app.run();
```

Then configure the port in `.env`:

```dotenv
SERVER_PORT=8080
```

### Forgetting to return after an error

When you send an error response inside a route, return immediately.

```cpp
app.get("/users/{id}", [](Request &req, Response &res) {
  const std::string id = req.param("id");

  if (id == "0")
  {
    res.status(404).json({
      "ok", false,
      "error", "user not found"
    });

    return;
  }

  res.json({
    "ok", true,
    "id", id
  });
});
```

Without `return`, the handler may continue and try to send another response.

### Port 8080 is already in use

If the server cannot start, change the port in `.env`:

```dotenv
SERVER_PORT=3000
```

Then run again:

```bash
vix dev
```

### Running from the wrong folder

Run project commands from inside the project directory:

```bash
cd ~/tmp/hello
vix dev
```

## What you should remember

The basic Vix HTTP model is:

```txt
App -> route -> Request -> Response -> app.run()
```

The minimal server is:

```cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request &, Response &res) {
    res.text("Hello from Vix.cpp");
  });

  app.run();

  return 0;
}
```

Configuration stays outside the code:

```dotenv
SERVER_PORT=8080
```

Use development mode while editing:

```bash
vix dev
```

Use direct run mode when you simply want to start the app:

```bash
vix run
```

## You finished Getting Started

You now know how to:

- verify your Vix.cpp environment
- run a single C++ file
- create a simple Vix project
- configure a server with `.env`
- build and run your first HTTP server
- define routes
- return text and JSON
- read route parameters
- read query parameters

From here, continue with the path that matches what you want to build next:

- [Project Templates](/templates/)
- [The Vix Book](/book/01-introduction)
- [Build a REST API](/guides/build-rest-api)
- [C++ Runtime](/guides/cpp-runtime)
- [C++ Developer Toolkit](/guides/cpp-developer-toolkit)
