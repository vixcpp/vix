# Log

The `log` module provides structured application logging for Vix.

It is the production-oriented logging API.

```cpp
#include <vix/log.hpp>

int main()
{
  vix::log::info("Server started on port {}", 8080);
  vix::log::warn("Cache miss for key {}", "users");
  vix::log::error("Database connection failed");

  return 0;
}
```

## Header

```cpp
#include <vix/log.hpp>
```

This header includes the public logging API:

```cpp
#include <vix/log/GlobalLog.hpp>
#include <vix/log/LogAdapter.hpp>
#include <vix/log/LogConfig.hpp>
#include <vix/log/LogContext.hpp>
#include <vix/log/LogFormat.hpp>
#include <vix/log/LogLevel.hpp>
```

## What the log module does

The `vix::log` module gives you a stable logging API for Vix applications.

It supports:

- severity levels,
- formatted messages,
- key/value structured logs,
- JSON logs,
- pretty JSON logs,
- environment-based configuration,
- per-thread context,
- optional async logging,
- global logger configuration.

The public API lives in:

```cpp
namespace vix::log
```

Example:

```cpp
vix::log::info("User {} logged in", user_id);
```

## Log vs Console vs Print

Vix has several output APIs.

| API | Purpose |
| --- | --- |
| `vix::console` | JavaScript-like development console for people coming from JS. |
| `vix::print` | Simple readable C++ output. |
| `vix::inspect` | Deep debugging and diagnostics. |
| `vix::log` | Structured application logging. |

Use `vix::console` for familiar local development messages.

Use `vix::log` when you want real application logs with levels, formats, context, and structured fields.

## Quick example

```cpp
#include <vix/log.hpp>

int main()
{
  vix::log::info("Application started");
  vix::log::warn("Using default config file: {}", ".env");
  vix::log::error("Failed to open {}", "database.db");

  return 0;
}
```

Output shape:

```txt
09:31:25 [vix] [info] Application started
09:31:25 [vix] [warning] Using default config file: .env
09:31:25 [vix] [error] Failed to open database.db
```

The exact visual style depends on the active log format and terminal color support.

## Formatted messages

The log module uses fmt-style formatting.

```cpp
vix::log::info("Server started on port {}", 8080);
vix::log::debug("User id: {}", 42);
vix::log::error("File not found: {}", path);
```

Use `{}` placeholders for values.

```cpp
std::string user = "Ada";
int id = 1;

vix::log::info("User {} has id {}", user, id);
```

## Public API overview

| API | Purpose |
| --- | --- |
| `vix::log::trace(...)` | Log a trace message. |
| `vix::log::debug(...)` | Log a debug message. |
| `vix::log::info(...)` | Log an info message. |
| `vix::log::warn(...)` | Log a warning message. |
| `vix::log::error(...)` | Log an error message. |
| `vix::log::critical(...)` | Log a critical message. |
| `vix::log::log(level, ...)` | Log at a dynamic level. |
| `vix::log::logf(level, message, kv...)` | Log a structured key/value message. |
| `vix::log::configure(config)` | Configure the global logger. |
| `vix::log::set_level(level)` | Set the active log level. |
| `vix::log::level()` | Get the active log level. |
| `vix::log::enabled(level)` | Check whether a level is enabled. |
| `vix::log::set_format(format)` | Set the output format. |
| `vix::log::set_async(bool)` | Enable or disable async logging. |
| `vix::log::set_context(ctx)` | Set per-thread log context. |
| `vix::log::clear_context()` | Clear per-thread log context. |
| `vix::log::context()` | Get current per-thread context. |
| `vix::log::set_level_from_env()` | Read level from environment. |
| `vix::log::set_format_from_env()` | Read format from environment. |
| `vix::log::parse_level(value)` | Parse a log level from string. |
| `vix::log::parse_format(value)` | Parse a log format from string. |
| `vix::log::global()` | Access the global public log adapter. |

## Log levels

```cpp
enum class LogLevel
{
  Trace,
  Debug,
  Info,
  Warn,
  Error,
  Critical,
  Off
};
```

| Level | Use for |
| --- | --- |
| `Trace` | Very detailed internal events. |
| `Debug` | Debug information useful during development. |
| `Info` | Normal application lifecycle events. |
| `Warn` | Recoverable problems or unexpected conditions. |
| `Error` | Operation failed, but the process can continue. |
| `Critical` | Serious failure requiring immediate attention. |
| `Off` | Disable logging. |

## Default level

The default level is usually:

```txt
Info
```

You can override it with:

```bash
VIX_LOG_LEVEL=debug ./app
```

or in code:

```cpp
vix::log::set_level(vix::log::LogLevel::Debug);
```

## Trace

```cpp
vix::log::trace("Parser entered state {}", state_id);
```

Trace is the most verbose level.

Use it for deep internals.

## Debug

```cpp
vix::log::debug("Cache size: {}", cache_size);
```

Debug is useful during development and troubleshooting.

## Info

```cpp
vix::log::info("Server started on port {}", 8080);
```

Info is for normal application events.

## Warn

```cpp
vix::log::warn("Config file {} not found, using defaults", ".env");
```

Warn is for recoverable problems.

## Error

```cpp
vix::log::error("Failed to connect to database: {}", reason);
```

Error is for failed operations.

## Critical

```cpp
vix::log::critical("Fatal startup error: {}", reason);
```

Critical is for serious failures.

## Dynamic level logging

Use `log` when the level is known at runtime.

```cpp
vix::log::LogLevel level = vix::log::LogLevel::Info;

vix::log::log(level, "Dynamic message with id {}", 42);
```

## Check if a level is enabled

Use `enabled` to avoid expensive work.

```cpp
if (vix::log::enabled(vix::log::LogLevel::Debug))
{
  std::string debug_info = build_expensive_debug_string();
  vix::log::debug("debug_info={}", debug_info);
}
```

This is useful when preparing the log message is expensive.

## Set the log level

```cpp
vix::log::set_level(vix::log::LogLevel::Debug);
```

Example:

```cpp
#include <vix/log.hpp>

int main()
{
  vix::log::debug("hidden by default");

  vix::log::set_level(vix::log::LogLevel::Debug);

  vix::log::debug("visible now");

  return 0;
}
```

## Get the active log level

```cpp
auto level = vix::log::level();
```

Example:

```cpp
if (vix::log::level() == vix::log::LogLevel::Debug)
{
  vix::log::info("Debug logging is enabled");
}
```

## Disable logging

```cpp
vix::log::set_level(vix::log::LogLevel::Off);
```

or:

```bash
VIX_LOG_LEVEL=off ./app
```

## Parse log levels

```cpp
auto level = vix::log::parse_level("debug");
```

Supported values include:

- `trace`
- `debug`
- `info`
- `warn`
- `warning`
- `error`
- `critical`
- `fatal`
- `off`
- `never`
- `none`
- `silent`
- `0`

Unknown values fall back to the backend default parsing behavior.

## Environment log level

Set the level using:

```bash
VIX_LOG_LEVEL=trace ./app
VIX_LOG_LEVEL=debug ./app
VIX_LOG_LEVEL=info ./app
VIX_LOG_LEVEL=warn ./app
VIX_LOG_LEVEL=error ./app
VIX_LOG_LEVEL=critical ./app
VIX_LOG_LEVEL=off ./app
```

You can also read from a custom environment variable:

```cpp
vix::log::set_level_from_env("MY_APP_LOG_LEVEL");
```

Example:

```bash
MY_APP_LOG_LEVEL=debug ./app
```

```cpp
vix::log::set_level_from_env("MY_APP_LOG_LEVEL");
```

## Log formats

```cpp
enum class LogFormat
{
  KV,
  JSON,
  JSON_PRETTY
};
```

| Format | Meaning |
| --- | --- |
| `KV` | Human-readable key/value text. |
| `JSON` | Single-line JSON. |
| `JSON_PRETTY` | Pretty JSON with indentation. |

Default format:

```txt
KV
```

## Set the log format

```cpp
vix::log::set_format(vix::log::LogFormat::KV);
vix::log::set_format(vix::log::LogFormat::JSON);
vix::log::set_format(vix::log::LogFormat::JSON_PRETTY);
```

## Parse log formats

```cpp
auto format = vix::log::parse_format("json");
```

Supported values:

- `kv`
- `json`
- `json-pretty`
- `pretty-json`
- `json_pretty`

Unknown values fall back to `KV`.

## Environment log format

Use:

```bash
VIX_LOG_FORMAT=kv ./app
VIX_LOG_FORMAT=json ./app
VIX_LOG_FORMAT=json_pretty ./app
```

You can also read from a custom environment variable:

```cpp
vix::log::set_format_from_env("MY_APP_LOG_FORMAT");
```

## Configure the logger

Use `LogConfig` when you want to configure level, format, and async mode together.

```cpp
vix::log::LogConfig config;
config.level = vix::log::LogLevel::Debug;
config.format = vix::log::LogFormat::JSON;
config.async = false;

vix::log::configure(config);
```

## LogConfig

```cpp
struct LogConfig
{
  LogLevel level{LogLevel::Info};
  LogFormat format{LogFormat::KV};
  bool async{false};
};
```

| Field | Default | Purpose |
| --- | --- | --- |
| `level` | `Info` | Minimum active log level. |
| `format` | `KV` | Output format. |
| `async` | `false` | Whether async logging mode is enabled. |

## Async logging

Enable async logging:

```cpp
vix::log::set_async(true);
```

Disable async logging:

```cpp
vix::log::set_async(false);
```

You can also configure it with `LogConfig`:

```cpp
vix::log::LogConfig config;
config.async = true;

vix::log::configure(config);
```

Use async mode when you want logging to have less impact on the request path.

## Key/value logs

Use `logf` for structured key/value logs.

```cpp
vix::log::logf(
  vix::log::LogLevel::Info,
  "request completed",
  "method", "GET",
  "path", "/users",
  "status", 200,
  "duration_ms", 12
);
```

In KV format, output shape:

```txt
09:31:25 [vix] [info] request completed method=GET path=/users status=200 duration_ms=12
```

In JSON format, output shape:

```json
{"level":"info","msg":"request completed","method":"GET","path":"/users","status":200,"duration_ms":12}
```

## Key/value argument rules

`logf` expects alternating key/value pairs:

```cpp
vix::log::logf(
  vix::log::LogLevel::Info,
  "user login",
  "user_id", 42,
  "ip", "127.0.0.1"
);
```

The pattern is:

```txt
"key1", value1, "key2", value2, ...
```

Keys should be string literals or `const char *`.

Good:

```cpp
vix::log::logf(
  vix::log::LogLevel::Info,
  "payment processed",
  "amount", 99.5,
  "currency", "USD"
);
```

Avoid odd key/value counts.

## Log context

`LogContext` carries per-thread metadata.

```cpp
struct LogContext
{
  std::string request_id;
  std::string module;
  std::unordered_map<std::string, std::string> fields;
};
```

Use it for request IDs, module names, and shared metadata.

## Set context

```cpp
vix::log::LogContext ctx;
ctx.request_id = "req-123";
ctx.module = "auth";
ctx.fields["ip"] = "127.0.0.1";
ctx.fields["user_agent"] = "curl";

vix::log::set_context(ctx);
```

Now logs include the context.

```cpp
vix::log::info("User logged in");
```

In KV format, output shape:

```txt
09:31:25 [vix] [info] User logged in rid=req-123 mod=auth ip=127.0.0.1 user_agent=curl
```

## Clear context

```cpp
vix::log::clear_context();
```

Use this after a request finishes.

## Get context

```cpp
vix::log::LogContext ctx = vix::log::context();
```

## Per-thread context

The logging context is per-thread.

This means each request thread can have its own context:

```cpp
void handle_request(const std::string &request_id)
{
  vix::log::LogContext ctx;
  ctx.request_id = request_id;
  ctx.module = "http";
  vix::log::set_context(ctx);

  vix::log::info("Handling request");

  vix::log::clear_context();
}
```

## Request logging example

```cpp
#include <vix.hpp>
#include <vix/log.hpp>

using namespace vix;

int main()
{
  log::set_level_from_env();
  log::set_format_from_env();

  App app;

  app.get("/users/{id}", [](Request &req, Response &res) {
    log::LogContext ctx;
    ctx.request_id = req.header("X-Request-Id", "local-request");
    ctx.module = "users";
    ctx.fields["path"] = "/users/{id}";
    log::set_context(ctx);

    const std::string id = req.param("id");

    log::info("Loading user {}", id);

    res.json({
      "ok", true,
      "id", id
    });

    log::clear_context();
  });

  app.run(8080);

  return 0;
}
```

## JSON logging

Set JSON format:

```cpp
vix::log::set_format(vix::log::LogFormat::JSON);
```

Example:

```cpp
vix::log::logf(
  vix::log::LogLevel::Info,
  "request completed",
  "method", "GET",
  "path", "/health",
  "status", 200,
  "duration_ms", 3
);
```

Output shape:

```json
{"level":"info","msg":"request completed","method":"GET","path":"/health","status":200,"duration_ms":3}
```

Use JSON when logs are consumed by tools.

## Pretty JSON logging

Set pretty JSON:

```cpp
vix::log::set_format(vix::log::LogFormat::JSON_PRETTY);
```

or:

```bash
VIX_LOG_FORMAT=json_pretty ./app
```

Example output shape:

```json
{
  "level": "info",
  "msg": "request completed",
  "method": "GET",
  "path": "/health",
  "status": 200,
  "duration_ms": 3
}
```

Pretty JSON is useful for local debugging and demos.

For production log ingestion, prefer single-line JSON.

## KV logging

Set KV format:

```cpp
vix::log::set_format(vix::log::LogFormat::KV);
```

Example:

```cpp
vix::log::logf(
  vix::log::LogLevel::Warn,
  "slow request",
  "method", "GET",
  "path", "/reports",
  "duration_ms", 1200
);
```

Output shape:

```txt
09:31:25 [vix] [warning] slow request method=GET path=/reports duration_ms=1200
```

KV is readable in the terminal and still easy to grep.

## Colors

The underlying logger supports colored console output.

For JSON pretty output, colors follow:

```bash
VIX_COLOR=always
VIX_COLOR=never
NO_COLOR=1
```

Examples:

```bash
VIX_COLOR=never ./app
NO_COLOR=1 ./app
VIX_COLOR=always ./app
```

`NO_COLOR` disables colors.

## Default logger backend

The public `vix::log` module is a façade.

Internally:

```txt
vix::log::* → LogAdapter → vix::utils::Logger → spdlog
```

You should use the public API:

```cpp
vix::log::info("hello");
```

instead of directly depending on:

```cpp
vix::utils::Logger
```

Use the public module to keep application code stable.

## Access the global adapter

```cpp
vix::log::LogAdapter &adapter = vix::log::global();
```

Usually, you do not need this.

Use top-level helpers instead:

```cpp
vix::log::info("hello");
vix::log::set_level(vix::log::LogLevel::Debug);
```

## LogAdapter

`LogAdapter` bridges the public log module to the internal logger backend.

Main methods:

```cpp
auto &log = vix::log::global();

log.set_level(vix::log::LogLevel::Info);
log.set_format(vix::log::LogFormat::KV);
log.info("Server started");
```

For normal application code, prefer the free functions:

```cpp
vix::log::info("Server started");
```

## Complete example

```cpp
#include <vix/log.hpp>

int main()
{
  vix::log::LogConfig config;
  config.level = vix::log::LogLevel::Debug;
  config.format = vix::log::LogFormat::KV;
  config.async = false;

  vix::log::configure(config);

  vix::log::LogContext ctx;
  ctx.request_id = "req-001";
  ctx.module = "example";
  ctx.fields["env"] = "dev";

  vix::log::set_context(ctx);

  vix::log::trace("trace message");
  vix::log::debug("debug message");
  vix::log::info("info message");
  vix::log::warn("warn message");
  vix::log::error("error message");
  vix::log::critical("critical message");

  vix::log::logf(
    vix::log::LogLevel::Info,
    "request completed",
    "method", "GET",
    "path", "/",
    "status", 200,
    "duration_ms", 5
  );

  vix::log::clear_context();

  return 0;
}
```

## Complete HTTP example

```cpp
#include <vix.hpp>
#include <vix/log.hpp>

using namespace vix;

static void setup_logging()
{
  log::set_level_from_env();
  log::set_format_from_env();
}

int main()
{
  setup_logging();

  App app;

  app.get("/", [](Request &, Response &res) {
    log::info("GET /");

    res.json({
      "message", "Hello from Vix",
      "module", "log"
    });
  });

  app.get("/health", [](Request &, Response &res) {
    log::logf(
      log::LogLevel::Info,
      "health check",
      "method", "GET",
      "path", "/health",
      "status", 200
    );

    res.json({
      "ok", true
    });
  });

  app.run(8080);

  return 0;
}
```

Run:

```bash
VIX_LOG_LEVEL=debug VIX_LOG_FORMAT=kv vix run main.cpp
```

Test:

```bash
curl -i http://127.0.0.1:8080/
curl -i http://127.0.0.1:8080/health
```

## Environment examples

### Debug logs

```bash
VIX_LOG_LEVEL=debug vix run main.cpp
```

### Warnings and errors only

```bash
VIX_LOG_LEVEL=warn vix run main.cpp
```

### JSON logs

```bash
VIX_LOG_FORMAT=json vix run main.cpp
```

### Pretty JSON logs

```bash
VIX_LOG_FORMAT=json_pretty vix run main.cpp
```

### JSON logs without colors

```bash
VIX_LOG_FORMAT=json_pretty VIX_COLOR=never vix run main.cpp
```

### Disable logs

```bash
VIX_LOG_LEVEL=off vix run main.cpp
```

## Recommended production setup

For production services, prefer:

```dotenv
VIX_LOG_LEVEL=info
VIX_LOG_FORMAT=json
VIX_COLOR=never
```

Why:

- `info` keeps useful lifecycle and request logs,
- `json` is machine-readable,
- `VIX_COLOR=never` avoids ANSI escape codes in log files,
- single-line logs are easier for log collectors.

Example systemd environment:

```ini
Environment=VIX_LOG_LEVEL=info
Environment=VIX_LOG_FORMAT=json
Environment=VIX_COLOR=never
```

## Common mistakes

### Using console instead of log for application logs

This is for JavaScript-like development output:

```cpp
vix::console.log("user created");
```

This is for structured application logging:

```cpp
vix::log::info("user created");
```

### Forgetting the header

```cpp
// Wrong
vix::log::info("hello");
```

Fix:

```cpp
#include <vix/log.hpp>
```

### Expecting debug logs to appear by default

```cpp
vix::log::debug("debug value {}", 42);
```

Debug logs are hidden unless the level is Debug or lower.

Enable debug:

```cpp
vix::log::set_level(vix::log::LogLevel::Debug);
```

or:

```bash
VIX_LOG_LEVEL=debug ./app
```

### Passing wrong key/value pairs to `logf`

Wrong:

```cpp
vix::log::logf(
  vix::log::LogLevel::Info,
  "request completed",
  "method"
);
```

Correct:

```cpp
vix::log::logf(
  vix::log::LogLevel::Info,
  "request completed",
  "method", "GET"
);
```

### Forgetting to clear context

If a worker thread handles multiple requests, clear context after each request.

```cpp
vix::log::clear_context();
```

### Using pretty JSON for production ingestion

Pretty JSON is nice for humans.

For production log collectors, prefer:

```bash
VIX_LOG_FORMAT=json
```

## Best practices

Use `info` for normal lifecycle events:

```cpp
vix::log::info("server started on port {}", 8080);
```

Use `debug` for development details:

```cpp
vix::log::debug("cache size {}", cache_size);
```

Use `warn` for recoverable problems:

```cpp
vix::log::warn("using fallback config");
```

Use `error` for failed operations:

```cpp
vix::log::error("database query failed: {}", reason);
```

Use `critical` for fatal problems:

```cpp
vix::log::critical("cannot start service: {}", reason);
```

Use `logf` for request logs and structured events:

```cpp
vix::log::logf(
  vix::log::LogLevel::Info,
  "request completed",
  "method", "GET",
  "path", "/users",
  "status", 200,
  "duration_ms", 12
);
```

Use context for request-scoped metadata:

```cpp
vix::log::LogContext ctx;
ctx.request_id = request_id;
ctx.module = "http";
vix::log::set_context(ctx);
```

## API reference

### `global`

```cpp
LogAdapter &global() noexcept;
```

Returns the global public log adapter.

```cpp
auto &logger = vix::log::global();
logger.info("hello");
```

### `configure`

```cpp
void configure(const LogConfig &config);
```

Configures the global logger.

```cpp
vix::log::LogConfig config;
config.level = vix::log::LogLevel::Debug;
config.format = vix::log::LogFormat::JSON;

vix::log::configure(config);
```

### `set_level`

```cpp
void set_level(LogLevel level);
```

Sets the active log level.

```cpp
vix::log::set_level(vix::log::LogLevel::Warn);
```

### `level`

```cpp
LogLevel level() noexcept;
```

Returns the current log level.

```cpp
auto current = vix::log::level();
```

### `enabled`

```cpp
bool enabled(LogLevel level) noexcept;
```

Checks whether a level is enabled.

```cpp
if (vix::log::enabled(vix::log::LogLevel::Debug))
{
  vix::log::debug("debug enabled");
}
```

### `set_format`

```cpp
void set_format(LogFormat format);
```

Sets the output format.

```cpp
vix::log::set_format(vix::log::LogFormat::JSON);
```

### `set_async`

```cpp
void set_async(bool enable);
```

Enables or disables async logging.

```cpp
vix::log::set_async(true);
```

### `set_context`

```cpp
void set_context(LogContext ctx);
```

Sets the current per-thread log context.

```cpp
vix::log::LogContext ctx;
ctx.request_id = "req-1";
ctx.module = "auth";

vix::log::set_context(ctx);
```

### `clear_context`

```cpp
void clear_context();
```

Clears the current per-thread context.

```cpp
vix::log::clear_context();
```

### `context`

```cpp
LogContext context();
```

Returns a copy of the current per-thread context.

```cpp
auto ctx = vix::log::context();
```

### `set_level_from_env`

```cpp
void set_level_from_env(std::string_view env_name = "VIX_LOG_LEVEL");
```

Reads the log level from an environment variable.

```cpp
vix::log::set_level_from_env();
```

### `set_format_from_env`

```cpp
void set_format_from_env(std::string_view env_name = "VIX_LOG_FORMAT");
```

Reads the log format from an environment variable.

```cpp
vix::log::set_format_from_env();
```

### `parse_level`

```cpp
LogLevel parse_level(std::string_view value);
```

Parses a log level string.

```cpp
auto level = vix::log::parse_level("debug");
```

### `parse_format`

```cpp
LogFormat parse_format(std::string_view value);
```

Parses a log format string.

```cpp
auto format = vix::log::parse_format("json");
```

### `trace`

```cpp
template <typename... Args>
void trace(fmt::format_string<Args...> fmtstr, Args &&...args);
```

Logs a trace message.

```cpp
vix::log::trace("state={}", state);
```

### `debug`

```cpp
template <typename... Args>
void debug(fmt::format_string<Args...> fmtstr, Args &&...args);
```

Logs a debug message.

```cpp
vix::log::debug("cache size={}", size);
```

### `info`

```cpp
template <typename... Args>
void info(fmt::format_string<Args...> fmtstr, Args &&...args);
```

Logs an info message.

```cpp
vix::log::info("server started");
```

### `warn`

```cpp
template <typename... Args>
void warn(fmt::format_string<Args...> fmtstr, Args &&...args);
```

Logs a warning message.

```cpp
vix::log::warn("config missing: {}", path);
```

### `error`

```cpp
template <typename... Args>
void error(fmt::format_string<Args...> fmtstr, Args &&...args);
```

Logs an error message.

```cpp
vix::log::error("failed: {}", reason);
```

### `critical`

```cpp
template <typename... Args>
void critical(fmt::format_string<Args...> fmtstr, Args &&...args);
```

Logs a critical message.

```cpp
vix::log::critical("fatal error: {}", reason);
```

### `log`

```cpp
template <typename... Args>
void log(LogLevel level_value, fmt::format_string<Args...> fmtstr, Args &&...args);
```

Logs a formatted message at a dynamic level.

```cpp
vix::log::log(vix::log::LogLevel::Info, "hello {}", "Vix");
```

### `logf`

```cpp
template <typename... Args>
void logf(LogLevel level_value, const std::string &message, Args &&...kvpairs);
```

Logs a message with structured key/value fields.

```cpp
vix::log::logf(
  vix::log::LogLevel::Info,
  "request completed",
  "method", "GET",
  "path", "/",
  "status", 200
);
```

## Summary

`vix::log` is the structured logging module for Vix applications.

Use it for:

- application logs,
- request logs,
- production services,
- JSON logs,
- key/value logs,
- request-scoped context,
- async logging,
- severity-based filtering.

Core API:

```cpp
vix::log::trace("...");
vix::log::debug("...");
vix::log::info("...");
vix::log::warn("...");
vix::log::error("...");
vix::log::critical("...");
```

```cpp
vix::log::logf(
  vix::log::LogLevel::Info,
  "request completed",
  "method", "GET",
  "status", 200
);
```

Use `vix::console` for JavaScript-like development output.

Use `vix::print` for simple readable output.

Use `vix::inspect` for deep debugging.

Use `vix::log` for structured application logging.
