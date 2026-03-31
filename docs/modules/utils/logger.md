# Logger

A practical guide to `vix::utils::Logger`.

This logger is a thin wrapper over `spdlog` with:
- A global singleton instance
- Level control via `VIX_LOG_LEVEL`
- Optional structured logging via `VIX_LOG_FORMAT`
- Per-thread context (request id, module, custom fields)
- Optional console synchronization to avoid interleaved output

## Include

```cpp
#include <vix/utils/Logger.hpp>
```

Get the singleton:

```cpp
auto &log = vix::utils::Logger::getInstance();
```

## Default behavior

- Output: console only (no log file is created)
- Default level: `info` (can be overridden by `VIX_LOG_LEVEL`)
- Default pattern: timestamp + `[vix]` + level + message

## Logging levels

Available levels:

- `trace`
- `debug`
- `info`
- `warn`
- `error`
- `critical`
- `off`

Set via environment:

```bash
export VIX_LOG_LEVEL=debug
```

Or set programmatically:

```cpp
auto &log = vix::utils::Logger::getInstance();
log.setLevel(vix::utils::Logger::Level::Debug);
```

Check if a level is enabled:

```cpp
if (log.enabled(vix::utils::Logger::Level::Debug))
{
  log.debug("Will only run when debug is enabled");
}
```

## Basic logging

```cpp
auto &log = vix::utils::Logger::getInstance();

log.info("Server started on port {}", 8080);
log.warn("Cache miss for key={}", key);
log.error("DB error: {}", err);
```

## Module prefix

If you want a visible module prefix like `[auth] ...`:

```cpp
auto &log = vix::utils::Logger::getInstance();

log.logModule("auth", vix::utils::Logger::Level::Info,
              "User {} logged in", user);
```

## Pattern

Override the console pattern:

```cpp
auto &log = vix::utils::Logger::getInstance();
log.setPattern("%T [%^%l%$] %v");
```

Common patterns:

- `%T` = `HH:MM:SS`
- `%l` = level name
- `%^` and `%$` = enable and disable level color
- `%v` = message

## Async mode

Enable async logging:

```cpp
auto &log = vix::utils::Logger::getInstance();
log.setAsync(true);
```

Disable async logging:

```cpp
log.setAsync(false);
```

Notes:
- Async mode uses an internal spdlog thread pool.
- If you use async, keep shutdown simple. Let the process exit normally.

## Per-thread context

Logger context is thread-local. It is designed for request scoped data.

### Set context

```cpp
auto &log = vix::utils::Logger::getInstance();

vix::utils::Logger::Context ctx;
ctx.request_id = "r-123";
ctx.module = "http";
ctx.fields["ip"] = "127.0.0.1";
ctx.fields["user_id"] = "42";

log.setContext(ctx);

log.info("Request started");
```

### Clear context

```cpp
auto &log = vix::utils::Logger::getInstance();
log.clearContext();
```

### Read context

```cpp
auto &log = vix::utils::Logger::getInstance();
auto ctx = log.getContext();
```

Tips:
- Set context at the beginning of a request handler.
- Clear it when leaving the handler if the thread is reused.

## Structured logging

You can log key/value pairs with `logf()`.

### KV format (default)

```cpp
auto &log = vix::utils::Logger::getInstance();

log.logf(vix::utils::Logger::Level::Info,
         "Login ok",
         "user", "alice",
         "latency_ms", 12,
         "status", 200);
```

Output stays on one line and appends fields like:
- your kvpairs
- `rid` and `mod` from the context
- all custom context fields

### JSON formats

Select output format via environment:

```bash
export VIX_LOG_FORMAT=json
# or
export VIX_LOG_FORMAT=json_pretty
```

Or set programmatically:

```cpp
auto &log = vix::utils::Logger::getInstance();
log.setFormat(vix::utils::Logger::Format::JSON);
```

Notes:
- JSON modes force the sink pattern to `%v` so the output is raw JSON.
- Pretty JSON may include ANSI colors unless disabled (see below).

## Colors and terminals

Pretty JSON colors are controlled by:
- `NO_COLOR` set => colors disabled
- `VIX_COLOR=never|0|false` => colors disabled
- `VIX_COLOR=always|1|true` => colors forced on
- Otherwise: best-effort detection (TTY on Unix, enabled on Windows)

Example:

```bash
export NO_COLOR=1
```

## Console synchronization

If you print banners or write to stdout from multiple threads, you can ask the logger
to synchronize console writes.

Enable:

```bash
export VIX_CONSOLE_SYNC=1
```

Disable:

```bash
unset VIX_CONSOLE_SYNC
# or
export VIX_CONSOLE_SYNC=0
```

What it does:
- Waits for banners to finish (via `ConsoleMutex`)
- Locks a shared console mutex before emitting a log line

## Windows macro safety

On Windows, some headers define macros like `ERROR`, `DEBUG`, `INFO`, `min`, `max`.
This can break enum values or identifiers. The header temporarily undefines them
and restores them after includes.

You do not need to do anything. It is handled by `Logger.hpp`.

## Throw helpers

You can log and throw in one call:

```cpp
auto &log = vix::utils::Logger::getInstance();
log.throwError("Invalid config: {}", reason);
```

This logs at `error` level and throws `std::runtime_error`.

## Recommended baseline

For typical apps:

```bash
export VIX_LOG_LEVEL=info
export VIX_LOG_FORMAT=kv
```

For debugging locally:

```bash
export VIX_LOG_LEVEL=debug
export VIX_LOG_FORMAT=json_pretty
```

