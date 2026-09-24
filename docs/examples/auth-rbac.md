# RBAC

This example shows how to protect Vix routes with role-based and permission-based authorization.

RBAC means:

```txt id="b0b4jf"
Role-Based Access Control
```

Use RBAC when a route should be accessible only to users with specific roles or permissions.

Examples:

```txt id="mx7jr9"
admin
seller
moderator
products:write
orders:read
users:delete
```

JWT authentication answers:

```txt id="lm4vrn"
who is the caller?
```

RBAC authorization answers:

```txt id="b4t61b"
what is the caller allowed to do?
```

## What this example builds

The app exposes:

```txt id="xun39k"
GET /
GET /api/public
GET /api/me
GET /api/admin
GET /api/products/write
```

Behavior:

```txt id="x9upwp"
GET /api/public
  public route

GET /api/me
  requires valid JWT

GET /api/admin
  requires valid JWT and role admin

GET /api/products/write
  requires valid JWT and permission products:write
```

The middleware order is:

```txt id="kp5avd"
JWT
  validates the token and stores JwtClaims

RBAC context
  builds Authz from JwtClaims

require_role or require_perm
  enforces authorization

handler
  runs only if authorization passed
```

## Header

For Vix `v2.6.2` and newer, use:

```cpp id="lvh6jq"
#include <vix/middleware.hpp>
```

For older `v2.6.0` or `v2.6.1`, App integration headers may need explicit includes:

```cpp id="dqqash"
#include <vix/middleware/app/adapter.hpp>
#include <vix/middleware/auth/jwt.hpp>
#include <vix/middleware/auth/rbac.hpp>
```

This example uses the modern public entry point.

## Project structure

Create:

```txt id="yilwhk"
auth_rbac_demo/
└── auth_rbac.cpp
```

Create the file:

```bash id="nxu533"
mkdir auth_rbac_demo
cd auth_rbac_demo
touch auth_rbac.cpp
```

## Source

Open:

```txt id="vg5b4v"
auth_rbac.cpp
```

Add:

```cpp id="oa3bmn"
#include <iostream>
#include <string>

#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

static const std::string kAdminToken =
  "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
  "eyJzdWIiOiJ1c2VyMTIzIiwicm9sZXMiOlsiYWRtaW4iXSwicGVybXMiOlsicHJvZHVjdHM6d3JpdGUiLCJvcmRlcnM6cmVhZCJdfQ."
  "w1y3nA2F1kq0oJ0x8wWc5wQx8zF4h2d6V7mYp0jYk3Q";

static const std::string kUserToken =
  "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
  "eyJzdWIiOiJ1c2VyNDU2Iiwicm9sZXMiOlsidXNlciJdLCJwZXJtcyI6WyJvcmRlcnM6cmVhZCJdfQ."
  "1lcu1TtxMHllkoYc5mlneK7vKLLQDe0PxUtcfPG4XVM";

static vix::App::Middleware jwt_middleware()
{
  middleware::auth::JwtOptions options;

  options.secret = "dev_secret";
  options.verify_exp = false;

  return middleware::app::adapt_ctx(
    middleware::auth::jwt(options)
  );
}

static vix::App::Middleware rbac_context_middleware()
{
  middleware::auth::RbacOptions options;

  options.require_auth = true;
  options.use_resolver = false;

  return middleware::app::adapt_ctx(
    middleware::auth::rbac_context(options)
  );
}

static vix::App::Middleware require_admin_role()
{
  return middleware::app::adapt_ctx(
    middleware::auth::require_role("admin")
  );
}

static vix::App::Middleware require_products_write()
{
  return middleware::app::adapt_ctx(
    middleware::auth::require_perm("products:write")
  );
}

static void install_common_middleware(App &app)
{
  app.use("/api", middleware::app::request_id_dev());
  app.use("/api", middleware::app::timing_dev());
  app.use("/api", middleware::app::security_headers_dev());
}

static void install_auth_middleware(App &app)
{
  app.use("/api/me", jwt_middleware());
  app.use("/api/me", rbac_context_middleware());

  app.use("/api/admin", jwt_middleware());
  app.use("/api/admin", rbac_context_middleware());
  app.use("/api/admin", require_admin_role());

  app.use("/api/products/write", jwt_middleware());
  app.use("/api/products/write", rbac_context_middleware());
  app.use("/api/products/write", require_products_write());
}

static void register_routes(App &app)
{
  app.get("/", [](Request &, Response &res)
  {
    res.send(
      "RBAC auth example\n"
      "\n"
      "Public:\n"
      "  curl -i http://127.0.0.1:8080/api/public\n"
      "\n"
      "Protected:\n"
      "  curl -i http://127.0.0.1:8080/api/me\n"
      "  curl -i -H \"Authorization: Bearer <TOKEN>\" http://127.0.0.1:8080/api/me\n"
      "\n"
      "Admin:\n"
      "  curl -i -H \"Authorization: Bearer <ADMIN_TOKEN>\" http://127.0.0.1:8080/api/admin\n"
      "\n"
      "Permission:\n"
      "  curl -i -H \"Authorization: Bearer <ADMIN_TOKEN>\" http://127.0.0.1:8080/api/products/write\n"
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
    auto &authz =
      req.state<middleware::auth::Authz>();

    res.json({
      "ok", true,
      "subject", authz.subject,
      "has_admin", authz.has_role("admin"),
      "can_write_products", authz.has_perm("products:write")
    });
  });

  app.get("/api/admin", [](Request &req, Response &res)
  {
    auto &authz =
      req.state<middleware::auth::Authz>();

    res.json({
      "ok", true,
      "message", "admin route accepted",
      "subject", authz.subject,
      "has_admin", authz.has_role("admin")
    });
  });

  app.get("/api/products/write", [](Request &req, Response &res)
  {
    auto &authz =
      req.state<middleware::auth::Authz>();

    res.json({
      "ok", true,
      "message", "products:write permission accepted",
      "subject", authz.subject,
      "can_write_products", authz.has_perm("products:write")
    });
  });
}

static void print_help()
{
  std::cout
    << "Vix RBAC auth example running:\n"
    << "  http://127.0.0.1:8080/\n"
    << "  http://127.0.0.1:8080/api/public\n"
    << "  http://127.0.0.1:8080/api/me\n"
    << "  http://127.0.0.1:8080/api/admin\n"
    << "  http://127.0.0.1:8080/api/products/write\n\n"
    << "Admin token:\n"
    << "  " << kAdminToken << "\n\n"
    << "User token:\n"
    << "  " << kUserToken << "\n\n";
}

int main()
{
  App app;

  install_common_middleware(app);
  install_auth_middleware(app);
  register_routes(app);

  print_help();

  app.run(8080);
  return 0;
}
```

## Run it

Run:

```bash id="i3my3z"
vix run auth_rbac.cpp
```

The server listens on:

```txt id="agaurf"
http://127.0.0.1:8080
```

## Test the public route

```bash id="kc3jea"
curl -i http://127.0.0.1:8080/api/public
```

Expected body:

```json id="l3w84k"
{
  "ok": true,
  "message": "public route"
}
```

This route does not require authentication.

## Set tokens

Admin token:

```bash id="zw8nyl"
ADMIN_TOKEN="eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJ1c2VyMTIzIiwicm9sZXMiOlsiYWRtaW4iXSwicGVybXMiOlsicHJvZHVjdHM6d3JpdGUiLCJvcmRlcnM6cmVhZCJdfQ.w1y3nA2F1kq0oJ0x8wWc5wQx8zF4h2d6V7mYp0jYk3Q"
```

User token:

```bash id="rt59ks"
USER_TOKEN="eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJ1c2VyNDU2Iiwicm9sZXMiOlsidXNlciJdLCJwZXJtcyI6WyJvcmRlcnM6cmVhZCJdfQ.1lcu1TtxMHllkoYc5mlneK7vKLLQDe0PxUtcfPG4XVM"
```

The admin token contains:

```json id="a1jm5v"
{
  "sub": "user123",
  "roles": ["admin"],
  "perms": ["products:write", "orders:read"]
}
```

The user token contains:

```json id="jqtghb"
{
  "sub": "user456",
  "roles": ["user"],
  "perms": ["orders:read"]
}
```

## Test missing token

```bash id="ts0bmg"
curl -i http://127.0.0.1:8080/api/me
```

Expected status:

```txt id="mqpuv5"
401 Unauthorized
```

The request has no valid JWT, so RBAC cannot build an authorization context.

## Test authenticated user

```bash id="k2lm18"
curl -i \
  -H "Authorization: Bearer $USER_TOKEN" \
  http://127.0.0.1:8080/api/me
```

Expected body shape:

```json id="r4efen"
{
  "ok": true,
  "subject": "user456",
  "has_admin": false,
  "can_write_products": false
}
```

This route only requires a valid JWT.

It does not require a specific role or permission.

## Test admin route with admin token

```bash id="urvy29"
curl -i \
  -H "Authorization: Bearer $ADMIN_TOKEN" \
  http://127.0.0.1:8080/api/admin
```

Expected body shape:

```json id="yfcc5a"
{
  "ok": true,
  "message": "admin route accepted",
  "subject": "user123",
  "has_admin": true
}
```

The admin token has:

```txt id="dj1ut7"
role = admin
```

so the request is accepted.

## Test admin route with normal user token

```bash id="vpi3e5"
curl -i \
  -H "Authorization: Bearer $USER_TOKEN" \
  http://127.0.0.1:8080/api/admin
```

Expected status:

```txt id="k46b45"
403 Forbidden
```

The user is authenticated, but not authorized.

That is the difference between:

```txt id="vehf2s"
401 Unauthorized
  authentication failed or missing

403 Forbidden
  authentication passed, authorization failed
```

## Test permission route with admin token

```bash id="qcvwq9"
curl -i \
  -H "Authorization: Bearer $ADMIN_TOKEN" \
  http://127.0.0.1:8080/api/products/write
```

Expected body shape:

```json id="dw0xk0"
{
  "ok": true,
  "message": "products:write permission accepted",
  "subject": "user123",
  "can_write_products": true
}
```

The admin token has:

```txt id="sdj5ka"
perm = products:write
```

so the request is accepted.

## Test permission route with normal user token

```bash id="wqzb11"
curl -i \
  -H "Authorization: Bearer $USER_TOKEN" \
  http://127.0.0.1:8080/api/products/write
```

Expected status:

```txt id="tfptvh"
403 Forbidden
```

The user token has:

```txt id="i84t6a"
orders:read
```

but not:

```txt id="s8kv9q"
products:write
```

so the request is rejected.

## How JWT and RBAC work together

JWT validates the token:

```cpp id="pmram0"
app.use("/api/admin", jwt_middleware());
```

RBAC builds the authorization context from JWT claims:

```cpp id="l6jmtl"
app.use("/api/admin", rbac_context_middleware());
```

Then the route requires a role:

```cpp id="ks5c7b"
app.use("/api/admin", require_admin_role());
```

The full chain is:

```txt id="szqrpq"
request
  -> JWT middleware
  -> RBAC context middleware
  -> require_role("admin")
  -> handler
```

If JWT fails, the handler does not run.

If RBAC fails, the handler does not run.

## Build RBAC context

RBAC context is built with:

```cpp id="x8udjp"
middleware::auth::RbacOptions options;

options.require_auth = true;
options.use_resolver = false;

app.use("/api/me", middleware::app::adapt_ctx(
  middleware::auth::rbac_context(options)
));
```

The middleware reads:

```cpp id="f8h8bq"
middleware::auth::JwtClaims
```

and creates:

```cpp id="o08zmx"
middleware::auth::Authz
```

The handler can then read:

```cpp id="yd0yua"
auto &authz =
  req.state<middleware::auth::Authz>();
```

## Authz

`Authz` contains:

```cpp id="ll69kq"
struct Authz
{
  std::string subject;
  std::unordered_set<std::string> roles;
  std::unordered_set<std::string> perms;

  bool has_role(std::string_view r) const;
  bool has_perm(std::string_view p) const;
};
```

Typical usage:

```cpp id="gj6zrh"
auto &authz =
  req.state<middleware::auth::Authz>();

if (authz.has_role("admin"))
{
  // admin logic
}

if (authz.has_perm("products:write"))
{
  // write product logic
}
```

Most of the time, prefer middleware checks instead of manual checks inside handlers.

## Require one role

Use:

```cpp id="o5nnyu"
middleware::auth::require_role("admin")
```

with App adaptation:

```cpp id="nu133c"
app.use("/api/admin", middleware::app::adapt_ctx(
  middleware::auth::require_role("admin")
));
```

This means the route requires:

```txt id="z1c4hf"
role = admin
```

## Require one permission

Use:

```cpp id="lsiewh"
middleware::auth::require_perm("products:write")
```

with App adaptation:

```cpp id="u22vpm"
app.use("/api/products/write", middleware::app::adapt_ctx(
  middleware::auth::require_perm("products:write")
));
```

This means the route requires:

```txt id="tfqc37"
permission = products:write
```

## Require any role

Use:

```cpp id="hidcup"
middleware::auth::require_any_role({
  "admin",
  "moderator"
})
```

This means the caller must have at least one of those roles.

Example:

```cpp id="dz7agi"
app.use("/api/moderation", middleware::app::adapt_ctx(
  middleware::auth::require_any_role({
    "admin",
    "moderator"
  })
));
```

## Require any permission

Use:

```cpp id="xlxmpc"
middleware::auth::require_any_perm({
  "products:write",
  "products:manage"
})
```

This means the caller must have at least one of those permissions.

## Require all permissions

Use:

```cpp id="e8dcgk"
middleware::auth::require_all_perms({
  "products:write",
  "products:publish"
})
```

This means the caller must have every listed permission.

Use this for sensitive operations that require several capabilities.

## Roles vs permissions

Roles are broad.

Permissions are specific.

Example roles:

```txt id="rjrglz"
admin
seller
support
moderator
```

Example permissions:

```txt id="qqqw5a"
products:write
products:delete
orders:read
orders:refund
users:ban
```

A practical design is:

```txt id="tq9sj5"
roles
  describe who the user is

permissions
  describe what the user can do
```

For critical routes, permissions are often clearer than roles.

## Claims accepted by RBAC

RBAC reads roles from the JWT payload.

Common keys:

```txt id="pt2djf"
roles
role
```

Permissions can come from:

```txt id="s9rcl0"
perms
permissions
scope
```

A `scope` string can contain space-separated permissions.

Example payload:

```json id="rq0hyx"
{
  "sub": "user123",
  "roles": ["admin"],
  "perms": ["products:write", "orders:read"],
  "scope": "products:write orders:read"
}
```

## Permission resolver

RBAC can also enrich roles and permissions through a resolver.

The interface is:

```cpp id="kdvmh5"
struct PermissionResolver
{
  virtual ~PermissionResolver() = default;

  virtual void resolve(
    std::string_view subject,
    std::unordered_set<std::string> &roles_inout,
    std::unordered_set<std::string> &perms_inout) = 0;
};
```

Use a resolver when permissions should come from a database or external service.

Example use cases:

```txt id="svdq0m"
load user roles from database
load team permissions
load tenant-specific permissions
merge JWT claims with stored permissions
```

For this example:

```cpp id="jt6al9"
options.use_resolver = false;
```

That keeps the example simple and fully token-based.

## Production secret

The example uses:

```txt id="b5kbjb"
dev_secret
```

Do not hardcode production secrets.

Production shape:

```cpp id="rk9flj"
const std::string jwt_secret =
  cfg.getString("jwt.secret", "");

if (jwt_secret.empty())
{
  throw std::runtime_error("jwt.secret is required");
}

middleware::auth::JwtOptions jwt_options;

jwt_options.secret = jwt_secret;
jwt_options.verify_exp = true;

app.use("/api/private", middleware::app::adapt_ctx(
  middleware::auth::jwt(jwt_options)
));
```

Use secrets from:

```txt id="aqs6wq"
environment variables
secret manager
deployment configuration
```

## Middleware order

The order must be correct.

Good:

```cpp id="ecse3b"
app.use("/api/admin", jwt_middleware());
app.use("/api/admin", rbac_context_middleware());
app.use("/api/admin", require_admin_role());
```

Wrong:

```cpp id="b7741u"
app.use("/api/admin", require_admin_role());
app.use("/api/admin", jwt_middleware());
app.use("/api/admin", rbac_context_middleware());
```

RBAC requirements need the `Authz` state.

`Authz` needs JWT claims.

JWT must run first.

## 401 vs 403

Use this model:

```txt id="f5bi7z"
401 Unauthorized
  no valid identity

403 Forbidden
  identity exists, but access is not allowed
```

Examples:

```txt id="wctmfo"
missing JWT
  401

invalid JWT
  401

valid JWT but missing admin role
  403

valid JWT but missing products:write permission
  403
```

This makes API behavior easier to debug.

## Protect route groups

You can protect a route group:

```cpp id="ee61nb"
app.use("/api/admin", jwt_middleware());
app.use("/api/admin", rbac_context_middleware());
app.use("/api/admin", require_admin_role());
```

Then these routes are protected:

```txt id="rxq3vh"
/api/admin
/api/admin/users
/api/admin/settings
```

If you want a public route under the same prefix, use a different prefix or install middleware more narrowly.

## Combine role and permission

For sensitive routes, you can require both:

```cpp id="sb9f5a"
app.use("/api/products/publish", jwt_middleware());
app.use("/api/products/publish", rbac_context_middleware());

app.use("/api/products/publish", middleware::app::adapt_ctx(
  middleware::auth::require_role("seller")
));

app.use("/api/products/publish", middleware::app::adapt_ctx(
  middleware::auth::require_perm("products:publish")
));
```

This means the caller must have:

```txt id="mtc1i8"
role = seller
permission = products:publish
```

## Keep handlers simple

Prefer this:

```cpp id="nfb4i8"
app.use("/api/admin", require_admin_role());

app.get("/api/admin", [](Request &, Response &res)
{
  res.json({
    "ok", true
  });
});
```

over this:

```cpp id="dpu68c"
app.get("/api/admin", [](Request &req, Response &res)
{
  auto &authz = req.state<middleware::auth::Authz>();

  if (!authz.has_role("admin"))
  {
    res.status(403).json({
      "ok", false
    });
    return;
  }

  res.json({
    "ok", true
  });
});
```

Authorization middleware keeps route handlers focused on business logic.

## Common mistakes

### Installing RBAC before JWT

Wrong:

```cpp id="mpraqt"
app.use("/admin", rbac_context_middleware());
app.use("/admin", jwt_middleware());
```

Correct:

```cpp id="znj86u"
app.use("/admin", jwt_middleware());
app.use("/admin", rbac_context_middleware());
```

### Forgetting RBAC context

Wrong:

```cpp id="hh9r1v"
app.use("/admin", jwt_middleware());
app.use("/admin", require_admin_role());
```

Correct:

```cpp id="ep3v8i"
app.use("/admin", jwt_middleware());
app.use("/admin", rbac_context_middleware());
app.use("/admin", require_admin_role());
```

`require_role` needs `Authz`.

`Authz` is created by `rbac_context`.

### Confusing role and permission

Avoid making roles too specific.

Bad role design:

```txt id="zjpmna"
products_write_user
orders_read_user
users_delete_user
```

Better:

```txt id="vl3wfd"
role
  seller

permissions
  products:write
  products:publish
```

### Trusting client-side UI only

Hiding a button in the frontend is not authorization.

The backend must enforce roles and permissions.

### Hardcoding production secrets

Use configuration for secrets.

Do not commit production secrets to the repository.

## Production notes

For production RBAC, consider:

```txt id="wyux10"
short-lived JWTs
expiration verification
issuer validation
audience validation
secret rotation
role and permission design
database-backed permission resolver
audit logs for sensitive actions
tests for protected routes
```

For enterprise apps, also consider tenant-aware permissions:

```txt id="l9itkb"
tenant:123:products:write
tenant:123:orders:read
tenant:456:products:write
```

Keep the permission format consistent.

## Complete test flow

Run:

```bash id="rbjjcz"
vix run auth_rbac.cpp
```

Set tokens:

```bash id="pgpg1l"
ADMIN_TOKEN="eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJ1c2VyMTIzIiwicm9sZXMiOlsiYWRtaW4iXSwicGVybXMiOlsicHJvZHVjdHM6d3JpdGUiLCJvcmRlcnM6cmVhZCJdfQ.w1y3nA2F1kq0oJ0x8wWc5wQx8zF4h2d6V7mYp0jYk3Q"

USER_TOKEN="eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJ1c2VyNDU2Iiwicm9sZXMiOlsidXNlciJdLCJwZXJtcyI6WyJvcmRlcnM6cmVhZCJdfQ.1lcu1TtxMHllkoYc5mlneK7vKLLQDe0PxUtcfPG4XVM"
```

Public:

```bash id="njdv00"
curl -i http://127.0.0.1:8080/api/public
```

Missing token:

```bash id="b13l73"
curl -i http://127.0.0.1:8080/api/me
```

Authenticated user:

```bash id="u4ef47"
curl -i \
  -H "Authorization: Bearer $USER_TOKEN" \
  http://127.0.0.1:8080/api/me
```

Admin accepted:

```bash id="s2l17y"
curl -i \
  -H "Authorization: Bearer $ADMIN_TOKEN" \
  http://127.0.0.1:8080/api/admin
```

Admin rejected:

```bash id="zob5zb"
curl -i \
  -H "Authorization: Bearer $USER_TOKEN" \
  http://127.0.0.1:8080/api/admin
```

Permission accepted:

```bash id="mzddpa"
curl -i \
  -H "Authorization: Bearer $ADMIN_TOKEN" \
  http://127.0.0.1:8080/api/products/write
```

Permission rejected:

```bash id="wseauf"
curl -i \
  -H "Authorization: Bearer $USER_TOKEN" \
  http://127.0.0.1:8080/api/products/write
```

## Summary

RBAC protects routes after JWT authentication.

Install JWT first:

```cpp id="qlmv23"
app.use("/api/admin", jwt_middleware());
```

Build authorization context:

```cpp id="wr3ue3"
app.use("/api/admin", rbac_context_middleware());
```

Require a role:

```cpp id="p3zvrb"
app.use("/api/admin", middleware::app::adapt_ctx(
  middleware::auth::require_role("admin")
));
```

Require a permission:

```cpp id="hb5pqw"
app.use("/api/products/write", middleware::app::adapt_ctx(
  middleware::auth::require_perm("products:write")
));
```

The mental model is:

```txt id="g0r577"
JWT
  proves identity

RBAC context
  extracts roles and permissions

require_role / require_perm
  enforces access

handler
  runs only when access is allowed
```
