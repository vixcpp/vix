# Errors and logging

In the previous chapter, you learned validation.
Now you will learn errors and logging.

```txt
request → validation → business logic → error or success → structured response → structured logs
```

A production application should return clear responses to clients and keep useful logs for developers.

## Why error handling matters

If every route returns a different error shape, the API becomes hard to use.
A Vix API should use one predictable shape.

## Recommended error shape

```json
{ "ok": false, "error": "message" }
{ "ok": false, "error": "validation_failed", "errors": [] }
{ "ok": true, "data": {} }
```

## HTTP status codes

| Status | Meaning                          |
| ------ | -------------------------------- |
| `200`  | OK, request succeeded.           |
| `201`  | Created, resource added.         |
| `400`  | Bad Request, invalid input.      |
| `401`  | Unauthorized, auth required.     |
| `403`  | Forbidden, access denied.        |
| `404`  | Not Found, resource missing.     |
| `409`  | Conflict, state mismatch.        |
| `429`  | Too Many Requests, rate limited. |
| `500`  | Internal Server Error.           |

Do not return 200 for errors.

## Basic error helper

```cpp
static void respond_error(
    vix::Response &res,
    int status,
    const std::string &code,
    const std::string &message)
{
  res.status(status).json(vix::json::kv({
      {"ok", vix::json::Json(false)},
      {"error", vix::json::Json(code)},
      {"message", vix::json::Json(message)},
  }));
}
```

Always return after sending an error:

```cpp
if (name.empty()) {
  respond_error(res, 400, "validation_failed", "name is required");
  return;
}
```

## Error codes

Use stable codes for production APIs:

```txt
invalid_request,
validation_failed,
unauthorized,
forbidden,
not_found,
conflict,
rate_limited,
internal_error,
user_not_found,
email_already_used,
invalid_credentials,
product_not_found,
invalid_token,
session_expired
```

## Do not leak internal errors

```cpp
catch (const std::exception &e)
{
  vix::log::error("unhandled route error", "details", e.what());
  res.status(500).json({
    "ok", false,
    "error", "internal_error",
    "message", "Internal server error"
  });
}
```

## Public logging header

```cpp
#include <vix/log.hpp>
```

## Basic logs

```cpp
vix::log::set_level(vix::log::LogLevel::Trace);
vix::log::trace("trace message");
vix::log::debug("debug message");
vix::log::info("info message");
vix::log::warn("warn message");
vix::log::error("error message");
vix::log::critical("critical message");
```

## Log levels

| Level      | Use                                     |
| ---------- | --------------------------------------- |
| `trace`    | Records very detailed debugging events. |
| `debug`    | Records useful debugging information.   |
| `info`     | Records normal application events.      |
| `warn`     | Records unusual but non-fatal events.   |
| `error`    | Records failed operations.              |
| `critical` | Records serious system failures.        |

Recommended: `info` in production, `debug` or `trace` during development.

## Structured logs

```cpp
vix::log::logf(
    vix::log::LogLevel::Info,
    "user authenticated successfully",
    "status", 200,
    "method", "POST",
    "path", "/login"
);
```

## Log formats

```cpp
vix::log::set_format(vix::log::LogFormat::KV);    // local development
vix::log::set_format(vix::log::LogFormat::JSON);   // production log collectors
```

## Log context

```cpp
vix::log::LogContext ctx;
ctx.request_id = "req-abc-123";
ctx.module = "auth";
ctx.fields["user_id"] = "42";
vix::log::set_context(ctx);
vix::log::info("user authenticated successfully");
vix::log::clear_context();
```

## Set log level

```cpp
vix::log::set_level(vix::log::LogLevel::Info);  // in code
// vix run main.cpp --log-level debug             // via CLI
// VIX_LOG_LEVEL=info                             // via environment
```

## What to log

Good:
app started,
user registered,
login failed,
database connection failed,
unexpected exception.

Never log:
passwords,
tokens,
private keys,
sensitive personal data.

## Complete example

```cpp
#include <vix.hpp>
#include <vix/log.hpp>
#include <vix/validation.hpp>

using namespace vix;

struct RegisterInput : vix::validation::BaseModel<RegisterInput>
{
  std::string email;
  std::string password;

  static vix::validation::Schema<RegisterInput> schema()
  {
    return vix::validation::schema<RegisterInput>()
        .field("email", &RegisterInput::email,
               vix::validation::field<std::string>().required().email().length_max(120))

        .field("password", &RegisterInput::password,
               vix::validation::field<std::string>().required().length_min(8).length_max(64));
  }
};

static json::Json validation_errors_to_json(const vix::validation::ValidationErrors &errors)
{
  json::Json items = json::Json::array();

  for (const auto &error : errors.all())
    items.push_back(json::kv({
      {"field", json::Json(error.field)},
      {"code", json::Json(vix::validation::to_string(error.code))},
      {"message", json::Json(error.message)}
    }));

  return items;
}

static void respond_error(Response &res, int status, const std::string &code, const std::string &message)
{
  res.status(status).json(json::kv({
    {"ok", json::Json(false)},
    {"error", json::Json(code)},
    {"message", json::Json(message)}
  }));
}

int main()
{
  vix::log::set_level(vix::log::LogLevel::Info);
  vix::log::set_format(vix::log::LogFormat::KV);
  vix::log::info("starting errors and logging example");

  App app;

  app.get("/health", [](Request &, Response &res){
    vix::log::debug("health check requested");
    res.json({
      "ok", true,
      "service", "errors-logging-example"
    });
  });

  app.post("/api/register", [](Request &req, Response &res){

    try{
      const auto &body = req.json();
      if (!body.is_object()) {
        respond_error(res, 400, "invalid_request", "Expected JSON object body");
        return;
      }

      RegisterInput input;
      input.email = body.value("email", "");
      input.password = body.value("password", "");

      auto result = input.validate();
      if (!result.ok()){
        vix::log::warn("register validation failed", "email", input.email);

        res.status(400).json(json::kv({
          {"ok", json::Json(false)},
          {"error", json::Json("validation_failed")},
          {"errors", validation_errors_to_json(result.errors)}
        }));

        return;
      }

      vix::log::logf(vix::log::LogLevel::Info, "user registered", "email", input.email);

      res.status(201).json({
        "ok", true,
        "message", "registered"
      });

    }
    catch (const std::exception &e){
      vix::log::error("register failed with exception", "details", e.what());
      respond_error(res, 500, "internal_error", "Internal server error");
    }

  });

  app.run(8080);

  return 0;
}
```

## Test

```bash
curl -i http://127.0.0.1:8080/health
curl -i -X POST http://127.0.0.1:8080/api/register \
  -H "Content-Type: application/json" \
  -d '{"email":"ada@example.com","password":"password123"}'
curl -i -X POST http://127.0.0.1:8080/api/register \
  -H "Content-Type: application/json" \
  -d '{"email":"bad-email","password":"123"}'
```

## Common mistakes

### Returning HTTP 200 for errors

Use the correct error status code always.

### Logging secrets

Never log passwords, tokens, private keys, or authorization headers.

### Exposing internal exceptions

```cpp
// Wrong
res.json({"error", e.what()});

// Correct
vix::log::error("failed", "details", e.what());

res.json({
  "ok", false,
  "error", "internal_error"
});
```

### Forgetting to return after an error

```cpp
if (!result.ok()) {
  respond_validation_error(res, result);
  return;
}
```

## Production config

```dotenv
VIX_LOG_LEVEL=info
VIX_LOG_FORMAT=json
VIX_COLOR=never
```

## What you should remember

Errors are for clients.
Logs are for developers and operators.
API errors should be consistent: `{ "ok": false, "error": "stable_code", "message": "Safe message" }`
Logs should keep useful internal context with structured fields.
The core idea:
a reliable app does not only work when everything succeeds — it also explains what happened when something fails.

## Next chapter

[Next: Database](/book/10-database)
