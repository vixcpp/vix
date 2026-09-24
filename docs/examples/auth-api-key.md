# API Key Auth

This example shows how to protect API routes with an API key.

It demonstrates:

```txt id="x7b3p9"
public routes
protected routes
missing key response
invalid key response
valid key response
reading ApiKey state inside a handler
custom API key options
```

API key authentication is useful for simple private routes, internal APIs, admin endpoints, service-to-service calls, and early prototypes.

For user login systems, JWT or sessions are usually a better fit.

## What this example builds

The app exposes:

```txt id="rskw25"
GET /api/health
GET /api/admin/status
GET /api/admin/metrics
```

The route `/api/health` is public.

The routes under `/api/admin` require:

```txt id="3dcx87"
x-api-key: secret
```

## Source

Create a file:

```txt id="z50pzi"
auth_api_key.cpp
```

Add this code:

```cpp id="m77naf"
#include <string>

#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

static void install_middleware(App &app)
{
  app.use("/api", middleware::app::request_id_dev());
  app.use("/api", middleware::app::timing_dev());
  app.use("/api", middleware::app::security_headers_dev());

  app.use("/api/admin", middleware::app::api_key_dev("secret"));
}

static void register_routes(App &app)
{
  app.get("/", [](Request &, Response &res)
  {
    res.text("API Key Auth example. Try /api/health or /api/admin/status.");
  });

  app.get("/api/health", [](Request &, Response &res)
  {
    res.json({
      "ok", true,
      "service", "auth-api-key"
    });
  });

  app.get("/api/admin/status", [](Request &req, Response &res)
  {
    auto &api_key = req.state<middleware::auth::ApiKey>();

    res.json({
      "ok", true,
      "admin", true,
      "message", "API key accepted",
      "key_size", static_cast<long long>(api_key.value.size())
    });
  });

  app.get("/api/admin/metrics", [](Request &, Response &res)
  {
    res.json({
      "ok", true,
      "requests", 42,
      "errors", 0
    });
  });
}

int main()
{
  App app;

  install_middleware(app);
  register_routes(app);

  app.run(8080);
  return 0;
}
```

## Run it

```bash id="csahxw"
vix run auth_api_key.cpp
```

The server listens on:

```txt id="vhaj9e"
http://127.0.0.1:8080
```

## Test the public route

```bash id="9yco3c"
curl -i http://127.0.0.1:8080/api/health
```

Expected status:

```txt id="n1ey48"
200 OK
```

Expected body shape:

```json id="s2dn2y"
{
  "ok": true,
  "service": "auth-api-key"
}
```

This route is public because the API key middleware is installed only on:

```cpp id="y06l89"
app.use("/api/admin", middleware::app::api_key_dev("secret"));
```

So it applies to:

```txt id="f7jdfi"
/api/admin/status
/api/admin/metrics
```

It does not apply to:

```txt id="zoml06"
/api/health
/
```

## Test missing API key

```bash id="m9ax0s"
curl -i http://127.0.0.1:8080/api/admin/status
```

Expected status:

```txt id="wi38qm"
401 Unauthorized
```

Expected error code:

```txt id="tdlsze"
missing_api_key
```

The handler is not called.

The middleware stops the request before the route logic runs.

## Test invalid API key

```bash id="85ksnf"
curl -i \
  http://127.0.0.1:8080/api/admin/status \
  -H "x-api-key: wrong"
```

Expected status:

```txt id="yatsk4"
403 Forbidden
```

Expected error code:

```txt id="xmq4jm"
invalid_api_key
```

The request provided a key, but the key was not accepted.

## Test valid API key

```bash id="zcqb7i"
curl -i \
  http://127.0.0.1:8080/api/admin/status \
  -H "x-api-key: secret"
```

Expected status:

```txt id="bbhd1h"
200 OK
```

Expected body shape:

```json id="w6yjvm"
{
  "ok": true,
  "admin": true,
  "message": "API key accepted",
  "key_size": 6
}
```

The route can access the key through typed request state:

```cpp id="whqwl0"
auto &api_key = req.state<middleware::auth::ApiKey>();
```

In a real application, do not return the API key value to the client.

This example returns only the key size to show that the state exists.

## Test another protected route

```bash id="daxh7t"
curl -i \
  http://127.0.0.1:8080/api/admin/metrics \
  -H "x-api-key: secret"
```

Expected body shape:

```json id="o1fcwk"
{
  "ok": true,
  "requests": 42,
  "errors": 0
}
```

Because the middleware is installed on `/api/admin`, all routes under that prefix are protected.

## How it works

The important line is:

```cpp id="yaef4c"
app.use("/api/admin", middleware::app::api_key_dev("secret"));
```

This installs API key authentication only for the `/api/admin` prefix.

The request flow for `/api/admin/status` is:

```txt id="hs4gj6"
request
  -> request id
  -> timing
  -> security headers
  -> API key middleware
  -> route handler
  -> response
```

If the key is missing:

```txt id="eephu7"
request
  -> API key middleware
  -> 401 response
```

If the key is invalid:

```txt id="r46vi7"
request
  -> API key middleware
  -> 403 response
```

If the key is valid:

```txt id="sg0pzd"
request
  -> API key middleware
  -> route handler
  -> 200 response
```

## Why 401 and 403 are different

A missing key returns:

```txt id="sc1kgk"
401 Unauthorized
```

That means the request is not authenticated.

An invalid key returns:

```txt id="z3b15l"
403 Forbidden
```

That means credentials were provided but rejected.

This distinction makes debugging and client behavior clearer.

## Custom API key configuration

The preset is good for simple examples.

For custom behavior, use `ApiKeyOptions`.

This version accepts:

```txt id="p0n3lv"
x-api-key: secret
```

and also:

```txt id="oc5vkc"
?api_key=secret
```

```cpp id="w3hy6n"
#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

int main()
{
  App app;

  middleware::auth::ApiKeyOptions opt;

  opt.header = "x-api-key";
  opt.query_param = "api_key";
  opt.required = true;
  opt.allowed_keys = {"secret"};

  app.use("/api/admin", middleware::app::adapt_ctx(
    middleware::auth::api_key(opt)
  ));

  app.get("/api/admin/status", [](Request &req, Response &res)
  {
    auto &api_key = req.state<middleware::auth::ApiKey>();

    res.json({
      "ok", true,
      "admin", true,
      "key_size", static_cast<long long>(api_key.value.size())
    });
  });

  app.run(8080);
}
```

Test with header:

```bash id="wsxq7q"
curl -i \
  http://127.0.0.1:8080/api/admin/status \
  -H "x-api-key: secret"
```

Test with query parameter:

```bash id="u6k094"
curl -i \
  "http://127.0.0.1:8080/api/admin/status?api_key=secret"
```

Prefer headers for production APIs.

Query parameters can be stored in logs, browser history, reverse proxy logs, and analytics systems.

## Custom validation function

You can validate keys dynamically.

```cpp id="ecx3vr"
middleware::auth::ApiKeyOptions opt;

opt.header = "x-api-key";
opt.required = true;

opt.validate = [](const std::string &key)
{
  return key == "secret";
};

app.use("/api/admin", middleware::app::adapt_ctx(
  middleware::auth::api_key(opt)
));
```

Use this pattern when the key must be checked against:

```txt id="njnvv9"
a database
a cache
a configuration file
a tenant table
an internal service
```

The middleware accepts the key only if `validate(...)` returns `true`.

## Custom extraction function

You can also control how the key is extracted from the request.

```cpp id="m2prdt"
middleware::auth::ApiKeyOptions opt;

opt.required = true;

opt.extract = [](const middleware::Request &req)
{
  std::string key = req.header("x-api-key");

  if (!key.empty())
    return key;

  return req.header("x-admin-key");
};

opt.validate = [](const std::string &key)
{
  return key == "secret";
};

app.use("/api/admin", middleware::app::adapt_ctx(
  middleware::auth::api_key(opt)
));
```

Use `extract` when the API key can come from a custom header or a special gateway convention.

## Optional API key

Sometimes a route can accept an API key when present but still allow anonymous requests.

Use:

```cpp id="rutq7u"
middleware::auth::ApiKeyOptions opt;

opt.header = "x-api-key";
opt.required = false;
opt.allowed_keys = {"secret"};

app.use("/api", middleware::app::adapt_ctx(
  middleware::auth::api_key(opt)
));
```

When `required` is false:

```txt id="qxqmj1"
missing key
  request continues

invalid key
  request is rejected

valid key
  ApiKey state is stored
```

In the handler, use `try_state`:

```cpp id="vr2iua"
auto *api_key = req.try_state<middleware::auth::ApiKey>();

if (api_key)
{
  res.json({
    "ok", true,
    "authenticated", true
  });
  return;
}

res.json({
  "ok", true,
  "authenticated", false
});
```

Use optional API keys for routes that can behave differently for trusted clients but still allow public access.

## API key with other middleware

API key auth should usually run after broad security middleware and before protected handlers.

A practical order is:

```cpp id="hmcgsw"
app.use("/api", middleware::app::request_id_dev());
app.use("/api", middleware::app::timing_dev());
app.use("/api", middleware::app::security_headers_dev());
app.use("/api", middleware::app::cors_dev({"https://example.com"}));
app.use("/api", middleware::app::rate_limit_dev());

app.use("/api/admin", middleware::app::api_key_dev("secret"));
```

The idea is:

```txt id="lhzly4"
request id
  identify the request

timing
  measure the request

security headers
  harden responses

CORS
  handle browser access

rate limit
  block abusive clients

API key
  protect private routes
```

For routes with JSON bodies, add the parser after the broad safety middleware:

```cpp id="gq1t8u"
app.use("/api/admin/products", middleware::app::api_key_dev("secret"));
app.use("/api/admin/products", middleware::app::json_strict_dev(4096));
```

## API key vs JWT

Use API keys when the caller is usually a system, script, service, or admin tool.

```txt id="nnhopd"
internal service
deployment tool
private admin endpoint
simple automation
```

Use JWT when the caller is usually a user or sessionless client identity.

```txt id="jewd99"
user login
mobile app
frontend app
role-based API
permission-based API
```

A simple rule:

```txt id="jrvedy"
API key
  simple service access

JWT
  user identity and claims

RBAC
  roles and permissions after JWT
```

## Production notes

For production:

```txt id="wmjki6"
do not hardcode secrets in source code
read API keys from environment or config
do not log full API keys
prefer headers over query parameters
rotate keys when needed
use HTTPS
combine with rate limiting
```

Example using a configuration value:

```cpp id="bpn26b"
const std::string admin_key = cfg.getString("security.admin_api_key", "");

app.use("/api/admin", middleware::app::api_key_dev(admin_key));
```

Make sure the key is not empty before starting the server.

## Complete test flow

Run:

```bash id="ydxu1j"
vix run auth_api_key.cpp
```

Public route:

```bash id="f7qk18"
curl -i http://127.0.0.1:8080/api/health
```

Missing key:

```bash id="g5poe0"
curl -i http://127.0.0.1:8080/api/admin/status
```

Invalid key:

```bash id="knsril"
curl -i \
  http://127.0.0.1:8080/api/admin/status \
  -H "x-api-key: wrong"
```

Valid key:

```bash id="vxmt45"
curl -i \
  http://127.0.0.1:8080/api/admin/status \
  -H "x-api-key: secret"
```

Another protected route:

```bash id="v4qap1"
curl -i \
  http://127.0.0.1:8080/api/admin/metrics \
  -H "x-api-key: secret"
```

## Summary

API key authentication is the simplest way to protect a route group.

Use:

```cpp id="aig58f"
app.use("/api/admin", middleware::app::api_key_dev("secret"));
```

Then read the authenticated key state when needed:

```cpp id="qbcmjw"
auto &api_key = req.state<middleware::auth::ApiKey>();
```

The behavior is:

```txt id="y4s6df"
missing key
  401 missing_api_key

invalid key
  403 invalid_api_key

valid key
  handler runs
```

Use API keys for simple private access.

Use JWT and RBAC when user identity, claims, roles, and permissions matter.
