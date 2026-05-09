# First HTTP server

Now you will build your first HTTP server and understand the core Vix application model:
`App`,
`Request`,
`Response`,
`routes`,
and `server startup`.

## The smallest HTTP server

```cpp
#include <vix.hpp>
using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request &, Response &res){
    res.send("Hello world");
  });

  app.run(8080);
  return 0;
}
```

```bash
vix dev
curl -i http://127.0.0.1:8080/
```

## Core concepts

| Part | Purpose |
|------|---------|
| `#include <vix.hpp>` | Import Vix |
| `using namespace vix;` | Use `App`, `Request`, `Response` without prefix |
| `App app;` | Create the application |
| `app.get("/", handler)` | Register a GET route |
| `app.run(8080)` | Start the server |

**Request** — reads what the client sent: path params, query params, headers, body, JSON body.

**Response** — sends what your app returns: text, JSON, files, status codes, headers.

## Response methods

```cpp
res.text("Hello Vix");                    // plain text
res.json({"message", "Hello"});           // JSON
res.status(201).json({"ok", true});       // with status code
res.header("Cache-Control", "no-cache"); // set header
res.file("public/index.html");            // serve file
```

## Path parameters

```cpp
app.get("/hello/{name}", [](Request &req, Response &res){
  const std::string name = req.param("name");
  res.json({
    "greeting", "Hello " + name,
    "powered_by", "Vix.cpp"
  });
});
```

```bash
curl -i http://127.0.0.1:8080/hello/Gaspard
```

## Multiple path parameters

```cpp
app.get("/posts/{year}/{slug}", [](Request &req, Response &res){
  const std::string year = req.param("year");
  const std::string slug = req.param("slug");
  res.json({
    "year", year,
    "slug", slug,
    "title", "Post: " + slug
  });
});
```

## Query parameters

```cpp
app.get("/users/{id}", [](Request &req, Response &res){
  const std::string id = req.param("id");
  const std::string page = req.query_value("page", "1");
  const std::string limit = req.query_value("limit", "20");

  res.json({
    "id", id,
    "page", page,
    "limit", limit});
});
```

```bash
curl -i "http://127.0.0.1:8080/users/42?page=2&limit=10"
```

## Status codes

```cpp
app.get("/not-found", [](Request &, Response &res){
  res.status(404).json({
    "ok", false,
    "error", "not found"
  });
});
```

| Status | Meaning               |
|--------|-----------------------|
| 200    | OK                    |
| 201    | Created               |
| 400    | Bad Request           |
| 401    | Unauthorized          |
| 403    | Forbidden             |
| 404    | Not Found             |
| 429    | Too Many Requests     |
| 500    | Internal Server Error |

## Keep routes organized

```cpp
int main()
{
  App app;

  app.get("/", [](Request &, Response &res){
    res.json({"message", "Hello from Vix"});
  });

  app.get("/health", [](Request &, Response &res){
    res.json({"ok", true});
  });

  app.run(8080);
  return 0;
}
```

## Run with config

```cpp
int main()
{
  config::Config cfg{".env"};
  App app;

  app.get("/", [](Request &, Response &res){
    res.json({"message", "Hello from Vix"});
  });

  app.get("/health", [](Request &, Response &res){
    res.json({"ok", true});
  });

  app.run(cfg.getServerPort());    // port from .env
  return 0;
}
```

## Alternative: `app.run(cfg)`

```cpp
app.run(cfg);    // reads server config directly from Config (port, TLS, etc.)
```

## Listen in the background

```cpp
app.listen(8080, [](){
  console.log("Server ready");
});

app.wait();
```

## Dynamic port

```cpp
app.listen_port(0, [](int port){
  console.log("Listening on http://localhost:", port);
});

app.wait();
```

## Complete example

```cpp
#include <vix.hpp>
using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request &, Response &res){
    res.json({
      "message", "Hello from your first Vix HTTP server",
      "framework", "Vix.cpp"
    });
  });

  app.get("/health", [](Request &, Response &res){
    res.json({
      "ok", true, "service",
      "first-http-server"
    });
  });

  app.get("/hello/{name}", [](Request &req, Response &res){
    res.json({
      "greeting", "Hello " + req.param("name"),
      "powered_by", "Vix.cpp"
    });
  });

  app.get("/users/{id}", [](Request &req, Response &res){
    res.json({
      "id", req.param("id"),
      "page", req.query_value("page", "1"),
      "limit", req.query_value("limit", "20")
    });
  });

  app.get("/not-found", [](Request &, Response &res){
    res.status(404).json({
      "ok", false,
      "error", "not found"
    });
  });

  app.run(8080);

  return 0;
}
```

```bash
vix dev
curl -i http://127.0.0.1:8080/
curl -i http://127.0.0.1:8080/health
curl -i http://127.0.0.1:8080/hello/Gaspard
curl -i "http://127.0.0.1:8080/users/42?page=2&limit=10"
```

## Common mistakes

### Forgetting `app.run(...)`

Routes won't start without it.

### Forgetting to return after an error

```cpp
// Wrong
if (bad) {
  res.status(400).json({"error", "bad request"});
}

res.json({"ok", true});

// Correct
if (bad) {
  res.status(400).json({"error", "bad request"});
  return;
}

res.json({"ok", true});
```

### Confusing path params and query params

```cpp
// Path param: /users/{id}
req.param("id")

// Query param: /users/42?page=2
req.query_value("page", "1")
```

## What you should remember

```txt
App registers routes.
Request reads input.
Response sends output.
app.run starts the server.
```

## Next chapter

[Next: Routes](/book/08-routes)
