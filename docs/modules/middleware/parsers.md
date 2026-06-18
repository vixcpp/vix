# Parsers

The `parsers` group turns raw HTTP request bodies into typed data your handlers can use safely.

A backend should not parse the same input manually in every route.

The parser middleware handles reusable request-body work:

```txt id="b55qwp"
check Content-Type
check body size
parse the body
store typed request state
stop invalid requests before the handler
```

Then the route handler reads typed state and focuses on application logic.

The parser middleware lives under:

```cpp id="un2oq4"
namespace vix::middleware::parsers
```

When using `vix::App`, prefer the App helpers:

```cpp id="hq1d1e"
namespace vix::middleware::app
```

## What parsers provides

The parser group includes:

| Middleware         | Purpose                                           |
| ------------------ | ------------------------------------------------- |
| `json()`           | Parse `application/json` bodies                   |
| `form()`           | Parse `application/x-www-form-urlencoded` bodies  |
| `multipart()`      | Validate multipart metadata and boundary          |
| `multipart_save()` | Parse multipart form-data and save uploaded files |

For normal `vix::App` applications, use the App presets:

```cpp id="zmpqfx"
middleware::app::json_dev(...)
middleware::app::json_strict_dev(...)
middleware::app::form_dev(...)
middleware::app::multipart_dev(...)
middleware::app::multipart_save_dev(...)
```

## Why parsers matter

Without parser middleware, every route must do this manually:

```txt id="eylg2x"
read raw body
check Content-Type
handle empty body
enforce max size
parse JSON or form data
handle parse errors
return consistent error responses
```

With parser middleware, this reusable work happens before the handler.

```cpp id="fxea15"
app.use("/api/users", middleware::app::json_strict_dev(4096));

app.post("/api/users", [](vix::Request &req, vix::Response &res)
{
  auto &body = req.state<vix::middleware::parsers::JsonBody>();

  res.json({
    "ok", true,
    "received", body.value.dump()
  });
});
```

The handler can assume that the body was already parsed.

If parsing fails, the handler is not called.

## Recommended order

Parser middleware should usually run after security and body size checks.

```cpp id="nqg8r2"
app.use("/api", middleware::app::security_headers_dev());
app.use("/api", middleware::app::cors_dev());
app.use("/api", middleware::app::rate_limit_dev());
app.use("/api", middleware::app::body_limit_write_dev(1024 * 1024));

app.use("/api/users", middleware::app::json_strict_dev(4096));
```

The order matters.

```txt id="ium1xx"
rate limit
  rejects abusive clients

body limit
  rejects oversized bodies

parser
  parses only valid-sized bodies

handler
  uses typed state
```

Avoid parsing large or invalid requests before they have passed basic limits.

## JSON parser

`json()` parses JSON request bodies and stores:

```cpp id="c9oox8"
vix::middleware::parsers::JsonBody
```

Use JSON parsing for API routes.

The most common App preset is:

```cpp id="yzyqfe"
app.use("/api/users", middleware::app::json_strict_dev(4096));
```

`json_strict_dev()` is useful when a route requires:

```txt id="rwvibc"
Content-Type: application/json
non-empty body
valid JSON
body size under the configured limit
```

## JSON example

```cpp id="dsks8g"
#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

int main()
{
  App app;

  app.use("/api/users", middleware::app::json_strict_dev(4096));

  app.post("/api/users", [](Request &req, Response &res)
  {
    auto &body = req.state<middleware::parsers::JsonBody>();

    const std::string name = body.value.value("name", "");
    const std::string email = body.value.value("email", "");

    if (name.empty())
    {
      res.status(422).json({
        "ok", false,
        "error", "Missing name"
      });
      return;
    }

    if (email.empty())
    {
      res.status(422).json({
        "ok", false,
        "error", "Missing email"
      });
      return;
    }

    res.status(201).json({
      "ok", true,
      "user", {
        "name", name,
        "email", email
      }
    });
  });

  app.run(8080);
}
```

Run:

```bash id="srjzug"
vix run json_parser_demo.cpp
```

Send valid JSON:

```bash id="x7aprg"
curl -i \
  -X POST http://127.0.0.1:8080/api/users \
  -H "Content-Type: application/json" \
  -d '{"name":"Ada","email":"ada@example.com"}'
```

Expected status:

```txt id="mqgtax"
201 Created
```

Send invalid JSON:

```bash id="cz84h3"
curl -i \
  -X POST http://127.0.0.1:8080/api/users \
  -H "Content-Type: application/json" \
  -d '{"name":}'
```

Expected status:

```txt id="eslnh8"
400 Bad Request
```

Send wrong content type:

```bash id="cipwji"
curl -i \
  -X POST http://127.0.0.1:8080/api/users \
  -H "Content-Type: text/plain" \
  -d '{"name":"Ada"}'
```

Expected status:

```txt id="skt5xw"
415 Unsupported Media Type
```

Send empty body:

```bash id="fmb8gi"
curl -i \
  -X POST http://127.0.0.1:8080/api/users \
  -H "Content-Type: application/json" \
  -d ''
```

Expected status with strict JSON:

```txt id="qedcsm"
400 Bad Request
```

## Strict JSON vs relaxed JSON

Use strict JSON when the route requires a body.

```cpp id="xvw9o7"
app.use("/api/users", middleware::app::json_strict_dev(4096));
```

Use relaxed JSON when an empty body is acceptable.

```cpp id="iq3aui"
app.use("/api/search", middleware::app::json_dev(
  4096,
  true,
  true
));
```

The practical rule is:

| Route type             | Parser                 |
| ---------------------- | ---------------------- |
| `POST /api/users`      | `json_strict_dev(...)` |
| `PUT /api/users/:id`   | `json_strict_dev(...)` |
| `PATCH /api/users/:id` | `json_strict_dev(...)` |
| Optional filter body   | `json_dev(...)`        |
| Health or GET route    | No JSON parser         |

Do not install JSON parsing globally unless every route under that prefix expects JSON.

Prefer route-specific parser prefixes.

## JSON options

Use lower-level options for exact behavior.

```cpp id="zl5zu0"
vix::middleware::parsers::JsonParserOptions opt;

opt.require_content_type = true;
opt.allow_empty = false;
opt.max_bytes = 4096;
opt.store_in_state = true;

app.use("/api/json", vix::middleware::app::adapt_ctx(
  vix::middleware::parsers::json(opt)
));
```

Main options:

| Option                 | Purpose                                  |
| ---------------------- | ---------------------------------------- |
| `require_content_type` | Require `Content-Type: application/json` |
| `allow_empty`          | Allow empty body and store `{}`          |
| `max_bytes`            | Maximum body size for the parser         |
| `store_in_state`       | Store `JsonBody` in request state        |

Common JSON errors:

| Status | Code                     | Meaning                          |
| ------ | ------------------------ | -------------------------------- |
| `400`  | `empty_body`             | Body is required but empty       |
| `400`  | `invalid_json`           | Body could not be parsed as JSON |
| `413`  | `payload_too_large`      | Body exceeds parser limit        |
| `415`  | `unsupported_media_type` | Content type is not JSON         |

## Form parser

`form()` parses:

```txt id="lyrq52"
application/x-www-form-urlencoded
```

It stores:

```cpp id="jykn7g"
vix::middleware::parsers::FormBody
```

Use it for classic HTML forms and simple form posts.

## Form example

```cpp id="qyfcli"
#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

int main()
{
  App app;

  app.use("/form", middleware::app::form_dev(1024));

  app.get("/", [](Request &, Response &res)
  {
    res.text("POST /form with application/x-www-form-urlencoded");
  });

  app.post("/form", [](Request &req, Response &res)
  {
    auto &form = req.state<middleware::parsers::FormBody>();

    const auto name = form.fields.find("name");

    if (name == form.fields.end() || name->second.empty())
    {
      res.status(422).json({
        "ok", false,
        "error", "Missing name"
      });
      return;
    }

    res.json({
      "ok", true,
      "name", name->second
    });
  });

  app.run(8080);
}
```

Test:

```bash id="z9dcjw"
curl -i \
  -X POST http://127.0.0.1:8080/form \
  -H "Content-Type: application/x-www-form-urlencoded" \
  --data "name=Ada&city=Kampala"
```

Expected status:

```txt id="qm2wqz"
200 OK
```

Wrong content type:

```bash id="gh6qps"
curl -i \
  -X POST http://127.0.0.1:8080/form \
  -H "Content-Type: text/plain" \
  --data "name=Ada"
```

Expected status:

```txt id="glfa84"
415 Unsupported Media Type
```

## Form options

Use lower-level options when needed.

```cpp id="iy74p7"
vix::middleware::parsers::FormParserOptions opt;

opt.require_content_type = true;
opt.max_bytes = 1024;
opt.store_in_state = true;

app.use("/form", vix::middleware::app::adapt_ctx(
  vix::middleware::parsers::form(opt)
));
```

Main options:

| Option                 | Purpose                               |
| ---------------------- | ------------------------------------- |
| `require_content_type` | Require URL-encoded form content type |
| `max_bytes`            | Maximum body size                     |
| `store_in_state`       | Store `FormBody` in request state     |

Common form errors:

| Status | Code                     | Meaning                              |
| ------ | ------------------------ | ------------------------------------ |
| `413`  | `payload_too_large`      | Body exceeds parser limit            |
| `415`  | `unsupported_media_type` | Content type is not URL-encoded form |

## Multipart parser

`multipart()` validates multipart request metadata.

It checks:

```txt id="ju21fz"
Content-Type starts with multipart/form-data
boundary exists when required
body size does not exceed the configured limit
```

It stores:

```cpp id="r243gq"
vix::middleware::parsers::MultipartInfo
```

This parser is useful when you only need to validate multipart metadata.

For real file uploads, use `multipart_save()` through the App helper.

## Multipart save

`multipart_save()` handles multipart form-data and saves uploaded files.

The App preset is usually:

```cpp id="e8dfsy"
app.use("/upload", middleware::app::multipart_save_dev("uploads"));
```

It stores a parsed multipart form in request state.

```cpp id="lxbtpw"
vix::middleware::parsers::MultipartForm
```

A handler can then read the parsed form and return a response.

## Multipart upload example

```cpp id="vxd1rf"
#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

int main()
{
  App app;

  app.use("/upload", middleware::app::multipart_save_dev("uploads"));

  app.get("/", [](Request &, Response &res)
  {
    res.text("POST /upload with multipart/form-data");
  });

  app.post("/upload", [](Request &req, Response &res)
  {
    auto &form = req.state<middleware::parsers::MultipartForm>();

    res.json(middleware::app::multipart_json(form));
  });

  app.run(8080);
}
```

Run:

```bash id="lo56rf"
vix run multipart_upload_demo.cpp
```

Upload fields:

```bash id="dswgpw"
curl -i \
  -X POST http://127.0.0.1:8080/upload \
  -F "title=Profile" \
  -F "description=Avatar upload"
```

Upload a file:

```bash id="gqyn7f"
curl -i \
  -X POST http://127.0.0.1:8080/upload \
  -F "title=Avatar" \
  -F "file=@./avatar.png"
```

The middleware saves uploaded files into:

```txt id="kzscus"
uploads/
```

The handler receives the parsed multipart state.

## Multipart with CORS

If a browser frontend uploads files from another origin, install CORS before multipart parsing.

```cpp id="n0ge4a"
app.use("/upload", middleware::app::cors_dev({"http://localhost:5173"}));
app.use("/upload", middleware::app::multipart_save_dev("uploads"));

app.options("/upload", [](Request &, Response &res)
{
  res.status(204).send();
});
```

The CORS middleware must be able to answer preflight requests before upload parsing runs.

## Multipart options

Use lower-level options when you only need metadata validation.

```cpp id="p0pzcc"
vix::middleware::parsers::MultipartOptions opt;

opt.require_boundary = true;
opt.max_bytes = 1024 * 1024;
opt.store_in_state = true;

app.use("/multipart", vix::middleware::app::adapt_ctx(
  vix::middleware::parsers::multipart(opt)
));
```

Main options:

| Option             | Purpose                                |
| ------------------ | -------------------------------------- |
| `require_boundary` | Require the multipart boundary         |
| `max_bytes`        | Maximum body size                      |
| `store_in_state`   | Store `MultipartInfo` in request state |

Common multipart errors:

| Status | Code                     | Meaning                                 |
| ------ | ------------------------ | --------------------------------------- |
| `400`  | `missing_boundary`       | Multipart boundary is missing           |
| `413`  | `payload_too_large`      | Body exceeds parser limit               |
| `415`  | `unsupported_media_type` | Content type is not multipart form-data |

## Choosing the right parser

Use this rule:

| Request body             | Middleware                |
| ------------------------ | ------------------------- |
| JSON API body            | `json_strict_dev(...)`    |
| Optional JSON body       | `json_dev(...)`           |
| HTML form post           | `form_dev(...)`           |
| Multipart metadata check | `multipart(...)`          |
| File uploads             | `multipart_save_dev(...)` |

Install parsers only where the route expects that body format.

Good:

```cpp id="l92w49"
app.use("/api/users", middleware::app::json_strict_dev(4096));
app.use("/upload", middleware::app::multipart_save_dev("uploads"));
```

Avoid:

```cpp id="yly77p"
app.use("/", middleware::app::json_strict_dev(4096));
```

A global strict parser can reject routes that do not have request bodies.

## Parsers and typed state

Every parser stores a typed object.

| Parser             | State type      |
| ------------------ | --------------- |
| `json()`           | `JsonBody`      |
| `form()`           | `FormBody`      |
| `multipart()`      | `MultipartInfo` |
| `multipart_save()` | `MultipartForm` |

Read state in the handler:

```cpp id="qu9r8d"
auto &body = req.state<vix::middleware::parsers::JsonBody>();
```

Use `state<T>()` when the parser must exist.

Use `try_state<T>()` when the parser may be optional.

```cpp id="bnmr6m"
auto *body = req.try_state<vix::middleware::parsers::JsonBody>();

if (!body)
{
  res.status(500).json({
    "ok", false,
    "error", "json_state_missing"
  });
  return;
}
```

## Parsers and validation

Parsers are not full business validators.

A parser answers:

```txt id="e1rvbb"
Is the body syntactically valid?
Can it be decoded?
Can the result be stored?
```

Application validation answers:

```txt id="k889ub"
Is this field required?
Is this email valid?
Is this quantity positive?
Is this product available?
```

Example:

```cpp id="iays10"
app.post("/api/orders", [](vix::Request &req, vix::Response &res)
{
  auto &body = req.state<vix::middleware::parsers::JsonBody>();

  const int quantity = body.value.value("quantity", 0);

  if (quantity <= 0)
  {
    res.status(422).json({
      "ok", false,
      "error", "Quantity must be positive"
    });
    return;
  }

  res.status(201).json({
    "ok", true
  });
});
```

The parser validates the body format.

The handler validates application rules.

## Parsers and body limits

Use body limits before parsers.

```cpp id="s06cax"
app.use("/api", middleware::app::body_limit_write_dev(1024 * 1024));
app.use("/api/users", middleware::app::json_strict_dev(4096));
```

This avoids doing parser work on oversized requests.

A parser can still have its own `max_bytes`.

The body limit protects the broader route group.

The parser limit protects the specific body format.

## Complete example

This example combines JSON, form data, and multipart upload routes.

```cpp id="lbkuwz"
#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

int main()
{
  App app;

  app.use("/api", middleware::app::security_headers_dev());
  app.use("/api", middleware::app::rate_limit_dev());
  app.use("/api", middleware::app::body_limit_write_dev(1024 * 1024));

  app.use("/api/users", middleware::app::json_strict_dev(4096));
  app.use("/form", middleware::app::form_dev(4096));
  app.use("/upload", middleware::app::multipart_save_dev("uploads"));

  app.post("/api/users", [](Request &req, Response &res)
  {
    auto &body = req.state<middleware::parsers::JsonBody>();

    res.status(201).json({
      "ok", true,
      "json", body.value.dump()
    });
  });

  app.post("/form", [](Request &req, Response &res)
  {
    auto &form = req.state<middleware::parsers::FormBody>();

    res.json({
      "ok", true,
      "fields_count", static_cast<long long>(form.fields.size())
    });
  });

  app.post("/upload", [](Request &req, Response &res)
  {
    auto &form = req.state<middleware::parsers::MultipartForm>();

    res.json(middleware::app::multipart_json(form));
  });

  app.run(8080);
}
```

Test JSON:

```bash id="dzeg8h"
curl -i \
  -X POST http://127.0.0.1:8080/api/users \
  -H "Content-Type: application/json" \
  -d '{"name":"Ada"}'
```

Test form:

```bash id="b8mzcm"
curl -i \
  -X POST http://127.0.0.1:8080/form \
  -H "Content-Type: application/x-www-form-urlencoded" \
  --data "name=Ada&city=Kampala"
```

Test multipart:

```bash id="v9xy26"
curl -i \
  -X POST http://127.0.0.1:8080/upload \
  -F "title=Avatar" \
  -F "file=@./avatar.png"
```

## Summary

Use parser middleware to keep handlers focused.

A parser should:

```txt id="uk9rwa"
check the request body format
reject invalid input early
store typed request state
let the handler work with parsed data
```

A good default pattern is:

```cpp id="adpg91"
app.use("/api", middleware::app::body_limit_write_dev(1024 * 1024));
app.use("/api/users", middleware::app::json_strict_dev(4096));
```

Then inside the handler:

```cpp id="qlfi3k"
auto &body = req.state<middleware::parsers::JsonBody>();
```

Remember:

```txt id="bi59xt"
body limit before parser
parser before handler
business validation inside the handler or validation layer
```
