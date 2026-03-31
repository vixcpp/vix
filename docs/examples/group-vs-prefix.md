# Groups vs Prefix Middleware

This guide explains the difference between:

- Prefix middleware: `app.use("/api", mw)` and `middleware::app::install(app, "/api/", mw)`
- Route groups: `app.group("/api", ...)` and `api.use(...)` / `api.protect(...)`

Both are valid. The best choice depends on how you want to structure your routes.

------------------------------------------------------------------------

## 1) Mental model

### Prefix middleware
You attach middleware to a path prefix.

- The middleware runs for every route whose path starts with that prefix.
- You usually keep route registration at the top level `app.get(...)`, `app.post(...)`.

Example:
- Apply API key to everything under `/api/`

### Groups
A group is a scoped router builder.

- You register routes inside a `group("/api", ...)` block.
- You can install middleware once on the group and it applies to routes in that group.
- You can nest groups to model your API structure.

Example:
- `/api` group contains public routes
- `/api/admin` group contains protected admin routes

------------------------------------------------------------------------

## 2) Prefix middleware patterns

### 2.1) Protect a whole prefix with `app.use()`
This is the simplest prefix pattern.

```cpp
#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>
using namespace vix;

int main()
{
  App app;

  // Everything under /api is protected by API key
  app.use("/api", middleware::app::api_key_dev("secret"));

  app.get("/", [](Request&, Response& res){
    res.send("home");
  });

  app.get("/api/ping", [](Request&, Response& res){
    res.json({"ok", true, "msg", "pong"});
  });

  app.run(8080);
  return 0;
}
```

Try:

```bash
curl -i http://127.0.0.1:8080/api/ping
curl -i -H "x-api-key: secret" http://127.0.0.1:8080/api/ping
```

### 2.2) Protect a prefix with `middleware::app::install()`
This is useful when you want to be explicit about routing helpers.

```cpp
#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>
using namespace vix;

int main()
{
  App app;

  middleware::app::install(app, "/api/", middleware::app::api_key_dev("secret"));

  app.get("/api/users", [](Request&, Response& res){
    res.json({"ok", true, "data", json::array({"u1","u2"})});
  });

  app.run(8080);
  return 0;
}
```

### 2.3) Protect only one exact route
Use `install_exact` if you want a single route protected, not the whole prefix.

```cpp
#include <vix.hpp>
#include <vix/middleware/app/adapter.hpp>
using namespace vix;

static vix::middleware::HttpMiddleware require_header(std::string h, std::string v)
{
  return [h = std::move(h), v = std::move(v)](Request& req, Response& res, vix::middleware::Next next)
  {
    if (req.header(h) != v)
    {
      res.status(401).json(vix::json::obj({"ok", false, "error", "unauthorized"}));
      return;
    }
    next();
  };
}

int main()
{
  App app;

  middleware::app::install_exact(app, "/api/ping",
                                middleware::app::adapt(require_header("x-demo","1")));

  app.get("/api/ping", [](Request&, Response& res){
    res.json({"ok", true});
  });

  app.run(8080);
  return 0;
}
```

------------------------------------------------------------------------

## 3) Group patterns

### 3.1) Group with a protected sub-path using `protect()`
This makes the intent very clear: only that sub-path is protected.

```cpp
#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>
using namespace vix;

int main()
{
  App app;

  app.group("/api", [&](App::Group& api)
  {
    api.get("/public", [](Request&, Response& res){
      res.send("public");
    });

    api.protect("/secure", middleware::app::api_key_dev("secret"));

    api.get("/secure", [](Request& req, Response& res){
      auto& k = req.state<vix::middleware::auth::ApiKey>();
      res.json({"ok", true, "api_key", k.value});
    });
  });

  app.run(8080);
  return 0;
}
```

Try:

```bash
curl -i http://127.0.0.1:8080/api/public
curl -i http://127.0.0.1:8080/api/secure
curl -i -H "x-api-key: secret" http://127.0.0.1:8080/api/secure
```

### 3.2) Group builder style (return value)
This is compact for simple APIs.

```cpp
#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>
using namespace vix;

int main()
{
  App app;

  auto api = app.group("/api");

  api.get("/public", [](Request&, Response& res){
    res.send("public");
  });

  api.use(middleware::app::api_key_dev("secret"));

  api.get("/secure", [](Request&, Response& res){
    res.json({"ok", true});
  });

  app.run(8080);
  return 0;
}
```

### 3.3) Nested groups for clean structure
This is where groups shine the most.

- `/api` public stuff
- `/api/admin` secured stuff
- install JWT + RBAC once on the admin group

```cpp
#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>
using namespace vix;

int main()
{
  App app;

  app.group("/api", [&](App::Group& api)
  {
    api.get("/health", [](Request&, Response& res){
      res.json({"ok", true});
    });

    api.group("/admin", [&](App::Group& admin)
    {
      admin.use(middleware::app::jwt_auth("dev_secret"));
      admin.use(middleware::app::rbac_admin());

      admin.get("/dashboard", [](Request& req, Response& res)
      {
        auto& authz = req.state<vix::middleware::auth::Authz>();
        res.json({"ok", true, "sub", authz.subject, "role", "admin"});
      });
    });
  });

  app.run(8080);
  return 0;
}
```

------------------------------------------------------------------------

## 4) When to use which

### Use prefix middleware when
- You already have a flat route list and want a quick protection layer.
- You want one line like `app.use("/api", ...)` and keep everything else unchanged.
- You want to attach cross cutting middleware globally or by prefix easily.

### Use groups when
- You want your route tree to look like your API structure.
- You want nested scopes like `/api/v1`, `/api/admin`, `/api/internal`.
- You want to install auth once at the right level, not repeat `use("/api/admin", ...)` patterns.
- You want the code to read like: "inside admin, everything is protected".

Practical rule:
- Small apps: prefix is fastest to write.
- Medium and large apps: groups scale better, especially with nesting.

------------------------------------------------------------------------

## 5) Common mistakes

1) Mixing prefix strings inconsistently
- `/api` vs `/api/` can lead to confusion. Pick one style and keep it consistent.

2) Installing auth on the wrong level
- If you do `app.use("/api", jwt)` you may protect routes that should be public.
- With groups, you can isolate `admin.use(jwt)` only inside `/api/admin`.

3) Duplicating middleware per route
- Prefer a group or prefix so you do not repeat the same install code everywhere.

------------------------------------------------------------------------

# Complete Example (copy paste)

This single file demonstrates both approaches side by side:
- `/p/*` uses prefix protection
- `/g/*` uses groups with protect and nested auth

Save as: `group_vs_prefix_demo.cpp`

```cpp
#include <iostream>
#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>

using namespace vix;

static void install_prefix_routes(App& app)
{
  // Prefix protected section
  app.use("/p/secure", middleware::app::api_key_dev("secret"));

  app.get("/p/public", [](Request&, Response& res){
    res.send("prefix public");
  });

  app.get("/p/secure/who", [](Request& req, Response& res){
    auto& k = req.state<vix::middleware::auth::ApiKey>();
    res.json({"ok", true, "mode", "prefix", "api_key", k.value});
  });
}

static void install_group_routes(App& app)
{
  app.group("/g", [&](App::Group& g)
  {
    g.get("/public", [](Request&, Response& res){
      res.send("group public");
    });

    g.protect("/secure", middleware::app::api_key_dev("secret"));

    g.get("/secure/who", [](Request& req, Response& res){
      auto& k = req.state<vix::middleware::auth::ApiKey>();
      res.json({"ok", true, "mode", "group", "api_key", k.value});
    });

    g.group("/admin", [&](App::Group& admin)
    {
      admin.use(middleware::app::jwt_auth("dev_secret"));
      admin.use(middleware::app::rbac_admin());

      admin.get("/dashboard", [](Request& req, Response& res)
      {
        auto& authz = req.state<vix::middleware::auth::Authz>();
        res.json({"ok", true, "mode", "group_admin", "sub", authz.subject});
      });
    });
  });
}

int main()
{
  App app;

  app.get("/", [](Request&, Response& res){
    res.send(
      "Try:\n"
      "  /p/public\n"
      "  /p/secure/who (needs x-api-key: secret)\n"
      "  /g/public\n"
      "  /g/secure/who (needs x-api-key: secret)\n"
      "  /g/admin/dashboard (needs JWT admin token)\n"
    );
  });

  install_prefix_routes(app);
  install_group_routes(app);

  std::cout
    << "Running:\n"
    << "  http://localhost:8080/\n"
    << "  http://localhost:8080/p/public\n"
    << "  http://localhost:8080/p/secure/who\n"
    << "  http://localhost:8080/g/public\n"
    << "  http://localhost:8080/g/secure/who\n"
    << "  http://localhost:8080/g/admin/dashboard\n\n"
    << "API key: secret\n";

  app.run(8080);
  return 0;
}
```

Run:

```bash
vix run group_vs_prefix_demo.cpp
```

Quick tests:

```bash
curl -i http://localhost:8080/p/public
curl -i http://localhost:8080/p/secure/who
curl -i -H "x-api-key: secret" http://localhost:8080/p/secure/who

curl -i http://localhost:8080/g/public
curl -i http://localhost:8080/g/secure/who
curl -i -H "x-api-key: secret" http://localhost:8080/g/secure/who
```

