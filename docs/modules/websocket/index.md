# WebSocket

The `websocket` module provides native realtime communication for Vix applications.

It is designed for applications that need persistent connections, typed messages, rooms, broadcasting, fallback delivery, metrics, and integration with the Vix runtime.

```cpp
#include <vix/websocket.hpp>
```

## What the WebSocket module provides

The module includes the main building blocks needed to build realtime systems in C++:

- a native WebSocket server
- a native WebSocket client
- per-connection sessions
- event routing
- typed JSON messages
- rooms and broadcasting
- long-polling fallback
- HTTP bridge APIs
- Prometheus-style metrics
- message persistence interfaces
- SQLite-backed message storage
- OpenAPI documentation helpers
- integration with `vix::App`
- shared runtime execution through `RuntimeExecutor`

The goal is to make realtime C++ applications easier to build while keeping the architecture explicit.

## Header

Use the umbrella header:

```cpp
#include <vix/websocket.hpp>
```

This includes the main public WebSocket APIs:

```cpp
#include <vix/websocket/config.hpp>
#include <vix/websocket/protocol.hpp>
#include <vix/websocket/client.hpp>
#include <vix/websocket/server.hpp>
#include <vix/websocket/session.hpp>
#include <vix/websocket/router.hpp>
#include <vix/websocket/MessageStore.hpp>
#include <vix/websocket/SqliteMessageStore.hpp>
#include <vix/websocket/Metrics.hpp>
#include <vix/websocket/App.hpp>
#include <vix/websocket/HttpApi.hpp>
#include <vix/websocket/LongPolling.hpp>
#include <vix/websocket/LongPollingBridge.hpp>
#include <vix/websocket/AttachedRuntime.hpp>
#include <vix/websocket/Runtime.hpp>
```

## Basic model

The WebSocket module is organized around this model:

```txt
Server
  -> LowLevelServer
  -> Session
  -> Router
  -> user callbacks
```

For applications that combine HTTP and WebSocket:

```txt
vix::App
  -> HTTP server

vix::websocket::Server
  -> WebSocket server

AttachedRuntime
  -> connects both
  -> shares RuntimeExecutor
  -> coordinates shutdown
```

## Server

`vix::websocket::Server` is the high-level WebSocket server.

It manages:

- the low-level WebSocket engine
- active sessions
- connection lifecycle callbacks
- raw message callbacks
- typed JSON message callbacks
- rooms
- broadcasts
- optional long-polling bridge integration

```cpp
vix::websocket::Server ws(config, executor);

ws.on_open([](vix::websocket::Session &session)
{
  (void)session;
});

ws.on_message([](vix::websocket::Session &session, const std::string &message)
{
  session.send_text("echo: " + message);
});

ws.start();
```

## Session

`vix::websocket::Session` represents one connected WebSocket client.

A session handles:

- HTTP Upgrade handshake
- WebSocket frame reads
- WebSocket frame writes
- text messages
- binary messages
- close handling
- heartbeat lifecycle
- idle timeout behavior
- router event dispatch

Application code usually receives a session inside callbacks:

```cpp
ws.on_message([](vix::websocket::Session &session, const std::string &message)
{
  session.send_text(message);
});
```

## Router

`vix::websocket::Router` dispatches WebSocket events to user callbacks.

It supports:

```cpp
router.on_open(...);
router.on_close(...);
router.on_error(...);
router.on_message(...);
```

The router is used internally by the server and session layers.

Most applications use the higher-level `Server` callbacks instead.

## Typed messages

The module supports a typed JSON message convention.

A message can look like this:

```json
{
  "type": "chat.message",
  "payload": {
    "text": "Hello"
  }
}
```

Typed handlers receive:

```cpp
Session &
const std::string &type
const vix::json::kvs &payload
```

Example:

```cpp
ws.on_typed_message(
  [](vix::websocket::Session &session,
     const std::string &type,
     const vix::json::kvs &payload)
  {
    (void)payload;

    if (type == "chat.message")
    {
      session.send_text("message received");
    }
  });
```

## Client

`vix::websocket::Client` is a native WebSocket client.

It provides:

- async TCP connect
- WebSocket HTTP Upgrade handshake
- text frame sending
- text frame receiving
- typed JSON message sending
- heartbeat ping
- optional auto-reconnect

```cpp
auto client = vix::websocket::Client::create("127.0.0.1", "9090");

client->on_open([client]()
{
  client->send_text("hello");
});

client->on_message([](const std::string &message)
{
  vix::print("received:", message);
});

client->connect();
```

## Long-polling fallback

The module also provides long-polling support for environments where WebSocket is unavailable.

The long-polling layer is built around:

```cpp
vix::websocket::LongPollingSession
vix::websocket::LongPollingManager
vix::websocket::LongPollingBridge
```

The bridge connects WebSocket messages to HTTP polling sessions.

This makes it possible to expose fallback endpoints such as:

```txt
GET  /ws/poll
POST /ws/send
```

## Metrics

`WebSocketMetrics` provides lightweight Prometheus-style counters and gauges.

It tracks values such as:

- total WebSocket connections
- active WebSocket connections
- inbound messages
- outbound messages
- errors
- long-polling sessions
- long-polling polls
- buffered long-polling messages

Metrics can be rendered as Prometheus text:

```cpp
vix::websocket::WebSocketMetrics metrics;

std::string output = metrics.render_prometheus();
```

## Message store

The module includes an abstract message persistence interface:

```cpp
vix::websocket::IMessageStore
```

It can be used for:

- chat history
- event replay
- room-based queries
- durable message storage

The SQLite implementation is:

```cpp
vix::websocket::SqliteMessageStore
```

It stores `JsonMessage` values in SQLite and enables WAL-friendly persistence.

## Attached runtime

`AttachedRuntime` connects a WebSocket server to an existing HTTP app.

It starts the WebSocket server, registers shutdown coordination, shares the executor, and keeps HTTP and WebSocket under one lifecycle model.

```cpp
vix::App app;

auto executor = std::make_shared<vix::executor::RuntimeExecutor>(4);

vix::websocket::Server ws(app.config(), executor);

vix::websocket::AttachedRuntime runtime(app, ws, executor);

app.run(8080);
```

The key rule is:

```txt
HTTP shutdown callback requests async WebSocket stop.
Final blocking shutdown happens from a safe control path.
```

This avoids blocking inside internal shutdown callbacks.

## Configuration

WebSocket settings are loaded from the core Vix configuration.

Common environment values:

```dotenv
WEBSOCKET_MAX_MESSAGE_SIZE=65536
WEBSOCKET_IDLE_TIMEOUT=60
WEBSOCKET_ENABLE_DEFLATE=true
WEBSOCKET_PING_INTERVAL=30
WEBSOCKET_AUTO_PING_PONG=true
```

The WebSocket config controls:

- maximum message size
- idle timeout
- per-message deflate
- ping interval
- automatic ping/pong behavior

## OpenAPI integration

The module can register documentation for WebSocket-related endpoints.

Documented routes include:

```txt
GET  /ws
GET  /ws/poll
POST /ws/send
GET  /metrics
```

The WebSocket upgrade route is documented for clients, even though Swagger UI cannot fully execute a WebSocket `101 Switching Protocols` upgrade.

## When to use this module

Use the WebSocket module when your application needs:

- realtime chat
- notifications
- live dashboards
- collaborative features
- multiplayer or realtime state updates
- event streaming
- browser-to-server persistent connections
- fallback delivery through HTTP polling
- room-based broadcasts
- durable message replay

## Documentation map

| Page                                                  | Purpose                                          |
| ----------------------------------------------------- | ------------------------------------------------ |
| [Quick Start](./quick-start.md)                       | Build a minimal WebSocket server.                |
| [Concepts](./concepts.md)                             | Understand the WebSocket module architecture.    |
| [Configuration](./configuration.md)                   | Configure limits, timeouts, ping, and deflate.   |
| [Server](./server.md)                                 | Use `vix::websocket::Server`.                    |
| [Session](./session.md)                               | Understand one WebSocket connection.             |
| [Router](./router.md)                                 | Dispatch open, close, error, and message events. |
| [Messages](./messages.md)                             | Use raw and typed JSON messages.                 |
| [Client](./client.md)                                 | Use the native WebSocket client.                 |
| [Rooms and Broadcasting](./rooms-and-broadcasting.md) | Broadcast globally or by room.                   |
| [Long polling](./long-polling.md)                     | Use HTTP fallback delivery.                      |
| [HTTP API](./http-api.md)                             | Expose WebSocket bridge endpoints through HTTP.  |
| [Attached runtime](./attached-runtime.md)             | Run HTTP and WebSocket together.                 |
| [Metrics](./metrics.md)                               | Export Prometheus-style metrics.                 |
| [Message store](./message-store.md)                   | Use the message persistence abstraction.         |
| [SQLite message store](./sqlite-message-store.md)     | Store messages in SQLite.                        |
| [OpenAPI](./openapi.md)                               | Register WebSocket API documentation.            |
| [Shutdown](./shutdown.md)                             | Stop WebSocket servers safely.                   |
| [API Reference](./api-reference.md)                   | Public API summary.                              |
