# Concepts

This page explains the main concepts behind the Vix WebSocket module.

Use it when you want to understand how the WebSocket server, sessions, router, typed messages, long-polling fallback, metrics, persistence, and HTTP integration fit together.

## Mental model

The WebSocket module gives Vix applications a realtime communication layer.

The basic model is:

```txt
Client
  -> TCP connection
  -> HTTP Upgrade
  -> WebSocket Session
  -> Router
  -> Server callbacks
  -> application code
```

The server accepts a TCP connection, upgrades it to WebSocket, creates a session, then dispatches events to user callbacks.

For applications that combine HTTP and WebSocket:

```txt
vix::App
  -> HTTP server
  -> routes
  -> middleware
  -> static files

vix::websocket::Server
  -> WebSocket listener
  -> sessions
  -> messages
  -> rooms

AttachedRuntime
  -> connects both
  -> shares RuntimeExecutor
  -> coordinates shutdown
```

## Core building blocks

The public WebSocket module is built around these types:

| Type                                 | Purpose                                            |
| ------------------------------------ | -------------------------------------------------- |
| `vix::websocket::Server`             | High-level WebSocket server.                       |
| `vix::websocket::LowLevelServer`     | Low-level async listener and connection engine.    |
| `vix::websocket::Session`            | One connected WebSocket client.                    |
| `vix::websocket::Router`             | Dispatches open, close, error, and message events. |
| `vix::websocket::Client`             | Native WebSocket client.                           |
| `vix::websocket::Config`             | WebSocket runtime configuration.                   |
| `vix::websocket::JsonMessage`        | Typed JSON message model.                          |
| `vix::websocket::LongPollingManager` | HTTP polling session manager.                      |
| `vix::websocket::LongPollingBridge`  | Bridge between WebSocket and long-polling.         |
| `vix::websocket::WebSocketMetrics`   | Prometheus-style counters and gauges.              |
| `vix::websocket::IMessageStore`      | Abstract message persistence interface.            |
| `vix::websocket::SqliteMessageStore` | SQLite-backed message store.                       |
| `vix::websocket::AttachedRuntime`    | HTTP + WebSocket runtime integration.              |

## High-level server

`vix::websocket::Server` is the main server API most applications use.

It wraps the low-level engine and provides:

- connection callbacks
- close callbacks
- error callbacks
- raw message callbacks
- typed message callbacks
- session tracking
- rooms
- broadcasting
- optional long-polling bridge integration

```cpp
vix::websocket::Server ws{config, executor};

ws.on_open([](vix::websocket::Session &session)
{
  (void)session;
});

ws.on_message(
  [](vix::websocket::Session &session, const std::string &message)
  {
    session.send_text("echo: " + message);
  });

ws.start();
```

The high-level server is the best entry point when you want to build realtime application behavior.

## Low-level server

`LowLevelServer` owns the transport-level WebSocket server engine.

It is responsible for:

- creating the native Vix TCP listener
- binding to the configured host and port
- starting I/O worker threads
- accepting TCP connections
- creating WebSocket sessions
- starting session tasks
- stopping the listener
- joining internal threads

Conceptually:

```txt
Server
  -> LowLevelServer
    -> io_context
    -> TCP listener
    -> accept loop
    -> Session
```

Most applications do not use `LowLevelServer` directly.

They use:

```cpp
vix::websocket::Server ws{config, executor};
```

## Session

`vix::websocket::Session` represents one connected client.

A session manages:

- HTTP Upgrade handshake
- frame reading
- frame writing
- text messages
- binary messages
- close frames
- ping/pong handling
- idle timeout
- heartbeat lifecycle
- error dispatch
- immediate shutdown during server stop

Application callbacks receive a session reference:

```cpp
ws.on_message(
  [](vix::websocket::Session &session, const std::string &message)
  {
    session.send_text(message);
  });
```

The session is the object you use to send data back to the connected client.

## Router

`vix::websocket::Router` is the event dispatch layer.

It supports four core events:

```cpp
router.on_open(...);
router.on_close(...);
router.on_error(...);
router.on_message(...);
```

The router separates low-level session events from application callbacks.

The flow is:

```txt
Session event
  -> Router
  -> Server callback
  -> user code
```

Most users register callbacks on `Server`, but internally the server connects those callbacks to the router.

## Raw messages

A raw WebSocket message is a text frame received from a client.

```cpp
ws.on_message(
  [](vix::websocket::Session &session, const std::string &message)
  {
    session.send_text("received: " + message);
  });
```

Raw messages are useful when:

- the client sends plain text
- the application defines its own format
- you want simple echo behavior
- you want to parse the payload manually

## Typed JSON messages

The module also supports a typed JSON convention.

A typed message has this shape:

```json
{
  "type": "chat.message",
  "payload": {
    "text": "Hello"
  }
}
```

The server parses raw text as a `JsonMessage`.

If parsing succeeds, the typed message handler is called:

```cpp
ws.on_typed_message(
  [](vix::websocket::Session &session,
     const std::string &type,
     const vix::json::kvs &payload)
  {
    (void)payload;

    if (type == "chat.message")
    {
      session.send_text("chat message received");
    }
  });
```

Typed messages are useful for application protocols.

Instead of creating one WebSocket URL for every event, you can keep one connection and route by message type:

```txt
chat.message
notification.created
presence.update
room.join
room.leave
```

## JsonMessage

`JsonMessage` is the module-level typed message structure.

It can carry fields such as:

```txt
id
kind
room
type
ts
payload
```

This makes it useful for:

- realtime events
- chat messages
- room-based delivery
- persistence
- replay
- long-polling fallback
- HTTP bridge APIs

The common shape is:

```json
{
  "id": "optional-id",
  "kind": "event",
  "room": "general",
  "type": "chat.message",
  "ts": "2026-05-17T10:00:00Z",
  "payload": {
    "text": "Hello"
  }
}
```

## Rooms

Rooms allow the server to group sessions.

The model is:

```txt
Session A -> room: general
Session B -> room: general
Session C -> room: support
```

Then the server can broadcast to one room instead of every connected client.

This is useful for:

- chat rooms
- project rooms
- user-specific channels
- team dashboards
- tenant-specific events
- multiplayer sessions

Conceptually:

```txt
broadcast to room "general"
  -> send to all sessions in that room
```

## Broadcasting

Broadcasting means sending one message to many sessions.

There are two common forms:

```txt
global broadcast
  -> all active sessions

room broadcast
  -> sessions in a specific room
```

Broadcasting is useful for:

- notifications
- live dashboards
- chat rooms
- presence updates
- server events
- collaborative updates

## Client

`vix::websocket::Client` is the native WebSocket client.

It provides:

- async DNS/TCP connection
- HTTP Upgrade handshake
- text frame sending
- message callbacks
- close callbacks
- error callbacks
- typed JSON sending
- heartbeat ping
- optional auto-reconnect

```cpp
auto client = vix::websocket::Client::create(
    "127.0.0.1",
    "9090",
    "/");

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

The client is useful for:

- tests
- internal services
- local agents
- CLI tools
- integration checks
- realtime workers

## Runtime executor

The WebSocket module uses `vix::executor::RuntimeExecutor`.

The executor is shared with the WebSocket server and can also be shared with the HTTP app.

```cpp
auto executor =
    std::make_shared<vix::executor::RuntimeExecutor>(4);

vix::websocket::Server ws{config, executor};
```

The executor provides the runtime foundation for scheduled work and safe lifecycle management.

When WebSocket is attached to HTTP, both can use the same executor:

```txt
HTTP App
  -> RuntimeExecutor

WebSocket Server
  -> same RuntimeExecutor
```

This keeps the application lifecycle easier to coordinate.

## Async I/O

The WebSocket engine uses Vix async primitives.

The low-level model is:

```txt
io_context
  -> TCP listener
  -> async accept
  -> Session
  -> async read
  -> async write
```

Each accepted client is handled as an asynchronous session.

This lets the server keep many connections active without blocking one thread per connection.

## Configuration

WebSocket configuration is derived from the core Vix config.

The WebSocket-specific config includes:

| Field                     | Purpose                                  |
| ------------------------- | ---------------------------------------- |
| `maxMessageSize`          | Maximum accepted message size.           |
| `idleTimeout`             | Idle connection timeout.                 |
| `enablePerMessageDeflate` | Enables per-message compression support. |
| `autoPingPong`            | Automatically handles ping/pong frames.  |
| `pingInterval`            | Interval for heartbeat ping frames.      |

Example `.env` values:

```dotenv
WEBSOCKET_MAX_MESSAGE_SIZE=65536
WEBSOCKET_IDLE_TIMEOUT=60
WEBSOCKET_ENABLE_DEFLATE=true
WEBSOCKET_PING_INTERVAL=30
WEBSOCKET_AUTO_PING_PONG=true
```

The server host and port are usually configured with:

```dotenv
WEBSOCKET_HOST=0.0.0.0
WEBSOCKET_PORT=9090
```

## Long-polling fallback

Long-polling provides an HTTP fallback when WebSocket is not available.

The model is:

```txt
Client polls HTTP
  -> server returns queued messages

Client sends HTTP POST
  -> server queues or forwards message
```

The module provides:

```cpp
vix::websocket::LongPollingSession
vix::websocket::LongPollingManager
vix::websocket::LongPollingBridge
```

`LongPollingManager` manages session buffers.

`LongPollingBridge` connects WebSocket messages to long-polling sessions.

Typical HTTP fallback endpoints are:

```txt
GET  /ws/poll
POST /ws/send
```

## LongPollingSession

A long-polling session stores:

```txt
session id
last activity time
bounded FIFO message buffer
```

It can:

- update its last activity time
- detect expiration by TTL
- enqueue messages
- drop old messages when the buffer is full
- drain messages during polling

## LongPollingManager

`LongPollingManager` is thread-safe.

It handles:

- creating sessions
- pushing messages to a session
- polling messages from a session
- limiting buffer size
- sweeping expired sessions
- updating metrics

This gives HTTP clients a way to receive realtime-like events through repeated polling.

## LongPollingBridge

`LongPollingBridge` connects WebSocket and long-polling.

The default session resolver is:

```txt
if message has room:
  session id = "room:" + room
else:
  session id = "broadcast"
```

This means WebSocket events can be mirrored into long-polling buffers.

The bridge can also forward HTTP-originated messages back to WebSocket clients.

## HTTP API

The WebSocket module includes HTTP helper APIs for bridge behavior.

The common endpoints are:

```txt
GET  /ws/poll
POST /ws/send
```

`GET /ws/poll` lets HTTP clients pull queued messages.

`POST /ws/send` lets HTTP clients push a typed message into the system.

This is useful when you want one realtime layer that can work with both WebSocket and HTTP fallback clients.

## Metrics

`WebSocketMetrics` exposes counters and gauges for WebSocket and long-polling.

It tracks:

| Metric area  | Examples                                               |
| ------------ | ------------------------------------------------------ |
| Connections  | total accepted connections, active connections         |
| Messages     | inbound total, outbound total                          |
| Errors       | total errors                                           |
| Long-polling | sessions total, active sessions, poll calls            |
| Buffers      | buffered messages, enqueued messages, drained messages |

Metrics can be rendered in Prometheus text format:

```cpp
vix::websocket::WebSocketMetrics metrics;

std::string output = metrics.render_prometheus();
```

A minimal HTTP exporter can expose:

```txt
GET /metrics
```

## Message persistence

The module defines:

```cpp
vix::websocket::IMessageStore
```

This interface provides:

```cpp
append(...)
list_by_room(...)
replay_from(...)
```

Use it when your application needs:

- chat history
- durable events
- room history
- replay after reconnect
- persistent realtime logs

## SQLite message store

`SqliteMessageStore` implements `IMessageStore` using SQLite.

It stores messages with fields such as:

```txt
id
kind
room
type
ts
payload_json
```

It enables WAL mode and creates the message table if needed.

```cpp
vix::websocket::SqliteMessageStore store{"messages.db"};
```

This makes it a practical default for local development and durable message history.

## Attached runtime

`AttachedRuntime` connects a WebSocket server to a `vix::App`.

It does three important things:

```txt
1. starts the WebSocket server
2. registers HTTP shutdown coordination
3. finalizes WebSocket and executor shutdown safely
```

Example:

```cpp
vix::App app;

auto executor =
    std::make_shared<vix::executor::RuntimeExecutor>(4);

vix::websocket::Server ws{app.config(), executor};

vix::websocket::AttachedRuntime runtime{app, ws, executor};

app.run(8080);
```

This is the recommended model when one application needs both HTTP and WebSocket.

## Shutdown model

Shutdown is split into two phases.

First, request a non-blocking stop:

```txt
request_stop
  -> ws.stop_async()
```

Then finalize blocking shutdown later:

```txt
finalize_shutdown
  -> ws.stop()
  -> executor.stop()
```

This avoids blocking inside HTTP shutdown callbacks.

The rule is:

```txt
Do not perform final blocking WebSocket shutdown inside the HTTP shutdown callback.
```

## OpenAPI documentation

The module can register OpenAPI documentation for WebSocket-related routes.

Documented routes include:

```txt
GET  /ws
GET  /ws/poll
POST /ws/send
GET  /metrics
```

The WebSocket route is documented as an upgrade endpoint.

Swagger UI cannot fully execute the `101 Switching Protocols` WebSocket upgrade, but documenting the route still helps users discover the realtime API.

## Common application architecture

A complete realtime Vix application can look like this:

```txt
vix::App
  -> HTTP routes
  -> static frontend
  -> OpenAPI docs

vix::websocket::Server
  -> WebSocket clients
  -> sessions
  -> rooms
  -> broadcasts

LongPollingBridge
  -> HTTP fallback clients

SqliteMessageStore
  -> durable message history

WebSocketMetrics
  -> Prometheus metrics

AttachedRuntime
  -> shared lifecycle
  -> safe shutdown
```

## How data flows

A typical typed WebSocket message flow:

```txt
Browser
  -> sends JSON text frame
  -> Session reads frame
  -> Router dispatches message
  -> Server parses JsonMessage
  -> typed handler runs
  -> handler broadcasts or replies
```

With persistence:

```txt
typed message
  -> validate
  -> store.append(message)
  -> broadcast to room
```

With long-polling fallback:

```txt
typed message
  -> LongPollingBridge
  -> LongPollingManager
  -> HTTP clients poll queued messages
```

## When to use raw messages

Use raw messages when:

- the protocol is simple
- the client sends plain text
- you want manual parsing
- you are building a quick prototype
- you need full control over the payload

```cpp
ws.on_message(
  [](vix::websocket::Session &session, const std::string &message)
  {
    session.send_text("received: " + message);
  });
```

## When to use typed messages

Use typed messages when:

- the application has multiple realtime events
- you want a stable message contract
- clients need to route by event type
- you want persistence or replay
- you want WebSocket and HTTP fallback to share one message model

```json
{
  "type": "notification.created",
  "payload": {
    "title": "New order"
  }
}
```

## When to use rooms

Use rooms when:

- only some users should receive an event
- messages belong to a chat room
- updates belong to one project
- events belong to one tenant
- dashboards subscribe to specific channels

Rooms prevent sending every event to every connected client.

## When to use long-polling

Use long-polling when:

- WebSocket is blocked
- the client is HTTP-only
- you need fallback behavior
- proxies or firewalls are unreliable
- you want progressive realtime support

It should be treated as a fallback transport, not as the main realtime path.

## When to use message persistence

Use message persistence when:

- users can reconnect and request missed messages
- chat history matters
- events must be replayable
- room history must be queryable
- the system needs local durable state

For simple notifications, in-memory delivery may be enough.

For serious chat or replay, use a message store.

## Next steps

Continue with:

- [Configuration](./configuration.md)
- [Server](./server.md)
- [Session](./session.md)
- [Messages](./messages.md)
- [Attached runtime](./attached-runtime.md)
