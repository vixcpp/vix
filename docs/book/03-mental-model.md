# Mental model

This chapter explains how to think about Vix.

Before learning every command and module, you need one clear picture:

```txt
Vix is a runtime workflow for C++ applications.
```

It connects four layers:

```txt
CLI
  ↓
Runtime
  ↓
Application
  ↓
Modules
```

The CLI helps you work.
The runtime runs your code.
The application layer exposes APIs such as `App`, `Request`, and `Response`.
The modules add capabilities like JSON, database, middleware, validation, WebSocket, cache, sync, and P2P.

## The simplest mental model

```cpp
#include <vix.hpp>
using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request &, Response &res){
    res.text("Hello Vix");
  });

  app.run(8080);
  return 0;
}
```

```bash
vix run main.cpp
```

This example contains most of the Vix mental model:
- `vix run` → CLI workflow,
- `App` → application object,
- `app.get` → route registration,
- `Request` → incoming request,
- `Response` → outgoing response,
- `app.run` → runtime starts the server.

## Layer 1: The CLI

The CLI is the developer entrypoint.
It provides commands such as:
`vix run`,
`vix new`,
`vix dev`,
`vix build`,
`vix check`,
`vix tests`,
`vix fmt`,
`vix doctor`.

It gives a consistent development loop: create → run → edit → reload → check → test → build → deploy.

| Command     | Purpose                                      |
|-------------|----------------------------------------------|
| `vix run`   | Builds and runs a file, project, or manifest.|
| `vix dev`   | Starts a development loop with watch reload. |
| `vix build` | Configures, compiles, and links the project. |
| `vix check` | Validates builds, tests, and sanitizers.     |

## Layer 2: The runtime

The runtime is what makes the app actually run. In a simple HTTP program:

```cpp
app.run(8080);
```

For advanced apps with HTTP + WebSocket together:

```cpp
struct Runtime
{
  vix::config::Config config{".env"};

  std::shared_ptr<vix::executor::RuntimeExecutor> executor{
      std::make_shared<vix::executor::RuntimeExecutor>(1u)
  };

  vix::App app{executor};
  vix::websocket::Server ws{config, executor};
};

vix::run_http_and_ws(runtime.app, runtime.ws, runtime.executor, http_port);
```

## Layer 3: The application

```cpp
App app;

app.get("/users/{id}", [](Request &req, Response &res){
  const std::string id = req.param("id");
  res.json({"id", id});
});
```

**Request** is read-only input from the client:
- path params,
- query params,
- headers,
- body,
- JSON body.

**Response** is how the route sends output back:
- text,
- JSON,
- files,
- status codes,
- headers.

### Keep `main()` small

```cpp
int main()
{
  App app;

  app.get("/", [](Request &, Response &res){
    res.json({"message", "Hello"});
  });

  app.run(8080);
  return 0;
}
```

## Layer 4: Modules

```cpp
#include <vix.hpp>           // core
#include <vix/db.hpp>        // database
#include <vix/websocket.hpp> // WebSocket
#include <vix/middleware.hpp>// middleware
#include <vix/validation.hpp>// validation
```

Use the smallest public module header that gives the feature you need.

### Key modules

**Middleware** runs around routes — CORS, rate limiting, authentication, security headers, static files.

**Validation** checks input before business logic:

```cpp
auto result = vix::validation::validate("email", email).required().email().result();
```

**Database** gives explicit access:

```cpp
auto db = vix::db::Database::sqlite("vix.db");
vix::db::PooledConn conn(db.pool());
auto stmt = conn->prepare("SELECT id, name FROM users WHERE id = ?");
stmt->bind(1, id);
```

**WebSocket** adds real-time events:

```cpp
ws.on_typed_message([&ws](auto &, const std::string &type, const auto &payload){
  if (type == "chat.message") ws.broadcast_json("chat.message", payload);
});
```

## Configuration

```cpp
vix::config::Config cfg{".env"};
const int port = cfg.getServerPort();
vix::db::Database db{cfg};
```

Environment variables:
```dotenv
SERVER_PORT=8080
DATABASE_ENGINE=sqlite
DATABASE_DEFAULT_NAME=vix.db
```

## Request lifecycle

```txt
client request → Nginx (production) → Vix runtime → middleware → route handler → Response
```

With database: request → middleware → route → validate → query database → return JSON

## Error flow

```txt
validation failure   → 400 Bad Request
missing token        → 401 Unauthorized
not allowed          → 403 Forbidden
missing resource     → 404 Not Found
too many requests    → 429 Too Many Requests
```

## Application growth

```txt
Start:  src/main.cpp

Later:  src/
        ├── main.cpp
        ├── routes/
        ├── validation/
        ├── database/
        └── services/
```

Start small. Move code into modules when it earns it. Keep `main()` as wiring.

## Production shape

```txt
browser → Nginx → Vix app on localhost → systemd
```

## What you should remember

The Vix mental model has four layers: **CLI** controls the workflow, **Runtime** executes the app, **Application** is built around `App`, `Request`, and `Response`, **Modules** add capabilities.

The best way to grow a Vix app: start with one file, keep `main()` small, register routes through functions, add modules when needed, move logic into services as the app grows.

## Next chapter

[Next: Installation](/book/04-installation)
