# Console

The `console` API provides a small JavaScript-like console for C++ developers.

It is designed for people who are used to writing:

```js
console.log("hello");
console.warn("careful");
console.error("failed");
```

In Vix, you can write:

```cpp
vix::console.log("hello");
vix::console.warn("careful");
vix::console.error("failed");
```

This API is for development comfort, demos, examples, quick debugging, and making C++ feel more familiar for developers coming from JavaScript.

It is not the production logging API.

## Header

```cpp
#include <vix/console.hpp>
```

Some projects may expose it through:

```cpp
#include <vix.hpp>
```

For direct usage, prefer:

```cpp
#include <vix/console.hpp>
```

## Basic usage

```cpp
#include <vix/console.hpp>

int main()
{
  vix::console.log("Hello from Vix");
  vix::console.info("Server started on port", 8080);
  vix::console.warn("Cache is empty");
  vix::console.error("Failed to open file");
  vix::console.debug("Debug value", 42);

  return 0;
}
```

Output shape:

```txt
09:31:25 [info] Hello from Vix
09:31:25 [info] Server started on port 8080
09:31:25 [warn] Cache is empty
09:31:25 [error] Failed to open file
```

`debug` is hidden by default because the default level is `Info`.

## Why this API exists

`vix::console` exists to make C++ friendlier for developers who are used to JavaScript.

Instead of writing:

```cpp
std::cout << "user " << id << " connected\n";
std::cerr << "error: failed\n";
```

you can write:

```cpp
vix::console.log("user", id, "connected");
vix::console.error("failed");
```

The goal is not to replace professional structured logging.

The goal is to provide a simple and familiar developer console.

## Public API overview

| API                             | Purpose                                     | Stream |
| ------------------------------- | ------------------------------------------- | ------ |
| `vix::console.log(...)`         | General development output.                 | stdout |
| `vix::console.info(...)`        | Informational development output.           | stdout |
| `vix::console.warn(...)`        | Warning output.                             | stderr |
| `vix::console.error(...)`       | Error output.                               | stderr |
| `vix::console.debug(...)`       | Debug output, hidden unless level is Debug. | stdout |
| `vix::console.set_level(level)` | Change the minimum visible level.           | -      |
| `vix::console.level()`          | Read the current level.                     | -      |

## Global object

The console API is exposed through a global object:

```cpp
vix::console
```

Example:

```cpp
vix::console.log("hello");
```

Internally, `vix::console` is an inline global instance of `vix::Console`.

## Levels

Console messages have levels.

```cpp
enum class Level : uint8_t
{
  Debug = 0,
  Info = 1,
  Warn = 2,
  Error = 3,
  Off = 4
};
```

The current level controls what is printed.

| Current level | Printed                  |
| ------------- | ------------------------ |
| `Debug`       | debug, info, warn, error |
| `Info`        | info, warn, error        |
| `Warn`        | warn, error              |
| `Error`       | error                    |
| `Off`         | nothing                  |

Default level:

```txt
Info
```

## `log`

`log` prints a normal development message.

```cpp
vix::console.log("Hello", "Vix");
```

Output shape:

```txt
09:31:25 [info] Hello Vix
```

`log` uses the `Info` level.

It writes to stdout.

## `info`

`info` is equivalent to an informational message.

```cpp
vix::console.info("Server ready on port", 8080);
```

Output shape:

```txt
09:31:25 [info] Server ready on port 8080
```

`info` writes to stdout.

## `warn`

`warn` prints a warning message.

```cpp
vix::console.warn("Missing optional config:", ".env.local");
```

Output shape:

```txt
09:31:25 [warn] Missing optional config: .env.local
```

`warn` writes to stderr.

Warnings are not rate-limited.

## `error`

`error` prints an error message.

```cpp
vix::console.error("Failed to connect to database");
```

Output shape:

```txt
09:31:25 [error] Failed to connect to database
```

`error` writes to stderr.

Errors are not rate-limited.

## `debug`

`debug` prints a debug message only when the console level allows it.

```cpp
vix::console.debug("user_id", 42);
```

By default, this is hidden because the default level is `Info`.

Enable debug:

```cpp
vix::console.set_level(vix::Console::Level::Debug);
vix::console.debug("user_id", 42);
```

Output shape:

```txt
09:31:25 [debug] user_id 42
```

## Set the level in code

```cpp
vix::console.set_level(vix::Console::Level::Debug);
```

Example:

```cpp
#include <vix/console.hpp>

int main()
{
  vix::console.debug("hidden by default");

  vix::console.set_level(vix::Console::Level::Debug);
  vix::console.debug("visible now");

  return 0;
}
```

## Read the current level

```cpp
auto level = vix::console.level();
```

Example:

```cpp
if (vix::console.level() == vix::Console::Level::Debug)
{
  vix::console.log("debug mode is enabled");
}
```

## Set the level with environment variables

`vix::console` reads environment variables during construction.

Use:

```bash
VIX_CONSOLE_LEVEL=debug ./app
```

Supported values:

- `debug`
- `trace`
- `info`
- `log`
- `warn`
- `warning`
- `error`
- `err`
- `off`
- `none`
- `silent`
- `never`
- `0`

Examples:

```bash
VIX_CONSOLE_LEVEL=debug ./app
VIX_CONSOLE_LEVEL=warn ./app
VIX_CONSOLE_LEVEL=error ./app
VIX_CONSOLE_LEVEL=off ./app
```

## Level aliases

| Value     | Result |
| --------- | ------ |
| `debug`   | Debug  |
| `trace`   | Debug  |
| `info`    | Info   |
| `log`     | Info   |
| `warn`    | Warn   |
| `warning` | Warn   |
| `error`   | Error  |
| `err`     | Error  |
| `off`     | Off    |
| `none`    | Off    |
| `silent`  | Off    |
| `never`   | Off    |
| `0`       | Off    |

If the value is unknown, Vix falls back to `Info`.

## Colors

Console supports ANSI colors.

Color behavior is controlled by:

```bash
VIX_COLOR=auto
VIX_COLOR=always
VIX_COLOR=never
```

Default:

```txt
auto
```

In auto mode, colors are enabled only when the output is a terminal.

## Disable colors

Use:

```bash
NO_COLOR=1 ./app
```

or:

```bash
VIX_COLOR=never ./app
```

`NO_COLOR` has priority.

## Force colors

```bash
VIX_COLOR=always ./app
```

This can be useful when your terminal supports ANSI but auto-detection is not enough.

## Output format

A console line has this shape:

```txt
HH:MM:SS [level] message
```

Example:

```txt
09:31:25 [info] Server ready
09:31:26 [warn] Missing optional file
09:31:27 [error] Failed to connect
```

The timestamp uses local time.

## Streams

| Method  | Stream |
| ------- | ------ |
| `log`   | stdout |
| `info`  | stdout |
| `debug` | stdout |
| `warn`  | stderr |
| `error` | stderr |

This mirrors the usual developer expectation:

```txt
normal messages → stdout
warnings/errors → stderr
```

## Arguments

Console methods accept multiple arguments:

```cpp
vix::console.log("user", 42, "connected");
vix::console.warn("retry", 3, "failed");
vix::console.error("port", 8080, "already in use");
```

Arguments are separated by a single space.

## Supported argument types

The console API is intentionally small and fast.

It supports common primitive and string-like values:

| Category                       | Examples                                                    |
| ------------------------------ | ----------------------------------------------------------- |
| Strings                        | `const char *`, `char *`, `std::string`, `std::string_view` |
| Booleans                       | `bool`                                                      |
| Integers                       | `int`, `long`, `std::uint64_t`, etc.                        |
| Floating point                 | `float`, `double`, `long double`                            |
| Enums                          | enum values are printed as their underlying integer         |
| Pointers                       | printed as addresses                                        |
| String-view convertible values | values convertible to `std::string_view`                    |
| Other objects                  | printed as `[object]`                                       |

## Strings

```cpp
vix::console.log("hello");
vix::console.log(std::string{"hello"});
vix::console.log(std::string_view{"hello"});
```

Output shape:

```txt
09:31:25 [info] hello
09:31:25 [info] hello
09:31:25 [info] hello
```

## Booleans

```cpp
vix::console.log(true, false);
```

Output:

```txt
09:31:25 [info] true false
```

## Numbers

```cpp
vix::console.log("port", 8080);
vix::console.log("ratio", 3.14159);
```

Output shape:

```txt
09:31:25 [info] port 8080
09:31:25 [info] ratio 3.14159
```

Floating point values are formatted compactly.

## Enums

```cpp
enum class Status
{
  Ready = 1,
  Failed = 2
};

vix::console.log("status", Status::Ready);
```

Output:

```txt
09:31:25 [info] status 1
```

Enums are printed as their underlying integer value.

## Pointers

```cpp
int value = 42;
int *ptr = &value;

vix::console.log("ptr", ptr);
vix::console.log("null", static_cast<int *>(nullptr));
```

Output shape:

```txt
09:31:25 [info] ptr 0x7ffd...
09:31:25 [info] null null
```

## Other objects

The console API does not deeply inspect arbitrary objects.

```cpp
struct User
{
  int id{};
};

User user{1};

vix::console.log(user);
```

Output:

```txt
09:31:25 [info] [object]
```

Use `vix::print` or `vix::inspect` when you want structured rendering for STL containers, custom types, or nested data.

## Console vs Print vs Inspect

| API            | Use for                                 |
| -------------- | --------------------------------------- |
| `vix::console` | JavaScript-like development messages.   |
| `vix::print`   | Friendly C++ output with STL rendering. |
| `vix::inspect` | Deep debugging and diagnostics.         |

Examples:

```cpp
vix::console.log("server started");
vix::print(std::vector<int>{1, 2, 3});
vix::inspect(config);
```

## Not for production logging

`vix::console` is intentionally a developer convenience API.

Use it for:

- examples,
- tutorials,
- demos,
- local debugging,
- quick development output,
- making C++ feel familiar to JavaScript developers.

Avoid treating it as a production logger.

It does not provide:

- structured log fields,
- JSON log format,
- log rotation,
- sinks,
- persistent storage,
- tracing context,
- request IDs,
- distributed tracing,
- log shipping,
- production retention policies.

For production logging, use your application logging system.

## Rate limiting

`log` and `info` are rate-limited to protect development output from hot-loop spam.

Default limit:

```txt
200 log/info lines per second
```

When the limit is exceeded, Vix suppresses extra log and info messages and may print a warning like:

```txt
09:31:25 [warn] (console) suppressed 120 log/info lines (rate limit)
```

`warn`, `error`, and `debug` are not rate-limited.

## Why rate limiting exists

This is a developer-safety feature.

A loop like this can flood your terminal:

```cpp
for (int i = 0; i < 1'000'000; ++i)
{
  vix::console.log("i", i);
}
```

The console rate limiter prevents the terminal from being overwhelmed.

## Thread safety

Console writes are protected by a global mutex.

This means each complete console line is written atomically, avoiding interleaved output between threads.

Example:

```cpp
std::thread a([] {
  vix::console.log("from thread A");
});

std::thread b([] {
  vix::console.log("from thread B");
});

a.join();
b.join();
```

You should see complete lines, not mixed half-lines.

## Fixed buffer

Console uses a fixed internal line buffer.

Capacity:

```txt
8192 bytes
```

If a line is too long, it is truncated and marked.

This keeps the console API simple and allocation-light.

## No iostream formatting

Console does not use `std::ostream` formatting for arbitrary types.

That is intentional.

The goal is:

```txt
fast simple console output for development
```

Not:

```txt
deep generic object formatting
```

For object formatting, use:

```cpp
vix::print(value);
vix::inspect(value);
```

## Examples

### Server startup

```cpp
vix::console.info("Server started on port", 8080);
```

Output:

```txt
09:31:25 [info] Server started on port 8080
```

### Route debugging

```cpp
app.get("/health", [](Request &, Response &res) {
  vix::console.log("GET /health");

  res.json({
    "ok", true
  });
});
```

### Warning

```cpp
vix::console.warn("Using default config");
```

### Error

```cpp
vix::console.error("Failed to read", "config.json");
```

### Debug

```cpp
vix::console.set_level(vix::Console::Level::Debug);
vix::console.debug("cache size", 42);
```

### Disable output

```cpp
vix::console.set_level(vix::Console::Level::Off);

vix::console.log("this will not print");
vix::console.error("this will not print either");
```

## Complete example

```cpp
#include <vix.hpp>
#include <vix/console.hpp>

using namespace vix;

int main()
{
  console.info("Starting API");

  App app;

  app.get("/", [](Request &, Response &res) {
    console.log("GET /");

    res.json({
      "message", "Hello from Vix",
      "console", "JavaScript-like developer API"
    });
  });

  app.get("/health", [](Request &, Response &res) {
    console.debug("health check called");

    res.json({
      "ok", true
    });
  });

  app.run(8080);

  return 0;
}
```

Run with debug enabled:

```bash
VIX_CONSOLE_LEVEL=debug vix run main.cpp
```

Test:

```bash
curl -i http://127.0.0.1:8080/
curl -i http://127.0.0.1:8080/health
```

## Environment variable examples

### Show debug messages

```bash
VIX_CONSOLE_LEVEL=debug vix run main.cpp
```

### Show only warnings and errors

```bash
VIX_CONSOLE_LEVEL=warn vix run main.cpp
```

### Show only errors

```bash
VIX_CONSOLE_LEVEL=error vix run main.cpp
```

### Disable console output

```bash
VIX_CONSOLE_LEVEL=off vix run main.cpp
```

### Disable colors

```bash
VIX_COLOR=never vix run main.cpp
```

or:

```bash
NO_COLOR=1 vix run main.cpp
```

## Common mistakes

### Expecting debug to print by default

```cpp
vix::console.debug("debug value", 42);
```

This is hidden by default.

Enable it:

```cpp
vix::console.set_level(vix::Console::Level::Debug);
```

or:

```bash
VIX_CONSOLE_LEVEL=debug vix run main.cpp
```

### Expecting arbitrary objects to be printed deeply

```cpp
std::vector<int> values = {1, 2, 3};

vix::console.log(values);
```

Output:

```txt
09:31:25 [info] [object]
```

Use:

```cpp
vix::print(values);
```

or:

```cpp
vix::inspect(values);
```

### Using console as production logging

This is fine for local development:

```cpp
vix::console.log("user connected");
```

But `vix::console` is not intended to be your production logging layer.

Use your production logging system for deployed services.

### Forgetting the namespace

```cpp
console.log("hello");
```

If you are not using `using namespace vix;`, write:

```cpp
vix::console.log("hello");
```

### Expecting `VIX_CONSOLE_LEVEL` to change after startup

The global console reads environment configuration during construction.

Set environment variables before starting the process:

```bash
VIX_CONSOLE_LEVEL=debug ./app
```

## Best practices

Use `console.log` for familiar development messages:

```cpp
vix::console.log("route called", "/users");
```

Use `console.info` for lifecycle messages:

```cpp
vix::console.info("server ready", 8080);
```

Use `console.warn` for recoverable warnings:

```cpp
vix::console.warn("missing optional config");
```

Use `console.error` for visible local errors:

```cpp
vix::console.error("cannot open file");
```

Use `console.debug` for noisy details:

```cpp
vix::console.debug("payload size", size);
```

Keep console messages short.

For complex values, use:

```cpp
vix::print(value);
vix::inspect(value);
```

## API reference

### `vix::Console`

```cpp
class Console final;
```

The console class behind the global object.

You usually do not need to create your own instance.

Use:

```cpp
vix::console.log("hello");
```

### `vix::console`

```cpp
inline Console console;
```

Global console object.

```cpp
vix::console.info("ready");
```

### `Console::Level`

```cpp
enum class Level : uint8_t
{
  Debug = 0,
  Info = 1,
  Warn = 2,
  Error = 3,
  Off = 4
};
```

Controls the minimum visible console level.

### `log`

```cpp
template <typename... Ts>
void log(Ts &&...xs) noexcept;
```

Prints an info-level message to stdout.

```cpp
vix::console.log("hello", 42);
```

### `info`

```cpp
template <typename... Ts>
void info(Ts &&...xs) noexcept;
```

Prints an info-level message to stdout.

```cpp
vix::console.info("server ready");
```

### `warn`

```cpp
template <typename... Ts>
void warn(Ts &&...xs) noexcept;
```

Prints a warning message to stderr.

```cpp
vix::console.warn("config missing");
```

### `error`

```cpp
template <typename... Ts>
void error(Ts &&...xs) noexcept;
```

Prints an error message to stderr.

```cpp
vix::console.error("failed");
```

### `debug`

```cpp
template <typename... Ts>
void debug(Ts &&...xs) noexcept;
```

Prints a debug message to stdout when the level is `Debug`.

```cpp
vix::console.debug("value", 42);
```

### `set_level`

```cpp
void set_level(Level lvl) noexcept;
```

Sets the minimum visible level.

```cpp
vix::console.set_level(vix::Console::Level::Warn);
```

### `level`

```cpp
Level level() const noexcept;
```

Returns the current console level.

```cpp
auto lvl = vix::console.level();
```

## Summary

`vix::console` is a small JavaScript-like developer console for C++.

Use it when you want familiar output:

```cpp
vix::console.log("hello");
vix::console.info("ready");
vix::console.warn("careful");
vix::console.error("failed");
vix::console.debug("value", 42);
```

It is best for:

- developers coming from JavaScript,
- examples,
- demos,
- tutorials,
- quick local debugging,
- friendly C++ onboarding.

It is not a production logging system.

For rich value formatting, use `vix::print`.

For deep diagnostics, use `vix::inspect`.
