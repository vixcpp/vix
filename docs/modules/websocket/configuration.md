# Configuration

This page explains how to configure the Vix WebSocket module.

Use it when you want to control the WebSocket host, port, message limits, idle timeout, compression, ping interval, and automatic ping/pong behavior.

## Header

```cpp
#include <vix/websocket/config.hpp>
```

Or use the umbrella header:

```cpp
#include <vix/websocket.hpp>
```

## What configuration controls

The WebSocket module uses configuration values to control the runtime behavior of WebSocket connections.

It controls:

- bind host
- bind port
- maximum message size
- idle timeout
- per-message deflate
- heartbeat ping interval
- automatic ping/pong handling

The WebSocket config is derived from the core Vix configuration.

```cpp
vix::config::Config core{".env"};

vix::websocket::Config wsConfig =
    vix::websocket::Config::from_core(core);
```

## Basic `.env`

A minimal WebSocket configuration can look like this:

```dotenv
WEBSOCKET_HOST=0.0.0.0
WEBSOCKET_PORT=9090

WEBSOCKET_MAX_MESSAGE_SIZE=65536
WEBSOCKET_IDLE_TIMEOUT=60
WEBSOCKET_ENABLE_DEFLATE=true
WEBSOCKET_PING_INTERVAL=30
WEBSOCKET_AUTO_PING_PONG=true
```

By default, the low-level server reads the WebSocket host from `websocket.host`, with `0.0.0.0` as fallback, and the WebSocket port from `websocket.port`, with `9090` as fallback. :contentReference[oaicite:0]{index=0}

## Runtime config type

The runtime configuration type is:

```cpp
vix::websocket::Config
```

It contains:

```cpp
struct Config
{
  std::size_t maxMessageSize = 64 * 1024;
  std::chrono::seconds idleTimeout{60};
  bool enablePerMessageDeflate = true;
  bool autoPingPong = true;
  std::chrono::seconds pingInterval{30};

  static Config from_core(const vix::config::Config &core);
};
```

## Default values

| Field                     | Default | Meaning                                           |
| ------------------------- | ------: | ------------------------------------------------- |
| `maxMessageSize`          | `65536` | Maximum accepted WebSocket message size in bytes. |
| `idleTimeout`             |   `60s` | Maximum idle time before closing a connection.    |
| `enablePerMessageDeflate` |  `true` | Enables per-message deflate support.              |
| `autoPingPong`            |  `true` | Automatically handles ping/pong behavior.         |
| `pingInterval`            |   `30s` | Interval between heartbeat ping frames.           |

## Environment variables

The common environment variables are:

| Variable                     |   Example | Purpose                                 |
| ---------------------------- | --------: | --------------------------------------- |
| `WEBSOCKET_HOST`             | `0.0.0.0` | Address used by the WebSocket listener. |
| `WEBSOCKET_PORT`             |    `9090` | Port used by the WebSocket listener.    |
| `WEBSOCKET_MAX_MESSAGE_SIZE` |   `65536` | Maximum accepted message size in bytes. |
| `WEBSOCKET_IDLE_TIMEOUT`     |      `60` | Idle timeout in seconds.                |
| `WEBSOCKET_ENABLE_DEFLATE`   |    `true` | Enable per-message deflate.             |
| `WEBSOCKET_PING_INTERVAL`    |      `30` | Ping interval in seconds.               |
| `WEBSOCKET_AUTO_PING_PONG`   |    `true` | Automatically handle ping/pong frames.  |

## Host

The host controls the address where the WebSocket server binds.

```dotenv
WEBSOCKET_HOST=0.0.0.0
```

Use `0.0.0.0` when the server should accept connections from available network interfaces.

For local-only development, you can use:

```dotenv
WEBSOCKET_HOST=127.0.0.1
```

Common values:

| Value       | Meaning                         |
| ----------- | ------------------------------- |
| `0.0.0.0`   | Listen on available interfaces. |
| `127.0.0.1` | Listen only on localhost.       |
| `localhost` | Local development host.         |

## Port

The port controls where clients connect.

```dotenv
WEBSOCKET_PORT=9090
```

Example client URL:

```txt
ws://localhost:9090
```

The low-level server rejects invalid ports. Ports below `1024`, except `0`, are treated as invalid by the current WebSocket server implementation. :contentReference[oaicite:1]{index=1}

## Maximum message size

The maximum message size protects the server from oversized frames.

```dotenv
WEBSOCKET_MAX_MESSAGE_SIZE=65536
```

This value is read into:

```cpp
Config::maxMessageSize
```

The default is:

```txt
65536 bytes
```

That is:

```txt
64 KiB
```

When the value is read from core config, the implementation enforces a minimum of `1024` bytes. :contentReference[oaicite:2]{index=2}

## Idle timeout

The idle timeout controls how long a connection can stay inactive before it is considered idle.

```dotenv
WEBSOCKET_IDLE_TIMEOUT=60
```

This value is read into:

```cpp
Config::idleTimeout
```

The value is expressed in seconds.

Use `0` to disable idle timeout:

```dotenv
WEBSOCKET_IDLE_TIMEOUT=0
```

When the configured value is less than or equal to `0`, the runtime config stores `std::chrono::seconds::zero()`. :contentReference[oaicite:3]{index=3}

## Per-message deflate

Per-message deflate controls compression support.

```dotenv
WEBSOCKET_ENABLE_DEFLATE=true
```

This value is read into:

```cpp
Config::enablePerMessageDeflate
```

Use `false` if you want to disable compression behavior:

```dotenv
WEBSOCKET_ENABLE_DEFLATE=false
```

## Ping interval

The ping interval controls heartbeat behavior.

```dotenv
WEBSOCKET_PING_INTERVAL=30
```

This value is read into:

```cpp
Config::pingInterval
```

The value is expressed in seconds.

Use `0` to disable periodic ping:

```dotenv
WEBSOCKET_PING_INTERVAL=0
```

When the configured value is less than or equal to `0`, the runtime config stores `std::chrono::seconds::zero()`. :contentReference[oaicite:4]{index=4}

## Automatic ping/pong

Automatic ping/pong lets the runtime handle WebSocket ping and pong frames.

```dotenv
WEBSOCKET_AUTO_PING_PONG=true
```

This value is read into:

```cpp
Config::autoPingPong
```

Use `false` when you want manual control:

```dotenv
WEBSOCKET_AUTO_PING_PONG=false
```

For most applications, keep this enabled.

## Load config from `.env`

The WebSocket server receives the core configuration object.

```cpp
#include <memory>

#include <vix/config/Config.hpp>
#include <vix/executor/RuntimeExecutor.hpp>
#include <vix/websocket.hpp>

int main()
{
  vix::config::Config config{".env"};

  auto executor =
      std::make_shared<vix::executor::RuntimeExecutor>(4);

  vix::websocket::Server ws{config, executor};

  ws.start();

  return 0;
}
```

## Derive WebSocket config manually

You can derive the WebSocket runtime config manually when you need to inspect the final values.

```cpp
#include <vix/config/Config.hpp>
#include <vix/print.hpp>
#include <vix/websocket/config.hpp>

int main()
{
  vix::config::Config core{".env"};

  vix::websocket::Config ws =
      vix::websocket::Config::from_core(core);

  vix::print("max message size:", ws.maxMessageSize);
  vix::print("idle timeout:", ws.idleTimeout.count());
  vix::print("ping interval:", ws.pingInterval.count());

  return 0;
}
```

## Configuration in attached runtime

When WebSocket is attached to an HTTP app, the WebSocket server can reuse the app configuration.

```cpp
#include <memory>

#include <vix.hpp>
#include <vix/websocket.hpp>

int main()
{
  vix::App app;

  auto executor =
      std::make_shared<vix::executor::RuntimeExecutor>(4);

  vix::websocket::Server ws{app.config(), executor};

  vix::websocket::AttachedRuntime runtime{app, ws, executor};

  app.run(8080);

  return 0;
}
```

In this model:

```txt
vix::App
  -> owns core config

vix::websocket::Server
  -> reads WebSocket config from app.config()
```

## Example production `.env`

```dotenv
# HTTP
SERVER_PORT=8080
SERVER_IO_THREADS=0
SERVER_REQUEST_TIMEOUT=2000
SERVER_SESSION_TIMEOUT_SEC=20

# WebSocket
WEBSOCKET_HOST=0.0.0.0
WEBSOCKET_PORT=9090
WEBSOCKET_MAX_MESSAGE_SIZE=65536
WEBSOCKET_IDLE_TIMEOUT=60
WEBSOCKET_ENABLE_DEFLATE=true
WEBSOCKET_PING_INTERVAL=30
WEBSOCKET_AUTO_PING_PONG=true

# Logging
LOGGING_ASYNC=true
LOGGING_QUEUE_MAX=20000
LOGGING_DROP_ON_OVERFLOW=true
```

## Recommended defaults

For most applications, start with:

```dotenv
WEBSOCKET_HOST=0.0.0.0
WEBSOCKET_PORT=9090
WEBSOCKET_MAX_MESSAGE_SIZE=65536
WEBSOCKET_IDLE_TIMEOUT=60
WEBSOCKET_ENABLE_DEFLATE=true
WEBSOCKET_PING_INTERVAL=30
WEBSOCKET_AUTO_PING_PONG=true
```

These defaults are good for:

- chat
- notifications
- dashboards
- local agents
- browser clients
- realtime application events

## Larger messages

If your application sends larger JSON payloads, increase the maximum message size.

```dotenv
WEBSOCKET_MAX_MESSAGE_SIZE=262144
```

This allows messages up to:

```txt
256 KiB
```

Be careful with very large message limits. WebSocket is best for realtime messages, not large file transfers.

For large files, prefer HTTP upload or a dedicated storage flow.

## Shorter heartbeat

If you want faster detection of dead connections, reduce the ping interval.

```dotenv
WEBSOCKET_PING_INTERVAL=10
```

This sends heartbeat pings more often.

Use this for:

- realtime dashboards
- presence tracking
- chat presence
- low-latency connection monitoring

## Longer idle timeout

If your users may stay connected but inactive for a long time, increase the idle timeout.

```dotenv
WEBSOCKET_IDLE_TIMEOUT=300
```

This allows inactive connections to remain open for five minutes.

## Disable idle timeout

For local development or controlled internal services:

```dotenv
WEBSOCKET_IDLE_TIMEOUT=0
```

This disables idle timeout behavior.

Do not use this blindly in public production systems unless you also have other connection limits.

## Disable compression

Compression can reduce bandwidth but may add CPU cost.

Disable it when:

- messages are already small
- CPU is more important than bandwidth
- you want simpler behavior during debugging

```dotenv
WEBSOCKET_ENABLE_DEFLATE=false
```

## Disable auto ping/pong

Most applications should keep automatic ping/pong enabled.

Disable it only if you are implementing custom heartbeat behavior.

```dotenv
WEBSOCKET_AUTO_PING_PONG=false
```

## Configuration flow

The configuration flow is:

```txt
.env
  -> vix::config::Config
  -> vix::websocket::Config::from_core(...)
  -> LowLevelServer
  -> Session
```

The server uses the core config for host and port.

The WebSocket runtime config controls per-session protocol behavior.

## Common issues

### Server does not accept external connections

Check the host.

```dotenv
WEBSOCKET_HOST=0.0.0.0
```

If the host is `127.0.0.1`, only local connections can connect.

### Browser cannot connect

Check the URL:

```txt
ws://localhost:9090
```

Also verify the port:

```dotenv
WEBSOCKET_PORT=9090
```

### Messages are rejected or connection closes

Check the message size:

```dotenv
WEBSOCKET_MAX_MESSAGE_SIZE=65536
```

If your payload is bigger than the configured limit, increase the limit carefully.

### Idle clients disconnect

Check the idle timeout:

```dotenv
WEBSOCKET_IDLE_TIMEOUT=60
```

Increase it or set it to `0` for development.

### Heartbeat is too slow

Reduce the ping interval:

```dotenv
WEBSOCKET_PING_INTERVAL=10
```

### Heartbeat is disabled

Check that the ping interval is greater than `0`:

```dotenv
WEBSOCKET_PING_INTERVAL=30
```

## Next steps

Continue with:

- [Server](./server.md)
- [Session](./session.md)
- [Messages](./messages.md)
- [Attached runtime](./attached-runtime.md)
