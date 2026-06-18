# API Reference

This page is a lookup reference for `vix::middleware`.

Read the guide pages first if you are new to the module:

1. [Quick Start](./quick-start)
2. [App Integration](./app-integration)
3. [Core Concepts](./concepts)

Use this page when you already know what you want and need the namespace, type, helper, option, or state object.

## Public include

For Vix.cpp `v2.6.2` and newer:

```cpp
#include <vix.hpp>
#include <vix/middleware.hpp>
```

For Vix.cpp `v2.6.0` and `v2.6.1`, App integration headers may need to be included explicitly:

```cpp
#include <vix.hpp>
#include <vix/middleware.hpp>

#include <vix/middleware/app/adapter.hpp>
#include <vix/middleware/app/app_middleware.hpp>
#include <vix/middleware/app/http_cache.hpp>
#include <vix/middleware/app/presets.hpp>
```

## Main namespaces

| Namespace                        | Purpose                                          |
| -------------------------------- | ------------------------------------------------ |
| `vix::middleware`                | Core middleware aliases, pipeline, HTTP cache    |
| `vix::middleware::app`           | `vix::App` integration helpers and presets       |
| `vix::middleware::basics`        | Recovery, request id, timing, body limit, logger |
| `vix::middleware::security`      | CORS, CSRF, headers, IP filter, rate limit       |
| `vix::middleware::auth`          | API key, JWT, RBAC, sessions                     |
| `vix::middleware::parsers`       | JSON, form, multipart parsers                    |
| `vix::middleware::performance`   | Compression, ETag, static response hook          |
| `vix::middleware::observability` | Tracing, metrics, debug trace                    |
| `vix::middleware::cookies`       | Cookie helpers                                   |
| `vix::middleware::utils`         | Internal utilities used by middleware            |

## Core types

Defined by the middleware module:

```cpp
namespace vix::middleware
{
  using Request = vix::http::Request;
  using Response = vix::http::ResponseWrapper;

  using NextFn = vix::mw::NextFn;
  using Next = vix::mw::Next;
  using NextOnce = vix::mw::NextOnce;

  using Error = vix::mw::Error;
  using Services = vix::mw::Services;
  using Context = vix::mw::Context;

  using MiddlewareFn = std::function<void(Context &, Next)>;

  using HttpMiddleware =
    std::function<void(Request &, Response &, Next)>;

  using Hooks = vix::mw::Hooks;
}
```

## Core helpers

```cpp
vix::middleware::from_http_middleware(...)
vix::middleware::to_http_middleware(...)
vix::middleware::noop()
vix::middleware::use_if(...)
```

Common usage:

```cpp
auto mw = vix::middleware::from_http_middleware(my_http_middleware);
```

## Error helpers

Re-exported from `vix::mw`:

```cpp
vix::middleware::ok(...)
vix::middleware::fail(...)
vix::middleware::bad_request(...)
vix::middleware::unauthorized(...)
vix::middleware::forbidden(...)
vix::middleware::not_found(...)
vix::middleware::conflict(...)
vix::middleware::internal(...)
vix::middleware::normalize(...)
vix::middleware::to_json(...)
```

Common pattern:

```cpp
vix::middleware::Error err;

err.status = 401;
err.code = "unauthorized";
err.message = "Missing token";

ctx.send_error(vix::middleware::normalize(std::move(err)));
```

## App integration helpers

Namespace:

```cpp
namespace vix::middleware::app
```

Helpers:

```cpp
vix::middleware::app::adapt(...)
vix::middleware::app::adapt_ctx(...)
vix::middleware::app::when(...)
vix::middleware::app::protect_path(...)
vix::middleware::app::protect_prefix_mw(...)
vix::middleware::app::protect(...)
vix::middleware::app::protect_prefix(...)
vix::middleware::app::install(...)
vix::middleware::app::install_exact(...)
vix::middleware::app::chain(...)
```

### `adapt_ctx`

Converts `MiddlewareFn` to `vix::App::Middleware`.

```cpp
app.use(vix::middleware::app::adapt_ctx(
  vix::middleware::basics::request_id()
));
```

### `adapt`

Converts `HttpMiddleware` to `vix::App::Middleware`.

```cpp
app.use(vix::middleware::app::adapt(my_http_middleware));
```

### `when`

Runs middleware only when the predicate matches.

```cpp
auto only_post = vix::middleware::app::when(
  [](const vix::Request &req)
  {
    return req.method() == "POST";
  },
  vix::middleware::app::body_limit_write_dev(1024)
);
```

### `protect`

Installs middleware on one exact path.

```cpp
vix::middleware::app::protect(
  app,
  "/admin",
  vix::middleware::app::api_key_dev("secret")
);
```

### `protect_prefix`

Installs middleware on a route prefix.

```cpp
vix::middleware::app::protect_prefix(
  app,
  "/admin",
  vix::middleware::app::api_key_dev("secret")
);
```

### `chain`

Combines App middlewares in order.

```cpp
auto stack = vix::middleware::app::chain(
  vix::middleware::app::body_limit_write_dev(4096),
  vix::middleware::app::json_strict_dev(4096)
);

app.use("/api/users", std::move(stack));
```

## App presets

The App presets return `vix::App::Middleware`.

Common presets include:

```cpp
vix::middleware::app::recovery_dev(...)
vix::middleware::app::request_id_dev(...)
vix::middleware::app::timing_dev(...)
vix::middleware::app::body_limit_dev(...)
vix::middleware::app::body_limit_write_dev(...)

vix::middleware::app::security_headers_dev(...)
vix::middleware::app::cors_dev(...)
vix::middleware::app::csrf_dev(...)
vix::middleware::app::ip_filter_dev(...)
vix::middleware::app::ip_filter_allow_deny_dev(...)
vix::middleware::app::rate_limit_dev(...)
vix::middleware::app::rate_limit_custom_dev(...)

vix::middleware::app::api_key_dev(...)
vix::middleware::app::jwt_dev(...)
vix::middleware::app::session_dev(...)

vix::middleware::app::json_dev(...)
vix::middleware::app::json_strict_dev(...)
vix::middleware::app::form_dev(...)
vix::middleware::app::multipart_dev(...)
vix::middleware::app::multipart_save_dev(...)

vix::middleware::app::http_cache(...)
vix::middleware::app::http_cache_mw(...)
vix::middleware::app::install_http_cache(...)
```

Example:

```cpp
app.use("/api", vix::middleware::app::security_headers_dev());
app.use("/api", vix::middleware::app::cors_dev());
app.use("/api", vix::middleware::app::rate_limit_dev());
app.use("/api/users", vix::middleware::app::json_strict_dev(4096));
```

## Basics

Namespace:

```cpp
namespace vix::middleware::basics
```

### Recovery

```cpp
vix::middleware::basics::recovery(...)
```

Options:

```cpp
struct RecoveryOptions
{
  bool include_exception_message;
  bool include_code_location;
  std::string code;
  std::string message;
};
```

App usage:

```cpp
app.use("/api", vix::middleware::app::recovery_dev());
```

Lower-level usage:

```cpp
vix::middleware::basics::RecoveryOptions opt;

opt.include_exception_message = false;
opt.code = "internal_server_error";
opt.message = "Internal Server Error";

app.use("/api", vix::middleware::app::adapt_ctx(
  vix::middleware::basics::recovery(opt)
));
```

### Request id

```cpp
vix::middleware::basics::request_id(...)
```

State type:

```cpp
vix::middleware::basics::RequestId
```

Options:

```cpp
struct RequestIdOptions
{
  std::string header_name;
  bool accept_incoming;
  bool generate_if_missing;
  bool always_set_response_header;
};
```

App usage:

```cpp
app.use("/api", vix::middleware::app::request_id_dev());
```

Read state:

```cpp
auto *rid = req.try_state<vix::middleware::basics::RequestId>();
```

### Timing

```cpp
vix::middleware::basics::timing(...)
```

State type:

```cpp
vix::middleware::basics::Timing
```

Options:

```cpp
struct TimingOptions
{
  bool set_x_response_time;
  bool set_server_timing;
  std::string x_response_time_header;
  std::string server_timing_header;
  std::string server_timing_metric;
  bool store_in_state;
};
```

App usage:

```cpp
app.use("/api", vix::middleware::app::timing_dev());
```

Read state:

```cpp
auto *timing = req.try_state<vix::middleware::basics::Timing>();
```

### Body limit

```cpp
vix::middleware::basics::body_limit(...)
```

Options:

```cpp
struct BodyLimitOptions
{
  std::size_t max_bytes;
  bool apply_to_get;
  bool allow_chunked;

  std::function<bool(const vix::middleware::Context &)> should_apply;
};
```

App usage:

```cpp
app.use("/api", vix::middleware::app::body_limit_write_dev(1024 * 1024));
```

Common errors:

| Status | Code                |
| ------ | ------------------- |
| `411`  | `length_required`   |
| `413`  | `payload_too_large` |

### Logger

```cpp
vix::middleware::basics::logger(...)
```

Options:

```cpp
enum class LogFormat
{
  Text,
  Json
};

struct LoggerOptions
{
  LogFormat format;
  bool log_request_id;
  bool log_timing;
  bool level_from_status;
  bool include_user_agent;
  bool include_forwarded_for;
  bool require_timing;
};
```

Logger service interface:

```cpp
struct ILogger
{
  virtual ~ILogger() = default;

  virtual void info(std::string_view msg) = 0;
  virtual void warn(std::string_view msg) = 0;
  virtual void error(std::string_view msg) = 0;
};
```

## Security

Namespace:

```cpp
namespace vix::middleware::security
```

### Security headers

```cpp
vix::middleware::security::headers(...)
```

Options:

```cpp
struct SecurityHeadersOptions
{
  bool x_content_type_options;
  bool x_frame_options;
  bool x_xss_protection;
  bool referrer_policy;
  bool permissions_policy;

  bool hsts;
  int hsts_max_age;
  bool hsts_include_subdomains;
  bool hsts_preload;

  std::string content_security_policy;
};
```

App usage:

```cpp
app.use("/api", vix::middleware::app::security_headers_dev());
```

### CORS

```cpp
vix::middleware::security::cors(...)
```

Options:

```cpp
struct CorsOptions
{
  std::vector<std::string> allowed_origins;
  bool allow_any_origin;
  bool allow_credentials;

  std::vector<std::string> allow_methods;
  std::vector<std::string> allow_headers;
  std::vector<std::string> expose_headers;

  int max_age_seconds;
  bool vary_origin;
};
```

App usage:

```cpp
app.use("/api", vix::middleware::app::cors_dev({
  "https://example.com"
}));
```

Common error:

| Status | Code             |
| ------ | ---------------- |
| `403`  | `cors_forbidden` |

### CSRF

```cpp
vix::middleware::security::csrf(...)
```

Options:

```cpp
struct CsrfOptions
{
  std::string cookie_name;
  std::string header_name;
  bool protect_get;
};
```

App usage:

```cpp
app.use("/api", vix::middleware::app::csrf_dev());
```

Common error:

| Status | Code          |
| ------ | ------------- |
| `403`  | `csrf_failed` |

### IP filter

```cpp
vix::middleware::security::ip_filter(...)
```

Options:

```cpp
struct IpFilterOptions
{
  std::vector<std::string> allow;
  std::vector<std::string> deny;
  std::string header_name;
  bool use_remote_addr_fallback;
};
```

App usage:

```cpp
app.use("/api", vix::middleware::app::ip_filter_allow_deny_dev(
  "x-forwarded-for",
  {"10.0.0.1"},
  {"9.9.9.9"},
  true
));
```

Common errors:

| Status | Code             |
| ------ | ---------------- |
| `403`  | `ip_denied`      |
| `403`  | `ip_not_allowed` |

### Rate limit

```cpp
vix::middleware::security::rate_limit(...)
```

Options:

```cpp
struct RateLimitOptions
{
  double capacity;
  double refill_per_sec;
  bool add_headers;
  std::string key_header;

  std::function<std::string(const vix::middleware::Request &)> key_fn;
};
```

App usage:

```cpp
app.use("/api", vix::middleware::app::rate_limit_dev());
```

Custom usage:

```cpp
app.use("/api", vix::middleware::app::rate_limit_custom_dev(
  5.0,
  0.0,
  "x-forwarded-for"
));
```

State:

```cpp
vix::middleware::security::RateLimiterState
```

Common error:

| Status | Code           |
| ------ | -------------- |
| `429`  | `rate_limited` |

## Authentication

Namespace:

```cpp
namespace vix::middleware::auth
```

### API key

```cpp
vix::middleware::auth::api_key(...)
```

State type:

```cpp
vix::middleware::auth::ApiKey
```

Options:

```cpp
struct ApiKeyOptions
{
  std::string header;
  std::string query_param;
  bool required;

  std::unordered_set<std::string> allowed_keys;

  std::function<std::string(const vix::middleware::Request &)> extract;
  std::function<bool(const std::string &)> validate;
};
```

App usage:

```cpp
app.use("/secure", vix::middleware::app::api_key_dev("secret"));
```

Read state:

```cpp
auto &key = req.state<vix::middleware::auth::ApiKey>();
```

Common errors:

| Status | Code              |
| ------ | ----------------- |
| `401`  | `missing_api_key` |
| `403`  | `invalid_api_key` |

### JWT

```cpp
vix::middleware::auth::jwt(...)
```

State type:

```cpp
vix::middleware::auth::JwtClaims
```

App usage:

```cpp
app.use("/secure", vix::middleware::app::jwt_dev("dev_secret"));
```

Read state:

```cpp
auto &claims = req.state<vix::middleware::auth::JwtClaims>();
```

Common errors include missing, malformed, invalid, or unauthorized Bearer tokens.

Use the JWT guide for exact behavior in your build configuration.

### RBAC

```cpp
vix::middleware::auth::rbac_context(...)
vix::middleware::auth::require_role(...)
vix::middleware::auth::require_any_role(...)
vix::middleware::auth::require_perm(...)
vix::middleware::auth::require_any_perm(...)
vix::middleware::auth::require_all_perms(...)
```

State type:

```cpp
vix::middleware::auth::Authz
```

Options:

```cpp
struct RbacOptions
{
  std::string roles_key;
  std::string perms_key;
  bool require_auth;
  bool use_resolver;
};
```

App usage:

```cpp
app.use("/admin", vix::middleware::app::adapt_ctx(
  vix::middleware::auth::rbac_context()
));

app.use("/admin", vix::middleware::app::adapt_ctx(
  vix::middleware::auth::require_role("admin")
));
```

Read state:

```cpp
auto &authz = req.state<vix::middleware::auth::Authz>();

authz.has_role("admin");
authz.has_perm("products:write");
```

Resolver interface:

```cpp
struct PermissionResolver
{
  virtual ~PermissionResolver() = default;

  virtual void resolve(
    std::string_view subject,
    std::unordered_set<std::string> &roles_inout,
    std::unordered_set<std::string> &perms_inout) = 0;
};
```

Common errors:

| Status | Code            |
| ------ | --------------- |
| `401`  | `missing_auth`  |
| `401`  | `missing_authz` |
| `403`  | `forbidden`     |

### Session

```cpp
vix::middleware::auth::session(...)
```

State type:

```cpp
vix::middleware::auth::Session
```

Options:

```cpp
struct SessionOptions
{
  std::shared_ptr<ISessionStore> store;

  std::string secret;
  std::string cookie_name;
  std::string cookie_path;

  bool secure;
  bool http_only;
  std::string same_site;

  std::chrono::seconds ttl;
  bool auto_create;
};
```

Store interface:

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

Built-in store:

```cpp
vix::middleware::auth::InMemorySessionStore
```

App usage:

```cpp
app.use(vix::middleware::app::adapt_ctx(
  vix::middleware::auth::session({
    .secret = "dev"
  })
));
```

Read state:

```cpp
auto &session = req.state<vix::middleware::auth::Session>();

session.set("n", "1");
auto value = session.get("n");
session.erase("n");
session.destroy();
```

## Parsers

Namespace:

```cpp
namespace vix::middleware::parsers
```

### JSON

```cpp
vix::middleware::parsers::json(...)
```

State type:

```cpp
vix::middleware::parsers::JsonBody
```

Options:

```cpp
struct JsonParserOptions
{
  bool require_content_type;
  bool allow_empty;
  std::size_t max_bytes;
  bool store_in_state;
};
```

App usage:

```cpp
app.use("/api/users", vix::middleware::app::json_strict_dev(4096));
```

Read state:

```cpp
auto &body = req.state<vix::middleware::parsers::JsonBody>();
```

Common errors:

| Status | Code                     |
| ------ | ------------------------ |
| `400`  | `empty_body`             |
| `400`  | `invalid_json`           |
| `413`  | `payload_too_large`      |
| `415`  | `unsupported_media_type` |

### Form

```cpp
vix::middleware::parsers::form(...)
```

State type:

```cpp
vix::middleware::parsers::FormBody
```

Options:

```cpp
struct FormParserOptions
{
  bool require_content_type;
  std::size_t max_bytes;
  bool store_in_state;
};
```

App usage:

```cpp
app.use("/form", vix::middleware::app::form_dev(4096));
```

Read state:

```cpp
auto &form = req.state<vix::middleware::parsers::FormBody>();
```

### Multipart

```cpp
vix::middleware::parsers::multipart(...)
vix::middleware::parsers::multipart_save(...)
```

State types:

```cpp
vix::middleware::parsers::MultipartInfo
vix::middleware::parsers::MultipartForm
```

Options for metadata parser:

```cpp
struct MultipartOptions
{
  bool require_boundary;
  std::size_t max_bytes;
  bool store_in_state;
};
```

App usage:

```cpp
app.use("/upload", vix::middleware::app::multipart_save_dev("uploads"));
```

Read state:

```cpp
auto &form = req.state<vix::middleware::parsers::MultipartForm>();
```

Common errors:

| Status | Code                     |
| ------ | ------------------------ |
| `400`  | `missing_boundary`       |
| `413`  | `payload_too_large`      |
| `415`  | `unsupported_media_type` |

## HTTP helpers

Namespace:

```cpp
namespace vix::middleware::cookies
```

Types and helpers:

```cpp
vix::middleware::cookies::Cookie

vix::middleware::cookies::parse(...)
vix::middleware::cookies::get(...)
vix::middleware::cookies::set(...)
vix::middleware::cookies::build_set_cookie_value(...)
```

Example:

```cpp
app.get("/cookie", [](vix::Request &, vix::Response &res)
{
  vix::middleware::cookies::Cookie cookie;

  cookie.name = "hello";
  cookie.value = "vix";
  cookie.max_age = 3600;
  cookie.http_only = true;
  cookie.same_site = "Lax";

  vix::middleware::cookies::set(res, cookie);

  res.text("cookie set");
});
```

## HTTP cache

Namespace:

```cpp
namespace vix::middleware
namespace vix::middleware::app
```

Lower-level middleware:

```cpp
vix::middleware::http_cache(...)
```

App helpers:

```cpp
vix::middleware::app::http_cache(...)
vix::middleware::app::http_cache_mw(...)
vix::middleware::app::install_http_cache(...)
vix::middleware::app::make_default_cache(...)
```

App config:

```cpp
struct HttpCacheAppConfig
{
  std::string prefix;
  bool only_get;
  int ttl_ms;

  bool allow_bypass;
  std::string bypass_header;
  std::string bypass_value;

  std::vector<std::string> vary_headers;
  std::shared_ptr<vix::cache::Cache> cache;

  bool add_debug_header;
  std::string debug_header;
};
```

Lower-level options:

```cpp
struct HttpCacheOptions
{
  std::vector<std::string> vary_headers;
  bool cache_200_only;
  bool require_body;

  bool allow_bypass;
  std::string bypass_header;
  std::string bypass_value;

  std::function<vix::cache::CacheContext(Request &)> context_provider;
};
```

App usage:

```cpp
app.use("/api", vix::middleware::app::http_cache({
  .ttl_ms = 30'000,
  .allow_bypass = true,
  .bypass_header = "x-vix-cache",
  .bypass_value = "bypass",
  .vary_headers = {"accept-language"}
}));
```

Common cache status header:

```txt
x-vix-cache-status: miss
x-vix-cache-status: hit
x-vix-cache-status: bypass
```

## Performance

Namespace:

```cpp
namespace vix::middleware::performance
```

### Compression

```cpp
vix::middleware::performance::compression(...)
```

Options:

```cpp
struct CompressionOptions
{
  std::size_t min_size;
  bool add_vary;
  bool enabled;
  int gzip_level;
};
```

App usage:

```cpp
app.use("/api", vix::middleware::app::adapt_ctx(
  vix::middleware::performance::compression({
    .min_size = 1024,
    .add_vary = true,
    .enabled = true
  })
));
```

### ETag

```cpp
vix::middleware::performance::etag(...)
```

Options:

```cpp
struct EtagOptions
{
  bool weak;
  bool add_cache_control_if_missing;
  std::string cache_control;
  std::size_t min_body_size;
};
```

App usage:

```cpp
app.use("/api", vix::middleware::app::adapt_ctx(
  vix::middleware::performance::etag({
    .weak = true,
    .add_cache_control_if_missing = false,
    .min_body_size = 1
  })
));
```

### Static response compression hook

Static files are served by Core through:

```cpp
app.static_dir(...);
```

The middleware module can only enhance static responses through a hook:

```cpp
vix::App::set_static_response_hook(
  vix::middleware::performance::compressed_static_response_hook()
);
```

With options:

```cpp
vix::middleware::performance::CompressionOptions options{
  .min_size = 1024,
  .add_vary = true,
  .enabled = true
};

vix::App::set_static_response_hook(
  vix::middleware::performance::compressed_static_response_hook(options)
);
```

## Observability

Namespace:

```cpp
namespace vix::middleware::observability
```

### Tracing

```cpp
vix::middleware::observability::tracing_hooks(...)
vix::middleware::observability::tracing_mw(...)
```

State type:

```cpp
vix::middleware::observability::TraceContext
```

Options:

```cpp
struct TracingOptions
{
  std::string trace_header;
  std::string span_header;
  std::string parent_span_header;

  bool accept_incoming_trace;
  bool accept_incoming_span;
  bool emit_response_headers;
  bool include_parent_in_response;

  std::function<void(
    vix::middleware::Context &,
    TraceContext &
  )> enrich;
};
```

App usage:

```cpp
app.use("/api", vix::middleware::app::adapt_ctx(
  vix::middleware::observability::tracing_mw()
));
```

Read state:

```cpp
auto *trace =
  req.try_state<vix::middleware::observability::TraceContext>();
```

### Debug trace

```cpp
vix::middleware::observability::debug_trace_hooks(...)
vix::middleware::observability::debug_trace_mw(...)
```

Sink interface:

```cpp
class IDebugTraceSink
{
public:
  virtual ~IDebugTraceSink() = default;
  virtual void log(std::string_view line) = 0;
};
```

Built-in sink:

```cpp
vix::middleware::observability::InMemoryDebugTrace
```

Options:

```cpp
struct DebugTraceOptions
{
  bool include_method;
  bool include_path;
  bool include_status;
  bool include_duration_ms;
  bool include_trace_ids;
  std::string prefix;
};
```

App usage:

```cpp
auto sink =
  std::make_shared<vix::middleware::observability::InMemoryDebugTrace>();

app.use("/api", vix::middleware::app::adapt_ctx(
  vix::middleware::observability::debug_trace_mw(sink)
));
```

### Metrics

```cpp
vix::middleware::observability::metrics_hooks(...)
```

Built-in sink:

```cpp
vix::middleware::observability::InMemoryMetrics
```

Pipeline usage:

```cpp
auto metrics =
  std::make_shared<vix::middleware::observability::InMemoryMetrics>();

vix::middleware::HttpPipeline pipeline;

pipeline.set_hooks(
  vix::middleware::observability::metrics_hooks(metrics)
);
```

### Safe labels

```cpp
vix::middleware::observability::safe_method(req)
vix::middleware::observability::safe_path(req)
```

## Pipeline

Namespace:

```cpp
namespace vix::middleware
```

Type:

```cpp
vix::middleware::HttpPipeline
```

Main methods:

```cpp
pipeline.use(...)
pipeline.run(...)
pipeline.clear()
pipeline.size()
pipeline.services()
pipeline.hooks()
pipeline.set_hooks(...)
pipeline.enable_dev_observability(...)
```

Example:

```cpp
vix::middleware::HttpPipeline pipeline;

pipeline.use(vix::middleware::basics::request_id());
pipeline.use(vix::middleware::basics::timing());

pipeline.run(req, res, [](auto &, auto &out)
{
  out.status(200).text("OK");
});
```

Use `HttpPipeline` for tests and custom integrations.

Use `vix::App` for normal applications.

## Utilities

Namespace:

```cpp
namespace vix::middleware::utils
```

Common utilities:

```cpp
vix::middleware::utils::Clock
vix::middleware::utils::TokenBucket
vix::middleware::utils::JsonWriter
vix::middleware::utils::KeyBuilder

vix::middleware::utils::to_lower(...)
vix::middleware::utils::iequals(...)
vix::middleware::utils::trim_copy(...)
vix::middleware::utils::split_csv(...)
vix::middleware::utils::join_csv(...)
vix::middleware::utils::first_token(...)
vix::middleware::utils::normalize_keys_in_place(...)
```

Most users do not need these directly.

They are mainly used by middleware implementations.

## Common state types

| Middleware         | State type                                     |
| ------------------ | ---------------------------------------------- |
| `request_id()`     | `vix::middleware::basics::RequestId`           |
| `timing()`         | `vix::middleware::basics::Timing`              |
| `api_key()`        | `vix::middleware::auth::ApiKey`                |
| `jwt()`            | `vix::middleware::auth::JwtClaims`             |
| `rbac_context()`   | `vix::middleware::auth::Authz`                 |
| `session()`        | `vix::middleware::auth::Session`               |
| `json()`           | `vix::middleware::parsers::JsonBody`           |
| `form()`           | `vix::middleware::parsers::FormBody`           |
| `multipart()`      | `vix::middleware::parsers::MultipartInfo`      |
| `multipart_save()` | `vix::middleware::parsers::MultipartForm`      |
| `tracing_mw()`     | `vix::middleware::observability::TraceContext` |

Read required state:

```cpp
auto &body = req.state<vix::middleware::parsers::JsonBody>();
```

Read optional state:

```cpp
auto *trace = req.try_state<vix::middleware::observability::TraceContext>();
```

## Common status codes

| Status | Typical code             | Source               |
| ------ | ------------------------ | -------------------- |
| `400`  | `empty_body`             | JSON parser          |
| `400`  | `invalid_json`           | JSON parser          |
| `400`  | `missing_boundary`       | Multipart            |
| `401`  | `missing_api_key`        | API key              |
| `401`  | `missing_auth`           | JWT or RBAC          |
| `401`  | `missing_authz`          | RBAC                 |
| `403`  | `invalid_api_key`        | API key              |
| `403`  | `forbidden`              | RBAC                 |
| `403`  | `csrf_failed`            | CSRF                 |
| `403`  | `cors_forbidden`         | CORS                 |
| `403`  | `ip_denied`              | IP filter            |
| `403`  | `ip_not_allowed`         | IP filter            |
| `411`  | `length_required`        | Body limit           |
| `413`  | `payload_too_large`      | Body limit or parser |
| `415`  | `unsupported_media_type` | Parser               |
| `429`  | `rate_limited`           | Rate limit           |
| `500`  | `internal_server_error`  | Recovery             |

## Recommended backend stack

A practical starting stack:

```cpp
#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

int main()
{
  App app;

  app.use("/api", middleware::app::recovery_dev());
  app.use("/api", middleware::app::request_id_dev());
  app.use("/api", middleware::app::timing_dev());

  app.use("/api", middleware::app::security_headers_dev());
  app.use("/api", middleware::app::cors_dev({"https://example.com"}));
  app.use("/api", middleware::app::rate_limit_dev());

  app.use("/api", middleware::app::body_limit_write_dev(1024 * 1024));

  app.use("/api/users", middleware::app::json_strict_dev(4096));
  app.use("/api/admin", middleware::app::api_key_dev("secret"));

  app.get("/api/health", [](Request &, Response &res)
  {
    res.json({
      "ok", true
    });
  });

  app.post("/api/users", [](Request &req, Response &res)
  {
    auto &body = req.state<middleware::parsers::JsonBody>();

    res.status(201).json({
      "ok", true,
      "body", body.value.dump()
    });
  });

  app.get("/api/admin/status", [](Request &, Response &res)
  {
    res.json({
      "ok", true,
      "admin", true
    });
  });

  app.run(8080);
}
```

## Static files note

Static file serving is not a middleware feature.

Use Core:

```cpp
app.static_dir(...);
```

Use middleware only for optional static response enhancement:

```cpp
vix::App::set_static_response_hook(
  vix::middleware::performance::compressed_static_response_hook()
);
```

Remember:

```txt
Core serves static files.
Middleware enhances HTTP behavior.
```
