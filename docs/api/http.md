# HTTP API

This page documents the core HTTP primitives in Vix.

It covers:

-   App
-   Routing methods
-   Request
-   Response

All examples are minimal and placed entirely inside `main()`.

------------------------------------------------------------------------

# App

Header:

``` cpp
#include <vix.hpp>
```

Core type:

``` cpp
vix::App
```

### Constructor

``` cpp
App app;
```

### Run

``` cpp
app.run(8080);
```

Starts the HTTP server on the given port (blocking call).

------------------------------------------------------------------------

# Routing Methods

## GET

``` cpp
app.get(path, handler);
```

## POST

``` cpp
app.post(path, handler);
```

## PUT

``` cpp
app.put(path, handler);
```

## DELETE

``` cpp
app.del(path, handler);
```

------------------------------------------------------------------------

## Minimal Example

``` cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request&, Response& res) {
    res.send("Hello");
  });

  app.run(8080);
}
```

------------------------------------------------------------------------

# Request

Type:

``` cpp
vix::Request
```

Provides access to:

-   Path parameters
-   Query parameters
-   Headers
-   Body
-   JSON payload

------------------------------------------------------------------------

## Path parameter

``` cpp
req.param("id", "0");
```

Returns string value or fallback.

------------------------------------------------------------------------

## Query parameter

``` cpp
req.query_value("page", "1");
```

------------------------------------------------------------------------

## Headers

``` cpp
req.header("User-Agent");
req.has_header("Authorization");
```

------------------------------------------------------------------------

## Body

``` cpp
std::string body = req.body();
```

------------------------------------------------------------------------

## JSON

``` cpp
const auto& j = req.json();
```

Returns parsed JSON (high-level JSON layer).

------------------------------------------------------------------------

# Response

Type:

``` cpp
vix::Response
```

Controls HTTP output.

------------------------------------------------------------------------

## Status

``` cpp
res.status(201);
res.set_status(404);
```

------------------------------------------------------------------------

## Send text

``` cpp
res.send("Hello");
```

------------------------------------------------------------------------

## Send JSON

``` cpp
res.json({"message", "ok"});
```

------------------------------------------------------------------------

## Auto-send return

If a route handler returns:

-   `std::string` → text response
-   JSON object → JSON response

Example:

``` cpp
app.get("/auto", [](Request&, Response&) {
  return vix::json::o("message", "auto");
});
```

------------------------------------------------------------------------

# Behavior Notes

-   If `res.send()` or `res.json()` is called, returned values are
    ignored.
-   If nothing is sent, the handler must return a value.
-   Status defaults to 200 unless changed.
-   Handlers execute synchronously per request context.

------------------------------------------------------------------------

# Design Philosophy

The HTTP layer is:

-   Minimal
-   Explicit
-   Deterministic
-   Zero hidden middleware stack by default

You control exactly what happens in each handler.

