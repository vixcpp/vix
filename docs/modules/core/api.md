# HTTP

Vix gives you explicit control over the HTTP layer.

You control:

-   Status codes
-   Headers
-   Body
-   Content type

Nothing is hidden.

------------------------------------------------------------------------

## Setting status codes

``` cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/ok", [](Request&, Response& res) {
    res.status(200).json({"status", "ok"});
  });

  app.get("/created", [](Request&, Response& res) {
    res.status(201).json({"status", "created"});
  });

  app.get("/error", [](Request&, Response& res) {
    res.status(500).json({"error", "internal error"});
  });

  app.run(8080);
}
```

------------------------------------------------------------------------

## Setting headers

``` cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/headers", [](Request&, Response& res) {
    res.set_header("X-App", "Vix");
    res.set_header("Cache-Control", "no-store");

    res.json({"ok", true});
  });

  app.run(8080);
}
```

Test:

``` bash
curl -i http://127.0.0.1:8080/headers
```

------------------------------------------------------------------------

## Sending plain text

``` cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/text", [](Request&, Response& res) {
    res.send("Hello from Vix");
  });

  app.run(8080);
}
```

------------------------------------------------------------------------

## Sending JSON

``` cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/json", [](Request&, Response& res) {
    res.json({
      "service", "vix",
      "version", 1,
      "ok", true
    });
  });

  app.run(8080);
}
```

------------------------------------------------------------------------

## Reading request headers

``` cpp
#include <vix.hpp>
#include <string>

using namespace vix;

int main()
{
  App app;

  app.get("/inspect", [](Request& req, Response& res) {
    const std::string ua = req.header("User-Agent");
    const std::string host = req.header("Host");

    res.json({
      "user_agent", ua,
      "host", host
    });
  });

  app.run(8080);
}
```

------------------------------------------------------------------------

## Checking header existence

``` cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/auth/check", [](Request& req, Response& res) {
    const bool hasAuth = req.has_header("Authorization");

    res.json({
      "has_authorization", hasAuth
    });
  });

  app.run(8080);
}
```

------------------------------------------------------------------------

## Raw body access

``` cpp
#include <vix.hpp>
#include <string>

using namespace vix;

int main()
{
  App app;

  app.get("/echo", [](Request& req, Response& res) {
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

## Content behavior rule

-   `res.send()` sends raw content
-   `res.json()` sets JSON content type automatically
-   `res.status()` controls HTTP status
-   `res.set_header()` sets custom headers

Everything is explicit.



