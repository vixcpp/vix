# Long polling

This page explains the long-polling fallback system in the Vix WebSocket module.

Use it when you want HTTP clients to receive realtime-like messages even when WebSocket is unavailable.

## Header

```cpp
#include <vix/websocket/LongPolling.hpp>
#include <vix/websocket/LongPollingBridge.hpp>
```

Or use the umbrella header:

```cpp
#include <vix/websocket.hpp>
```

## What long polling provides

Long polling provides an HTTP-compatible fallback for realtime delivery.

It is useful when:

- WebSocket is blocked by a proxy
- WebSocket is not available in the client environment
- clients can only use HTTP
- a firewall prevents persistent WebSocket connections
- you want progressive realtime support

The WebSocket module provides two main pieces:

```cpp
vix::websocket::LongPollingManager
vix::websocket::LongPollingBridge
```

## Basic model

The basic model is:

```txt
HTTP client
  -> polls messages
  -> receives queued messages

HTTP client
  -> sends message
  -> message is queued or forwarded

WebSocket server
  -> receives typed message
  -> forwards to long-polling bridge
  -> bridge buffers message for HTTP clients
```

Long polling does not replace WebSocket.

It provides a fallback path.

## Main types

| Type                 | Purpose                                               |
| -------------------- | ----------------------------------------------------- |
| `LongPollingSession` | Stores buffered messages for one polling session.     |
| `LongPollingManager` | Manages long-polling sessions and message buffers.    |
| `LongPollingBridge`  | Bridges WebSocket messages and long-polling sessions. |

## LongPollingSession

`LongPollingSession` represents one HTTP polling session.

It stores:

```txt
session id
last activity time
FIFO message buffer
```

Conceptually:

```cpp
struct LongPollingSession
{
  std::string id;
  std::chrono::steady_clock::time_point lastSeen;
  std::deque<JsonMessage> buffer;
};
```

A session can:

- refresh its last activity time
- detect expiration
- enqueue messages
- drain messages

## Session id

Each long-polling client uses a session id.

Example:

```txt
session_id=browser-123
session_id=user:42
session_id=room:general
```

The session id is used to find the message buffer for that client or channel.

## Message buffer

Each session has a bounded FIFO buffer.

```txt
push message A
push message B
push message C

poll
  -> returns A, B, C
  -> buffer is drained
```

If the buffer grows beyond the configured maximum size, old messages are dropped first.

This prevents unbounded memory growth.

## LongPollingManager

`LongPollingManager` is the thread-safe manager for polling sessions.

It handles:

- session creation
- message enqueue
- message polling
- session expiration
- buffer size limits
- optional metrics updates

Constructor shape:

```cpp
vix::websocket::LongPollingManager manager{
    std::chrono::seconds{60},
    256,
    &metrics};
```

Arguments:

| Argument              | Purpose                                   |
| --------------------- | ----------------------------------------- |
| `sessionTtl`          | Inactivity time before a session expires. |
| `maxBufferPerSession` | Maximum queued messages per session.      |
| `metrics`             | Optional metrics collector.               |

## Create a manager

```cpp
vix::websocket::WebSocketMetrics metrics;

vix::websocket::LongPollingManager manager{
    std::chrono::seconds{60},
    256,
    &metrics};
```

This creates a manager with:

```txt
session TTL = 60 seconds
max buffer per session = 256 messages
metrics enabled
```

## Push a message

Use `push_to(...)` to enqueue a message for a session.

```cpp
vix::websocket::JsonMessage message;
message.kind = "event";
message.room = "general";
message.type = "chat.message";
message.payload = {
  {"text", "Hello"}
};

manager.push_to("room:general", message);
```

This stores the message in the buffer for:

```txt
room:general
```

## Poll messages

Use `poll(...)` to drain messages for a session.

```cpp
auto messages = manager.poll("room:general", 50, true);
```

Arguments:

| Argument          | Purpose                                           |
| ----------------- | ------------------------------------------------- |
| `sessionId`       | Session buffer to read from.                      |
| `maxMessages`     | Maximum messages to return.                       |
| `createIfMissing` | Whether to create a session if it does not exist. |

The returned messages are removed from the buffer.

## Poll without creating

```cpp
auto messages = manager.poll("room:general", 50, false);
```

If the session does not exist, this returns an empty vector.

## Buffer size

Use `buffer_size(...)` to check how many messages are buffered for a session.

```cpp
std::size_t size = manager.buffer_size("room:general");
```

This is useful for diagnostics and tests.

## Session count

Use `session_count()` to check how many long-polling sessions are active.

```cpp
std::size_t count = manager.session_count();
```

## Sweep expired sessions

The manager can remove expired sessions.

Conceptually:

```txt
current time
  -> compare with session.lastSeen
  -> remove sessions older than TTL
```

This prevents inactive clients from keeping memory forever.

## LongPollingBridge

`LongPollingBridge` connects WebSocket messages to long-polling sessions.

It can:

- receive WebSocket `JsonMessage` events
- resolve which polling session should receive them
- push messages into the manager
- poll buffered messages
- receive HTTP-originated messages
- optionally forward HTTP-originated messages back to WebSocket clients

## Create a bridge with an existing manager

```cpp
vix::websocket::LongPollingBridge bridge{manager};
```

This uses the manager you created.

## Create a bridge that owns its manager

```cpp
vix::websocket::WebSocketMetrics metrics;

vix::websocket::LongPollingBridge bridge{
    &metrics,
    std::chrono::seconds{60},
    256};
```

This creates an internal `LongPollingManager`.

## Default resolver

The bridge uses a resolver to decide where a message should be buffered.

The default behavior is:

```txt
if message.room is not empty:
  session id = "room:" + message.room
else:
  session id = "broadcast"
```

Example:

```json
{
  "room": "general",
  "type": "chat.message",
  "payload": {
    "text": "Hello"
  }
}
```

is buffered under:

```txt
room:general
```

A message without a room is buffered under:

```txt
broadcast
```

## Custom resolver

You can provide a custom resolver.

```cpp
vix::websocket::LongPollingBridge bridge{
    manager,
    [](const vix::websocket::JsonMessage &message)
    {
      if (!message.room.empty())
      {
        return "room:" + message.room;
      }

      return std::string{"broadcast"};
    }};
```

Use a custom resolver when you want to route by:

- user id
- tenant id
- project id
- message type
- custom channel
- application-specific sharding

## Handle WebSocket messages

Use `on_ws_message(...)` to push a WebSocket message into long-polling buffers.

```cpp
bridge.on_ws_message(message);
```

Flow:

```txt
WebSocket message
  -> LongPollingBridge::on_ws_message
  -> resolve session id
  -> LongPollingManager::push_to
```

## Poll through the bridge

Use `poll(...)` on the bridge.

```cpp
auto messages = bridge.poll("room:general", 50, true);
```

This calls the underlying manager.

## Send from HTTP

Use `send_from_http(...)` when a message comes from an HTTP endpoint.

```cpp
bridge.send_from_http("room:general", message);
```

This pushes the message to the long-polling buffer.

If an HTTP-to-WebSocket forward callback exists, it can also forward the message to WebSocket clients.

## HTTP to WebSocket forwarding

The bridge supports an optional callback:

```cpp
using HttpToWsForward =
    std::function<void(const JsonMessage &)>;
```

This is useful for `/ws/send`.

Conceptually:

```txt
POST /ws/send
  -> parse JsonMessage
  -> bridge.send_from_http(...)
  -> enqueue for long-polling clients
  -> optionally forward to WebSocket clients
```

## Attach bridge to Server

The WebSocket server can attach a long-polling bridge.

```cpp
vix::websocket::WebSocketMetrics metrics;

vix::websocket::LongPollingBridge bridge{
    &metrics,
    std::chrono::seconds{60},
    256};

ws.attach_long_polling_bridge(&bridge);
```

After this, parsed WebSocket typed messages can be forwarded to the bridge.

The flow becomes:

```txt
Session receives text frame
  -> Server parses JsonMessage
  -> LongPollingBridge receives message
  -> LongPollingManager buffers message
  -> HTTP clients poll message
```

## Common HTTP endpoints

Long-polling is usually exposed through HTTP endpoints:

```txt
GET  /ws/poll
POST /ws/send
```

`GET /ws/poll` lets clients receive queued messages.

`POST /ws/send` lets clients send typed messages through HTTP.

## GET /ws/poll

A polling request usually includes:

```txt
session_id
max
```

Example:

```txt
GET /ws/poll?session_id=room:general&max=50
```

Expected response shape:

```json
[
  {
    "kind": "event",
    "room": "general",
    "type": "chat.message",
    "payload": {
      "text": "Hello"
    }
  }
]
```

## POST /ws/send

A send request usually contains a typed JSON message.

```json
{
  "session_id": "room:general",
  "room": "general",
  "type": "chat.message",
  "payload": {
    "text": "Hello from HTTP"
  }
}
```

Expected behavior:

```txt
HTTP body
  -> parsed as JsonMessage
  -> pushed into long-polling buffer
  -> optionally forwarded to WebSocket clients
```

## Browser polling example

```js
async function poll() {
  const response = await fetch("/ws/poll?session_id=room:general&max=50");
  const messages = await response.json();

  for (const message of messages) {
    console.log("message:", message);
  }

  setTimeout(poll, 1000);
}

poll();
```

## Browser send example

```js
await fetch("/ws/send", {
  method: "POST",
  headers: {
    "Content-Type": "application/json",
  },
  body: JSON.stringify({
    session_id: "room:general",
    room: "general",
    type: "chat.message",
    payload: {
      text: "Hello from HTTP",
    },
  }),
});
```

## Metrics

Long-polling can update `WebSocketMetrics`.

Relevant metrics include:

| Metric                       | Meaning                               |
| ---------------------------- | ------------------------------------- |
| `lp_sessions_total`          | Total long-polling sessions created.  |
| `lp_sessions_active`         | Current active long-polling sessions. |
| `lp_polls_total`             | Total poll calls served.              |
| `lp_messages_buffered`       | Current buffered messages.            |
| `lp_messages_enqueued_total` | Total messages enqueued.              |
| `lp_messages_drained_total`  | Total messages drained.               |

Example:

```cpp
vix::websocket::WebSocketMetrics metrics;

vix::websocket::LongPollingManager manager{
    std::chrono::seconds{60},
    256,
    &metrics};
```

## Buffer limits

Each session has a maximum buffer size.

```cpp
vix::websocket::LongPollingManager manager{
    std::chrono::seconds{60},
    256,
    &metrics};
```

Here, each session can store up to:

```txt
256 messages
```

If more messages arrive, the oldest messages are dropped.

This protects memory usage.

## Session TTL

The session TTL controls when inactive polling sessions expire.

```cpp
std::chrono::seconds{60}
```

If a client stops polling, the session can be removed after the TTL.

This prevents inactive HTTP clients from keeping buffers forever.

## WebSocket and long-polling together

A complete realtime flow can support both:

```txt
WebSocket clients
  -> persistent connection
  -> receive messages immediately

HTTP fallback clients
  -> poll /ws/poll
  -> receive queued messages
```

Both can share the same message model:

```cpp
vix::websocket::JsonMessage
```

## Recommended use

Use WebSocket as the primary realtime transport.

Use long-polling as fallback.

```txt
try WebSocket first
  -> if unavailable, use long-polling
```

This gives the best realtime experience while still supporting restricted environments.

## Best practices

Use clear session ids.

Use room-based session ids for shared channels.

Keep buffers bounded.

Set a reasonable session TTL.

Use metrics in production.

Use typed JSON messages for long-polling payloads.

Use long-polling as a fallback, not the default realtime path.

Do not store large payloads in polling buffers.

## Common mistakes

### Unbounded buffers

Do not allow unlimited queued messages.

Use a maximum buffer size per session.

```cpp
vix::websocket::LongPollingManager manager{
    std::chrono::seconds{60},
    256,
    &metrics};
```

### No session expiration

Always use a TTL for inactive sessions.

```cpp
std::chrono::seconds{60}
```

### Confusing session id and room

A session id identifies the long-polling buffer.

A room identifies the logical WebSocket room.

The default bridge maps:

```txt
room = general
  -> session id = room:general
```

### Using long-polling for large files

Do not buffer large files through long-polling.

Use HTTP download/upload endpoints.

### Replacing WebSocket with polling

Long-polling is a fallback.

Use WebSocket when persistent connections are available.

## Next steps

Continue with:

- [HTTP API](./http-api.md)
- [Messages](./messages.md)
- [Rooms and Broadcasting](./rooms-and-broadcasting.md)
- [Metrics](./metrics.md)
- [Shutdown](./shutdown.md)
