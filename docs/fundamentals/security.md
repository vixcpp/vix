# Security

Security in Vix is explicit.

Vix does not automatically inject security layers. You control:

-   Authentication
-   Authorization
-   Validation
-   Headers
-   Error exposure

------------------------------------------------------------------------

## Basic header check

``` cpp
#include <vix.hpp>
#include <string>

using namespace vix;

int main()
{
  App app;

  app.get("/secure", [](Request& req, Response& res) {
    const std::string token = req.header("Authorization");

    if (token.empty())
    {
      res.status(401).json({
        "error", "Missing Authorization header"
      });
      return;
    }

    res.json({"ok", true});
  });

  app.run(8080);
}
```

------------------------------------------------------------------------

## Token validation pattern

``` cpp
app.get("/admin", [](Request& req, Response& res) {
  const std::string token = req.header("Authorization");

  if (token != "Bearer secret")
  {
    res.status(403).json({
      "error", "Forbidden"
    });
    return;
  }

  res.json({"access", "granted"});
});
```

Always validate tokens properly in production.

------------------------------------------------------------------------

## Input validation

Never trust user input.

``` cpp
app.get("/validate", [](Request& req, Response& res) {
  const std::string email = req.query_value("email", "");

  if (email.find('@') == std::string::npos)
  {
    res.status(400).json({
      "error", "Invalid email"
    });
    return;
  }

  res.json({"ok", true});
});
```

------------------------------------------------------------------------

## Avoid leaking internal details

Bad practice:

``` cpp
res.status(500).json({
  "error", e.what()
});
```

Better practice:

``` cpp
res.status(500).json({
  "error", "Internal server error"
});
```

Do not expose stack traces or internal logic.

------------------------------------------------------------------------

## Security headers

You can explicitly set security-related headers:

``` cpp
app.get("/", [](Request&, Response& res) {
  res.set_header("X-Content-Type-Options", "nosniff");
  res.set_header("X-Frame-Options", "DENY");
  res.set_header("Content-Security-Policy", "default-src 'self'");

  res.json({"ok", true});
});
```

------------------------------------------------------------------------

## HTTPS

For production:

-   Always run behind HTTPS
-   Use a reverse proxy (Nginx, Caddy, etc.)
-   Terminate TLS properly
-   Keep certificates updated

Vix focuses on runtime control. Transport security should be properly
configured at deployment level.

------------------------------------------------------------------------

## Rate limiting and protection

Vix does not enforce rate limiting automatically.

You can:

-   Implement custom counters
-   Use middleware patterns
-   Use a reverse proxy for rate limiting

------------------------------------------------------------------------

## Security mindset

Security is not a framework feature.

It is an architecture decision.

In Vix:

-   Nothing is automatic
-   Nothing is hidden
-   Everything is explicit

That makes auditing easier and behavior predictable.

