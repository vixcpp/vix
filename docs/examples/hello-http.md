# Hello HTTP

This page shows **all the common "Hello" styles** in Vix.cpp, using the
smallest possible snippets.

Each snippet is a complete `main()` you can copy into a real file (for
example `main.cpp`) and run.

------------------------------------------------------------------------

## 1) Hello as plain text (explicit send)

``` cpp
#include <vix.hpp>
using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request&, Response& res)
  {
    res.send("Hello, Vix!");
  });

  app.run(8080);
  return 0;
}
```

Try:

``` bash
curl -i http://localhost:8080/
```

------------------------------------------------------------------------

## 2) Hello as plain text (return style)

If you return a value and you did not send anything explicitly, Vix
auto-sends it.

``` cpp
#include <vix.hpp>
using namespace vix;

int main()
{
  App app;

  app.get("/txt", [](Request&, Response&)
  {
    return "Hello, Vix!";
  });

  app.run(8080);
  return 0;
}
```

Try:

``` bash
curl -i http://localhost:8080/txt
```

------------------------------------------------------------------------

## 3) Hello as JSON (explicit json)

``` cpp
#include <vix.hpp>
using namespace vix;

int main()
{
  App app;

  app.get("/json", [](Request&, Response& res)
  {
    res.json({
      "message", "Hello",
      "framework", "Vix.cpp",
      "ok", true
    });
  });

  app.run(8080);
  return 0;
}
```

Try:

``` bash
curl -s http://localhost:8080/json
```

------------------------------------------------------------------------

## 4) Hello as JSON (return object style)

``` cpp
#include <vix.hpp>
#include <vix/json/json.hpp>
using namespace vix;

int main()
{
  App app;

  app.get("/auto-json", [](Request&, Response&)
  {
    return vix::json::o(
      "message", "Hello",
      "mode", "auto-return",
      "ok", true
    );
  });

  app.run(8080);
  return 0;
}
```

Try:

``` bash
curl -s http://localhost:8080/auto-json
```

------------------------------------------------------------------------

## 5) Hello with status code + JSON

``` cpp
#include <vix.hpp>
using namespace vix;

int main()
{
  App app;

  app.get("/created", [](Request&, Response& res)
  {
    res.status(201).json({
      "message", "Created",
      "ok", true
    });
  });

  app.run(8080);
  return 0;
}
```

Try:

``` bash
curl -i http://localhost:8080/created
```

------------------------------------------------------------------------

## 6) Hello with headers + text

``` cpp
#include <vix.hpp>
using namespace vix;

int main()
{
  App app;

  app.get("/headers", [](Request&, Response& res)
  {
    res.header("X-Powered-By", "Vix.cpp");
    res.send("Hello with headers");
  });

  app.run(8080);
  return 0;
}
```

Try:

``` bash
curl -i http://localhost:8080/headers
```

------------------------------------------------------------------------

## 7) Hello with path param

``` cpp
#include <vix.hpp>
using namespace vix;

int main()
{
  App app;

  app.get("/hello/{name}", [](Request& req, Response& res)
  {
    const auto name = req.param("name");
    res.json({
      "message", "Hello",
      "name", name
    });
  });

  app.run(8080);
  return 0;
}
```

Try:

``` bash
curl -s http://localhost:8080/hello/Ada
```

------------------------------------------------------------------------

## 8) Hello with query param

``` cpp
#include <vix.hpp>
using namespace vix;

int main()
{
  App app;

  app.get("/hello", [](Request& req, Response& res)
  {
    const auto name = req.query_value("name", "world");
    res.send(std::string("Hello, ") + name + "!");
  });

  app.run(8080);
  return 0;
}
```

Try:

``` bash
curl -i "http://localhost:8080/hello?name=Ada"
```

------------------------------------------------------------------------

## 9) Mixing send + return (return gets ignored)

If you already sent a response (send/json), any returned value is
ignored.

``` cpp
#include <vix.hpp>
#include <vix/json/json.hpp>
using namespace vix;

int main()
{
  App app;

  app.get("/mix", [](Request&, Response& res)
  {
    res.status(200).send("Hello (explicit)");

    // Already sent -> ignored
    return vix::json::o("ignored", true);
  });

  app.run(8080);
  return 0;
}
```

Try:

``` bash
curl -i http://localhost:8080/mix
```

------------------------------------------------------------------------

## What this teaches

-   The two styles: **explicit send** vs **auto-send return**
-   Text and JSON responses
-   Status codes and headers
-   Path and query parameters
-   The "already sent" rule

