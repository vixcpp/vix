`vix::App` is the main entry point for building an HTTP server with Vix.

It owns and wires together:

- Router (routes, middleware, groups)
- HTTP server (listen loop, request dispatch)
- Execution context (executor used to run handlers)

---

## Quick Start

```cpp
#include <vix.hpp>
using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request&, Response& res){
    res.send("Hello Vix");
  });

  app.run(8080);
}
```

---

## Lifecycle

### `run(port)`

Starts the server and blocks the current thread until the server stops.

```cpp
App app;
app.run(8080);
```

### `listen(port, on_listen)`

Starts the server in a background thread and returns immediately.

```cpp
App app;

app.listen(8080, [](){
  vix::console.info("server is ready");
});

// ... do other work ...
app.wait();
```

### `close()` and `wait()`

`close()` requests a graceful stop.
`wait()` joins the background thread when `listen()` was used.

```cpp
app.close();
app.wait();
```

---

## Routing

App exposes HTTP verb helpers that register routes on the internal router:

- `get`, `post`, `put`, `patch`, `del`, `head`, `options`
- `get_heavy`, `post_heavy` (mark a route as heavy work)

```cpp
App app;

app.get("/health", [](Request&, Response& res){
  res.json({ "ok", true });
});

app.post("/items", [](Request&, Response& res){
  res.status(201).send("created");
});
```

### Handler Signatures

Vix accepts two handler styles:

1. Facade handler (beginner friendly)

```cpp
(Request&, Response&)
```

2. Raw handler (advanced)

```cpp
(const vix::vhttp::RawRequest&, Response&)
```

At compile time, Vix validates the handler type via `static_assert`.

---

## Auto-send Behavior

A handler may either:

- return `void` and send explicitly (`res.send()`, `res.json()`, ...)
- or return a value and let Vix auto-send it if the response is still empty

Example:

```cpp
app.get("/ping", [](Request&, Response&){
  return "pong";
});
```

If the handler returns a value, Vix only auto-sends it when no body and no `Content-Length`
were set yet.

---

## Middleware

Middlewares can be attached:

- globally with `use(mw)`
- to a prefix with `use("/api", mw)` or `protect("/api", mw)`
- to an exact path with `protect_exact("/admin", mw)`

Signature:

```cpp
using Next = std::function<void()>;
using Middleware = std::function<void(Request&, Response&, Next)>;
```

Example:

```cpp
App app;

app.use([](Request& req, Response& res, App::Next next){
  vix::console.info(req.method(), req.path());
  next();
});
```

Note: `vix::console` is intended for developer-facing output. For production logging,
persistence, and structured pipelines, use `vix::utils::Logger` from `modules/utils`.

---

## Groups

Groups prefix routes and share middleware registration.

```cpp
App app;

app.group("/api", [&](App::Group& g){
  g.use(auth_mw);

  g.get("/users", users_handler);
  g.post("/users", create_user_handler);
});
```

Groups can be nested.

---

## Static Files

`static_dir()` serves a directory of files under a mount path.

```cpp
app.static_dir("public", "/");
```

Behavior is implemented via a pluggable global static handler:

- `App::set_static_handler(...)`

This design allows swapping static-file implementations without changing the public API.

---

## Dev Mode

Dev mode is an application flag that can be toggled:

```cpp
app.setDevMode(true);
if (app.isDevMode()) {
  vix::console.debug("dev mode");
}
```

---

## Executor and Concurrency

Handlers are executed using an `IExecutor`.

- Default executor is provided by the runtime.
- Advanced users can inject a custom executor:

```cpp
auto exec = std::make_shared<MyExecutor>();
App app(exec);
```

The executor is shared by the HTTP server and used to run request handlers.

---

## Signal-safe Stop

`request_stop_from_signal()` is designed for SIGINT/SIGTERM handlers.

It only sets flags and avoids heavy work, to remain safe in a signal context.

---

## Introspection

`server_ready_info()` returns the last ready info captured when binding succeeded.
This is useful when binding to port `0` (ephemeral port).

```cpp
app.listen_port(0, [](int bound){
  vix::console.info("bound port:", bound);
});
```

---

## Notes

- `App` is non-copyable and non-movable.
- An OPTIONS route is automatically ensured for each non-OPTIONS route path.
- Middleware chains are collected per path and executed in order before the final handler.

