# Messages

This page explains how messages work in the Vix WebSocket module.

Use it when you want to understand raw text messages, typed JSON messages, payloads, rooms, message persistence, replay, and the connection between WebSocket and long-polling.

## Header

```cpp
#include <vix/websocket/protocol.hpp>
```

Or use the umbrella header:

```cpp
#include <vix/websocket.hpp>
```

## What messages provide

The WebSocket module supports two main message styles:

| Style               | Purpose                                                  |
| ------------------- | -------------------------------------------------------- |
| Raw text messages   | Simple text payloads received from WebSocket frames.     |
| Typed JSON messages | Structured application events with `type` and `payload`. |

Raw messages are useful for simple protocols.

Typed messages are better for real applications with multiple realtime events.

## Raw text messages

A raw message is the text payload received from a WebSocket text frame.

```cpp
ws.on_message(
  [](vix::websocket::Session &session, const std::string &message)
  {
    session.send_text("echo: " + message);
  });
```

Example client message:

```txt
hello
```

Example server response:

```txt
echo: hello
```

Use raw messages when:

- the protocol is simple
- the client sends plain text
- you want to parse the payload manually
- you are building a quick prototype
- you do not need event names

## Typed JSON messages

A typed message is a JSON object with a `type` and a `payload`.

```json
{
  "type": "chat.message",
  "payload": {
    "text": "Hello"
  }
}
```

The type identifies the application event.

The payload contains the event data.

## Typed message handler

Use `on_typed_message(...)` to receive typed messages.

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

The handler receives:

| Argument  | Purpose                         |
| --------- | ------------------------------- |
| `session` | The connected WebSocket client. |
| `type`    | The message type.               |
| `payload` | The structured payload.         |

## Typed message flow

The flow is:

```txt
client sends JSON text frame
  -> Session reads text frame
  -> Router dispatches raw message
  -> Server receives raw message
  -> Server parses JsonMessage
  -> Server calls typed handler
```

The session handles WebSocket frames.

The server handles typed message parsing.

## JsonMessage

`JsonMessage` is the structured message model used by the module.

It can represent more than just `{ type, payload }`.

A complete message can contain:

```txt
id
kind
room
type
ts
payload
```

Example:

```json
{
  "id": "00000000000000000001",
  "kind": "event",
  "room": "general",
  "type": "chat.message",
  "ts": "2026-05-17T10:00:00Z",
  "payload": {
    "text": "Hello"
  }
}
```

## Common fields

| Field     | Purpose                          |
| --------- | -------------------------------- |
| `id`      | Optional message identifier.     |
| `kind`    | Message category, often `event`. |
| `room`    | Optional room or channel target. |
| `type`    | Application event type.          |
| `ts`      | Optional timestamp.              |
| `payload` | Event data.                      |

## Minimal typed message

The minimal useful typed message is:

```json
{
  "type": "chat.message",
  "payload": {
    "text": "Hello"
  }
}
```

The `type` field is the most important field.

Without a type, the server cannot route the message as a structured application event.

## Event type names

Use clear names for message types.

Recommended format:

```txt
domain.action
```

Examples:

```txt
chat.message
chat.typing
room.join
room.leave
presence.update
notification.created
order.updated
dashboard.refresh
```

This keeps the WebSocket protocol readable.

## Payload

The payload contains the data for the event.

Example:

```json
{
  "type": "chat.message",
  "payload": {
    "text": "Hello",
    "user_id": "42"
  }
}
```

In C++, the payload is passed as:

```cpp
const vix::json::kvs &payload
```

Example:

```cpp
ws.on_typed_message(
  [](vix::websocket::Session &session,
     const std::string &type,
     const vix::json::kvs &payload)
  {
    (void)session;

    if (type == "chat.message")
    {
      vix::print("payload:", payload);
    }
  });
```

## Send raw text

Use `Session::send_text(...)`.

```cpp
session.send_text("hello");
```

Inside a handler:

```cpp
ws.on_message(
  [](vix::websocket::Session &session, const std::string &message)
  {
    session.send_text("received: " + message);
  });
```

## Send typed JSON from the client

The native client provides `send_json_message(...)`.

```cpp
auto client =
    vix::websocket::Client::create("127.0.0.1", "9090", "/");

client->on_open([client]()
{
  client->send_json_message(
      "chat.message",
      {
        {"text", "Hello from client"}
      });
});

client->connect();
```

This sends a JSON message with:

```txt
type = chat.message
payload.text = Hello from client
```

## Send typed JSON from JavaScript

From a browser client:

```js
const socket = new WebSocket("ws://localhost:9090");

socket.onopen = () => {
  socket.send(
    JSON.stringify({
      type: "chat.message",
      payload: {
        text: "Hello from browser",
      },
    }),
  );
};
```

## Handle a chat message

```cpp
ws.on_typed_message(
  [](vix::websocket::Session &session,
     const std::string &type,
     const vix::json::kvs &payload)
  {
    if (type != "chat.message")
    {
      return;
    }

    (void)payload;

    session.send_text("chat message received");
  });
```

## Multiple message types

A single WebSocket connection can carry many event types.

```cpp
ws.on_typed_message(
  [&ws](vix::websocket::Session &session,
        const std::string &type,
        const vix::json::kvs &payload)
  {
    (void)payload;

    if (type == "room.join")
    {
      ws.join_room(session.shared_from_this(), "general");
      session.send_text("joined room");
      return;
    }

    if (type == "chat.message")
    {
      ws.broadcast_text_to_room("general", "new message");
      return;
    }

    if (type == "presence.update")
    {
      ws.broadcast_text("presence changed");
      return;
    }
  });
```

This is the main benefit of typed messages.

You keep one persistent connection, but route by message type.

## Room messages

A typed message can include a `room` field.

```json
{
  "room": "general",
  "type": "chat.message",
  "payload": {
    "text": "Hello room"
  }
}
```

Room messages are useful for:

- chat rooms
- team channels
- project dashboards
- multiplayer matches
- tenant-specific streams

## Broadcast room messages

```cpp
ws.on_typed_message(
  [&ws](vix::websocket::Session &session,
        const std::string &type,
        const vix::json::kvs &payload)
  {
    (void)session;
    (void)payload;

    if (type == "chat.message")
    {
      ws.broadcast_text_to_room("general", "new chat message");
    }
  });
```

## Raw vs typed messages

| Need                       | Use           |
| -------------------------- | ------------- |
| Echo simple text           | Raw message   |
| Quick prototype            | Raw message   |
| Structured events          | Typed message |
| Multiple realtime features | Typed message |
| Persistence and replay     | Typed message |
| Long-polling bridge        | Typed message |
| Room-based fallback        | Typed message |

## Message parsing

The server receives raw text first.

Then it tries to parse it as a `JsonMessage`.

If parsing fails, only the raw message handler runs.

If parsing succeeds, the typed message handler can run.

```txt
raw text
  -> on_message
  -> try JsonMessage::parse
      success -> on_typed_message
      failure -> stop there
```

This means you can support both simple text clients and structured JSON clients.

## Invalid JSON

If a client sends invalid JSON:

```txt
hello
```

or:

```json
{
  "payload": {
    "text": "missing type"
  }
}
```

The typed handler will not run.

Use the raw message handler if you want to detect and respond to invalid typed messages manually.

```cpp
ws.on_message(
  [](vix::websocket::Session &session, const std::string &message)
  {
    if (message.empty())
    {
      session.close("empty message");
      return;
    }

    vix::print("raw:", message);
  });
```

## Message persistence

For durable message history, use `IMessageStore`.

```cpp
vix::websocket::IMessageStore
```

The persistence interface supports:

```cpp
append(...)
list_by_room(...)
replay_from(...)
```

Use message persistence when:

- chat history matters
- clients reconnect and need missed messages
- room events must be replayable
- realtime events must be durable
- the application needs auditability

## SQLite message persistence

`SqliteMessageStore` stores `JsonMessage` values in SQLite.

```cpp
vix::websocket::SqliteMessageStore store{"messages.db"};
```

Append a message:

```cpp
vix::websocket::JsonMessage message;
message.kind = "event";
message.room = "general";
message.type = "chat.message";
message.payload = {
  {"text", "Hello"}
};

store.append(message);
```

List messages by room:

```cpp
auto messages = store.list_by_room("general", 50);
```

Replay messages:

```cpp
auto replay = store.replay_from("00000000000000000001", 50);
```

`SqliteMessageStore` stores message fields such as `id`, `kind`, `room`, `type`, `ts`, and `payload_json`. It also enables SQLite WAL mode when opening the database.

## Long-polling bridge messages

The long-polling bridge also uses `JsonMessage`.

The bridge can receive a WebSocket typed message and buffer it for HTTP polling clients.

```txt
WebSocket JsonMessage
  -> LongPollingBridge
  -> LongPollingManager
  -> HTTP client polls messages
```

This lets WebSocket and HTTP fallback clients share the same message model.

## Long-polling session resolution

By default, long-polling bridge delivery uses the message room when available.

```txt
if message.room is not empty:
  session id = "room:" + message.room
else:
  session id = "broadcast"
```

This makes room-based fallback possible.

## HTTP push message

An HTTP client can push a typed message through `/ws/send`.

Example request body:

```json
{
  "type": "chat.message",
  "room": "general",
  "payload": {
    "text": "Hello from HTTP"
  }
}
```

The bridge can then enqueue the message for long-polling and optionally forward it to WebSocket clients.

## OpenAPI message shape

The WebSocket OpenAPI helper documents the long-polling send endpoint with a JSON body containing:

```txt
session_id
room
type
kind
id
ts
payload
```

The required field is:

```txt
type
```

Example:

```json
{
  "type": "chat.message",
  "room": "general",
  "payload": {
    "text": "Hello"
  }
}
```

## Message size

Message size is controlled by:

```dotenv
WEBSOCKET_MAX_MESSAGE_SIZE=65536
```

Keep WebSocket messages small.

WebSocket is best for:

- realtime events
- small JSON payloads
- notifications
- chat messages
- presence events
- state updates

For large files, use HTTP upload or a dedicated storage path.

## Message ordering

Messages sent through one session are written through the session write queue.

Conceptually:

```txt
send_text
  -> enqueue
  -> flush write loop
  -> async write
```

This keeps outgoing messages for the same session ordered.

## Recommended message design

Use this shape for most application events:

```json
{
  "type": "domain.action",
  "room": "optional-room",
  "payload": {
    "key": "value"
  }
}
```

Example:

```json
{
  "type": "notification.created",
  "room": "user:42",
  "payload": {
    "title": "New order",
    "order_id": "A-100"
  }
}
```

## Best practices

Use typed messages for real application protocols.

Use raw messages only for simple protocols or debugging.

Use clear `domain.action` names.

Keep payloads small.

Use `room` when only some clients should receive the message.

Use a message store when clients need history or replay.

Use long-polling bridge when HTTP fallback clients must receive the same events.

Do not send large files through WebSocket.

## Common mistakes

### Missing type

This is not a useful typed message:

```json
{
  "payload": {
    "text": "Hello"
  }
}
```

Add a type:

```json
{
  "type": "chat.message",
  "payload": {
    "text": "Hello"
  }
}
```

### Too many unrelated payload shapes

Avoid making every message completely different.

Prefer stable event contracts.

```json
{
  "type": "chat.message",
  "payload": {
    "text": "Hello"
  }
}
```

### Using WebSocket for large files

Avoid this:

```txt
send 50 MB file through WebSocket
```

Prefer HTTP upload.

### Broadcasting private messages globally

Avoid:

```cpp
ws.broadcast_text("private message");
```

Prefer room-based delivery:

```cpp
ws.broadcast_text_to_room("user:42", "private message");
```

## Next steps

Continue with:

- [Client](./client.md)
- [Rooms and Broadcasting](./rooms-and-broadcasting.md)
- [Long polling](./long-polling.md)
- [Message store](./message-store.md)
- [SQLite message store](./sqlite-message-store.md)
