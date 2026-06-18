# JSON with HTTP

Vix HTTP applications commonly use JSON for request bodies and responses.

This page shows how the JSON module fits into that workflow. It does not replace the HTTP guide. The focus here is on how to parse request bodies, validate JSON fields, build response payloads, and keep JSON handling explicit inside route handlers.

The normal Vix route shape is:

```cpp id="ga9s3g"
app.post("/path", [](Request &req, Response &res) {
  // read request
  // write response
});
```

A route handler receives the request by reference and writes into the response. It does not need to return a response object.

## Header

For HTTP applications, include the main Vix header:

```cpp id="xgrwbm"
#include <vix.hpp>
```

For the JSON helpers, include:

```cpp id="qo3bsu"
#include <vix/json.hpp>
```

The examples on this page use:

```cpp id="cmu0mx"
using namespace vix;
```

and use JSON helpers from:

```cpp id="uxzy3z"
namespace vix::json
```

## What JSON does in an HTTP route

A JSON route usually has four steps.

First, read the request body as a string.

```cpp id="jnsp9g"
req.body()
```

Second, parse the body with the JSON module.

```cpp id="u2lelv"
auto body = vix::json::try_loads(req.body());
```

Third, read and validate the fields.

```cpp id="kogobz"
auto name = vix::json::get_opt<std::string>(*body, "name");
```

Fourth, send a JSON response.

```cpp id="pbr14n"
res.json({
  "ok", true,
  "name", *name
});
```

This keeps the route logic clear. The HTTP layer handles the request and response. The JSON module handles parsing, field access, validation support, and payload construction.

## Return a JSON response

For simple responses, use `res.json(...)` directly.

```cpp id="has2id"
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/health", [](Request &, Response &res) {
    res.json({
      "ok", true,
      "service", "api"
    });
  });

  app.run();

  return 0;
}
```

Test it:

```bash id="v0b3mi"
curl -i http://127.0.0.1:8080/health
```

Expected response shape:

```json id="hdlat6"
{
  "ok": true,
  "service": "api"
}
```

This is the simplest HTTP JSON pattern in Vix. The handler writes into `Response &res`, and `res.json(...)` produces the JSON response.

## Set a status code

Use `res.status(...).json(...)` when the response needs a specific HTTP status code.

```cpp id="bnf8g7"
app.get("/created", [](Request &, Response &res) {
  res.status(http::CREATED).json({
    "ok", true,
    "status", "created"
  });
});
```

The status constants come from the Vix HTTP API.

```cpp id="x589iy"
http::OK
http::CREATED
http::BAD_REQUEST
http::UNPROCESSABLE_ENTITY
http::NOT_FOUND
http::INTERNAL_ERROR
```

Prefer named constants when the meaning matters. They make the route easier to read than bare numeric values.

## Parse a request body

Use `try_loads()` for request bodies.

A malformed request body is not an internal application failure. It is client input that should produce a controlled error response.

```cpp id="rtgsx8"
#include <vix.hpp>
#include <vix/json.hpp>

using namespace vix;

int main()
{
  App app;

  app.post("/echo", [](Request &req, Response &res) {
    using namespace vix::json;

    auto body = try_loads(req.body());

    if (!body)
    {
      res.status(http::BAD_REQUEST).json({
        "error", "Invalid JSON"
      });
      return;
    }

    res.json({
      "ok", true,
      "body", *body
    });
  });

  app.run();

  return 0;
}
```

Test it:

```bash id="ci3452"
curl -i \
  -X POST http://127.0.0.1:8080/echo \
  -H "Content-Type: application/json" \
  -d '{"message":"Hello"}'
```

Expected response shape:

```json id="qztnff"
{
  "ok": true,
  "body": {
    "message": "Hello"
  }
}
```

With invalid JSON:

```bash id="l49y66"
curl -i \
  -X POST http://127.0.0.1:8080/echo \
  -H "Content-Type: application/json" \
  -d 'not-json'
```

Expected response shape:

```json id="m4nen8"
{
  "error": "Invalid JSON"
}
```

## Validate required fields

Parsing only tells you that the body is valid JSON. It does not tell you that the payload is valid for your route.

Use `get_opt<T>()` for required fields when you need to validate presence and type.

```cpp id="xwi3dv"
#include <vix.hpp>
#include <vix/json.hpp>

using namespace vix;

int main()
{
  App app;

  app.post("/users", [](Request &req, Response &res) {
    using namespace vix::json;

    auto body = try_loads(req.body());

    if (!body)
    {
      res.status(http::BAD_REQUEST).json({
        "error", "Invalid JSON"
      });
      return;
    }

    auto name = get_opt<std::string>(*body, "name");
    auto email = get_opt<std::string>(*body, "email");

    if (!name || name->empty())
    {
      res.status(http::UNPROCESSABLE_ENTITY).json({
        "error", "Missing required field",
        "field", "name"
      });
      return;
    }

    if (!email || email->empty())
    {
      res.status(http::UNPROCESSABLE_ENTITY).json({
        "error", "Missing required field",
        "field", "email"
      });
      return;
    }

    res.status(http::CREATED).json({
      "ok", true,
      "user", {
        "name", *name,
        "email", *email
      }
    });
  });

  app.run();

  return 0;
}
```

Test it:

```bash id="f6gu7r"
curl -i \
  -X POST http://127.0.0.1:8080/users \
  -H "Content-Type: application/json" \
  -d '{"name":"Ada","email":"ada@example.com"}'
```

Expected response shape:

```json id="eu4kle"
{
  "ok": true,
  "user": {
    "name": "Ada",
    "email": "ada@example.com"
  }
}
```

This route separates parsing from validation. `try_loads()` handles malformed JSON. `get_opt()` handles missing or wrong-type fields. The route then decides which HTTP status to send.

## Use defaults for optional fields

Use `get_or<T>()` when a missing field has a normal default value.

```cpp id="oflvu1"
app.post("/search", [](Request &req, Response &res) {
  using namespace vix::json;

  auto body = try_loads(req.body());

  if (!body)
  {
    res.status(http::BAD_REQUEST).json({
      "error", "Invalid JSON"
    });
    return;
  }

  const std::string query = get_or<std::string>(*body, "query", "");
  const int page = get_or<int>(*body, "page", 1);
  const int limit = get_or<int>(*body, "limit", 20);

  if (query.empty())
  {
    res.status(http::UNPROCESSABLE_ENTITY).json({
      "error", "Missing required field",
      "field", "query"
    });
    return;
  }

  res.json({
    "ok", true,
    "query", query,
    "page", page,
    "limit", limit
  });
});
```

This is a good fit for pagination, feature flags, optional filters, and fields with stable application defaults.

Avoid using defaults for required fields when the client should receive a validation error. A default can hide bad input.

## Read nested request data

Use `jget()` when the request body contains nested objects or arrays.

```cpp id="un560h"
#include <vix.hpp>
#include <vix/json.hpp>

using namespace vix;

int main()
{
  App app;

  app.post("/profiles", [](Request &req, Response &res) {
    using namespace vix::json;

    auto body = try_loads(req.body());

    if (!body)
    {
      res.status(http::BAD_REQUEST).json({
        "error", "Invalid JSON"
      });
      return;
    }

    const std::string name = get_or<std::string>(
      jget(*body, "user.profile.name"),
      ""
    );

    const std::string country = get_or<std::string>(
      jget(*body, "user.profile.country"),
      "unknown"
    );

    if (name.empty())
    {
      res.status(http::UNPROCESSABLE_ENTITY).json({
        "error", "Missing required field",
        "field", "user.profile.name"
      });
      return;
    }

    res.json({
      "ok", true,
      "name", name,
      "country", country
    });
  });

  app.run();

  return 0;
}
```

Example request:

```bash id="hm8xwy"
curl -i \
  -X POST http://127.0.0.1:8080/profiles \
  -H "Content-Type: application/json" \
  -d '{"user":{"profile":{"name":"Ada","country":"UG"}}}'
```

Expected response shape:

```json id="lwapwg"
{
  "ok": true,
  "name": "Ada",
  "country": "UG"
}
```

`jget()` returns `nullptr` when a path is missing, and `get_or()` can turn that into a fallback value.

## Build response payloads before sending

For very small responses, writing directly in `res.json(...)` is fine.

```cpp id="fztaiq"
res.json({
  "ok", true
});
```

For larger responses, build the payload first. This keeps the route easier to read and gives you a normal `Json` value to pass around before sending.

```cpp id="s90k4a"
app.get("/users/{id}", [](Request &req, Response &res) {
  using namespace vix::json;

  const std::string id = req.param("id");

  Json payload = o(
    "ok", true,
    "user", o(
      "id", id,
      "name", "Ada"
    ),
    "links", o(
      "self", "/users/" + id
    )
  );

  res.json(payload);
});
```

The JSON module builds the payload. The HTTP response layer sends it.

## Add server-side metadata

`jset()` is useful when a route needs to enrich a parsed payload before returning it or passing it to another layer.

```cpp id="j2apjm"
app.post("/events", [](Request &req, Response &res) {
  using namespace vix::json;

  auto body = try_loads(req.body());

  if (!body)
  {
    res.status(http::BAD_REQUEST).json({
      "error", "Invalid JSON"
    });
    return;
  }

  jset(*body, "meta.received", true);
  jset(*body, "meta.source", "http");

  res.status(http::CREATED).json({
    "ok", true,
    "event", *body
  });
});
```

This pattern is useful when an endpoint accepts JSON, validates it, adds application metadata, and then forwards the document to storage, a queue, or another internal system.

## Request JSON cache

The Vix `Request` object also exposes a JSON-oriented API.

```cpp id="hhrw6b"
const vix::json::Json& data = req.json();
```

and:

```cpp id="ai9rsu"
auto value = req.json_as<MyType>();
```

These APIs are convenient when the body is expected to be valid JSON and exceptions are acceptable for the route or middleware design.

For public endpoints, `try_loads(req.body())` is often clearer because it lets the route convert malformed JSON into a controlled `400 Bad Request` response.

```cpp id="ijb7u2"
auto body = try_loads(req.body());

if (!body)
{
  res.status(http::BAD_REQUEST).json({
    "error", "Invalid JSON"
  });
  return;
}
```

Use the request JSON API when the route is internal, when the framework-level error handling is intentional, or when middleware has already validated the body.

## Content-Type

For JSON endpoints, clients should send:

```http id="b8ce8s"
Content-Type: application/json
```

The examples use this header in `curl` commands:

```bash id="r72s7s"
-H "Content-Type: application/json"
```

The JSON parser reads the body text. Header validation is an application decision. If your endpoint only accepts JSON, validate the header before parsing or before processing the body.

```cpp id="wty2we"
const std::string content_type = req.header("Content-Type");

if (content_type.find("application/json") == std::string::npos)
{
  res.status(http::UNSUPPORTED_MEDIA_TYPE).json({
    "error", "Expected application/json"
  });
  return;
}
```

Then parse the body:

```cpp id="pjxmda"
auto body = try_loads(req.body());
```

## Consistent error responses

For APIs, keep JSON error responses consistent.

```cpp id="xh184y"
res.status(http::BAD_REQUEST).json({
  "error", "Invalid JSON"
});
```

```cpp id="vzu3ap"
res.status(http::UNPROCESSABLE_ENTITY).json({
  "error", "Missing required field",
  "field", "email"
});
```

```cpp id="bquoyv"
res.status(http::UNSUPPORTED_MEDIA_TYPE).json({
  "error", "Expected application/json"
});
```

A consistent error shape is easier for clients to handle than route-specific error formats.

For larger APIs, you can wrap this pattern in helper functions.

```cpp id="bp3pio"
void json_error(Response& res, int status, std::string_view message)
{
  res.status(status).json({
    "error", std::string(message)
  });
}
```

Then use it in routes:

```cpp id="l7im32"
json_error(res, http::BAD_REQUEST, "Invalid JSON");
return;
```

## Complete example

This example combines content-type checking, safe parsing, required fields, optional defaults, nested access, and a JSON response.

```cpp id="n0p4gi"
#include <vix.hpp>
#include <vix/json.hpp>

using namespace vix;

void json_error(Response& res, int status, std::string_view message)
{
  res.status(status).json({
    "error", std::string(message)
  });
}

int main()
{
  App app;

  app.post("/orders", [](Request &req, Response &res) {
    using namespace vix::json;

    const std::string content_type = req.header("Content-Type");

    if (content_type.find("application/json") == std::string::npos)
    {
      json_error(res, http::UNSUPPORTED_MEDIA_TYPE, "Expected application/json");
      return;
    }

    auto body = try_loads(req.body());

    if (!body)
    {
      json_error(res, http::BAD_REQUEST, "Invalid JSON");
      return;
    }

    auto customer_id = get_opt<std::string>(*body, "customer_id");

    if (!customer_id || customer_id->empty())
    {
      res.status(http::UNPROCESSABLE_ENTITY).json({
        "error", "Missing required field",
        "field", "customer_id"
      });
      return;
    }

    const std::string currency = get_or<std::string>(*body, "currency", "USD");
    const std::string city = get_or<std::string>(
      jget(*body, "shipping.city"),
      "unknown"
    );

    Json response = o(
      "ok", true,
      "order", o(
        "customer_id", *customer_id,
        "currency", currency,
        "shipping_city", city
      )
    );

    res.status(http::CREATED).json(response);
  });

  app.run();

  return 0;
}
```

Example request:

```bash id="wsdisa"
curl -i \
  -X POST http://127.0.0.1:8080/orders \
  -H "Content-Type: application/json" \
  -d '{"customer_id":"cust_42","currency":"UGX","shipping":{"city":"Kampala"}}'
```

Expected response shape:

```json id="v41l15"
{
  "ok": true,
  "order": {
    "customer_id": "cust_42",
    "currency": "UGX",
    "shipping_city": "Kampala"
  }
}
```

## What to keep in mind

Use `res.json(...)` for normal JSON responses.

Use `try_loads(req.body())` for public request bodies.

Use `get_opt()` when a field is required and must be validated.

Use `get_or()` when a field is optional and has a real default.

Use `jget()` when the payload is nested.

Use `jset()` when the route needs to enrich or transform a JSON document before responding.

The HTTP route should stay focused on request handling. The JSON module should handle JSON parsing, access, construction, and mutation.

## Next steps

Return to [JSON](/guides/json/) for the module overview, or continue to the API reference when you need exact function signatures.
