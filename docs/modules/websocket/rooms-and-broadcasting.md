# Rooms and Broadcasting

This page explains rooms and broadcasting in the Vix WebSocket module.

Use it when you want to send messages to many clients, group sessions by room, broadcast globally, or broadcast only to a selected channel.

## Header

```cpp
#include <vix/websocket/server.hpp>
```

Or use the umbrella header:

```cpp
#include <vix/websocket.hpp>
```

## What rooms provide

Rooms let the server group WebSocket sessions under a string identifier.

Instead of sending every message to every connected client, you can send a message only to clients that joined a room.

Conceptually:

```txt
room: general
  -> session A
  -> session B

room: support
  -> session C
  -> session D
```

Then the server can send:

```txt
message to "general"
  -> session A
  -> session B
```

without sending it to `support`.

## What broadcasting provides

Broadcasting means sending one message to multiple sessions.

The module supports two common broadcast models:

| Broadcast type   | Meaning                                 |
| ---------------- | --------------------------------------- |
| Global broadcast | Send to every active WebSocket session. |
| Room broadcast   | Send only to sessions inside one room.  |

Use global broadcast for system-wide events.

Use room broadcast for targeted realtime delivery.

## Basic model

The server tracks active sessions and rooms.

```txt
Server
  -> active sessions
  -> rooms
      -> room id
      -> sessions
```

When a session opens, the server registers it.

When a session closes, the server unregisters it and removes it from all rooms.

This keeps room membership clean during connection shutdown.

## Join a room

Use `join_room(...)` to add a session to a room.

```cpp
ws.on_open([&ws](vix::websocket::Session &session)
{
  ws.join_room(session.shared_from_this(), "general");
});
```

After this, the session belongs to the `general` room.

## Leave a room

Use `leave_room(...)` to remove a session from a room.

```cpp
ws.leave_room(session.shared_from_this(), "general");
```

Use this when:

- a user leaves a chat room
- a dashboard unsubscribes from a channel
- a client switches project context
- a multiplayer client leaves a match
- a user logs out of a realtime area

## Broadcast to all clients

Use `broadcast_text(...)` to send a text message to all active sessions.

```cpp
ws.broadcast_text("server update");
```

This is useful for:

- global maintenance messages
- system status changes
- application-wide notifications
- live reload events
- public announcements

## Broadcast to one room

Use `broadcast_text_to_room(...)` to send a message only to one room.

```cpp
ws.broadcast_text_to_room("general", "hello general room");
```

This is useful for:

- chat messages
- user-specific notifications
- project-specific updates
- tenant-specific events
- dashboard channels
- game rooms

## Minimal room example

```cpp
#include <memory>
#include <string>

#include <vix/config/Config.hpp>
#include <vix/executor/RuntimeExecutor.hpp>
#include <vix/print.hpp>
#include <vix/websocket.hpp>

int main()
{
  vix::config::Config config{".env"};

  auto executor =
      std::make_shared<vix::executor::RuntimeExecutor>(4);

  vix::websocket::Server ws{config, executor};

  ws.on_open([&ws](vix::websocket::Session &session)
  {
    ws.join_room(session.shared_from_this(), "general");

    session.send_text("joined general");
  });

  ws.on_message(
    [&ws](vix::websocket::Session &session, const std::string &message)
    {
      (void)session;

      ws.broadcast_text_to_room("general", "room message: " + message);
    });

  ws.start();

  return 0;
}
```

## Join rooms with typed messages

A common pattern is to let the client send a typed message to join a room.

Client message:

```json
{
  "type": "room.join",
  "payload": {
    "room": "general"
  }
}
```

Server handler:

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
      session.send_text("joined room: general");
    }
  });
```

## Broadcast chat messages to a room

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

## Room ids

A room id is a string.

Examples:

```txt
general
support
project:42
user:100
tenant:acme
order:A-100
game:match-7
```

Use clear room names.

A good room id should describe who should receive the event.

## Suggested room naming

Use prefixes when the application has several room types.

```txt
room:general
user:42
project:12
tenant:acme
order:A-100
```

This makes room usage easier to debug.

## Global vs room broadcast

Use global broadcast only when every connected client should receive the message.

```cpp
ws.broadcast_text("server restarting soon");
```

Use room broadcast when the message belongs to a specific group.

```cpp
ws.broadcast_text_to_room("project:42", "project updated");
```

| Need                | Use                           |
| ------------------- | ----------------------------- |
| Send to all clients | `broadcast_text(...)`         |
| Send to one group   | `broadcast_text_to_room(...)` |
| Reply to one client | `session.send_text(...)`      |

## Reply to one client

Broadcasting is not always needed.

If only the sender should receive the response, use the session.

```cpp
ws.on_message(
  [](vix::websocket::Session &session, const std::string &message)
  {
    session.send_text("you said: " + message);
  });
```

This avoids sending private data to other clients.

## Room-based notifications

Rooms are useful for private notification channels.

Example:

```cpp
ws.on_open([&ws](vix::websocket::Session &session)
{
  ws.join_room(session.shared_from_this(), "user:42");
});
```

Then the server can send a private notification:

```cpp
ws.broadcast_text_to_room("user:42", "new notification");
```

## Project dashboards

For live dashboards, each project can have its own room.

```cpp
ws.join_room(session.shared_from_this(), "project:42");
```

Then broadcast project updates:

```cpp
ws.broadcast_text_to_room("project:42", "build completed");
```

Only clients watching project `42` receive the update.

## Tenant isolation

For multi-tenant applications, each tenant can have its own room.

```cpp
ws.join_room(session.shared_from_this(), "tenant:acme");
```

Then broadcast tenant-scoped events:

```cpp
ws.broadcast_text_to_room("tenant:acme", "tenant event");
```

This prevents sending tenant-specific updates to unrelated clients.

## Chat rooms

A chat room can map directly to a WebSocket room.

```cpp
ws.join_room(session.shared_from_this(), "chat:general");
```

Then send chat messages:

```cpp
ws.broadcast_text_to_room("chat:general", "Ada: Hello");
```

For real chat applications, prefer typed JSON messages instead of plain text.

## Typed room message shape

A typed room message can use this shape:

```json
{
  "type": "chat.message",
  "room": "chat:general",
  "payload": {
    "text": "Hello"
  }
}
```

The `room` field identifies the target room.

This shape is useful because it can also be persisted and replayed by the message store.

## Room messages and persistence

When messages include a room field, they can be stored and queried by room.

```cpp
vix::websocket::JsonMessage message;
message.kind = "event";
message.room = "chat:general";
message.type = "chat.message";
message.payload = {
  {"text", "Hello"}
};

store.append(message);
```

Then query the room history:

```cpp
auto messages = store.list_by_room("chat:general", 50);
```

This is useful for:

- chat history
- missed messages
- reconnect recovery
- room replay
- audit logs

## Rooms and long-polling

Rooms also work well with the long-polling bridge.

By default, the bridge resolves long-polling sessions like this:

```txt
if message.room is not empty:
  session id = "room:" + message.room
else:
  session id = "broadcast"
```

So a WebSocket message with:

```json
{
  "room": "general",
  "type": "chat.message",
  "payload": {
    "text": "Hello"
  }
}
```

can be buffered under:

```txt
room:general
```

for HTTP polling clients.

## Broadcast with long-polling fallback

A complete realtime system can use both WebSocket rooms and long-polling buffers.

```txt
WebSocket clients
  -> receive room broadcast directly

HTTP fallback clients
  -> poll room buffer through long-polling
```

This lets the same event model support both persistent WebSocket clients and HTTP-only clients.

## Cleanup on close

When a session closes, the server removes it from rooms.

Conceptually:

```txt
session close
  -> unregister session
  -> remove session from all rooms
  -> call user close handler
```

This prevents room lists from keeping dead connections.

## Avoid global overuse

Global broadcast is simple, but it can become expensive and unsafe.

Avoid:

```cpp
ws.broadcast_text("private event");
```

Prefer:

```cpp
ws.broadcast_text_to_room("user:42", "private event");
```

This keeps delivery targeted.

## Example: room join and leave

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
      session.send_text("joined general");
      return;
    }

    if (type == "room.leave")
    {
      ws.leave_room(session.shared_from_this(), "general");
      session.send_text("left general");
      return;
    }
  });
```

## Example: global announcement

```cpp
ws.broadcast_text("maintenance starts in 10 minutes");
```

Use this for public server-wide messages.

## Example: dashboard update

```cpp
ws.broadcast_text_to_room(
    "project:42",
    "deployment completed");
```

Only clients subscribed to `project:42` receive the update.

## Example: private user notification

```cpp
ws.broadcast_text_to_room(
    "user:42",
    "you have a new message");
```

This is useful after joining each authenticated user to a private user room.

## Best practices

Use rooms for targeted delivery.

Use global broadcast only for truly global events.

Use clear room ids.

Use typed messages for room events.

Use `session.send_text(...)` for one-client replies.

Remove or leave rooms when the client changes context.

Use message persistence when room history matters.

Use long-polling bridge when HTTP fallback clients need the same room events.

## Common mistakes

### Broadcasting private data globally

Avoid:

```cpp
ws.broadcast_text("private notification");
```

Prefer:

```cpp
ws.broadcast_text_to_room("user:42", "private notification");
```

### Using unclear room names

Avoid:

```txt
42
abc
general2
```

Prefer:

```txt
user:42
project:42
chat:general
```

### Sending room events without joining the room

Broadcasting to a room only helps if sessions joined that room.

```cpp
ws.join_room(session.shared_from_this(), "general");
```

### Using rooms instead of authorization

Rooms are a delivery mechanism.

They are not a security system by themselves.

Validate that a user is allowed to join a room before calling `join_room(...)`.

### Using WebSocket for large room payloads

Keep room broadcasts small.

For large files or exports, use HTTP download links.

## Next steps

Continue with:

- [Long polling](./long-polling.md)
- [Messages](./messages.md)
- [Server](./server.md)
- [Message store](./message-store.md)
- [Shutdown](./shutdown.md)
