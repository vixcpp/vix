# Middleware

This section demonstrates how middleware works in Vix.cpp.

Each example is minimal and self-contained.

---

## 1. Global Middleware

Runs before every route.

```cpp
#include <vix.hpp>
#include <vix/middleware/app/adapter.hpp>

using namespace vix;

int main()
{
  App app;

  app.use(vix::middleware::app::adapt(
    [](Request& req, Response& res, vix::middleware::Next next)
    {
      res.header("x-powered-by", "Vix");
      next();
    }
  ));

  app.get("/", [](Request&, Response& res)
  {
    res.send("Hello with middleware");
  });

  app.run(8080);
  return 0;
}
```

---

## 2. Prefix Middleware

Middleware only applied to a path prefix.

```cpp
#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>

using namespace vix;

int main()
{
  App app;

  using namespace vix::middleware::app;

  install(app, "/api/",
    rate_limit_dev(60, std::chrono::minutes(1))
  );

  app.get("/api/ping", [](Request&, Response& res)
  {
    res.json({"ok", true});
  });

  app.get("/", [](Request&, Response& res)
  {
    res.send("Public route");
  });

  app.run(8080);
  return 0;
}
```

---

## 3. Simple Auth Middleware

Protect a route with a header check.

```cpp
#include <vix.hpp>
#include <vix/middleware/app/adapter.hpp>

using namespace vix;

int main()
{
  App app;

  app.use(vix::middleware::app::adapt(
    [](Request& req, Response& res, vix::middleware::Next next)
    {
      if (req.header("x-api-key") != "secret")
      {
        res.status(401).json({
          "ok", false,
          "error", "Unauthorized"
        });
        return;
      }
      next();
    }
  ));

  app.get("/secure", [](Request&, Response& res)
  {
    res.json({"ok", true});
  });

  app.run(8080);
  return 0;
}
```

Test:

    curl -H "x-api-key: secret" http://localhost:8080/secure

---

## 4. Chaining Middlewares

```cpp
#include <vix.hpp>
#include <vix/middleware/app/adapter.hpp>

using namespace vix;

int main()
{
  App app;

  app.use(vix::middleware::app::adapt(
    [](Request&, Response& res, vix::middleware::Next next)
    {
      res.header("x-a", "1");
      next();
    }
  ));

  app.use(vix::middleware::app::adapt(
    [](Request&, Response& res, vix::middleware::Next next)
    {
      res.header("x-b", "2");
      next();
    }
  ));

  app.get("/", [](Request&, Response& res)
  {
    res.send("Check headers");
  });

  app.run(8080);
  return 0;
}
```

---

## 5. Context-Based Middleware

Using adapt_ctx with Context API.

```cpp
#include <vix.hpp>
#include <vix/middleware/app/adapter.hpp>

using namespace vix;

int main()
{
  App app;

  app.use(vix::middleware::app::adapt_ctx(
    [](vix::middleware::Context& ctx, vix::middleware::Next next)
    {
      ctx.res().header("x-context", "true");
      next();
    }
  ));

  app.get("/", [](Request&, Response& res)
  {
    res.send("Context middleware");
  });

  app.run(8080);
  return 0;
}
```

---

## What this teaches

- Global middleware
- Prefix-based middleware
- Route protection
- Middleware chaining
- Context-based middleware

