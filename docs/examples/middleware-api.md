# Middleware API

This example shows how to build a small but serious API with Vix middleware.

The goal is to demonstrate a realistic middleware stack around API routes:

```txt id="g8xrap"
recovery
request id
timing
security headers
CORS
rate limit
body limit
strict JSON parsing
route handlers
```

This is the kind of structure you can use as a base for a real backend.

## What this example builds

The API exposes:

```txt id="wnomhn"
GET  /api/health
GET  /api/products
POST /api/products
```

The middleware stack protects `/api` and parses JSON for `POST /api/products`.

```txt id="unjxsl"
/api
  recovery
  request id
  timing
  security headers
  CORS
  rate limit
  body limit

/api/products
  strict JSON parser
```

## Source

Create a file:

```txt id="zgm3rk"
middleware_api.cpp
```

Add this code:

```cpp id="u8ibh6"
#include <string>
#include <vector>

#include <vix.hpp>
#include <vix/middleware.hpp>
#include <vix/json.hpp>

using namespace vix;

struct Product
{
  int id;
  std::string name;
  double price;
  bool available;
};

static std::vector<Product> products{
  {1, "Laptop", 999.99, true},
  {2, "Phone", 499.50, true}
};

static vix::json::Json product_to_json(const Product &product)
{
  using namespace vix::json;

  return o(
    "id", product.id,
    "name", product.name,
    "price", product.price,
    "available", product.available
  );
}

static vix::json::Json products_to_json()
{
  using namespace vix::json;

  Json items = arr();

  for (const auto &product : products)
  {
    items.push_back(product_to_json(product));
  }

  return items;
}

static void install_api_middleware(App &app)
{
  app.use("/api", middleware::app::recovery_dev());
  app.use("/api", middleware::app::request_id_dev());
  app.use("/api", middleware::app::timing_dev());

  app.use("/api", middleware::app::security_headers_dev());

  app.use("/api", middleware::app::cors_dev({
    "http://localhost:5173",
    "http://127.0.0.1:5173",
    "https://example.com"
  }));

  app.use("/api", middleware::app::rate_limit_custom_dev(
    60.0,
    1.0,
    "x-forwarded-for"
  ));

  app.use("/api", middleware::app::body_limit_write_dev(
    1024 * 1024
  ));

  app.use("/api/products", middleware::app::json_strict_dev(
    4096,
    false,
    true
  ));
}

static void register_options_routes(App &app)
{
  app.options("/api/health", [](Request &, Response &res)
  {
    res.status(204).send();
  });

  app.options("/api/products", [](Request &, Response &res)
  {
    res.status(204).send();
  });
}

static void register_routes(App &app)
{
  app.get("/", [](Request &, Response &res)
  {
    res.text("Middleware API example. Try /api/health or /api/products.");
  });

  app.get("/api/health", [](Request &req, Response &res)
  {
    auto *request_id =
      req.try_state<middleware::basics::RequestId>();

    auto *timing =
      req.try_state<middleware::basics::Timing>();

    res.json({
      "ok", true,
      "service", "middleware-api",
      "request_id", request_id ? request_id->value : "",
      "duration_ms", timing ? timing->total_ms : 0
    });
  });

  app.get("/api/products", [](Request &, Response &res)
  {
    using namespace vix::json;

    res.json(o(
      "ok", true,
      "products", products_to_json()
    ));
  });

  app.post("/api/products", [](Request &req, Response &res)
  {
    using namespace vix::json;

    auto &body = req.state<middleware::parsers::JsonBody>();

    auto name = get_opt<std::string>(body.value, "name");
    const double price = get_or<double>(body.value, "price", 0.0);
    const bool available = get_or<bool>(body.value, "available", true);

    if (!name || name->empty())
    {
      res.status(422).json({
        "ok", false,
        "error", "Missing required field",
        "field", "name"
      });
      return;
    }

    if (price <= 0.0)
    {
      res.status(422).json({
        "ok", false,
        "error", "Price must be greater than zero",
        "field", "price"
      });
      return;
    }

    const int next_id = products.empty() ? 1 : products.back().id + 1;

    products.push_back(Product{
      next_id,
      *name,
      price,
      available
    });

    res.status(201).json(o(
      "ok", true,
      "product", product_to_json(products.back())
    ));
  });
}

int main()
{
  App app;

  install_api_middleware(app);
  register_options_routes(app);
  register_routes(app);

  app.run(8080);
  return 0;
}
```

## Run it

```bash id="xlaa71"
vix run middleware_api.cpp
```

The server listens on:

```txt id="nlrzqq"
http://127.0.0.1:8080
```

## Test the public route

```bash id="mhjb1d"
curl -i http://127.0.0.1:8080/
```

Expected body:

```txt id="suqo40"
Middleware API example. Try /api/health or /api/products.
```

The public route is not under `/api`, so the API middleware stack does not apply to it.

## Test the health route

```bash id="e84u20"
curl -i http://127.0.0.1:8080/api/health
```

Expected body shape:

```json id="ebto3j"
{
  "ok": true,
  "service": "middleware-api",
  "request_id": "...",
  "duration_ms": 0
}
```

Expected response headers may include:

```txt id="dzmbyf"
x-request-id: ...
x-response-time: ...
server-timing: total;dur=...
X-Content-Type-Options: nosniff
X-Frame-Options: DENY
Referrer-Policy: no-referrer
Permissions-Policy: ...
```

This shows that the middleware stack is active.

## Test the products list

```bash id="yxlaql"
curl -i http://127.0.0.1:8080/api/products
```

Expected body shape:

```json id="n8508y"
{
  "ok": true,
  "products": [
    {
      "id": 1,
      "name": "Laptop",
      "price": 999.99,
      "available": true
    },
    {
      "id": 2,
      "name": "Phone",
      "price": 499.5,
      "available": true
    }
  ]
}
```

This route uses JSON builders:

```cpp id="h3j4sb"
using namespace vix::json;

res.json(o(
  "ok", true,
  "products", products_to_json()
));
```

Use builders when responses become nested or when helper functions return JSON values.

## Create a product

```bash id="yi41p7"
curl -i \
  -X POST http://127.0.0.1:8080/api/products \
  -H "Content-Type: application/json" \
  -d '{"name":"Tablet","price":299.99,"available":true}'
```

Expected status:

```txt id="ujw0w4"
201 Created
```

Expected body shape:

```json id="b50f35"
{
  "ok": true,
  "product": {
    "id": 3,
    "name": "Tablet",
    "price": 299.99,
    "available": true
  }
}
```

The JSON body is parsed by middleware:

```cpp id="jxgkpd"
app.use("/api/products", middleware::app::json_strict_dev(
  4096,
  false,
  true
));
```

The handler reads the parsed body:

```cpp id="gzf3d7"
auto &body = req.state<middleware::parsers::JsonBody>();
```

Then it validates application fields.

## Test validation errors

Missing name:

```bash id="dwl0t3"
curl -i \
  -X POST http://127.0.0.1:8080/api/products \
  -H "Content-Type: application/json" \
  -d '{"price":299.99}'
```

Expected status:

```txt id="knxnd7"
422 Unprocessable Entity
```

Expected body shape:

```json id="erh40y"
{
  "ok": false,
  "error": "Missing required field",
  "field": "name"
}
```

Invalid price:

```bash id="y3wlda"
curl -i \
  -X POST http://127.0.0.1:8080/api/products \
  -H "Content-Type: application/json" \
  -d '{"name":"Broken product","price":0}'
```

Expected status:

```txt id="dhrk6v"
422 Unprocessable Entity
```

Expected body shape:

```json id="uqsfqb"
{
  "ok": false,
  "error": "Price must be greater than zero",
  "field": "price"
}
```

The parser validates JSON format.

The handler validates business rules.

## Test invalid JSON

```bash id="s4t1pn"
curl -i \
  -X POST http://127.0.0.1:8080/api/products \
  -H "Content-Type: application/json" \
  -d '{"name":}'
```

Expected status:

```txt id="s6bxaz"
400 Bad Request
```

The handler is not called.

The JSON parser middleware rejects the request before route logic runs.

## Test wrong Content-Type

```bash id="e253xg"
curl -i \
  -X POST http://127.0.0.1:8080/api/products \
  -H "Content-Type: text/plain" \
  -d '{"name":"Tablet","price":299.99}'
```

Expected status:

```txt id="cwt8dp"
415 Unsupported Media Type
```

The strict JSON parser requires:

```txt id="cegz0d"
Content-Type: application/json
```

or a compatible JSON content type such as:

```txt id="lv8ehv"
application/json; charset=utf-8
```

## Test body limit

The API has a broad body limit:

```cpp id="mt2hgg"
app.use("/api", middleware::app::body_limit_write_dev(
  1024 * 1024
));
```

And a smaller JSON parser limit for products:

```cpp id="c6jxmc"
app.use("/api/products", middleware::app::json_strict_dev(
  4096,
  false,
  true
));
```

Test a large JSON body:

```bash id="q0hny5"
BIG="$(python3 -c 'print("{\"name\":\"" + "x"*5000 + "\",\"price\":10}")')"

curl -i \
  -X POST http://127.0.0.1:8080/api/products \
  -H "Content-Type: application/json" \
  -d "$BIG"
```

Expected status:

```txt id="na7o1l"
413 Payload Too Large
```

The broad body limit protects the API group.

The parser limit protects the specific JSON route.

## Test CORS preflight

Allowed origin:

```bash id="gypbb4"
curl -i \
  -X OPTIONS http://127.0.0.1:8080/api/products \
  -H "Origin: http://localhost:5173" \
  -H "Access-Control-Request-Method: POST" \
  -H "Access-Control-Request-Headers: Content-Type"
```

Expected status:

```txt id="p6p8no"
204 No Content
```

Expected CORS header:

```txt id="z6x9cm"
Access-Control-Allow-Origin: http://localhost:5173
```

Blocked origin:

```bash id="z0ii3t"
curl -i \
  -X OPTIONS http://127.0.0.1:8080/api/products \
  -H "Origin: https://evil.com" \
  -H "Access-Control-Request-Method: POST" \
  -H "Access-Control-Request-Headers: Content-Type"
```

Expected status:

```txt id="lnm9vu"
403 Forbidden
```

CORS is installed before the JSON parser so browser preflight requests can be handled before body parsing.

## Test rate limit

The demo stack uses:

```cpp id="lczqig"
app.use("/api", middleware::app::rate_limit_custom_dev(
  60.0,
  1.0,
  "x-forwarded-for"
));
```

For normal testing, 60 requests is high.

To see rate limiting quickly, temporarily change it to:

```cpp id="o4dwhi"
app.use("/api", middleware::app::rate_limit_custom_dev(
  5.0,
  0.0,
  "x-forwarded-for"
));
```

Then run:

```bash id="qwim6z"
for i in $(seq 1 6); do
  echo "---- $i"
  curl -i \
    http://127.0.0.1:8080/api/health \
    -H "x-forwarded-for: 10.0.0.1"
done
```

Expected sixth response:

```txt id="c8kipp"
429 Too Many Requests
```

Rate limiting should happen before expensive parsing and handler work.

## Middleware order

The middleware installation function is:

```cpp id="rwv3kr"
static void install_api_middleware(App &app)
{
  app.use("/api", middleware::app::recovery_dev());
  app.use("/api", middleware::app::request_id_dev());
  app.use("/api", middleware::app::timing_dev());

  app.use("/api", middleware::app::security_headers_dev());

  app.use("/api", middleware::app::cors_dev({
    "http://localhost:5173",
    "http://127.0.0.1:5173",
    "https://example.com"
  }));

  app.use("/api", middleware::app::rate_limit_custom_dev(
    60.0,
    1.0,
    "x-forwarded-for"
  ));

  app.use("/api", middleware::app::body_limit_write_dev(
    1024 * 1024
  ));

  app.use("/api/products", middleware::app::json_strict_dev(
    4096,
    false,
    true
  ));
}
```

The order is intentional.

```txt id="qdxsde"
recovery
  catches unhandled exceptions

request id
  gives every request an identifier

timing
  measures downstream work

security headers
  hardens responses

CORS
  handles browser origin checks and preflight

rate limit
  rejects abusive clients early

body limit
  rejects oversized bodies before parsing

JSON parser
  parses only the route that expects JSON
```

The handler should be the last part of the flow.

## Why JSON parser is not global

This example installs the JSON parser on:

```cpp id="g4wz4o"
app.use("/api/products", middleware::app::json_strict_dev(...));
```

not on:

```cpp id="atx3ze"
app.use("/api", middleware::app::json_strict_dev(...));
```

That is important.

`GET /api/health` and `GET /api/products` do not need JSON request bodies.

If a strict parser is installed globally, routes without request bodies may be rejected unnecessarily.

Use route-specific parser prefixes.

## Recovery

The recovery middleware protects the server from uncaught exceptions escaping the pipeline.

```cpp id="nw8erf"
app.use("/api", middleware::app::recovery_dev());
```

It is installed early so downstream middleware and route handlers are protected.

In production, avoid returning internal exception details to clients.

## Request id

Request id middleware adds a stable identifier for each request.

```cpp id="qpkknx"
app.use("/api", middleware::app::request_id_dev());
```

It stores:

```cpp id="cnr67r"
middleware::basics::RequestId
```

Read it inside a handler:

```cpp id="dt5m5j"
auto *request_id =
  req.try_state<middleware::basics::RequestId>();
```

It can also be written to the response header.

Request ids are useful for logs, debugging, support, and tracing.

## Timing

Timing middleware measures the time spent downstream.

```cpp id="ofwlzt"
app.use("/api", middleware::app::timing_dev());
```

It stores:

```cpp id="d33dzn"
middleware::basics::Timing
```

Read it inside a handler:

```cpp id="b0l6y0"
auto *timing =
  req.try_state<middleware::basics::Timing>();
```

It can also emit headers such as:

```txt id="skg3cx"
x-response-time
server-timing
```

## Security headers

Security headers middleware adds browser hardening headers.

```cpp id="kb74js"
app.use("/api", middleware::app::security_headers_dev());
```

Typical headers include:

```txt id="kgs48s"
X-Content-Type-Options
X-Frame-Options
Referrer-Policy
Permissions-Policy
```

Use it on API and web routes where browser behavior matters.

## CORS

CORS controls which browser origins can call the API.

```cpp id="fj4zfc"
app.use("/api", middleware::app::cors_dev({
  "http://localhost:5173",
  "http://127.0.0.1:5173",
  "https://example.com"
}));
```

CORS should run before parsers.

Preflight requests are `OPTIONS` requests and usually have no body.

## Rate limit

Rate limiting protects the API from too many requests from the same client key.

```cpp id="s6ftnh"
app.use("/api", middleware::app::rate_limit_custom_dev(
  60.0,
  1.0,
  "x-forwarded-for"
));
```

The values mean:

```txt id="l5a5wu"
capacity = 60
refill_per_sec = 1
key header = x-forwarded-for
```

For production behind a proxy, make sure the proxy controls forwarding headers.

Do not blindly trust client-provided `X-Forwarded-For` when your app is exposed directly.

## Body limit

Body limit protects the API before parsers run.

```cpp id="u3v17s"
app.use("/api", middleware::app::body_limit_write_dev(
  1024 * 1024
));
```

It should be installed before JSON, form, or multipart parsers.

This prevents large invalid requests from reaching expensive parsing logic.

## JSON parser

The strict JSON parser is installed only where it is needed.

```cpp id="t24y7w"
app.use("/api/products", middleware::app::json_strict_dev(
  4096,
  false,
  true
));
```

It checks:

```txt id="bjtzca"
Content-Type
empty body
maximum size
JSON validity
```

Then it stores:

```cpp id="ihhafw"
middleware::parsers::JsonBody
```

The handler reads it with:

```cpp id="m6mhx5"
auto &body = req.state<middleware::parsers::JsonBody>();
```

## Complete request flow

For `POST /api/products`, the flow is:

```txt id="ul1pg2"
request
  -> recovery
  -> request id
  -> timing
  -> security headers
  -> CORS
  -> rate limit
  -> body limit
  -> strict JSON parser
  -> handler
  -> response
```

If any middleware rejects the request, the handler is not called.

Examples:

```txt id="ch35pg"
invalid origin
  CORS returns 403

too many requests
  rate limit returns 429

body too large
  body limit or parser returns 413

wrong content type
  JSON parser returns 415

invalid JSON
  JSON parser returns 400

business validation failed
  handler returns 422
```

## Why this example matters

A real backend should not put everything inside route handlers.

Bad route design:

```txt id="qz2h63"
handler parses JSON
handler checks body size
handler checks CORS
handler rate limits
handler validates business fields
handler writes response
```

Better route design:

```txt id="zzoy4v"
middleware handles reusable HTTP concerns
handler handles application rules
```

That is what this example demonstrates.

## Summary

This example shows a practical API stack:

```cpp id="p91ynh"
app.use("/api", middleware::app::recovery_dev());
app.use("/api", middleware::app::request_id_dev());
app.use("/api", middleware::app::timing_dev());
app.use("/api", middleware::app::security_headers_dev());
app.use("/api", middleware::app::cors_dev(...));
app.use("/api", middleware::app::rate_limit_dev());
app.use("/api", middleware::app::body_limit_write_dev(...));
app.use("/api/products", middleware::app::json_strict_dev(...));
```

The rule is:

```txt id="bq2waj"
middleware protects and prepares the request
handlers implement application behavior
```

Use this structure when you want a small Vix API that already looks like a real backend.
