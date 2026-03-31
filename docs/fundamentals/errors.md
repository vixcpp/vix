# Errors

Error handling in Vix is explicit.

There is no hidden exception translation layer. You decide:

-   The status code
-   The error structure
-   The message format

------------------------------------------------------------------------

## Basic error response

``` cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/error", [](Request&, Response& res) {
    res.status(400).json({
      "error", "Bad request"
    });
  });

  app.run(8080);
}
```

------------------------------------------------------------------------

## 404 example

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
      res.status(404).json({
        "error", "User not found",
        "id", id
      });
      return;
    }

    res.json({
      "id", id,
      "name", "User#" + id
    });
  });

  app.run(8080);
}
```

------------------------------------------------------------------------

## Validation error

``` cpp
#include <vix.hpp>
#include <string>

using namespace vix;

int main()
{
  App app;

  app.get("/validate", [](Request& req, Response& res) {
    const std::string email = req.query_value("email", "");

    if (email.find('@') == std::string::npos)
    {
      res.status(400).json({
        "error", "Invalid email",
        "field", "email"
      });
      return;
    }

    res.json({
      "ok", true,
      "email", email
    });
  });

  app.run(8080);
}
```

------------------------------------------------------------------------

## Internal server error pattern

``` cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/internal", [](Request&, Response& res) {
    // Simulated failure
    bool failure = true;

    if (failure)
    {
      res.status(500).json({
        "error", "Internal server error"
      });
      return;
    }

    res.json({"ok", true});
  });

  app.run(8080);
}
```

------------------------------------------------------------------------

## Error structure consistency

For real applications, keep a consistent format:

``` json
{
  "error": "message",
  "code": 400,
  "details": {}
}
```

Example:

``` cpp
res.status(400).json({
  "error", "Invalid input",
  "code", 400
});
```

------------------------------------------------------------------------

## Important rules

-   Always set an appropriate status code.
-   Always return a clear error message.
-   Avoid sending partial responses.
-   Send the response once.

------------------------------------------------------------------------

## Philosophy

Vix does not decide what an error looks like.

You do.

That makes the system:

-   Predictable
-   Transparent
-   Production-safe

