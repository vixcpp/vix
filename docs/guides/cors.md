# CORS

This guide shows how to configure CORS in a Vix API.

CORS is needed when a frontend running on one origin calls an API running on another origin.

```txt
Frontend: http://localhost:5173
API:      http://localhost:8080
```

## What is CORS?

An origin is composed of scheme + host + port.
The browser uses CORS to decide whether frontend JavaScript is allowed to call an API on another origin.

## Setup

```bash
vix new cors-api
cd cors-api
```

```cpp
#include <vix.hpp>
#include <vix/middleware.hpp>
```

## Simple CORS setup

```cpp
app.use(vix::middleware::app::cors_dev({
    "http://localhost:5173",
    "http://0.0.0.0:5173"
}));
```

## Custom CORS options

```cpp
vix::middleware::security::CorsOptions options;

options.allowed_origins = {"http://localhost:5173", "https://app.example.com"};
options.allow_any_origin = false;
options.allow_credentials = true;

options.allow_methods = {"GET", "POST", "PUT", "PATCH", "DELETE", "OPTIONS"};

options.allow_headers = {
    "Content-Type", "Authorization", "X-Requested-With", "Accept", "Origin"
};

options.expose_headers = {"X-Request-Id"};

app.use(vix::middleware::app::adapt_ctx(
    vix::middleware::security::cors(std::move(options))));
```

## Prefix-specific CORS

```cpp
vix::middleware::app::use_on_prefix(
    app,
    "/api",
    vix::middleware::app::cors_dev({"http://localhost:5173"}));
```

## Credentials

Enable when your frontend must send cookies or auth headers:

```cpp
options.allow_credentials = true;
```

> When credentials are enabled, do not use wildcard origins in production.

## Development vs production

```cpp
// Development
app.use(vix::middleware::app::cors_dev({"http://localhost:5173"}));

// Production (explicit origins only)
options.allowed_origins = {"https://app.example.com"};
```

## Test

```bash
curl -i http://127.0.0.1:8080/api/profile -H "Origin: http://localhost:5173"

# Preflight
curl -i -X OPTIONS http://127.0.0.1:8080/api/messages \
  -H "Origin: http://localhost:5173" \
  -H "Access-Control-Request-Method: POST" \
  -H "Access-Control-Request-Headers: Content-Type, Authorization"
```

## CORS headers explained

| Header                             | Purpose                                     |
| ---------------------------------- | ------------------------------------------- |
| `Access-Control-Allow-Origin`      | Defines which origin is allowed.            |
| `Access-Control-Allow-Methods`     | Defines which HTTP methods are allowed.     |
| `Access-Control-Allow-Headers`     | Defines which request headers are allowed.  |
| `Access-Control-Allow-Credentials` | Defines whether credentials are allowed.    |
| `Access-Control-Expose-Headers`    | Defines which response headers JS can read. |

## Common mistakes

### Allowing the API origin instead of frontend origin

```cpp
// Wrong
options.allowed_origins = {"http://localhost:8080"};

// Correct — allow the FRONTEND origin
options.allowed_origins = {"http://localhost:5173"};
```

### Using wildcard with credentials

```text
Access-Control-Allow-Origin: *
Access-Control-Allow-Credentials: true   // NEVER combine these
```

### Applying CORS after route logic

Install middleware before route registration.

## Production checklist

- Only real frontend origins are allowed
- No wildcard origin with credentials
- HTTPS is enabled
- `Authorization` header is allowed if using tokens
- Credentials enabled only if using cookies/sessions
- OPTIONS requests are handled

## What to use next

- [Rate limiting guide](/guides/rate-limiting)
- [Authentication guide](/guides/authentication)
- [Sessions guide](/guides/sessions)
