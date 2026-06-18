# vix ws

`vix ws` checks WebSocket endpoints for a Vix application.

Use it when you want to verify that a WebSocket server is reachable, that the TCP port is open, and that the WebSocket handshake works.

```bash
vix ws check ws://127.0.0.1:9090/ws
```

## Overview

`vix ws` is the WebSocket diagnostic command for Vix.

It can:

- check a WebSocket endpoint
- read WebSocket config from `vix.json`
- use a URL passed directly on the command line
- generate a local WebSocket URL from host, port, and path
- reuse proxy WebSocket port and path when configured
- validate WebSocket URL format
- run a TCP reachability probe
- run a WebSocket handshake when the CLI is built with WebSocket support
- classify common WebSocket failures
- print focused fixes
- optionally skip ping diagnostics
- show verbose target details

It is useful when debugging:

- local WebSocket servers
- production WebSocket ports
- Nginx WebSocket proxying
- wrong WebSocket paths
- missing upgrade headers
- connection refused errors
- DNS errors
- timeout errors
- bad proxy routing

## Usage

```bash
vix ws check [url] [options]
```

## Basic examples

```bash
# Check a local WebSocket endpoint
vix ws check ws://127.0.0.1:9090/ws

# Check with a longer timeout
vix ws check ws://127.0.0.1:9090/ws --timeout 5000

# Check without ping diagnostic
vix ws check ws://127.0.0.1:9090/ws --no-ping

# Print more diagnostics
vix ws check ws://127.0.0.1:9090/ws --verbose

# Use config from vix.json
vix ws check
```

## Command

| Command              | Purpose                     |
| -------------------- | --------------------------- |
| `vix ws check [url]` | Check a WebSocket endpoint. |

`check` is currently the main WebSocket command.

If no URL is passed, Vix uses the WebSocket configuration from `vix.json`.

## Options

| Option           | Description                                 |
| ---------------- | ------------------------------------------- |
| `--timeout <ms>` | Connection timeout in milliseconds.         |
| `--no-ping`      | Do not run ping diagnostic after handshake. |
| `--verbose`      | Print additional diagnostics.               |
| `-v`             | Alias for `--verbose`.                      |
| `-h, --help`     | Show help.                                  |

## URL selection

`vix ws check` selects the URL in this order:

```txt
1. URL passed on the command line
2. production.websocket.public_url
3. production.websocket.local_url
4. generated local URL from host, port, and path
```

Example:

```bash
vix ws check ws://127.0.0.1:9090/ws
```

uses the CLI URL.

Example:

```bash
vix ws check
```

uses configuration from `vix.json`.

## Configuration source

`vix ws` reads configuration from:

```txt
vix.json
```

under:

```txt
production.websocket
```

It can also reuse:

```txt
production.proxy.websocket
```

when WebSocket proxying is enabled.

## Full config example

```json
{
  "name": "PulseGrid",
  "production": {
    "websocket": {
      "host": "127.0.0.1",
      "port": 9090,
      "path": "/ws",
      "local_url": "ws://127.0.0.1:9090/ws",
      "public_url": "wss://pulsegrid.example.com/ws",
      "timeout_ms": 3000,
      "heartbeat": true
    },
    "proxy": {
      "websocket": {
        "enabled": true,
        "path": "/ws",
        "port": 9090
      }
    }
  }
}
```

## WebSocket config fields

| Field                                | Purpose                                                     |
| ------------------------------------ | ----------------------------------------------------------- |
| `production.websocket.host`          | Local WebSocket host.                                       |
| `production.websocket.port`          | Local WebSocket port.                                       |
| `production.websocket.path`          | WebSocket path.                                             |
| `production.websocket.local_url`     | Explicit local WebSocket URL.                               |
| `production.websocket.public_url`    | Explicit public WebSocket URL.                              |
| `production.websocket.timeout_ms`    | Default timeout in milliseconds.                            |
| `production.websocket.heartbeat`     | Whether heartbeat is expected or enabled in the app config. |
| `production.proxy.websocket.enabled` | Allows proxy WebSocket config to override local port/path.  |
| `production.proxy.websocket.path`    | WebSocket path used by the proxy.                           |
| `production.proxy.websocket.port`    | WebSocket upstream port used by the proxy.                  |

## Default values

If values are missing, Vix uses safe defaults.

| Field     | Default                                             |
| --------- | --------------------------------------------------- |
| app name  | `vix.json` name, `.vix` filename, or current folder |
| host      | `127.0.0.1`                                         |
| port      | `9090`                                              |
| path      | `/ws`                                               |
| timeout   | `3000` ms                                           |
| local URL | generated from host, port, and path                 |

A generated local URL looks like this:

```txt
ws://127.0.0.1:9090/ws
```

## Path normalization

WebSocket paths are normalized.

If the path does not start with `/`, Vix adds it.

Example:

```json
{
  "production": {
    "websocket": {
      "path": "ws"
    }
  }
}
```

is treated as:

```txt
/ws
```

If the path is empty, Vix uses:

```txt
/ws
```

## Proxy WebSocket integration

If `production.proxy.websocket.enabled` is true, Vix can reuse the proxy WebSocket port and path.

Example:

```json
{
  "production": {
    "websocket": {
      "host": "127.0.0.1"
    },
    "proxy": {
      "websocket": {
        "enabled": true,
        "path": "/ws",
        "port": 9090
      }
    }
  }
}
```

Vix uses:

```txt
port: 9090
path: /ws
```

This keeps `vix ws check` aligned with the Nginx proxy config.

## URL format

WebSocket URLs must start with:

```txt
ws://
wss://
```

Examples:

```txt
ws://127.0.0.1:9090/ws
ws://localhost:9090/ws
wss://pulsegrid.example.com/ws
```

Invalid:

```txt
http://127.0.0.1:9090/ws
https://pulsegrid.example.com/ws
127.0.0.1:9090/ws
```

If the URL is invalid, Vix prints a focused error and a fix.

Example:

```txt
WebSocket URL must start with ws:// or wss://
Fix: use a URL like ws://127.0.0.1:9090/ws
```

## Current TLS limitation

Native `wss://` checks are not supported yet.

If you run:

```bash
vix ws check wss://pulsegrid.example.com/ws
```

Vix reports:

```txt
wss:// checks are not supported yet.
Fix: use ws:// until TLS support is added
```

For now, use `ws://` for native local checks.

For public TLS health checks, use:

```bash
vix health websocket
```

because that command checks the WebSocket upgrade through HTTP-style health diagnostics.

## What `vix ws check` does

When you run:

```bash
vix ws check ws://127.0.0.1:9090/ws
```

Vix performs these steps:

1. loads WebSocket config from `vix.json`
2. applies command-line options
3. selects the URL
4. parses the WebSocket URL
5. prints the selected target
6. rejects `wss://` for now
7. runs a TCP connection probe
8. runs a WebSocket handshake if the CLI has WebSocket support
9. prints a clear result
10. prints a focused fix when something fails

## Output summary

Example output shape:

```txt
WebSocket
App: PulseGrid
Target: check
URL: ws://127.0.0.1:9090/ws
URL Source: cli
Local URL: ws://127.0.0.1:9090/ws
Public URL: wss://pulsegrid.example.com/ws
Configured Host: 127.0.0.1
Configured Port: 9090
Configured Path: /ws
Timeout: 3000ms
Ping: yes
Heartbeat: yes
Verbose: no
```

## Parsed target output

Vix prints the parsed WebSocket target.

Example:

```txt
WebSocket Target
Command: ws://127.0.0.1:9090/ws
scheme: ws
host: 127.0.0.1
port: 9090
path: /ws
```

This helps catch wrong ports, wrong paths, and wrong schemes quickly.

## TCP probe

The first real check is a TCP probe.

Example:

```txt
TCP
Command: connect 127.0.0.1:9090
TCP endpoint is reachable
```

This proves that the host and port are reachable.

If TCP fails, the WebSocket server is not reachable at the network level.

## WebSocket handshake

If the Vix CLI was built with the WebSocket module enabled, `vix ws check` also performs a WebSocket handshake.

Example:

```txt
Handshake
Command: connect ws://127.0.0.1:9090/ws
WebSocket handshake succeeded
WebSocket endpoint is reachable
```

If the CLI was built without WebSocket support, Vix still runs the TCP check.

Then it reports:

```txt
WebSocket client module is not available in this build
Fix: TCP check passed; rebuild the Vix CLI with the websocket module enabled for handshake checks
```

This means the port is reachable, but the current CLI cannot verify the full handshake.

## Ping diagnostic

By default, ping diagnostic is enabled in the option summary.

At the moment, ping diagnostic is disabled internally after handshake.

Vix prints:

```txt
ping diagnostic disabled
Fix: use --no-ping or check heartbeat logs
```

Use this to avoid ping-related diagnostic output:

```bash
vix ws check ws://127.0.0.1:9090/ws --no-ping
```

## Timeout

Use `--timeout` to control connection timeout.

```bash
vix ws check ws://127.0.0.1:9090/ws --timeout 5000
```

The timeout must be a positive integer in milliseconds.

Invalid:

```bash
vix ws check ws://127.0.0.1:9090/ws --timeout 0
```

If invalid, Vix reports:

```txt
invalid value for --timeout
Fix: vix ws --help
```

## Failure classification

`vix ws check` classifies common failures and gives focused fixes.

| Failure                  | Meaning                                  | Fix                                     |
| ------------------------ | ---------------------------------------- | --------------------------------------- |
| DNS resolution failed    | Domain cannot be resolved.               | Check domain and DNS resolver.          |
| Connection refused       | Nothing is listening on host/port.       | Check service port and listener.        |
| Timeout                  | Host or service did not respond in time. | Check firewall, upstream and service.   |
| WebSocket upgrade failed | Upgrade headers or handshake failed.     | Check Upgrade and Connection headers.   |
| Bad path                 | WebSocket route does not exist.          | Check WebSocket route/path.             |
| Proxy HTTP response      | Server returned HTTP instead of upgrade. | Check endpoint and proxy route.         |
| Bad handshake            | Handshake failed.                        | Check port, path and proxy upgrade.     |
| TLS unsupported          | URL uses `wss://`.                       | Use `ws://` until TLS support is added. |

## Common failure examples

### Connection refused

```txt
WebSocket TCP connection was refused.
Fix: check service port and listener
```

Check:

```bash
vix service status
ss -tulpn
```

### Timeout

```txt
WebSocket connection timed out.
Fix: check firewall, upstream and service
```

Check firewall, Nginx, upstream port, and service status.

### WebSocket upgrade failed

```txt
WebSocket upgrade failed.
Fix: check Upgrade and Connection headers
```

Check Nginx WebSocket config:

```bash
vix proxy nginx check
```

### Bad path

```txt
WebSocket path does not exist on the server.
Fix: check WebSocket route/path
```

Check that the app and proxy use the same path:

```txt
/ws
```

### HTTP instead of WebSocket upgrade

```txt
Server returned HTTP instead of WebSocket upgrade.
Fix: check endpoint and proxy route
```

This usually means you are hitting the wrong route or the proxy is not forwarding WebSocket upgrade correctly.

## Full local example

```json
{
  "name": "PulseGrid",
  "production": {
    "websocket": {
      "host": "127.0.0.1",
      "port": 9090,
      "path": "/ws",
      "timeout_ms": 3000,
      "heartbeat": true
    }
  }
}
```

Run:

```bash
vix ws check
```

Vix generates and checks:

```txt
ws://127.0.0.1:9090/ws
```

## Full proxy-aware example

```json
{
  "name": "PulseGrid",
  "production": {
    "websocket": {
      "host": "127.0.0.1",
      "heartbeat": true
    },
    "proxy": {
      "websocket": {
        "enabled": true,
        "path": "/ws",
        "port": 9090
      }
    }
  }
}
```

Run:

```bash
vix ws check
```

Vix uses the proxy WebSocket port and path to generate:

```txt
ws://127.0.0.1:9090/ws
```

## Explicit URL example

```bash
vix ws check ws://127.0.0.1:9090/ws
```

The CLI URL always wins over config.

This is useful when testing a temporary endpoint or debugging a different port.

## Relationship with `vix health websocket`

`vix ws check` is a native WebSocket diagnostic.

It checks TCP and, when supported, the native WebSocket handshake.

`vix health websocket` checks the configured WebSocket health endpoint using HTTP upgrade-style health logic.

Use both when debugging production WebSocket issues:

```bash
vix ws check ws://127.0.0.1:9090/ws
vix health websocket
```

## Relationship with `vix proxy nginx`

`vix proxy nginx` manages WebSocket proxy configuration.

`vix ws check` helps verify that the WebSocket endpoint is reachable.

A normal WebSocket production flow is:

```bash
vix proxy nginx check
vix ws check ws://127.0.0.1:9090/ws
vix health websocket
```

If `vix proxy nginx check` reports missing upgrade headers, fix the proxy first.

## Relationship with `vix logs`

Use logs when WebSocket checks fail.

```bash
vix logs errors --repeated
```

This helps detect:

```txt
connection reset by peer
client disconnected
websocket disconnected
timeout
connection refused
broken pipe
```

## Relationship with `vix doctor production`

`vix doctor production` gives a broader production readiness view.

Use it after WebSocket checks:

```bash
vix doctor production
```

It can show whether WebSocket production readiness is configured.

## Recommended workflow

For local WebSocket debugging:

```bash
vix service status
vix ws check ws://127.0.0.1:9090/ws
vix logs errors --repeated
```

For proxy debugging:

```bash
vix proxy nginx check
vix ws check ws://127.0.0.1:9090/ws
vix health websocket
```

For production debugging:

```bash
vix health websocket
vix logs errors --repeated
vix doctor production
```

## Commands reference

| Command                             | Description                   |
| ----------------------------------- | ----------------------------- |
| `vix ws check [url]`                | Check a WebSocket endpoint.   |
| `vix ws check [url] --timeout <ms>` | Check with custom timeout.    |
| `vix ws check [url] --no-ping`      | Skip ping diagnostic.         |
| `vix ws check [url] --verbose`      | Print additional diagnostics. |
| `vix ws --help`                     | Show help.                    |

## Common workflows

### Check local endpoint

```bash
vix ws check ws://127.0.0.1:9090/ws
```

### Check from config

```bash
vix ws check
```

### Increase timeout

```bash
vix ws check ws://127.0.0.1:9090/ws --timeout 5000
```

### Disable ping diagnostic

```bash
vix ws check ws://127.0.0.1:9090/ws --no-ping
```

### Show verbose diagnostics

```bash
vix ws check ws://127.0.0.1:9090/ws -v
```

## Common mistakes

### Using `https://` instead of `wss://`

Wrong:

```bash
vix ws check https://pulsegrid.example.com/ws
```

Correct URL shape:

```bash
vix ws check ws://127.0.0.1:9090/ws
```

For public TLS health, use:

```bash
vix health websocket
```

### Using `wss://` with native checker

Current native checker does not support `wss://` yet.

Use:

```bash
vix ws check ws://127.0.0.1:9090/ws
```

or use:

```bash
vix health websocket
```

for public TLS health.

### Forgetting the WebSocket path

Wrong:

```bash
vix ws check ws://127.0.0.1:9090
```

Better:

```bash
vix ws check ws://127.0.0.1:9090/ws
```

The path must match the route your app exposes.

### Checking the public domain before local endpoint

Start local.

```bash
vix ws check ws://127.0.0.1:9090/ws
```

Then check public health:

```bash
vix health websocket
```

This separates app problems from proxy or TLS problems.

### Expecting ping to prove heartbeat

Ping diagnostic is currently disabled internally.

Use heartbeat logs or app-level diagnostics for heartbeat behavior.

```bash
vix logs errors --repeated
```

### Ignoring Nginx upgrade headers

If WebSocket works locally but fails publicly, check proxy config:

```bash
vix proxy nginx check
```

## Troubleshooting

### Missing WebSocket URL

If Vix reports:

```txt
Missing WebSocket URL.
```

pass a URL:

```bash
vix ws check ws://127.0.0.1:9090/ws
```

or configure:

```json
{
  "production": {
    "websocket": {
      "local_url": "ws://127.0.0.1:9090/ws"
    }
  }
}
```

### Connection refused

Check whether the WebSocket server is running:

```bash
vix service status
```

Check listening ports:

```bash
ss -tulpn
```

Check logs:

```bash
vix logs app --errors
```

### Timeout

Check firewall and proxy layers:

```bash
vix proxy nginx check
vix logs proxy --errors
```

Also verify that the service is running:

```bash
vix service status
```

### Bad WebSocket path

Make sure all three layers use the same path:

```txt
app route
production.websocket.path
production.proxy.websocket.path
```

Usually:

```txt
/ws
```

### Missing WebSocket upgrade

Check Nginx config:

```bash
vix proxy nginx check
```

Vix proxy check can detect missing:

```nginx
proxy_set_header Upgrade $http_upgrade;
proxy_set_header Connection "upgrade";
```

### Native handshake not available

If Vix reports that the WebSocket client module is not available, the TCP probe already passed.

To check the full native handshake, rebuild the CLI with WebSocket module support.

### Public WebSocket fails but local works

Run:

```bash
vix proxy nginx check
vix health websocket
vix logs errors --repeated
```

Likely causes:

- wrong Nginx WebSocket path
- wrong Nginx upstream port
- missing upgrade headers
- TLS issue
- firewall or DNS issue

## Best practices

Check `ws://127.0.0.1:<port>/<path>` first.

Use `vix proxy nginx check` before debugging public WebSocket failures.

Keep WebSocket config in `vix.json`.

Use the same path in the app, proxy, and health config.

Use `vix logs errors --repeated` when WebSocket logs are noisy.

Use `vix health websocket` for public WebSocket health.

Do not rely on `wss://` in `vix ws check` until native TLS support is added.

## Related commands

| Command                      | Purpose                                            |
| ---------------------------- | -------------------------------------------------- |
| `vix health websocket`       | Check WebSocket health through health diagnostics. |
| `vix proxy nginx check`      | Validate Nginx WebSocket proxy config.             |
| `vix logs errors --repeated` | Analyze repeated WebSocket or network errors.      |
| `vix service status`         | Check whether the app service is running.          |
| `vix doctor production`      | Inspect complete production readiness.             |

## Next step

Check production health.

[Open the health guide](/cli/health)
