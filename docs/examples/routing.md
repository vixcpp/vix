# Routing

This section demonstrates how routing works in Vix.cpp.

Each example is minimal and self-contained.

---

## 1. Basic GET Route

```cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/hello", [](Request&, Response& res)
  {
    res.send("Hello");
  });

  app.run(8080);
  return 0;
}
```

---

## 2. Multiple HTTP Methods

```cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/users", [](Request&, Response& res)
  {
    res.send("GET users");
  });

  app.post("/users", [](Request&, Response& res)
  {
    res.send("POST users");
  });

  app.put("/users", [](Request&, Response& res)
  {
    res.send("PUT users");
  });

  app.del("/users", [](Request&, Response& res)
  {
    res.send("DELETE users");
  });

  app.run(8080);
  return 0;
}
```

---

## 3. Path Parameters

```cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/users/{id}", [](Request& req, Response& res)
  {
    auto id = req.param("id");
    res.json({
      "user_id", id
    });
  });

  app.run(8080);
  return 0;
}
```

Test:

    curl http://localhost:8080/users/42

---

## 4. Query Parameters

```cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/search", [](Request& req, Response& res)
  {
    auto q = req.query_value("q", "");
    auto page = req.query_value("page", "1");

    res.json({
      "query", q,
      "page", page
    });
  });

  app.run(8080);
  return 0;
}
```

Test:

    curl "http://localhost:8080/search?q=vix&page=2"

---

## 5. Heavy Routes

Use get_heavy when the handler performs blocking or heavy work.

```cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get_heavy("/report", [](Request&, Response& res)
  {
    // Simulated heavy work
    res.send("Heavy report generated");
  });

  app.run(8080);
  return 0;
}
```

---

## 6. Returning Values Directly

If you return a value instead of calling res.send(),
Vix automatically sends it.

```cpp
#include <vix.hpp>
#include <vix/json/json.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/auto", [](Request&, Response&)
  {
    return vix::json::o(
      "ok", true,
      "message", "Auto-sent response"
    );
  });

  app.run(8080);
  return 0;
}
```

---

## What this teaches

- Defining routes with different HTTP methods
- Using path parameters
- Using query parameters
- Heavy vs light routes
- Auto-send return style

