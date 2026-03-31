# Routing

Vix routes are designed to be explicit and minimal.

Everything happens inside handlers.

------------------------------------------------------------------------

## Minimal server

``` cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request&, Response& res) {
    res.json({"message", "Hello"});
  });

  app.run(8080);
}
```

------------------------------------------------------------------------

## Plain text

``` cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/txt", [](const Request&, Response&) {
    return "Hello world";
  });

  app.run(8080);
}
```

------------------------------------------------------------------------

## JSON response

``` cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/json", [](Request&, Response& res) {
    res.json({"ok", true, "service", "vix"});
  });

  app.run(8080);
}
```

------------------------------------------------------------------------

## Nested JSON

``` cpp
#include <vix.hpp>

using namespace vix;
namespace J = vix::json;

int main()
{
  App app;

  app.get("/user", [](Request&, Response& res) {
    res.json({
      "name", "Ada",
      "tags", J::array({"c++", "net", "http"}),
      "profile", J::obj({"id", 42, "vip", true})
    });
  });

  app.run(8080);
}
```

------------------------------------------------------------------------

## Status codes

``` cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/created", [](Request&, Response& res) {
    res.status(201).json({"message", "Created"});
  });

  app.get("/not-found", [](Request&, Response& res) {
    res.status(404).json({"error", "Not Found"});
  });

  app.run(8080);
}
```

------------------------------------------------------------------------

## Path parameters

``` cpp
#include <vix.hpp>
#include <string>

using namespace vix;

int main()
{
  App app;

  app.get("/users/{id}", [](Request& req, Response& res) {
    const std::string id = req.param("id", "0");

    if (id == "0")
    {
      res.status(404).json({"error", "User not found", "id", id});
      return;
    }

    res.json({
      "id", id,
      "name", "User#" + id,
      "vip", (id == "42")
    });
  });

  app.run(8080);
}
```

Test:

``` bash
curl http://127.0.0.1:8080/users/42
```

------------------------------------------------------------------------

## Query parameters

``` cpp
#include <vix.hpp>
#include <string>

using namespace vix;

int main()
{
  App app;

  app.get("/search", [](Request& req, Response& res) {
    const std::string q = req.query_value("q", "");
    const std::string page = req.query_value("page", "1");
    const std::string limit = req.query_value("limit", "10");

    res.json({
      "q", q,
      "page", page,
      "limit", limit,
      "hint", "Try /search?q=vix&page=2&limit=5"
    });
  });

  app.run(8080);
}
```

Test:

``` bash
curl "http://127.0.0.1:8080/search?q=vix&page=2&limit=5"
```

------------------------------------------------------------------------

## Headers

``` cpp
#include <vix.hpp>
#include <string>

using namespace vix;

int main()
{
  App app;

  app.get("/headers", [](Request& req, Response& res) {
    const std::string ua = req.header("User-Agent");
    const std::string host = req.header("Host");
    const bool hasAuth = req.has_header("Authorization");

    res.json({
      "host", host,
      "user_agent", ua,
      "has_authorization", hasAuth
    });
  });

  app.run(8080);
}
```

Test:

``` bash
curl -H "Authorization: Bearer test" http://127.0.0.1:8080/headers
```

------------------------------------------------------------------------

## Request body

### Raw body

``` cpp
#include <vix.hpp>
#include <string>

using namespace vix;

int main()
{
  App app;

  app.get("/echo/body", [](Request& req, Response& res) {
    const std::string body = req.body();

    res.json({
      "bytes", (long long)body.size(),
      "body", body
    });
  });

  app.run(8080);
}
```

------------------------------------------------------------------------

### JSON body

``` cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/echo/json", [](Request& req, Response& res) {
    res.json({"raw", req.json()});
  });

  app.run(8080);
}
```

------------------------------------------------------------------------

## Mixed behavior

If you send a response explicitly, returned values are ignored.

``` cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/mix", [](Request&, Response& res) {
    res.status(201).send("Created");
    return vix::json::o("ignored", true);
  });

  app.run(8080);
}
```

------------------------------------------------------------------------

## Rule of thumb

-   Use `res.status(...).json(...)` when you want full control
-   Return a payload when you want auto-send
-   Keep handlers small and explicit

