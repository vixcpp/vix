# Errors + Status Codes

This section shows how to handle errors and HTTP status codes in Vix.cpp.

Each example is minimal and self-contained.

---

## 1. Simple 404 Response

Return a JSON error with explicit status.

```cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/not-found", [](Request&, Response& res)
  {
    res.status(404).json({
      "ok", false,
      "error", "Resource not found"
    });
  });

  app.run(8080);
  return 0;
}
```

Test:

    curl -i http://localhost:8080/not-found

---

## 2. 400 Bad Request (Validation-style)

```cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/bad", [](Request&, Response& res)
  {
    res.status(400).json({
      "ok", false,
      "error", "Invalid input"
    });
  });

  app.run(8080);
  return 0;
}
```

---

## 3. 401 Unauthorized

```cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/private", [](Request&, Response& res)
  {
    res.status(401).json({
      "ok", false,
      "error", "Unauthorized"
    });
  });

  app.run(8080);
  return 0;
}
```

---

## 4. 403 Forbidden

```cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/admin", [](Request&, Response& res)
  {
    res.status(403).json({
      "ok", false,
      "error", "Forbidden"
    });
  });

  app.run(8080);
  return 0;
}
```

---

## 5. 500 Internal Server Error

You can manually return 500:

```cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/error", [](Request&, Response& res)
  {
    res.status(500).json({
      "ok", false,
      "error", "Internal Server Error"
    });
  });

  app.run(8080);
  return 0;
}
```

---

## 6. Throwing an Exception

If a handler throws, Vix will convert it into a 500 response
(depending on your dev/production configuration).

```cpp
#include <vix.hpp>
#include <stdexcept>

using namespace vix;

int main()
{
  App app;

  app.get("/boom", [](Request&, Response&)
  {
    throw std::runtime_error("Something went wrong");
    return "unreachable";
  });

  app.run(8080);
  return 0;
}
```

---

## 7. Using set_status() + send()

```cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/created", [](Request&, Response& res)
  {
    res.set_status(201);
    res.send("Created");
  });

  app.run(8080);
  return 0;
}
```

---

## 8. Returning JSON Automatically

If nothing is sent explicitly, returning JSON auto-sends the response.

```cpp
#include <vix.hpp>
#include <vix/json/json.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/auto-error", [](Request&, Response&)
  {
    return vix::json::o(
      "ok", false,
      "error", "Auto-sent error"
    );
  });

  app.run(8080);
  return 0;
}
```

---

## What this teaches

- How to set HTTP status codes
- How to return structured JSON errors
- How exceptions behave
- The difference between res.status() and set_status()
- Auto-send return style

