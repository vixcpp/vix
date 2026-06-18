# vix::App

`vix::App` is the main application object in Vix Core.

Use it to configure routes, middleware, templates, static files, server lifecycle, and application shutdown.

## Public header

```cpp
#include <vix.hpp>
```

You can also include the app header directly:

```cpp
#include <vix/app/App.hpp>
```

## What App provides

`App` is the high-level entry point for building a Vix HTTP application.

It provides:

- route registration
- route groups
- middleware
- protected route prefixes
- static file mounting
- template configuration
- server startup
- server shutdown
- access to the router
- access to the HTTP server
- access to the runtime executor
- access to configuration

Most applications only need to create one `App`.

```cpp
vix::App app;
```

## Minimal app

```cpp
#include <vix.hpp>

int main()
{
  vix::App app;

  app.get("/", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.text("Hello from Vix");
  });

  app.run(8080);

  return 0;
}
```

Run:

```bash
vix run main.cpp
```

Then open:

```text
http://localhost:8080
```

Expected response:

```text
Hello from Vix
```

## Create an app

Create an app with the default executor:

```cpp
vix::App app;
```

The default app owns:

```text
Config
Router
RuntimeExecutor
HTTPServer
```

The default constructor prepares the router, executor, HTTP server, logs, docs routes, access logs, static fallback, and internal benchmark route.

## Register routes

Use the HTTP method helpers to register routes.

```cpp
app.get("/", handler);
app.post("/users", handler);
app.put("/users/{id}", handler);
app.patch("/users/{id}", handler);
app.del("/users/{id}", handler);
app.head("/status", handler);
app.options("/status", handler);
```

A route handler receives:

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

  app.get("/status", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.json({
      "status", "ok"
    });
  });

  app.run(8080);

  return 0;
}
```

## Route parameters

Use `{name}` in a route path to capture a path parameter.

```cpp
#include <vix.hpp>

int main()
{
  vix::App app;

  app.get("/users/{id}", [](vix::Request &req, vix::Response &res)
  {
    const std::string id = req.param("id");

    res.json({
      "id", id
    });
  });

  app.run(8080);

  return 0;
}
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

## Return text

Use `res.text(...)`.

```cpp
app.get("/", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.text("Home");
});
```

## Return JSON

Use `res.json(...)`.

```cpp
app.get("/api/status", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.json({
    "status", "ok",
    "runtime", "Vix.cpp"
  });
});
```

## Set status

Use `res.status(...)`.

```cpp
app.get("/forbidden", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.status(403).json({
    "error", "forbidden"
  });
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

## Register middleware

Use `app.use(...)` to register global middleware.

```cpp
#include <vix.hpp>

int main()
{
  vix::App app;

  app.use([](vix::Request &req, vix::Response &res, vix::App::Next next)
  {
    (void)res;

    vix::print("request:", req.method(), req.path());

    next();
  });

  app.get("/", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.text("home");
  });

  app.run(8080);

  return 0;
}
```

Middleware receives:

```cpp
vix::Request &req
vix::Response &res
vix::App::Next next
```

Call `next()` to continue to the next middleware or final route handler.

## Prefix middleware

Use `app.use(prefix, middleware)` to apply middleware only to a path prefix.

```cpp
app.use("/admin", [](vix::Request &req, vix::Response &res, vix::App::Next next)
{
  const std::string token = req.header("Authorization");

  if (token.empty())
  {
    res.status(401).json({
      "error", "missing authorization header"
    });
    return;
  }

  next();
});
```

This middleware applies to:

```text
/admin
/admin/users
/admin/settings
```

It does not apply to:

```text
/api
/users
```

## Protect a prefix

Use `protect(...)` as a clearer name for prefix middleware.

```cpp
app.protect("/admin", [](vix::Request &req, vix::Response &res, vix::App::Next next)
{
  const bool allowed = true;

  if (!allowed)
  {
    res.status(403).json({
      "error", "forbidden"
    });
    return;
  }

  next();
});
```

## Protect an exact path

Use `protect_exact(...)` when the middleware should apply only to one path.

```cpp
app.protect_exact("/admin/settings", [](vix::Request &req, vix::Response &res, vix::App::Next next)
{
  (void)req;

  const bool allowed = true;

  if (!allowed)
  {
    res.status(403).text("Forbidden");
    return;
  }

  next();
});
```

This applies only to:

```text
/admin/settings
```

## Route groups

Use `group(...)` to organize routes under a shared prefix.

```cpp
#include <vix.hpp>

int main()
{
  vix::App app;

  app.group("/api", [](auto &api)
  {
    api.get("/status", [](vix::Request &req, vix::Response &res)
    {
      (void)req;

      res.json({"status", "ok"});
    });

    api.post("/users", [](vix::Request &req, vix::Response &res)
    {
      (void)req;

      res.status(201).json({"created", true});
    });
  });

  app.run(8080);

  return 0;
}
```

The routes become:

```text
GET /api/status
POST /api/users
```

## Nested groups

Groups can be nested.

```cpp
app.group("/api", [](auto &api)
{
  api.group("/v1", [](auto &v1)
  {
    v1.get("/status", [](vix::Request &req, vix::Response &res)
    {
      (void)req;

      res.json({"status", "ok"});
    });
  });
});
```

The route becomes:

```text
GET /api/v1/status
```

## Group middleware

Middleware can be attached to a group.

```cpp
app.group("/admin", [](auto &admin)
{
  admin.use([](vix::Request &req, vix::Response &res, vix::App::Next next)
  {
    const bool allowed = true;

    if (!allowed)
    {
      res.status(403).text("Forbidden");
      return;
    }

    next();
  });

  admin.get("/dashboard", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.text("Admin dashboard");
  });
});
```

The middleware applies to routes under `/admin`.

## Heavy routes

Use `get_heavy` or `post_heavy` for routes that may be CPU-heavy or database-heavy.

```cpp
app.get_heavy("/reports", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.json({
    "report", "ready"
  });
});
```

Heavy routes are registered with route metadata.

This allows the router and executor model to classify expensive routes separately when needed.

## Static files

Use `static_dir(...)` to mount a directory.

```cpp
#include <vix.hpp>

int main()
{
  vix::App app;

  app.static_dir("public", "/");

  app.run(8080);

  return 0;
}
```

Example:

```text
GET /style.css
```

can serve:

```text
public/style.css
```

Mount static files under a prefix:

```cpp
app.static_dir("public/assets", "/assets");
```

Example:

```text
GET /assets/app.css
```

can serve:

```text
public/assets/app.css
```

## Static file options

`static_dir` accepts more options.

```cpp
app.static_dir(
    "public",
    "/",
    "index.html",
    true,
    "public, max-age=3600",
    true);
```

Arguments:

```text
root
mount
index_file
add_cache_control
cache_control
fallthrough
```

## Templates

Use `templates(...)` to configure a template directory.

```cpp
#include <vix.hpp>

int main()
{
  vix::App app;

  app.templates("views");

  app.get("/", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    vix::tmpl::Context ctx;
    ctx.set("title", "Home");

    res.render("index.html", ctx);
  });

  app.run(8080);

  return 0;
}
```

`templates(...)` initializes:

```text
FileSystemLoader
Template Engine
TemplateView
```

## Check template support

Use `has_views()` to check whether templates are configured.

```cpp
if (app.has_views())
{
  vix::print("templates are ready");
}
```

Use `views()` to access the template view facade.

```cpp
auto &views = app.views();
```

If `templates(...)` was not called, `views()` throws.

## Run the app

Use `run(...)` to start the server and block until shutdown.

```cpp
app.run(8080);
```

Equivalent flow:

```text
listen
wait
close
```

## Listen without blocking

Use `listen(...)` to start the server asynchronously.

```cpp
#include <vix.hpp>

int main()
{
  vix::App app;

  app.get("/", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.text("hello");
  });

  app.listen(8080);

  app.wait();

  return 0;
}
```

`listen(...)` starts the HTTP server in a separate thread.

`wait()` keeps the application alive until stop is requested.

## Listen callback

You can pass a callback to `listen`.

```cpp
app.listen(8080, []()
{
  vix::print("server is listening");
});
```

Use `listen_port(...)` when the callback needs the bound port.

```cpp
app.listen_port(8080, [](int port)
{
  vix::print("listening on port", port);
});
```

This is useful when the app binds to port `0` and the operating system chooses a port.

## Stop the app

Use `close()` to stop the server.

```cpp
app.close();
```

`close()` requests shutdown, stops the HTTP server, joins the server thread, and marks the app as stopped.

## Shutdown callback

Use `set_shutdown_callback(...)` to run custom shutdown logic.

```cpp
app.set_shutdown_callback([]()
{
  vix::print("shutdown requested");
});
```

## Check running state

Use `is_running()`.

```cpp
if (app.is_running())
{
  vix::print("app is running");
}
```

## Access configuration

Use `config()` to access the app configuration.

```cpp
vix::App app;

app.config().setServerPort(8080);
```

You can also run with an explicit config.

```cpp
vix::config::Config cfg;

cfg.setServerPort(8080);

vix::App app;
app.run(cfg);
```

## Access router

Use `router()` to access the shared router.

```cpp
auto router = app.router();
```

This is useful for advanced integrations and tooling.

## Access server

Use `server()` to access the underlying HTTP server.

```cpp
auto &server = app.server();
```

Most applications do not need direct server access.

## Access executor

Use `executor()` to access the runtime executor.

```cpp
auto &executor = app.executor();
```

This is useful for advanced runtime or metrics integration.

## Development mode

Use `setDevMode(...)` to mark the app as running in development mode.

```cpp
app.setDevMode(true);
```

Check it with:

```cpp
if (app.isDevMode())
{
  vix::print("dev mode");
}
```

Development mode affects the runtime banner mode shown when the server starts.

## App lifecycle

The typical lifecycle is:

```text
construct App
register routes
register middleware
configure templates or static files
run or listen
wait
close
```

`run(...)` handles the common blocking lifecycle.

```cpp
app.run(8080);
```

`listen(...)` gives more control.

```cpp
app.listen(8080);
app.wait();
app.close();
```

## Complete example

```cpp
#include <vix.hpp>

int main()
{
  vix::App app;

  app.use([](vix::Request &req, vix::Response &res, vix::App::Next next)
  {
    (void)res;

    vix::print(req.method(), req.path());

    next();
  });

  app.get("/", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.text("Welcome to Vix");
  });

  app.get("/users/{id}", [](vix::Request &req, vix::Response &res)
  {
    res.json({
      "id", req.param("id")
    });
  });

  app.group("/api", [](auto &api)
  {
    api.get("/status", [](vix::Request &req, vix::Response &res)
    {
      (void)req;

      res.json({
        "status", "ok",
        "server", "Vix.cpp"
      });
    });
  });

  app.static_dir("public", "/assets");

  app.run(8080);

  return 0;
}
```

## API summary

| API                               | Purpose                                    |
| --------------------------------- | ------------------------------------------ |
| `App()`                           | Create an app with the default executor.   |
| `run(port)`                       | Start the server and block until shutdown. |
| `listen(port)`                    | Start the server asynchronously.           |
| `wait()`                          | Wait until shutdown is requested.          |
| `close()`                         | Stop the server and release resources.     |
| `get(path, handler)`              | Register a GET route.                      |
| `post(path, handler)`             | Register a POST route.                     |
| `put(path, handler)`              | Register a PUT route.                      |
| `patch(path, handler)`            | Register a PATCH route.                    |
| `del(path, handler)`              | Register a DELETE route.                   |
| `head(path, handler)`             | Register a HEAD route.                     |
| `options(path, handler)`          | Register an OPTIONS route.                 |
| `get_heavy(path, handler)`        | Register a heavy GET route.                |
| `post_heavy(path, handler)`       | Register a heavy POST route.               |
| `use(middleware)`                 | Register global middleware.                |
| `use(prefix, middleware)`         | Register prefix middleware.                |
| `protect(prefix, middleware)`     | Protect a prefix.                          |
| `protect_exact(path, middleware)` | Protect one exact path.                    |
| `group(prefix, fn)`               | Register grouped routes.                   |
| `static_dir(root, mount)`         | Mount static files.                        |
| `templates(directory)`            | Configure template rendering.              |
| `has_views()`                     | Check whether templates are configured.    |
| `views()`                         | Access the template view.                  |
| `config()`                        | Access app configuration.                  |
| `router()`                        | Access the router.                         |
| `server()`                        | Access the HTTP server.                    |
| `executor()`                      | Access the runtime executor.               |
| `setDevMode(value)`               | Enable or disable development mode.        |
| `is_running()`                    | Check whether the app is running.          |
| `set_shutdown_callback(cb)`       | Register shutdown logic.                   |

## Next steps

Read the next pages:

- [Architecture](./architecture.md)
- [Routing](./routing.md)
- [Handlers](./handlers.md)
- [Middleware](./middleware.md)
- [Static files](./static-files.md)
- [Templates](./templates.md)
- [HTTP server](./http-server.md)
