# Server Pretty Logs (Runtime Banner)

This guide explains `vix::utils::RuntimeBanner` and `vix::utils::ServerReadyInfo` from `ServerPrettyLogs.hpp`.

Goal: print a clean "server ready" banner on startup, with terminal-friendly colors, optional OSC-8 hyperlinks, and safe output in multithreaded apps.

## What this component does

When your server becomes ready, call:

- `RuntimeBanner::emit_server_ready(info)`

It prints (to **stderr**) a banner that includes:

- current local time (12h format)
- app identity + mode tag (`dev` or `run`)
- status pill (READY / RUNNING / LISTENING / WARN / ERROR)
- HTTP URL
- optional WebSocket URL
- optional config path and thread hints
- a hint line (`Ctrl+C ...`)

It also uses `vix::utils::console_mutex()` so banner output does not interleave with other threads.

## Include

```cpp
#include <vix/utils/ServerPrettyLogs.hpp>
```

## Basic usage

Create a `ServerReadyInfo`, set fields you care about, then emit the banner.

```cpp
#include <vix/utils/ServerPrettyLogs.hpp>

int main()
{
  vix::utils::ServerReadyInfo info;
  info.app = "my-app";
  info.version = "v1.0.0";
  info.mode = vix::utils::RuntimeBanner::mode_from_env(); // "run" or "dev"
  info.status = "ready";

  info.scheme = "http";
  info.host = "127.0.0.1";
  info.port = 8080;
  info.base_path = "/";

  info.show_ws = false; // hide WS row

  vix::utils::RuntimeBanner::emit_server_ready(info);
  return 0;
}
```

## HTTP + WebSocket example

```cpp
#include <vix/utils/ServerPrettyLogs.hpp>

int main()
{
  vix::utils::ServerReadyInfo info;
  info.app = "vix.cpp";
  info.version = "v1.34.0";
  info.ready_ms = 42;

  info.mode = "dev";
  info.status = "listening";

  info.scheme = "http";
  info.host = "localhost";
  info.port = 8080;
  info.base_path = "/";

  info.show_ws = true;
  info.ws_scheme = "ws";
  info.ws_host = "localhost";
  info.ws_port = 9090;
  info.ws_path = "/";

  info.show_hints = true;

  vix::utils::RuntimeBanner::emit_server_ready(info);
  return 0;
}
```

## Environment controls

### Colors

Colors follow these rules:

- If `NO_COLOR` is set (non-empty), colors are disabled.
- If `VIX_COLOR` is set:
  - `never|0|false` disables colors
  - `always|1|true` enables colors
- Otherwise, colors are enabled by default.

Examples:

```bash
# Disable colors
vix run main.cpp --log-color=never
```

```bash
# Force colors (even if terminal detection is weird)
vix run main.cpp --log-color=always
```

### Mode

`RuntimeBanner::mode_from_env()` reads `VIX_MODE` and normalizes it:

- `dev|watch|reload` -> `dev`
- everything else -> `run`

```bash
vix dev examples/http/server.cpp
```

### Hyperlinks (OSC 8)

If supported, the banner prints URLs as clickable links. It is controlled by:

- `VIX_NO_HYPERLINK` (set to disable)
- terminal detection and allowlist checks

```bash
VIX_NO_HYPERLINK=1 vix run main.cpp
```

### Animations

The `[dev]` tag can pulse when animations are allowed. It is controlled by:

- `VIX_NO_ANIM` (set to disable)
- stderr must be a TTY
- `NO_COLOR` disables animations too

```bash
VIX_NO_ANIM=1 vix run main.cpp
```

## Thread safety and output ordering

`emit_server_ready()` does two important things:

1. It calls `console_reset_banner()` and `console_mark_banner_done()` so other threads can coordinate with the banner.
2. It prints under `console_mutex()` so other console output will not interleave.

If you also print startup logs from worker threads, the banner stays readable.

## Field reference

`ServerReadyInfo` fields you will commonly set:

- `app` and `version`
- `ready_ms`
- `mode` and `status`
- HTTP: `scheme`, `host`, `port`, `base_path`
- WS: `show_ws`, `ws_scheme`, `ws_host`, `ws_port`, `ws_path`
- `config_path`
- `threads`, `max_threads`
- `show_hints`

## Typical pattern in a server

A clean approach is:

1. Start the server
2. When bind/listen succeeds, fill `ServerReadyInfo`
3. Emit the banner once

```cpp
void on_server_ready(int port, int startup_ms)
{
  vix::utils::ServerReadyInfo info;
  info.app = "my-app";
  info.version = "v1.0.0";
  info.mode = vix::utils::RuntimeBanner::mode_from_env();
  info.status = "ready";
  info.ready_ms = startup_ms;

  info.host = "0.0.0.0";
  info.port = port;
  info.base_path = "/";

  info.show_ws = false;

  vix::utils::RuntimeBanner::emit_server_ready(info);
}
```

## Troubleshooting

- If you see no colors: check `NO_COLOR` and `VIX_COLOR`.
- If links are not clickable: your terminal may not support OSC 8 or you are inside tmux/screen.
- If the banner interleaves with other output: ensure other console writes also respect `console_mutex()` when needed.

Keep it minimal. No hidden magic. Just explicit C++.

