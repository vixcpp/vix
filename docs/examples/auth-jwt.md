# JWT Auth

This example shows how to protect Vix routes with JWT authentication.

Use JWT when your API receives a signed token from a client and needs to read claims such as:

```txt id="qd3w6n"
subject
roles
permissions
expiration
custom payload fields
```

This example uses a development HS256 token signed with:

```txt id="u37qqw"
secret = dev_secret
```

The route `/api/me` requires:

```txt id="pr2pav"
Authorization: Bearer <token>
```

## What this example builds

The app exposes:

```txt id="d0ua4j"
GET /
GET /api/public
GET /api/me
```

Behavior:

```txt id="tca5o1"
GET /
  public help page

GET /api/public
  public JSON route

GET /api/me
  protected route, requires a valid JWT
```

The JWT middleware validates the token and stores claims in request state.

The handler reads:

```cpp id="vkj70j"
vix::middleware::auth::JwtClaims
```

## Header

For Vix `v2.6.2` and newer, use:

```cpp id="bkq8yw"
#include <vix/middleware.hpp>
```

For older `v2.6.0` or `v2.6.1`, App presets may need an explicit include:

```cpp id="yg3laz"
#include <vix/middleware/app/presets.hpp>
```

This example uses the modern public entry point.

## Project structure

Create:

```txt id="zw5jcu"
auth_jwt_demo/
└── auth_jwt.cpp
```

Create the file:

```bash id="lx6ej5"
mkdir auth_jwt_demo
cd auth_jwt_demo
touch auth_jwt.cpp
```

## Source

Open:

```txt id="d1o06h"
auth_jwt.cpp
```

Add:

```cpp id="p0usv9"
#include <iostream>
#include <string>

#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

static const std::string kToken =
  "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
  "eyJzdWIiOiJ1c2VyMTIzIiwicm9sZXMiOlsiYWRtaW4iXX0."
  "3HK5b1sXMbxkjC3Tllwtcuzxm-1OI0D184Fuav0-XQo";

static void install_middleware(App &app)
{
  app.use("/api", middleware::app::request_id_dev());
  app.use("/api", middleware::app::timing_dev());
  app.use("/api", middleware::app::security_headers_dev());

  app.use("/api/me", middleware::app::jwt_dev(
    "dev_secret"
  ));
}

static void register_routes(App &app)
{
  app.get("/", [](Request &, Response &res)
  {
    res.send(
      "JWT auth example\n"
      "\n"
      "Public:\n"
      "  curl -i http://127.0.0.1:8080/api/public\n"
      "\n"
      "Protected:\n"
      "  curl -i http://127.0.0.1:8080/api/me\n"
      "  curl -i -H \"Authorization: Bearer <TOKEN>\" http://127.0.0.1:8080/api/me\n"
    );
  });

  app.get("/api/public", [](Request &, Response &res)
  {
    res.json({
      "ok", true,
      "message", "public route"
    });
  });

  app.get("/api/me", [](Request &req, Response &res)
  {
    auto &claims =
      req.state<middleware::auth::JwtClaims>();

    res.json({
      "ok", true,
      "subject", claims.subject,
      "roles", claims.roles
    });
  });
}

static void print_help()
{
  std::cout
    << "Vix JWT auth example running:\n"
    << "  http://127.0.0.1:8080/\n"
    << "  http://127.0.0.1:8080/api/public\n"
    << "  http://127.0.0.1:8080/api/me\n\n"
    << "Development token:\n"
    << "  " << kToken << "\n\n"
    << "Try:\n"
    << "  curl -i http://127.0.0.1:8080/api/me\n"
    << "  curl -i -H \"Authorization: Bearer " << kToken
    << "\" http://127.0.0.1:8080/api/me\n";
}

int main()
{
  App app;

  install_middleware(app);
  register_routes(app);

  print_help();

  app.run(8080);
  return 0;
}
```

## Run it

Run:

```bash id="bejmip"
vix run auth_jwt.cpp
```

The server listens on:

```txt id="udl4yb"
http://127.0.0.1:8080
```

## Test the public route

```bash id="q5hgrt"
curl -i http://127.0.0.1:8080/api/public
```

Expected body:

```json id="ev3uk1"
{
  "ok": true,
  "message": "public route"
}
```

This route does not require a JWT.

## Test the protected route without token

```bash id="kh6zxr"
curl -i http://127.0.0.1:8080/api/me
```

Expected status:

```txt id="j9j29v"
401 Unauthorized
```

The middleware rejects the request before the handler runs.

The request is missing:

```txt id="h4skhu"
Authorization: Bearer <token>
```

## Test with a valid token

Store the token:

```bash id="aafde4"
TOKEN="eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJ1c2VyMTIzIiwicm9sZXMiOlsiYWRtaW4iXX0.3HK5b1sXMbxkjC3Tllwtcuzxm-1OI0D184Fuav0-XQo"
```

Call the protected route:

```bash id="pvdw31"
curl -i \
  -H "Authorization: Bearer $TOKEN" \
  http://127.0.0.1:8080/api/me
```

Expected body shape:

```json id="a8o3lx"
{
  "ok": true,
  "subject": "user123",
  "roles": ["admin"]
}
```

The token is valid because it was signed with:

```txt id="pitfwn"
dev_secret
```

and the middleware was installed with the same secret:

```cpp id="bkl13x"
app.use("/api/me", middleware::app::jwt_dev(
  "dev_secret"
));
```

## Test with an invalid token

```bash id="aqb9ws"
curl -i \
  -H "Authorization: Bearer invalid.token.value" \
  http://127.0.0.1:8080/api/me
```

Expected status:

```txt id="y3duzy"
401 Unauthorized
```

The middleware rejects invalid tokens before the handler runs.

## How it works

The JWT middleware is installed only on the protected route:

```cpp id="ljzreq"
app.use("/api/me", middleware::app::jwt_dev(
  "dev_secret"
));
```

The middleware validates the token.

If the token is valid, it stores the decoded claims in request state:

```cpp id="om8at7"
vix::middleware::auth::JwtClaims
```

The handler reads them:

```cpp id="wozvqk"
auto &claims =
  req.state<middleware::auth::JwtClaims>();
```

Then it can return values such as:

```cpp id="bbuqfh"
claims.subject
claims.roles
```

## Authorization header

The client must send:

```txt id="qu1vqw"
Authorization: Bearer <token>
```

Example:

```bash id="upwvbj"
curl -i \
  -H "Authorization: Bearer $TOKEN" \
  http://127.0.0.1:8080/api/me
```

Common mistakes:

```txt id="letb31"
missing Authorization header
missing Bearer prefix
extra quotes around the token
token signed with a different secret
expired token when expiration verification is enabled
```

## What is inside the token

The development token payload contains:

```json id="lc6xab"
{
  "sub": "user123",
  "roles": ["admin"]
}
```

The `sub` claim becomes:

```cpp id="wc2w3t"
claims.subject
```

The `roles` claim becomes:

```cpp id="ub7z1x"
claims.roles
```

The raw payload is also available through the claims object if you need custom fields.

## Protect a route group

You can protect a whole group.

Example:

```cpp id="u2x0cc"
app.use("/api/private", middleware::app::jwt_dev(
  "dev_secret"
));

app.get("/api/private/me", [](Request &req, Response &res)
{
  auto &claims =
    req.state<middleware::auth::JwtClaims>();

  res.json({
    "ok", true,
    "subject", claims.subject
  });
});
```

Now every route under:

```txt id="dm416x"
/api/private
```

requires a valid JWT.

## Protect one exact route

If only one route should be protected, install the middleware on that route prefix:

```cpp id="u1ahxm"
app.use("/api/me", middleware::app::jwt_dev(
  "dev_secret"
));
```

This keeps public routes public.

Example:

```txt id="f4g5zw"
/api/public
  no token required

/api/me
  token required
```

## Custom JWT options

For more control, use `JwtOptions` directly and adapt the context middleware to App middleware.

```cpp id="ovvnc1"
middleware::auth::JwtOptions options;

options.secret = "dev_secret";
options.verify_exp = false;

app.use("/api/me", middleware::app::adapt_ctx(
  middleware::auth::jwt(options)
));
```

Use this when you want to configure JWT behavior explicitly.

The preset:

```cpp id="t5m4fv"
middleware::app::jwt_dev("dev_secret")
```

is shorter and useful for examples.

## Development preset

This example uses:

```cpp id="x447hp"
middleware::app::jwt_dev("dev_secret")
```

The development preset is designed to make local examples simple.

For production, configure JWT intentionally.

Do not hardcode secrets in source code.

Use environment variables or secure configuration.

## Production secret

Bad:

```cpp id="htosgd"
middleware::app::jwt_dev("dev_secret");
```

Good shape:

```cpp id="zxskn2"
const std::string jwt_secret =
  cfg.getString("jwt.secret", "");

if (jwt_secret.empty())
{
  throw std::runtime_error("jwt.secret is required");
}

middleware::auth::JwtOptions options;
options.secret = jwt_secret;
options.verify_exp = true;

app.use("/api/private", middleware::app::adapt_ctx(
  middleware::auth::jwt(options)
));
```

In production, secrets should come from:

```txt id="id0qf4"
environment variables
secret manager
deployment configuration
```

not from source code.

## Expiration

JWTs commonly use the `exp` claim.

For local examples, expiration verification may be disabled.

For production, enable expiration verification:

```cpp id="u16n50"
middleware::auth::JwtOptions options;

options.secret = jwt_secret;
options.verify_exp = true;
```

Then expired tokens should be rejected.

Use short token lifetimes for sensitive APIs.

## JWT vs session

JWT and sessions solve related but different problems.

```txt id="q6kkhs"
session
  server stores state
  browser stores session id cookie

JWT
  token contains signed claims
  server verifies signature
  no server session store required for basic validation
```

Use sessions when the server should own state.

Use JWT when clients or services pass signed claims.

For browser apps, sessions are often simpler.

For APIs, mobile apps, and service-to-service calls, JWT can be useful.

## JWT vs API key

API keys identify applications or clients.

JWTs usually identify a user or principal with claims.

```txt id="twx65t"
API key
  simple client credential

JWT
  signed token with claims
```

Use API keys for simple service access.

Use JWT when you need structured identity claims.

## Middleware order

The example installs:

```cpp id="ad0gve"
app.use("/api", middleware::app::request_id_dev());
app.use("/api", middleware::app::timing_dev());
app.use("/api", middleware::app::security_headers_dev());

app.use("/api/me", middleware::app::jwt_dev(
  "dev_secret"
));
```

The order means:

```txt id="ypos0j"
request id
  identify request

timing
  measure request

security headers
  harden response

jwt
  validate token and store claims

handler
  reads JwtClaims
```

If you use RBAC, install it after JWT because RBAC needs JWT claims.

## JWT with RBAC

JWT authentication answers:

```txt id="smcpsn"
who is the caller?
```

RBAC authorization answers:

```txt id="ruxlpf"
what is the caller allowed to do?
```

The order is:

```txt id="jubf16"
jwt
rbac_context
require_role or require_perm
handler
```

Example shape:

```cpp id="o7tstx"
app.use("/admin", middleware::app::jwt_dev("dev_secret"));

app.use("/admin", middleware::app::adapt_ctx(
  middleware::auth::rbac_context()
));

app.use("/admin", middleware::app::adapt_ctx(
  middleware::auth::require_role("admin")
));
```

For a complete example, see `examples/auth-rbac.md`.

## Error behavior

Typical JWT failures:

```txt id="u3u7f5"
missing token
  401 Unauthorized

invalid token
  401 Unauthorized

bad signature
  401 Unauthorized

expired token
  401 Unauthorized when exp verification is enabled
```

Authorization failures such as missing role or permission belong to RBAC and usually return:

```txt id="txo9ro"
403 Forbidden
```

## Complete test flow

Run:

```bash id="w7iyjk"
vix run auth_jwt.cpp
```

Public route:

```bash id="szu6c5"
curl -i http://127.0.0.1:8080/api/public
```

Protected route without token:

```bash id="aucxkx"
curl -i http://127.0.0.1:8080/api/me
```

Set token:

```bash id="s321n2"
TOKEN="eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJ1c2VyMTIzIiwicm9sZXMiOlsiYWRtaW4iXX0.3HK5b1sXMbxkjC3Tllwtcuzxm-1OI0D184Fuav0-XQo"
```

Protected route with token:

```bash id="ooz0td"
curl -i \
  -H "Authorization: Bearer $TOKEN" \
  http://127.0.0.1:8080/api/me
```

Invalid token:

```bash id="jvzwn6"
curl -i \
  -H "Authorization: Bearer invalid.token.value" \
  http://127.0.0.1:8080/api/me
```

## Summary

Use JWT middleware when a route needs a signed bearer token.

Install it:

```cpp id="g4wxna"
app.use("/api/me", middleware::app::jwt_dev(
  "dev_secret"
));
```

Read claims:

```cpp id="a7jja6"
auto &claims =
  req.state<middleware::auth::JwtClaims>();
```

Return identity data:

```cpp id="uc3pss"
res.json({
  "ok", true,
  "subject", claims.subject,
  "roles", claims.roles
});
```

The mental model is:

```txt id="p0qqxg"
client sends Bearer token
JWT middleware verifies token
middleware stores JwtClaims
handler reads claims
RBAC can enforce roles and permissions later
```
