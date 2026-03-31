# Lifecycle

Understanding the request lifecycle is essential to using Vix correctly.

Vix keeps the lifecycle simple and explicit.

------------------------------------------------------------------------

## High-level flow

For every incoming HTTP request:

1.  The runtime accepts the connection.
2.  The route is matched.
3.  Your handler lambda executes.
4.  You build and send the response.
5.  The connection is finalized.

Nothing happens outside of this flow.

------------------------------------------------------------------------

## Minimal example

``` cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request&, Response& res) {
    res.json({"message", "Lifecycle example"});
  });

  app.run(8080);
}
```

When a client calls `/`:

-   The route `/` is matched.
-   The lambda runs.
-   `res.json(...)` sends the response.
-   The request ends.

------------------------------------------------------------------------

## Step-by-step breakdown

### 1. Route matching

When a request arrives, Vix checks the HTTP method and path.

Example:

``` cpp
app.get("/users/{id}", [](Request& req, Response& res) {
  const std::string id = req.param("id", "0");
  res.json({"id", id});
});
```

If the request is:

    GET /users/42

The route matches and `id` becomes `"42"`.

------------------------------------------------------------------------

### 2. Handler execution

Your lambda is executed synchronously.

Inside it you can:

-   Read request headers
-   Read query parameters
-   Read body
-   Parse JSON
-   Set status
-   Set headers
-   Send response

You control everything.

------------------------------------------------------------------------

### 3. Response finalization

A response is finalized when:

-   You call `res.send()`
-   You call `res.json()`
-   Or you return a value that auto-sends

Example:

``` cpp
app.get("/text", [](const Request&, Response&) {
  return "Auto-sent response";
});
```

If you explicitly send a response:

``` cpp
app.get("/mix", [](Request&, Response& res) {
  res.send("Explicit send");
  return "ignored";
});
```

The returned value is ignored.

------------------------------------------------------------------------

## Important rules

-   A response should only be sent once.
-   After sending, additional modifications are ignored.
-   If nothing is sent, behavior depends on runtime defaults.

Best practice: always explicitly send a response.

------------------------------------------------------------------------

## Lifecycle philosophy

Vix avoids:

-   Hidden middleware chains
-   Implicit async behavior
-   Magic auto transformations

The lifecycle is predictable:

Request → Handler → Response → End

That simplicity is intentional.

