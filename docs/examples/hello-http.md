# Hello HTTP

The smallest useful Vix HTTP server.

```txt
main.cpp → vix run → HTTP server
```

## What you will build

```txt
GET /              → text response
GET /health        → JSON health check
GET /hello         → JSON greeting
GET /hello/{name}  → route parameter
```

## Create a workspace

```bash
mkdir -p ~/tmp/vix-examples/hello-http
cd ~/tmp/vix-examples/hello-http
touch main.cpp
```

## Full code

```cpp
#include <vix.hpp>
using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request &, Response &res){
    res.text("Hello from Vix");
  });

  app.get("/health", [](Request &, Response &res){
    res.json({
      "ok", true,
      "service", "hello-http"
    });
  });

  app.get("/hello", [](Request &, Response &res){
    res.json({
      "message", "Hello from Vix",
      "framework", "Vix.cpp"
    });
  });

  app.get("/hello/{name}", [](Request &req, Response &res){
    const std::string name = req.param("name");
    res.json({
      "message", "Hello " + name,
      "framework", "Vix.cpp"
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
curl -i http://127.0.0.1:8080/
curl -i http://127.0.0.1:8080/health
curl -i http://127.0.0.1:8080/hello
curl -i http://127.0.0.1:8080/hello/Ada
```

Expected responses:
```
GET /        → "Hello from Vix"
GET /health  → { "ok": true, "service": "hello-http" }
GET /hello   → { "message": "Hello from Vix", "framework": "Vix.cpp" }
GET /hello/Ada → { "message": "Hello Ada", "framework": "Vix.cpp" }
```

## Code explanation

```cpp
App app;           // HTTP application object
app.get("/", ...); // register route for GET /
app.run(8080);     // start server, block until stopped

req.param("name")  // reads {name} from path
res.text("...")     // sends plain text
res.json({...})     // sends JSON
```

Why use `register_routes`: keeps `main()` clean and readable as the app grows.

## Extend it

```cpp
// Add a version route
app.get("/version", [](Request &, Response &res){
  res.json({
    "name", "hello-http",
    "version", "2.5.2"
  });
});

// Change port
app.run(9090);
```

## Common mistakes

```cpp
app.get("health", handler);   // Wrong — missing leading slash
app.get("/health", handler);  // Correct

// Wrong — req not named but used
app.get("/hello/{name}", [](Request &, Response &res) {
  req.param("name");
});
// Correct
app.get("/hello/{name}", [](Request &req, Response &res) {
  req.param("name");
});
```

## What you should remember

```cpp
#include <vix.hpp>
using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request &, Response &res) {
    res.text("Hello from Vix");
  });

  app.run(8080);
  return 0;
}
```

`App` registers routes and starts the server. `Request` reads what the client sent. `Response` sends what your app returns.

The core idea: **create App → register routes → run server.**

Next: [JSON API](/examples/json-api)
