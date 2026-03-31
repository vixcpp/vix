# Fundamentals

This section explains the core building blocks of Vix.

Before building larger systems, you should understand:

-   The App runtime
-   The Request object
-   The Response object
-   The routing model
-   The execution flow

Everything in Vix is explicit and predictable.

------------------------------------------------------------------------

## The Runtime

Every Vix application starts with:

``` cpp
App app;
```

This creates the HTTP runtime.

Routes are registered on the `app` instance.

The server starts with:

``` cpp
app.run(8080);
```

------------------------------------------------------------------------

## The Execution Model

When a request arrives:

1.  The route is matched.
2.  The handler lambda is executed.
3.  You control the response.
4.  The connection is finalized.

There is no hidden magic layer.

------------------------------------------------------------------------

## The Request Object

Inside handlers:

``` cpp
app.get("/", [](Request& req, Response& res) {
  // req gives you access to:
  // - path params
  // - query params
  // - headers
  // - body
});
```

Request is read-only. It represents incoming data.

------------------------------------------------------------------------

## The Response Object

Response is fully controlled by you.

You decide:

``` cpp
res.status(200);
res.set_header("X-App", "Vix");
res.send("text");
res.json({"ok", true});
```

Nothing is sent unless you explicitly send it or return a payload that
auto-sends.

------------------------------------------------------------------------

## Minimal complete example

``` cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request&, Response& res) {
    res.json({"message", "Fundamentals"});
  });

  app.run(8080);
}
```

------------------------------------------------------------------------

## Design Principles

Vix fundamentals are built on:

-   Explicit behavior
-   Predictable lifecycle
-   No implicit middleware
-   No runtime reflection
-   No hidden state machines

The goal is clarity and performance.

