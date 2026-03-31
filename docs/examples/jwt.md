# JWT (Beginner Guide)

Welcome 👋\
This page teaches you how to use **JWT authentication** in Vix.cpp with
minimal examples.

JWT is useful when you want:

-   stateless auth (no server sessions)
-   API tokens for mobile apps and CLIs
-   simple role based access (admin, user, etc.)
-   permissions like `products:write`


## What is a JWT?

A JWT is just a string with 3 parts:

    header.payload.signature

-   **header** says which algorithm is used (example: HS256)
-   **payload** contains claims (sub, roles, perms)
-   **signature** proves the token was created using your secret

In Vix.cpp, JWT middleware checks:

-   the token exists in `Authorization: Bearer <token>`
-   signature is valid for your secret
-   (optionally) exp is valid

# 1) Smallest JWT protected route (App)

This is the simplest pattern:

-   `/` is public
-   `/secure` requires a Bearer token

``` cpp
#include <iostream>
#include <string>

#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>

using namespace vix;

static const std::string kToken =
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
    "eyJzdWIiOiJ1c2VyMTIzIiwicm9sZXMiOlsiYWRtaW4iXX0."
    "3HK5b1sXMbxkjC3Tllwtcuzxm-1OI0D184Fuav0-XQo"; // HS256, secret=dev_secret

int main()
{
  App app;

  // Protect only /secure (verify_exp=false for beginner demo)
  app.use("/secure", middleware::app::jwt_dev("dev_secret"));

  app.get("/", [](Request &, Response &res)
  {
    res.send(
      "JWT example:\n"
      "  GET /secure requires Bearer token.\n"
      "\n"
      "Try:\n"
      "  curl -i http://localhost:8080/secure\n"
      "  curl -i -H \"Authorization: Bearer <TOKEN>\" http://localhost:8080/secure\n"
    );
  });

  app.get("/secure", [](Request &req, Response &res)
  {
    auto &claims = req.state<vix::middleware::auth::JwtClaims>();

    res.json({
      "ok", true,
      "sub", claims.subject,
      "roles", claims.roles
    });
  });

  std::cout
    << "Server: http://localhost:8080/\n"
    << "Secure: http://localhost:8080/secure\n\n"
    << "Token:\n" << kToken << "\n";

  app.run(8080);
  return 0;
}
```

## Test with curl

Run:

``` bash
vix run jwt_app_simple.cpp
```

No token:

``` bash
curl -i http://localhost:8080/secure
```

With token:

``` bash
TOKEN="eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJ1c2VyMTIzIiwicm9sZXMiOlsiYWRtaW4iXX0.3HK5b1sXMbxkjC3Tllwtcuzxm-1OI0D184Fuav0-XQo"
curl -i -H "Authorization: Bearer $TOKEN" http://localhost:8080/secure
```

# 2) Reading JWT claims inside your handler

After JWT middleware succeeds, claims are stored in request state:

``` cpp
auto &claims = req.state<vix::middleware::auth::JwtClaims>();

// Common fields:
claims.subject   // "sub"
claims.roles     // "roles" (array)
```

So you can do:

``` cpp
app.get("/secure", [](Request &req, Response &res)
{
  auto &claims = req.state<vix::middleware::auth::JwtClaims>();
  res.json({
    "ok", true,
    "sub", claims.subject,
    "roles", claims.roles
  });
});
```

# 3) Common beginner mistakes

1)  Secret mismatch\
    The token is signed with a secret. Your middleware must use the same
    secret.

2)  Forgetting the Bearer prefix\
    It must be:

`Authorization: Bearer <token>`

3)  Not enabling JWT in build\
    If your build has a flag like `VIX_ENABLE_JWT`, you must enable it,
    otherwise middleware might not exist.

4)  Forgetting expiration handling\
    For beginner demos we use `verify_exp=false`. In production, you
    usually want `verify_exp=true` and an `exp` claim.

# Summary

-   Protect a prefix: `app.use("/secure", jwt_dev("secret"))`
-   Send token in header: `Authorization: Bearer <token>`
-   Read claims from request state after middleware success
-   For dev, use a token generator tool

