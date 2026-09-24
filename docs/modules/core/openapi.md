# OpenAPI

`vix::openapi` provides OpenAPI generation and built-in API documentation routes for Vix Core.

Use it when you want to expose an OpenAPI 3 document from a Vix router and serve an offline Swagger UI page for your HTTP application.

## Public header

```cpp
#include <vix/openapi/OpenApi.hpp>
```

To register the documentation routes, include:

```cpp
#include <vix/openapi/register_docs.hpp>
```

## What OpenAPI provides

The OpenAPI module provides:

- OpenAPI 3.0.3 JSON generation
- route documentation from the Vix router
- extra documentation registered by modules
- stable `operationId` generation
- default responses when no route response metadata exists
- `/openapi.json` route registration
- `/docs` Swagger UI route registration
- embedded Swagger UI CSS and JavaScript assets
- offline documentation UI with no external CDN dependency

## Basic model

The OpenAPI generator reads route metadata from the router.

```text
Router
  -> routes()
  -> RouteDoc
  -> OpenAPI JSON
```

It also reads extra docs from the global OpenAPI registry.

```text
Registry
  -> extra module docs
  -> OpenAPI JSON
```

The final document is built as:

```text
router routes + registry docs
  -> OpenAPI 3.0.3 document
```

## Generate OpenAPI from a router

```cpp
#include <vix.hpp>
#include <vix/openapi/OpenApi.hpp>

int main()
{
  vix::App app;

  app.get("/status", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.json({"status", "ok"});
  });

  auto doc = vix::openapi::build_from_router(
      *app.router(),
      "My API",
      "1.0.0");

  vix::print(doc.dump(2));

  return 0;
}
```

## Register OpenAPI and docs routes

Use `register_openapi_and_docs(...)` to add built-in documentation routes to a router.

```cpp
#include <vix.hpp>
#include <vix/openapi/register_docs.hpp>

int main()
{
  vix::App app;

  app.get("/status", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.json({"status", "ok"});
  });

  vix::openapi::register_openapi_and_docs(
      *app.router(),
      "My API",
      "1.0.0");

  app.run(8080);

  return 0;
}
```

Then open:

```text
http://localhost:8080/openapi.json
```

or:

```text
http://localhost:8080/docs
```

## Registered routes

`register_openapi_and_docs(...)` registers these routes:

```text
GET /openapi.json
GET /docs
GET /docs/
GET /docs/index.html
GET /docs/swagger-ui.css
GET /docs/swagger-ui-bundle.js
```

## /openapi.json

The `/openapi.json` route returns the generated OpenAPI 3 document.

```text
GET /openapi.json
```

Response content type:

```text
application/json; charset=utf-8
```

The route also sets:

```text
Cache-Control: no-store
X-Content-Type-Options: nosniff
```

## /docs

The `/docs` route serves the Swagger UI page.

```text
GET /docs
```

It renders the generated `/openapi.json` document.

The Swagger UI assets are served locally from embedded assets.

```text
/docs/swagger-ui.css
/docs/swagger-ui-bundle.js
```

This means the docs UI can work without loading Swagger UI from an external CDN.

## /docs/ and /docs/index.html

These routes serve the same docs page as `/docs`.

```text
GET /docs/
GET /docs/index.html
```

They are registered directly to avoid redirect loops and to support static-like expectations.

## OpenAPI document shape

The generated document starts with:

```json
{
  "openapi": "3.0.3",
  "info": {
    "title": "Vix API",
    "version": "1.31.0"
  },
  "paths": {},
  "components": {}
}
```

The title and version can be customized.

```cpp
auto doc = vix::openapi::build_from_router(
    router,
    "My API",
    "1.0.0");
```

## Route documentation

OpenAPI metadata comes from `vix::router::RouteDoc`.

```cpp
vix::router::RouteDoc doc;

doc.summary = "Get status";
doc.description = "Returns the current service status.";
doc.tags = {"system"};
doc.responses["200"] = {
  {"description", "OK"}
};
```

A `RouteDoc` can contain:

| Field          | Purpose                             |
| -------------- | ----------------------------------- |
| `summary`      | Short one-line route summary.       |
| `description`  | Longer route description.           |
| `tags`         | OpenAPI tags.                       |
| `request_body` | OpenAPI request body object.        |
| `responses`    | OpenAPI responses object.           |
| `x`            | Vendor-specific OpenAPI extensions. |

## RouteDoc example

```cpp
vix::router::RouteDoc doc;

doc.summary = "Create user";
doc.description = "Creates a new user.";
doc.tags = {"users"};

doc.request_body = {
  {"required", true},
  {"content", {
    {"application/json", {
      {"schema", {
        {"type", "object"},
        {"properties", {
          {"name", {{"type", "string"}}}
        }}
      }}
    }}
  }}
};

doc.responses["201"] = {
  {"description", "User created"}
};
```

## Register a documented route manually

Advanced code can register a route directly on the router with documentation metadata.

```cpp
#include <vix.hpp>

int main()
{
  vix::App app;

  using Fn = std::function<void(vix::Request &, vix::Response &)>;

  auto handler = std::make_shared<vix::http::RequestHandler<Fn>>(
      "/status",
      [](vix::Request &req, vix::Response &res)
      {
        (void)req;

        res.json({"status", "ok"});
      });

  vix::router::RouteDoc doc;
  doc.summary = "Service status";
  doc.description = "Returns whether the service is running.";
  doc.tags = {"system"};
  doc.responses["200"] = {{"description", "OK"}};

  app.router()->add_route(
      "GET",
      "/status",
      handler,
      vix::router::RouteOptions{},
      doc);

  app.run(8080);

  return 0;
}
```

Most applications should register routes through `App`.

Direct router registration is mainly useful for tooling, generated routes, or framework internals.

## OpenAPI method conversion

`method_to_openapi(...)` converts HTTP methods to OpenAPI operation keys.

```cpp
vix::openapi::method_to_openapi("GET");
```

Result:

```text
get
```

Supported methods:

```text
GET
POST
PUT
DELETE
PATCH
HEAD
OPTIONS
TRACE
```

Unsupported methods return an empty string.

## Default responses

If a route has no response documentation, Vix adds a default response.

```json
{
  "200": {
    "description": "OK"
  }
}
```

This is produced by:

```cpp
vix::openapi::default_responses();
```

## Operation IDs

Vix generates a stable `operationId` from method and path.

```cpp
vix::openapi::make_operation_id("get", "/users/{id}");
```

Result shape:

```text
get__users__id
```

The function keeps letters and numbers.

Other characters are replaced with `_`.

This gives client generators a stable operation name.

## Build from router

Use `build_from_router(...)` to create the OpenAPI JSON document.

```cpp
nlohmann::json doc = vix::openapi::build_from_router(
    router,
    "Vix API",
    "1.31.0");
```

Signature:

```cpp
nlohmann::json build_from_router(
    const vix::router::Router &router,
    std::string title = "Vix API",
    std::string version = "1.31.0");
```

It includes:

- routes declared in the HTTP router
- extra docs registered in `vix::openapi::Registry`
- default responses for undocumented responses
- route summaries, descriptions, tags, request bodies, responses, and extensions

## Extra route docs

Some modules may need to document routes that are not registered directly in the core router.

Use `vix::openapi::Registry`.

```cpp
vix::router::RouteDoc doc;

doc.summary = "WebSocket endpoint";
doc.description = "Upgrades the connection to WebSocket.";
doc.tags = {"websocket"};
doc.responses["101"] = {{"description", "Switching Protocols"}};

vix::openapi::Registry::add(
    "GET",
    "/ws",
    doc);
```

The next call to `build_from_router(...)` includes this extra route documentation.

## Registry

`Registry` is a global registry for additional OpenAPI route docs.

It provides:

```cpp
vix::openapi::Registry::add(method, path, doc);
vix::openapi::Registry::snapshot();
vix::openapi::Registry::clear();
```

Use it for:

- WebSocket docs
- module docs
- generated docs
- routes managed outside the normal router
- plugin-style integrations

## ExtraRouteDoc

`ExtraRouteDoc` stores one extra documented route.

```cpp
struct ExtraRouteDoc
{
  std::string method;
  std::string path;
  vix::router::RouteDoc doc;
};
```

## Add extra docs

```cpp
vix::router::RouteDoc doc;

doc.summary = "Health check";
doc.tags = {"system"};
doc.responses["200"] = {{"description", "OK"}};

vix::openapi::Registry::add("GET", "/health", doc);
```

## Read registry snapshot

```cpp
const auto docs = vix::openapi::Registry::snapshot();

for (const auto &entry : docs)
{
  vix::print(entry.method, entry.path);
}
```

`snapshot()` returns a copy, so it is safe to iterate without holding the registry lock.

## Clear registry

```cpp
vix::openapi::Registry::clear();
```

This removes all registered extra route docs.

It is useful for tests or when rebuilding docs in controlled environments.

## Duplicate handling

When OpenAPI is built, duplicate route keys are ignored after the first occurrence.

The duplicate key is:

```text
method + path
```

Example:

```text
GET /status
```

If the same method and path appear more than once between the router and registry, only the first one is kept.

## Vendor extensions

`RouteDoc::x` can store OpenAPI vendor extensions.

```cpp
vix::router::RouteDoc doc;

doc.x["x-vix-runtime"] = "core";
doc.x["x-vix-heavy"] = true;
```

These fields are copied into the generated OpenAPI operation.

Result shape:

```json
{
  "x-vix-runtime": "core",
  "x-vix-heavy": true
}
```

## Request body metadata

Use `RouteDoc::request_body` to describe request bodies.

```cpp
doc.request_body = {
  {"required", true},
  {"content", {
    {"application/json", {
      {"schema", {
        {"type", "object"},
        {"properties", {
          {"name", {{"type", "string"}}},
          {"email", {{"type", "string"}}}
        }}
      }}
    }}
  }}
};
```

This becomes:

```json
{
  "requestBody": {
    "required": true,
    "content": {
      "application/json": {
        "schema": {
          "type": "object"
        }
      }
    }
  }
}
```

## Response metadata

Use `RouteDoc::responses` to describe possible responses.

```cpp
doc.responses["200"] = {
  {"description", "OK"}
};

doc.responses["404"] = {
  {"description", "User not found"}
};
```

If `responses` is empty, Vix adds:

```json
{
  "200": {
    "description": "OK"
  }
}
```

## Tags

Use tags to group routes in Swagger UI.

```cpp
doc.tags = {"users"};
```

Example with multiple tags:

```cpp
doc.tags = {"users", "admin"};
```

## Docs routes metadata

The docs routes also receive route documentation.

For example:

```text
GET /openapi.json
```

has a docs tag and a summary like:

```text
OpenAPI spec
```

This means the docs routes can appear in the generated OpenAPI document too.

## Enable docs with vix run

`vix run` keeps OpenAPI/docs disabled by default.

To enable docs for one run:

```bash
vix run api --docs
```

This sets:

```bash
VIX_DOCS=1
```

To keep docs disabled:

```bash
vix run api --no-docs
```

This sets:

```bash
VIX_DOCS=0
```

## Recommended local usage

During development:

```bash
vix run api --docs
```

Then open:

```text
http://localhost:8080/docs
```

For normal runs:

```bash
vix run api
```

Docs stay disabled unless you explicitly enable them.

## Recommended production usage

For production, expose docs only when needed.

Common approaches:

```text
disable docs completely
protect /docs behind auth
serve docs only in internal environments
serve /openapi.json only in CI or staging
```

If docs are enabled publicly, make sure your API metadata does not expose sensitive internal details.

## OpenAPI and App

In normal Vix Core, docs registration can be done through the router.

```cpp
vix::openapi::register_openapi_and_docs(
    *app.router(),
    "My API",
    "1.0.0");
```

After registration:

```text
/openapi.json
/docs
/docs/
/docs/index.html
/docs/swagger-ui.css
/docs/swagger-ui-bundle.js
```

are available.

## Complete example

```cpp
#include <vix.hpp>
#include <vix/openapi/register_docs.hpp>

int main()
{
  vix::App app;

  vix::router::RouteDoc status_doc;
  status_doc.summary = "Get status";
  status_doc.description = "Returns the current service status.";
  status_doc.tags = {"system"};
  status_doc.responses["200"] = {
    {"description", "OK"}
  };

  using Fn = std::function<void(vix::Request &, vix::Response &)>;

  auto status_handler = std::make_shared<vix::http::RequestHandler<Fn>>(
      "/api/status",
      [](vix::Request &req, vix::Response &res)
      {
        (void)req;

        res.json({
          "status", "ok"
        });
      });

  app.router()->add_route(
      "GET",
      "/api/status",
      status_handler,
      vix::router::RouteOptions{},
      status_doc);

  vix::openapi::register_openapi_and_docs(
      *app.router(),
      "Example API",
      "1.0.0");

  app.run(8080);

  return 0;
}
```

Open:

```text
http://localhost:8080/openapi.json
```

or:

```text
http://localhost:8080/docs
```

## API summary

| API                                                 | Purpose                                                   |
| --------------------------------------------------- | --------------------------------------------------------- |
| `method_to_openapi(method)`                         | Convert an HTTP method to an OpenAPI operation key.       |
| `default_responses()`                               | Return the default `200 OK` response object.              |
| `make_operation_id(method, path)`                   | Build a stable operation ID from method and path.         |
| `build_from_router(router, title, version)`         | Build an OpenAPI 3 document from the router and registry. |
| `register_openapi_and_docs(router, title, version)` | Register `/openapi.json` and `/docs` routes.              |
| `Registry::add(method, path, doc)`                  | Register extra route documentation.                       |
| `Registry::snapshot()`                              | Return a copy of registered extra docs.                   |
| `Registry::clear()`                                 | Clear extra docs.                                         |

## RouteDoc summary

| Field          | Purpose                          |
| -------------- | -------------------------------- |
| `summary`      | One-line operation summary.      |
| `description`  | Detailed operation description.  |
| `tags`         | Grouping tags for the operation. |
| `request_body` | OpenAPI request body metadata.   |
| `responses`    | OpenAPI responses metadata.      |
| `x`            | Vendor-specific extensions.      |

## Header map

| Area                    | Header                                   |
| ----------------------- | ---------------------------------------- |
| OpenAPI generator       | `<vix/openapi/OpenApi.hpp>`              |
| Registry                | `<vix/openapi/Registry.hpp>`             |
| Docs route registration | `<vix/openapi/register_docs.hpp>`        |
| Route documentation     | `<vix/router/RouteDoc.hpp>`              |
| Router                  | `<vix/router/Router.hpp>`                |
| Docs UI                 | `<vix/openapi/DocsUI.hpp>`               |
| Swagger assets          | `<vix/openapi/assets/SwaggerAssets.hpp>` |

## Best practices

Enable docs explicitly during development.

```bash
vix run api --docs
```

Use clear route summaries.

```cpp
doc.summary = "Create user";
```

Group routes with tags.

```cpp
doc.tags = {"users"};
```

Always document expected responses.

```cpp
doc.responses["200"] = {{"description", "OK"}};
doc.responses["404"] = {{"description", "Not found"}};
```

Use `Registry` for routes that are not directly stored in the core router.

```cpp
vix::openapi::Registry::add("GET", "/ws", doc);
```

Avoid exposing sensitive internal information in public docs.

```text
Keep production docs intentional.
```

## Next steps

Read the related pages:

- [Core API Reference](./api-reference.md)
- [Routing](./routing.md)
- [Handlers](./handlers.md)
- [Configuration](./configuration.md)
- [vix run](../../cli/run.md)
