# REST API (Minimal Patterns)

This page shows small, focused REST-style examples in Vix.cpp.

Each section is: - One concept - One minimal `main()` - One clear
purpose

------------------------------------------------------------------------

## 1) Basic GET (JSON)

Simple REST endpoint returning JSON.

``` cpp
#include <vix.hpp>
using namespace vix;

int main()
{
  App app;

  app.get("/users", [](Request&, Response& res)
  {
    res.json({
      "data", json::array({
        json::obj({"id", 1, "name", "Ada"}),
        json::obj({"id", 2, "name", "Bob"})
      })
    });
  });

  app.run(8080);
  return 0;
}
```

------------------------------------------------------------------------

## 2) Path Parameter

Access resource by ID.

``` cpp
#include <vix.hpp>
using namespace vix;

int main()
{
  App app;

  app.get("/users/{id}", [](Request& req, Response& res)
  {
    const auto id = req.param("id");

    res.json({
      "id", id,
      "name", "User#" + id
    });
  });

  app.run(8080);
  return 0;
}
```

------------------------------------------------------------------------

## 3) Query Parameters (Pagination)

Typical REST pagination pattern.

``` cpp
#include <vix.hpp>
using namespace vix;

int main()
{
  App app;

  app.get("/users", [](Request& req, Response& res)
  {
    auto page  = req.query_value("page", "1");
    auto limit = req.query_value("limit", "10");

    res.json({
      "page", page,
      "limit", limit
    });
  });

  app.run(8080);
  return 0;
}
```

------------------------------------------------------------------------

## 4) POST with JSON Body

Create resource.

``` cpp
#include <vix.hpp>
using namespace vix;

int main()
{
  App app;

  app.post("/users", [](Request& req, Response& res)
  {
    const auto& body = req.json();

    res.status(201).json({
      "created", true,
      "payload", body
    });
  });

  app.run(8080);
  return 0;
}
```

------------------------------------------------------------------------

## 5) Status Codes

Return custom HTTP status.

``` cpp
#include <vix.hpp>
using namespace vix;

int main()
{
  App app;

  app.get("/not-found", [](Request&, Response& res)
  {
    res.status(404).json({
      "error", "Resource not found"
    });
  });

  app.run(8080);
  return 0;
}
```

------------------------------------------------------------------------

## 6) Headers

Read request headers.

``` cpp
#include <vix.hpp>
using namespace vix;

int main()
{
  App app;

  app.get("/headers", [](Request& req, Response& res)
  {
    res.json({
      "user_agent", req.header("User-Agent"),
      "host", req.header("Host")
    });
  });

  app.run(8080);
  return 0;
}
```

------------------------------------------------------------------------

## 7) Auto-send Return Style

Return value instead of calling `res.json()`.

``` cpp
#include <vix.hpp>
#include <vix/json/json.hpp>
using namespace vix;

int main()
{
  App app;

  app.get("/auto", [](Request&, Response&)
  {
    return vix::json::o(
      "message", "Auto-sent JSON",
      "ok", true
    );
  });

  app.run(8080);
  return 0;
}
```

------------------------------------------------------------------------

## What This Teaches

-   REST routing structure
-   Path parameters
-   Query parameters
-   JSON request body
-   Status codes
-   Header access
-   Two response styles (explicit vs auto-send)

