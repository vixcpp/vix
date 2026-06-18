
# Server Pretty Logs

Server Pretty Logs is the runtime banner API used to display a clean server-ready message in the terminal.

It is responsible for output like:

```txt
9:31:25 AM  ● Vix.cpp   READY   v2.5.3 (1 ms)   run

  › HTTP:    http://localhost:8080/
  i Threads: 8/8
  i Mode:    run
  i Status:  ready
  i Hint:    Ctrl+C to stop the server
```

This API is not the same as application logging.

Use `vix::log` for structured application logs.

Use Server Pretty Logs for runtime startup banners and server-ready output.

## Header

```cpp
#include <vix/utils/ServerPrettyLogs.hpp>
```

## Namespace

```cpp
namespace vix::utils
```

The main public types are:

```cpp
vix::utils::ServerReadyInfo
vix::utils::RuntimeBanner
```

## What this API does

The Server Pretty Logs API prints a polished runtime banner when a server is ready.

It can display:

- application name,
- version,
- startup time,
- runtime mode,
- status,
- config path,
- HTTP URL,
- WebSocket URL,
- thread count,
- hints,
- colors,
- terminal hyperlinks,
- and aligned terminal rows.

The main entry point is:

```cpp
vix::utils::RuntimeBanner::emit_server_ready(info);
```

## Quick example

```cpp
#include <vix/utils/ServerPrettyLogs.hpp>

int main()
{
  vix::utils::ServerReadyInfo info;
  info.app = "Vix.cpp";
  info.version = "v2.5.3";
  info.ready_ms = 1;
  info.mode = "run";
  info.status = "ready";
  info.host = "localhost";
  info.port = 8080;
  info.scheme = "http";
  info.base_path = "/";
  info.show_ws = false;
  info.threads = 8;
  info.max_threads = 8;

  vix::utils::RuntimeBanner::emit_server_ready(info);

  return 0;
}
```

Output shape:

```txt
9:31:25 AM  ● Vix.cpp   READY   v2.5.3 (1 ms)   run

  › HTTP:    http://localhost:8080/
  i Threads: 8/8
  i Mode:    run
  i Status:  ready
  i Hint:    Ctrl+C to stop the server
```

The exact visual style depends on color support, terminal capabilities, and environment variables.

## Public API overview

| API | Purpose |
| --- | --- |
| `ServerReadyInfo` | Holds metadata displayed by the ready banner. |
| `RuntimeBanner::emit_server_ready(info)` | Prints the server-ready banner. |
| `RuntimeBanner::stdout_is_tty()` | Checks whether stdout is a TTY. |
| `RuntimeBanner::stderr_is_tty()` | Checks whether stderr is a TTY. |
| `RuntimeBanner::colors_enabled()` | Checks whether colors should be used. |
| `RuntimeBanner::mode_from_env()` | Reads and normalizes runtime mode from `VIX_MODE`. |
| `RuntimeBanner::hyperlinks_enabled()` | Checks whether OSC 8 terminal hyperlinks are supported. |
| `RuntimeBanner::osc8_link(url, text, on)` | Builds a terminal hyperlink when enabled. |

## ServerReadyInfo

`ServerReadyInfo` contains all metadata used to build the banner.

```cpp
vix::utils::ServerReadyInfo info;
```

### Fields

| Field | Default | Purpose |
| --- | --- | --- |
| `app` | `"vix.cpp"` | Application name displayed in the banner. |
| `version` | `""` | Optional version string. |
| `ready_ms` | `-1` | Startup time in milliseconds. |
| `mode` | `""` | Runtime mode, usually `run` or `dev`. |
| `status` | `"ready"` | Status label, usually `ready`, `running`, or `listening`. |
| `config_path` | `""` | Optional config file path. |
| `host` | `"localhost"` | HTTP host. |
| `port` | `8080` | HTTP port. |
| `scheme` | `"http"` | HTTP scheme, usually `http` or `https`. |
| `base_path` | `"/"` | HTTP base path. |
| `show_ws` | `true` | Whether to show the WebSocket row. |
| `ws_port` | `9090` | WebSocket port. |
| `ws_scheme` | `"ws"` | WebSocket scheme, usually `ws` or `wss`. |
| `ws_host` | `"localhost"` | WebSocket host. |
| `ws_path` | `"/"` | WebSocket path. |
| `show_hints` | `true` | Whether to show the Ctrl+C hint. |
| `threads` | `0` | Current thread count. |
| `max_threads` | `0` | Maximum thread count. |

## Minimal banner

```cpp
vix::utils::ServerReadyInfo info;

vix::utils::RuntimeBanner::emit_server_ready(info);
```

This prints a banner using default values.

Default HTTP URL:

```txt
http://localhost:8080/
```

Default WebSocket URL:

```txt
ws://localhost:9090/
```

## HTTP-only banner

Disable the WebSocket row with:

```cpp
info.show_ws = false;
```

Full example:

```cpp
vix::utils::ServerReadyInfo info;
info.app = "Vix.cpp";
info.version = "v2.5.3";
info.mode = "run";
info.status = "ready";
info.host = "localhost";
info.port = 8080;
info.scheme = "http";
info.base_path = "/";
info.show_ws = false;

vix::utils::RuntimeBanner::emit_server_ready(info);
```

Output shape:

```txt
9:31:25 AM  ● Vix.cpp   READY   v2.5.3   run

  › HTTP:    http://localhost:8080/
  i Mode:    run
  i Status:  ready
  i Hint:    Ctrl+C to stop the server
```

## HTTP and WebSocket banner

```cpp
vix::utils::ServerReadyInfo info;
info.app = "Vix.cpp";
info.version = "v2.5.3";

info.host = "localhost";
info.port = 8080;
info.scheme = "http";
info.base_path = "/";

info.show_ws = true;
info.ws_host = "localhost";
info.ws_port = 9090;
info.ws_scheme = "ws";
info.ws_path = "/";

vix::utils::RuntimeBanner::emit_server_ready(info);
```

Output shape:

```txt
9:31:25 AM  ● Vix.cpp   READY   v2.5.3

  › HTTP:    http://localhost:8080/
  › WS:      ws://localhost:9090/
  i Mode:    run
  i Status:  ready
  i Hint:    Ctrl+C to stop the server
```

## Version

Set the version shown next to the app name:

```cpp
info.version = "v2.5.3";
```

If version is empty, no version is shown.

## Startup time

Set `ready_ms` to show startup duration:

```cpp
info.ready_ms = 1;
```

Output shape:

```txt
Vix.cpp   READY   v2.5.3 (1 ms)
```

If `ready_ms` is less than `0`, startup time is hidden.

## Runtime mode

Set the runtime mode:

```cpp
info.mode = "run";
```

or:

```cpp
info.mode = "dev";
```

Common values:

- `run`
- `dev`

When mode is `dev`, the banner can render a development-style tag.

## Read mode from environment

Use:

```cpp
info.mode = vix::utils::RuntimeBanner::mode_from_env();
```

`mode_from_env()` reads:

```txt
VIX_MODE
```

Rules:

| `VIX_MODE` value | Result |
| --- | --- |
| `dev` | `dev` |
| `watch` | `dev` |
| `reload` | `dev` |
| any other value | `run` |
| not set | `run` |

Example:

```bash
VIX_MODE=dev ./app
```

## Status

Set the runtime status:

```cpp
info.status = "ready";
```

Common values:

- `ready`
- `running`
- `listening`
- `warn`
- `warning`
- `error`
- `failed`

The banner displays the status in uppercase.

Examples:

```cpp
info.status = "ready";
info.status = "running";
info.status = "listening";
info.status = "error";
```

## Config path

Show the config file path:

```cpp
info.config_path = ".env";
```

Output shape:

```txt
i Config:  .env
```

If `config_path` is empty, the config row is hidden.

## Threads

Show thread information:

```cpp
info.threads = 8;
info.max_threads = 8;
```

Output shape:

```txt
i Threads: 8/8
```

If only `threads` is set:

```cpp
info.threads = 8;
info.max_threads = 0;
```

Output shape:

```txt
i Threads: 8
```

If `threads` is `0`, the thread row is hidden.

## Hints

By default, the banner shows:

```txt
Hint: Ctrl+C to stop the server
```

Disable it:

```cpp
info.show_hints = false;
```

## URL building

The banner builds the HTTP URL from:

- `info.scheme`
- `info.host`
- `info.port`
- `info.base_path`

Example:

```cpp
info.scheme = "https";
info.host = "api.example.com";
info.port = 443;
info.base_path = "/v1";
```

Output:

```txt
https://api.example.com:443/v1
```

If `base_path` does not start with `/`, it is normalized.

```cpp
info.base_path = "api";
```

Output shape:

```txt
http://localhost:8080/api
```

## WebSocket URL building

The banner builds the WebSocket URL from:

- `info.ws_scheme`
- `info.ws_host`
- `info.ws_port`
- `info.ws_path`

Example:

```cpp
info.ws_scheme = "wss";
info.ws_host = "api.example.com";
info.ws_port = 443;
info.ws_path = "/ws";
```

Output:

```txt
wss://api.example.com:443/ws
```

If `ws_path` does not start with `/`, it is normalized.

## Colors

`RuntimeBanner::colors_enabled()` controls whether colors are used.

Rules:

- If `NO_COLOR` is set and non-empty, colors are disabled.
- If `VIX_COLOR=never`, `VIX_COLOR=0`, or `VIX_COLOR=false`, colors are disabled.
- If `VIX_COLOR=always`, `VIX_COLOR=1`, or `VIX_COLOR=true`, colors are enabled.
- Otherwise, colors are enabled by default.

## Disable colors

```bash
NO_COLOR=1 ./app
```

or:

```bash
VIX_COLOR=never ./app
```

## Force colors

```bash
VIX_COLOR=always ./app
```

## TTY detection

```cpp
bool out_tty = vix::utils::RuntimeBanner::stdout_is_tty();
bool err_tty = vix::utils::RuntimeBanner::stderr_is_tty();
```

On Unix-like systems, this uses terminal checks.

On Windows, these functions currently return `true`.

## Terminal hyperlinks

The banner can use OSC 8 hyperlinks when supported.

```cpp
bool enabled = vix::utils::RuntimeBanner::hyperlinks_enabled();
```

Hyperlinks are disabled if:

```txt
VIX_NO_HYPERLINK
```

is set and non-empty.

Hyperlinks require stderr to be a TTY.

They are enabled for supported terminal environments such as VS Code terminal, Windows Terminal, WezTerm, iTerm, Apple Terminal, Kitty, and VTE-based terminals.

They are typically disabled for tmux or screen.

## Build an OSC 8 link

```cpp
std::string link = vix::utils::RuntimeBanner::osc8_link(
  "http://localhost:8080/",
  "http://localhost:8080/",
  true
);
```

If the third argument is false, the plain text is returned.

```cpp
std::string text = vix::utils::RuntimeBanner::osc8_link(
  "http://localhost:8080/",
  "Open server",
  false
);
```

Result:

```txt
Open server
```

## Disable terminal hyperlinks

```bash
VIX_NO_HYPERLINK=1 ./app
```

## Dev animation

In dev mode, the banner can render an animated dev tag.

Animation is disabled when:

```txt
VIX_NO_ANIM
```

is set and non-empty.

Animation also requires stderr to be a TTY and colors to be enabled.

Disable animation:

```bash
VIX_NO_ANIM=1 ./app
```

## Thread synchronization

The runtime banner uses console synchronization helpers to avoid output interleaving.

Internally, it uses:

```cpp
vix::utils::console_mutex()
vix::utils::console_reset_banner()
vix::utils::console_mark_banner_done()
```

When `emit_server_ready()` starts, it resets the banner state.

When the banner finishes, it marks the banner as done.

This helps other console output wait until the banner is complete.

## Output stream

The banner writes to:

```txt
stderr
```

This keeps startup/runtime status separate from normal stdout output.

## Complete example

```cpp
#include <vix/utils/ServerPrettyLogs.hpp>

int main()
{
  vix::utils::ServerReadyInfo info;

  info.app = "Vix.cpp";
  info.version = "v2.5.3";
  info.ready_ms = 1;

  info.mode = vix::utils::RuntimeBanner::mode_from_env();
  info.status = "ready";

  info.scheme = "http";
  info.host = "localhost";
  info.port = 8080;
  info.base_path = "/";

  info.show_ws = true;
  info.ws_scheme = "ws";
  info.ws_host = "localhost";
  info.ws_port = 9090;
  info.ws_path = "/ws";

  info.config_path = ".env";
  info.threads = 8;
  info.max_threads = 8;
  info.show_hints = true;

  vix::utils::RuntimeBanner::emit_server_ready(info);

  return 0;
}
```

## Example for an HTTP server

```cpp
#include <vix.hpp>
#include <vix/utils/ServerPrettyLogs.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request &, Response &res) {
    res.json({
      "message", "Hello from Vix"
    });
  });

  vix::utils::ServerReadyInfo info;
  info.app = "Vix.cpp";
  info.version = "v2.5.3";
  info.ready_ms = 1;
  info.mode = vix::utils::RuntimeBanner::mode_from_env();
  info.status = "ready";
  info.host = "localhost";
  info.port = 8080;
  info.show_ws = false;
  info.threads = 8;
  info.max_threads = 8;

  vix::utils::RuntimeBanner::emit_server_ready(info);

  app.run(8080);

  return 0;
}
```

In a real runtime, the banner is usually emitted by the runtime/server layer when the server is ready, not manually inside every app.

## Environment variables

| Variable | Purpose |
| --- | --- |
| `NO_COLOR` | Disable colors. |
| `VIX_COLOR` | Control colors: `always`, `never`, `1`, `0`, `true`, `false`. |
| `VIX_MODE` | Runtime mode: `run`, `dev`, `watch`, `reload`. |
| `VIX_NO_HYPERLINK` | Disable OSC 8 terminal hyperlinks. |
| `VIX_NO_ANIM` | Disable banner animations. |

## Common outputs

### Run mode

```txt
9:31:25 AM  ● Vix.cpp   READY   v2.5.3 (1 ms)   run

  › HTTP:    http://localhost:8080/
  i Threads: 8/8
  i Mode:    run
  i Status:  ready
  i Hint:    Ctrl+C to stop the server
```

### Dev mode

```txt
9:31:25 AM  ◆ Vix.cpp   READY   v2.5.3 (1 ms)   dev

  › HTTP:    http://localhost:8080/
  i Threads: 8/8
  i Mode:    dev (watch/reload)
  i Status:  ready
  i Hint:    Ctrl+C to stop the server
```

### With WebSocket

```txt
9:31:25 AM  ● Vix.cpp   READY   v2.5.3 (1 ms)   run

  › HTTP:    http://localhost:8080/
  › WS:      ws://localhost:9090/
  i Threads: 8/8
  i Mode:    run
  i Status:  ready
  i Hint:    Ctrl+C to stop the server
```

## Common mistakes

### Using it for application logs

This API is for ready banners:

```cpp
vix::utils::RuntimeBanner::emit_server_ready(info);
```

For application logs, use:

```cpp
vix::log::info("request completed");
```

### Forgetting to set `show_ws`

By default:

```cpp
info.show_ws = true;
```

If your app does not use WebSocket, set:

```cpp
info.show_ws = false;
```

### Forgetting the base path slash

This is okay:

```cpp
info.base_path = "api";
```

The banner normalizes it to:

```txt
/api
```

But for clarity, prefer:

```cpp
info.base_path = "/api";
```

### Expecting colors to appear in every environment

Colors can be disabled by:

```bash
NO_COLOR=1
VIX_COLOR=never
```

Some environments may also display ANSI output differently.

### Expecting hyperlinks in every terminal

OSC 8 hyperlinks depend on terminal support.

Disable them when needed:

```bash
VIX_NO_HYPERLINK=1 ./app
```

### Printing another banner at the same time

The banner uses console synchronization, but you should still avoid printing your own large startup output at the exact same time from another thread.

## Best practices

Use `ServerReadyInfo` to describe the runtime state clearly:

```cpp
info.app = "Vix.cpp";
info.version = "v2.5.3";
info.mode = "run";
info.status = "ready";
```

Hide WebSocket when not used:

```cpp
info.show_ws = false;
```

Show thread information when available:

```cpp
info.threads = worker_count;
info.max_threads = max_worker_count;
```

Use environment-derived mode:

```cpp
info.mode = vix::utils::RuntimeBanner::mode_from_env();
```

Disable colors in CI or log files:

```bash
NO_COLOR=1
```

Use this API from runtime/server infrastructure, not from normal route handlers.

## API reference

### `ServerReadyInfo`

```cpp
struct ServerReadyInfo;
```

Metadata displayed by the runtime ready banner.

```cpp
vix::utils::ServerReadyInfo info;
info.app = "Vix.cpp";
info.port = 8080;
```

### `RuntimeBanner`

```cpp
class RuntimeBanner final;
```

Static utility class for runtime banner output.

You do not instantiate it.

### `emit_server_ready`

```cpp
static void emit_server_ready(const ServerReadyInfo &info);
```

Prints the ready banner to stderr.

```cpp
vix::utils::RuntimeBanner::emit_server_ready(info);
```

### `stdout_is_tty`

```cpp
static bool stdout_is_tty();
```

Returns whether stdout is a terminal.

```cpp
bool is_tty = vix::utils::RuntimeBanner::stdout_is_tty();
```

### `stderr_is_tty`

```cpp
static bool stderr_is_tty();
```

Returns whether stderr is a terminal.

```cpp
bool is_tty = vix::utils::RuntimeBanner::stderr_is_tty();
```

### `colors_enabled`

```cpp
static bool colors_enabled();
```

Returns whether colors are enabled for the banner.

```cpp
if (vix::utils::RuntimeBanner::colors_enabled())
{
  // terminal colors are enabled
}
```

### `mode_from_env`

```cpp
static std::string mode_from_env();
```

Reads `VIX_MODE` and normalizes it to `dev` or `run`.

```cpp
info.mode = vix::utils::RuntimeBanner::mode_from_env();
```

### `hyperlinks_enabled`

```cpp
static bool hyperlinks_enabled();
```

Returns whether OSC 8 terminal hyperlinks are enabled.

```cpp
bool links = vix::utils::RuntimeBanner::hyperlinks_enabled();
```

### `osc8_link`

```cpp
static std::string osc8_link(
  const std::string &url,
  const std::string &text,
  bool on
);
```

Builds a terminal hyperlink when `on` is true.

```cpp
auto link = vix::utils::RuntimeBanner::osc8_link(
  "http://localhost:8080/",
  "Open server",
  true
);
```

## Summary

Server Pretty Logs is the startup banner API for Vix runtimes and servers.

Use it to display:

- app name,
- version,
- HTTP URL,
- WebSocket URL,
- mode,
- status,
- config path,
- thread count,
- and stop hints.

Core usage:

```cpp
vix::utils::ServerReadyInfo info;
info.app = "Vix.cpp";
info.version = "v2.5.3";
info.port = 8080;
info.show_ws = false;

vix::utils::RuntimeBanner::emit_server_ready(info);
```

Use `vix::log` for structured application logs.

Use Server Pretty Logs for polished runtime-ready output.
