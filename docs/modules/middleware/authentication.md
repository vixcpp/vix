# Authentication

The `auth` group identifies callers and protects private routes.

It provides API key authentication, JWT authentication, RBAC authorization, permission checks, and sessions.

Security middleware protects the HTTP surface.

```txt
CORS
CSRF
security headers
IP filter
rate limit
```

Authentication middleware protects access to private application behavior.

```txt
API keys
JWT claims
roles
permissions
sessions
```

The authentication middleware lives under:

```cpp
namespace vix::middleware::auth
```

When using `vix::App`, prefer the App helpers:

```cpp
namespace vix::middleware::app
```

## What authentication provides

The `auth` group includes:

| Middleware            | Purpose                                                |
| --------------------- | ------------------------------------------------------ |
| `api_key()`           | Authenticate requests using an API key                 |
| `jwt()`               | Authenticate requests using a Bearer JWT               |
| `rbac_context()`      | Build an authorization context from JWT claims         |
| `require_role()`      | Require one role                                       |
| `require_any_role()`  | Require at least one role                              |
| `require_perm()`      | Require one permission                                 |
| `require_any_perm()`  | Require at least one permission                        |
| `require_all_perms()` | Require all listed permissions                         |
| `session()`           | Load, create, update, and destroy server-side sessions |

For normal `vix::App` applications, use App presets when available:

```cpp
middleware::app::api_key_dev(...)
middleware::app::jwt_dev(...)
middleware::app::session_dev(...)
```

For RBAC and custom options, use the lower-level middleware with `app::adapt_ctx(...)`.

## Authentication vs authorization

Authentication verifies identity.

```txt
Who is calling?
```

Authorization verifies access.

```txt
Is this caller allowed to do this?
```

Example:

```txt
JWT authentication
  validates the token
  stores JwtClaims

RBAC authorization
  reads JwtClaims
  builds Authz
  checks roles and permissions
```

Keep this mental model:

```txt
authentication first
authorization after
handler last
```

Example order:

```cpp
app.use("/admin", middleware::app::jwt_dev("dev_secret"));

app.use("/admin", middleware::app::adapt_ctx(
  middleware::auth::rbac_context({
    .require_auth = true,
    .use_resolver = false
  })
));

app.use("/admin", middleware::app::adapt_ctx(
  middleware::auth::require_role("admin")
));
```

## API key authentication

API keys are useful for:

```txt
internal APIs
admin routes
service-to-service calls
simple private endpoints
early prototypes
```

The App preset is:

```cpp
app.use("/secure", middleware::app::api_key_dev("secret"));
```

Example:

```cpp
#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

int main()
{
  App app;

  app.use("/secure", middleware::app::api_key_dev("secret"));

  app.get("/", [](Request &, Response &res)
  {
    res.text("GET /secure requires x-api-key: secret");
  });

  app.get("/secure", [](Request &req, Response &res)
  {
    auto &key = req.state<middleware::auth::ApiKey>();

    res.json({
      "ok", true,
      "key_size", static_cast<long long>(key.value.size())
    });
  });

  app.run(8080);
}
```

Test missing key:

```bash
curl -i http://127.0.0.1:8080/secure
```

Expected status:

```txt
401 Unauthorized
```

Test invalid key:

```bash
curl -i \
  http://127.0.0.1:8080/secure \
  -H "x-api-key: wrong"
```

Expected status:

```txt
403 Forbidden
```

Test valid key:

```bash
curl -i \
  http://127.0.0.1:8080/secure \
  -H "x-api-key: secret"
```

Expected status:

```txt
200 OK
```

The API key middleware stores:

```cpp
vix::middleware::auth::ApiKey
```

The handler can read it with:

```cpp
auto &key = req.state<vix::middleware::auth::ApiKey>();
```

Do not return real API keys in production responses.

The example only shows how typed request state works.

## API key from query parameter

API keys can also be extracted from a query parameter when configured.

```cpp
vix::middleware::auth::ApiKeyOptions opt;

opt.header = "x-api-key";
opt.query_param = "api_key";
opt.allowed_keys = {"secret"};

app.use("/secure", vix::middleware::app::adapt_ctx(
  vix::middleware::auth::api_key(opt)
));
```

Now both requests can work:

```bash
curl -i \
  http://127.0.0.1:8080/secure \
  -H "x-api-key: secret"
```

```bash
curl -i \
  "http://127.0.0.1:8080/secure?api_key=secret"
```

Prefer headers for production APIs.

Query parameters can appear in logs, browser history, proxy logs, and analytics tools.

## Configure API key authentication

Use `ApiKeyOptions` for explicit behavior.

```cpp
vix::middleware::auth::ApiKeyOptions opt;

opt.header = "x-api-key";
opt.query_param = "";
opt.required = true;
opt.allowed_keys = {"secret"};

app.use("/secure", vix::middleware::app::adapt_ctx(
  vix::middleware::auth::api_key(opt)
));
```

Main options:

| Option         | Purpose                           |
| -------------- | --------------------------------- |
| `header`       | Header used to read the API key   |
| `query_param`  | Query parameter used as fallback  |
| `required`     | Whether missing keys are rejected |
| `allowed_keys` | Static set of accepted keys       |
| `extract`      | Custom extraction function        |
| `validate`     | Custom validation function        |

Use `validate` when the key must be checked against a database, cache, or external service.

```cpp
opt.validate = [](const std::string &key)
{
  return key == "secret";
};
```

Common errors:

| Status | Code              | Meaning                         |
| ------ | ----------------- | ------------------------------- |
| `401`  | `missing_api_key` | No key was provided             |
| `403`  | `invalid_api_key` | A key was provided but rejected |

## JWT authentication

JWT authentication validates a Bearer token and stores claims in request state.

The App preset is:

```cpp
app.use("/secure", middleware::app::jwt_dev("dev_secret"));
```

Example:

```cpp
#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

int main()
{
  App app;

  app.use("/secure", middleware::app::jwt_dev("dev_secret"));

  app.get("/", [](Request &, Response &res)
  {
    res.text("GET /secure requires Authorization: Bearer <token>");
  });

  app.get("/secure", [](Request &req, Response &res)
  {
    auto &claims = req.state<middleware::auth::JwtClaims>();

    res.json({
      "ok", true,
      "sub", claims.subject
    });
  });

  app.run(8080);
}
```

Test without token:

```bash
curl -i http://127.0.0.1:8080/secure
```

Expected status:

```txt
401 Unauthorized
```

Test with token:

```bash
TOKEN="..."

curl -i \
  http://127.0.0.1:8080/secure \
  -H "Authorization: Bearer $TOKEN"
```

Expected status:

```txt
200 OK
```

When the token is valid, the middleware stores:

```cpp
vix::middleware::auth::JwtClaims
```

The handler can read:

```cpp
auto &claims = req.state<vix::middleware::auth::JwtClaims>();
```

## JWT claims

JWT middleware extracts identity information into `JwtClaims`.

Common values include:

```txt
subject
roles
payload
```

Example handler:

```cpp
app.get("/secure", [](vix::Request &req, vix::Response &res)
{
  auto &claims = req.state<vix::middleware::auth::JwtClaims>();

  res.json({
    "ok", true,
    "sub", claims.subject,
    "roles_count", static_cast<long long>(claims.roles.size())
  });
});
```

Use the claims to identify the caller.

Use RBAC middleware to decide what the caller is allowed to do.

## Configure JWT authentication

Use lower-level options when the preset is not enough.

```cpp
vix::middleware::auth::JwtOptions opt;

opt.secret = "dev_secret";
opt.verify_exp = false;

app.use("/secure", vix::middleware::app::adapt_ctx(
  vix::middleware::auth::jwt(opt)
));
```

In development examples, `verify_exp` may be disabled to make demo tokens easier.

For production, expiration validation should normally be enabled.

Keep secrets out of source code in real applications. Read them from configuration or environment variables.

## RBAC

RBAC means role-based access control.

The usual flow is:

```txt
JWT middleware
  validates token
  stores JwtClaims

rbac_context()
  reads JwtClaims
  creates Authz

require_role() / require_perm()
  checks Authz

handler
  runs only when authorization passes
```

Example:

```cpp
#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

int main()
{
  App app;

  vix::middleware::auth::JwtOptions jwt_opt;
  jwt_opt.secret = "dev_secret";
  jwt_opt.verify_exp = false;

  vix::middleware::auth::RbacOptions rbac_opt;
  rbac_opt.require_auth = true;
  rbac_opt.use_resolver = false;

  app.use("/admin", middleware::app::adapt_ctx(
    middleware::auth::jwt(jwt_opt)
  ));

  app.use("/admin", middleware::app::adapt_ctx(
    middleware::auth::rbac_context(rbac_opt)
  ));

  app.use("/admin", middleware::app::adapt_ctx(
    middleware::auth::require_role("admin")
  ));

  app.use("/admin", middleware::app::adapt_ctx(
    middleware::auth::require_perm("products:write")
  ));

  app.get("/admin", [](Request &req, Response &res)
  {
    auto &authz = req.state<middleware::auth::Authz>();

    res.json({
      "ok", true,
      "sub", authz.subject,
      "has_admin", authz.has_role("admin"),
      "has_products_write", authz.has_perm("products:write")
    });
  });

  app.run(8080);
}
```

This route requires:

```txt
valid JWT
role: admin
permission: products:write
```

If any step fails, the handler is not called.

## Authz state

`rbac_context()` stores:

```cpp
vix::middleware::auth::Authz
```

`Authz` contains:

```txt
subject
roles
perms
```

The handler can read it:

```cpp
auto &authz = req.state<vix::middleware::auth::Authz>();

const bool is_admin = authz.has_role("admin");
const bool can_write = authz.has_perm("products:write");
```

Authorization checks should usually be done by middleware before the handler.

Handler checks are useful for optional behavior or response shaping.

## RBAC options

Use `RbacOptions` to control how RBAC state is built.

```cpp
vix::middleware::auth::RbacOptions opt;

opt.roles_key = "roles";
opt.perms_key = "perms";
opt.require_auth = true;
opt.use_resolver = true;
```

Main options:

| Option         | Purpose                                         |
| -------------- | ----------------------------------------------- |
| `roles_key`    | JWT payload key used for roles                  |
| `perms_key`    | JWT payload key used for permissions            |
| `require_auth` | Reject request when `JwtClaims` is missing      |
| `use_resolver` | Use a `PermissionResolver` service if available |

`rbac_context()` can read permissions from JWT claims.

It can also enrich roles and permissions through a resolver service.

## Permission resolver

A `PermissionResolver` can enrich authorization data.

```cpp
struct MyResolver final : vix::middleware::auth::PermissionResolver
{
  void resolve(
    std::string_view subject,
    std::unordered_set<std::string> &roles,
    std::unordered_set<std::string> &perms) override
  {
    if (subject == "user123")
    {
      roles.insert("admin");
      perms.insert("products:write");
    }
  }
};
```

This is useful when:

```txt
JWT contains identity
database contains roles
permissions are managed centrally
tenants have dynamic access rules
```

The resolver is looked up from middleware services.

Use this pattern for advanced integrations.

For simple examples, keep `use_resolver = false`.

## Role and permission middleware

Available checks include:

```cpp
vix::middleware::auth::require_role("admin")
vix::middleware::auth::require_any_role({"admin", "owner"})
vix::middleware::auth::require_perm("products:write")
vix::middleware::auth::require_any_perm({"products:write", "products:delete"})
vix::middleware::auth::require_all_perms({"products:write", "orders:read"})
```

Install them with `adapt_ctx()`:

```cpp
app.use("/admin", vix::middleware::app::adapt_ctx(
  vix::middleware::auth::require_role("admin")
));
```

Common errors:

| Status | Code            | Meaning                         |
| ------ | --------------- | ------------------------------- |
| `401`  | `missing_auth`  | JWT/auth context is missing     |
| `401`  | `missing_authz` | RBAC context is missing         |
| `403`  | `forbidden`     | Role or permission check failed |

## Sessions

`session()` loads or creates a server-side session and stores it in request state.

A session can hold small key/value data for the current client.

Example:

```cpp
#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

int main()
{
  App app;

  vix::middleware::auth::SessionOptions opt;
  opt.secret = "dev";

  app.use(middleware::app::adapt_ctx(
    middleware::auth::session(opt)
  ));

  app.get("/session", [](Request &req, Response &res)
  {
    auto &session = req.state<middleware::auth::Session>();

    int n = 0;

    if (auto current = session.get("n"))
      n = std::stoi(*current);

    session.set("n", std::to_string(n + 1));

    res.text("n=" + std::to_string(n + 1));
  });

  app.run(8080);
}
```

Test:

```bash
curl -i -c cookies.txt -b cookies.txt http://127.0.0.1:8080/session
curl -i -c cookies.txt -b cookies.txt http://127.0.0.1:8080/session
curl -i -c cookies.txt -b cookies.txt http://127.0.0.1:8080/session
```

Expected response shape:

```txt
n=1
n=2
n=3
```

The middleware stores:

```cpp
vix::middleware::auth::Session
```

The handler can read and update it.

## Session options

Use `SessionOptions` to configure session behavior.

```cpp
vix::middleware::auth::SessionOptions opt;

opt.secret = "dev";
opt.cookie_name = "sid";
opt.cookie_path = "/";
opt.secure = false;
opt.http_only = true;
opt.same_site = "Lax";
opt.auto_create = true;
opt.ttl = std::chrono::hours(24 * 7);

app.use(vix::middleware::app::adapt_ctx(
  vix::middleware::auth::session(opt)
));
```

Main options:

| Option        | Purpose                               |
| ------------- | ------------------------------------- |
| `store`       | Custom session store                  |
| `secret`      | Secret used by the session middleware |
| `cookie_name` | Session cookie name                   |
| `cookie_path` | Cookie path                           |
| `secure`      | Add `Secure` to session cookie        |
| `http_only`   | Add `HttpOnly` to session cookie      |
| `same_site`   | SameSite cookie policy                |
| `ttl`         | Session lifetime                      |
| `auto_create` | Create a session when missing         |

Use `secure = true` only when serving over HTTPS.

In production, use a strong secret from configuration.

## Session store

The module provides an in-memory session store.

```cpp
vix::middleware::auth::InMemorySessionStore
```

In-memory storage is useful for development and simple demos.

For production, use a store that survives process restarts and works across instances.

Possible production stores include:

```txt
database-backed store
Redis-backed store
custom durable store
```

Implement:

```cpp
class ISessionStore
{
public:
  virtual ~ISessionStore() = default;

  virtual std::optional<Session> load(const std::string &sid) = 0;
  virtual void save(const Session &s, std::chrono::seconds ttl) = 0;
  virtual void destroy(const std::string &sid) = 0;
};
```

Then pass it through `SessionOptions`.

```cpp
vix::middleware::auth::SessionOptions opt;

opt.secret = "prod_secret";
opt.store = std::make_shared<MySessionStore>();
```

## API key vs JWT vs session

Use this rule:

| Need                         | Use                   |
| ---------------------------- | --------------------- |
| Simple service route         | API key               |
| Stateless API authentication | JWT                   |
| Role and permission checks   | JWT + RBAC            |
| Browser session state        | Session               |
| Cookie-based form protection | Session + CSRF        |
| Internal admin endpoint      | API key or JWT + RBAC |

A common API stack:

```cpp
app.use("/api", middleware::app::security_headers_dev());
app.use("/api", middleware::app::cors_dev());
app.use("/api", middleware::app::rate_limit_dev());

app.use("/api/admin", middleware::app::api_key_dev("secret"));
```

A common JWT stack:

```cpp
app.use("/api/private", middleware::app::jwt_dev("dev_secret"));

app.use("/api/private", middleware::app::adapt_ctx(
  middleware::auth::rbac_context({
    .require_auth = true,
    .use_resolver = false
  })
));
```

A common browser stack:

```cpp
app.use("/app", middleware::app::security_headers_dev());
app.use("/app", middleware::app::csrf_dev());

app.use("/app", middleware::app::adapt_ctx(
  middleware::auth::session({
    .secret = "dev"
  })
));
```

## Complete example

This example protects an admin route using API key authentication and keeps a simple session counter.

```cpp
#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

int main()
{
  App app;

  app.use("/api", middleware::app::security_headers_dev());
  app.use("/api", middleware::app::rate_limit_dev());

  app.use("/api/admin", middleware::app::api_key_dev("secret"));

  vix::middleware::auth::SessionOptions session_opt;
  session_opt.secret = "dev";

  app.use("/session", middleware::app::adapt_ctx(
    middleware::auth::session(session_opt)
  ));

  app.get("/api/health", [](Request &, Response &res)
  {
    res.json({
      "ok", true
    });
  });

  app.get("/api/admin/status", [](Request &req, Response &res)
  {
    auto &key = req.state<middleware::auth::ApiKey>();

    res.json({
      "ok", true,
      "admin", true,
      "key_size", static_cast<long long>(key.value.size())
    });
  });

  app.get("/session", [](Request &req, Response &res)
  {
    auto &session = req.state<middleware::auth::Session>();

    int n = 0;

    if (auto current = session.get("n"))
      n = std::stoi(*current);

    session.set("n", std::to_string(n + 1));

    res.text("n=" + std::to_string(n + 1));
  });

  app.run(8080);
}
```

Run:

```bash
vix run authentication_demo.cpp
```

Test public route:

```bash
curl -i http://127.0.0.1:8080/api/health
```

Test admin route without key:

```bash
curl -i http://127.0.0.1:8080/api/admin/status
```

Test admin route with key:

```bash
curl -i \
  http://127.0.0.1:8080/api/admin/status \
  -H "x-api-key: secret"
```

Test session route:

```bash
curl -i -c cookies.txt -b cookies.txt http://127.0.0.1:8080/session
curl -i -c cookies.txt -b cookies.txt http://127.0.0.1:8080/session
```

## Summary

Use the authentication group to identify callers and protect private routes.

Start simple:

```cpp
app.use("/api/admin", middleware::app::api_key_dev("secret"));
```

Use JWT for stateless API identity:

```cpp
app.use("/api/private", middleware::app::jwt_dev("dev_secret"));
```

Use RBAC after JWT when roles and permissions matter:

```cpp
app.use("/api/private", middleware::app::adapt_ctx(
  middleware::auth::rbac_context()
));

app.use("/api/private", middleware::app::adapt_ctx(
  middleware::auth::require_role("admin")
));
```

Use sessions when browser or server-side state matters:

```cpp
app.use(middleware::app::adapt_ctx(
  middleware::auth::session({
    .secret = "dev"
  })
));
```

Remember the order:

```txt
security first
authentication next
authorization after authentication
handler last
```
