# Message store

This page explains the message persistence abstraction in the Vix WebSocket module.

Use it when you want to store WebSocket messages, list room history, replay messages after reconnect, or plug your own durable backend into the realtime layer.

## Header

```cpp
#include <vix/websocket/MessageStore.hpp>
```

Or use the umbrella header:

```cpp
#include <vix/websocket.hpp>
```

## What the message store provides

The WebSocket module defines an abstract message storage interface:

```cpp
vix::websocket::IMessageStore
```

It is used to persist `JsonMessage` values.

A message store can support:

- chat history
- room history
- missed message replay
- event replay
- durable realtime logs
- local-first message persistence
- custom storage backends

The module also provides a SQLite implementation:

```cpp
vix::websocket::SqliteMessageStore
```

## Basic model

The model is:

```txt
WebSocket message
  -> JsonMessage
  -> IMessageStore::append(...)
  -> durable storage
```

Then clients can read history:

```txt
room id
  -> IMessageStore::list_by_room(...)
  -> previous messages
```

Or replay messages:

```txt
start id
  -> IMessageStore::replay_from(...)
  -> messages after cursor
```

## IMessageStore

`IMessageStore` is the abstract persistence interface.

```cpp
class IMessageStore
{
public:
  virtual ~IMessageStore() = default;

  virtual void append(const JsonMessage &msg) = 0;

  virtual std::vector<JsonMessage> list_by_room(
      const std::string &room,
      std::size_t limit,
      const std::optional<std::string> &before_id = std::nullopt) = 0;

  virtual std::vector<JsonMessage> replay_from(
      const std::string &start_id,
      std::size_t limit) = 0;
};
```

It stores and returns `vix::websocket::JsonMessage`.

## JsonMessage

The store works with the WebSocket typed message model.

A message can contain:

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

## append

Use `append(...)` to persist a message.

```cpp
store.append(message);
```

Example:

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

Use `append(...)` when:

- a chat message is accepted
- an event must be durable
- a room update should be replayable
- clients may reconnect and need missed messages

## list_by_room

Use `list_by_room(...)` to load messages for one room.

```cpp
auto messages = store.list_by_room("general", 50);
```

Arguments:

| Argument    | Purpose                               |
| ----------- | ------------------------------------- |
| `room`      | Room identifier.                      |
| `limit`     | Maximum number of messages to return. |
| `before_id` | Optional cursor for pagination.       |

Example with cursor:

```cpp
auto messages =
    store.list_by_room("general", 50, "00000000000000000020");
```

Use `list_by_room(...)` for:

- chat history
- room timeline
- previous events
- paginated message history
- loading old messages when a client joins

## replay_from

Use `replay_from(...)` to replay messages starting from a cursor.

```cpp
auto messages = store.replay_from("00000000000000000001", 100);
```

Arguments:

| Argument   | Purpose                               |
| ---------- | ------------------------------------- |
| `start_id` | Cursor id to start replay from.       |
| `limit`    | Maximum number of messages to return. |

Use `replay_from(...)` for:

- reconnect recovery
- missed events
- event replay
- local-first synchronization
- durable realtime streams

## Basic usage

```cpp
#include <vix/websocket.hpp>

int main()
{
  vix::websocket::SqliteMessageStore store{"messages.db"};

  vix::websocket::JsonMessage message;
  message.kind = "event";
  message.room = "general";
  message.type = "chat.message";
  message.payload = {
    {"text", "Hello from Vix"}
  };

  store.append(message);

  auto history = store.list_by_room("general", 50);

  (void)history;

  return 0;
}
```

## Store messages from WebSocket handlers

A common pattern is to store typed messages before broadcasting them.

```cpp
vix::websocket::SqliteMessageStore store{"messages.db"};

ws.on_typed_message(
  [&ws, &store](vix::websocket::Session &session,
                const std::string &type,
                const vix::json::kvs &payload)
  {
    (void)session;

    if (type != "chat.message")
    {
      return;
    }

    vix::websocket::JsonMessage message;
    message.kind = "event";
    message.room = "general";
    message.type = type;
    message.payload = payload;

    store.append(message);

    ws.broadcast_text_to_room("general", "new chat message");
  });
```

The flow is:

```txt
client sends typed message
  -> server validates type
  -> message store appends JsonMessage
  -> server broadcasts to room
```

## Store before broadcast

For durable realtime systems, prefer storing before broadcasting.

```txt
receive message
  -> validate
  -> append to store
  -> broadcast
```

This makes the message durable before clients are notified.

That is useful when clients may reconnect and ask for missed messages.

## Room history

Messages with a `room` field can be queried by room.

```cpp
vix::websocket::JsonMessage message;
message.room = "chat:general";
message.type = "chat.message";
message.payload = {
  {"text", "Hello"}
};

store.append(message);
```

Then load the room history:

```cpp
auto messages = store.list_by_room("chat:general", 50);
```

This is useful for:

- chat rooms
- project timelines
- tenant events
- user channels
- dashboards

## Reconnect replay

When a client reconnects, it can ask for messages after the last message id it received.

Conceptually:

```txt
client reconnects
  -> sends last_seen_id
  -> server calls replay_from(last_seen_id, limit)
  -> server sends missed messages
```

Example:

```cpp
auto missed = store.replay_from(lastSeenId, 100);

for (const auto &message : missed)
{
  session.send_text(message.to_json_string());
}
```

## Custom message store

You can implement your own message store by inheriting from `IMessageStore`.

```cpp
class MyMessageStore final : public vix::websocket::IMessageStore
{
public:
  void append(const vix::websocket::JsonMessage &msg) override
  {
    // Store message in your backend.
  }

  std::vector<vix::websocket::JsonMessage> list_by_room(
      const std::string &room,
      std::size_t limit,
      const std::optional<std::string> &before_id = std::nullopt) override
  {
    // Query room history from your backend.
    return {};
  }

  std::vector<vix::websocket::JsonMessage> replay_from(
      const std::string &start_id,
      std::size_t limit) override
  {
    // Replay messages from your backend.
    return {};
  }
};
```

Use a custom store when you want to persist messages in:

- PostgreSQL
- MySQL
- Redis Streams
- files
- object storage
- distributed logs
- custom local-first storage

## SQLite implementation

The built-in implementation is:

```cpp
vix::websocket::SqliteMessageStore
```

It stores messages in SQLite and enables WAL mode.

```cpp
vix::websocket::SqliteMessageStore store{"messages.db"};
```

The SQLite table stores fields such as:

```txt
id
kind
room
type
ts
payload_json
```

Use SQLite when you want:

- a simple durable local store
- chat history
- local development
- embedded message persistence
- WAL-friendly storage

## Message id

A message id is used for cursors, replay, and pagination.

If your store generates ids automatically, the application can leave `id` empty.

If your application needs deterministic ids, set `message.id` before appending.

```cpp
message.id = "00000000000000000001";
```

Use stable ids when:

- clients track last seen messages
- replay must be deterministic
- events are synchronized across nodes
- messages are deduplicated

## Timestamp

A message can carry a timestamp in `ts`.

```cpp
message.ts = "2026-05-17T10:00:00Z";
```

The SQLite store can fill missing timestamps automatically.

Use timestamps for:

- chat history display
- ordering
- audit logs
- replay diagnostics
- debugging

## Message kind

The `kind` field lets you categorize messages.

Common value:

```txt
event
```

Other possible values:

```txt
command
system
notification
presence
```

Keep `kind` simple and consistent.

## Message type

The `type` field identifies the application event.

Examples:

```txt
chat.message
room.join
room.leave
presence.update
notification.created
order.updated
```

Use clear `domain.action` names.

## Payload

The payload contains message data.

Example:

```cpp
message.payload = {
  {"text", "Hello"},
  {"user_id", "42"}
};
```

Keep payloads small.

For large files, store the file elsewhere and send a reference.

## Message store with rooms

A good room message shape is:

```json
{
  "kind": "event",
  "room": "chat:general",
  "type": "chat.message",
  "payload": {
    "text": "Hello"
  }
}
```

This gives you:

```txt
room history
typed routing
future replay
long-polling compatibility
```

## Message store with long-polling

The same `JsonMessage` can be used by the long-polling bridge.

Flow:

```txt
WebSocket typed message
  -> append to message store
  -> LongPollingBridge buffers message
  -> HTTP clients poll message
```

This gives both persistence and fallback delivery.

## Message store with broadcasting

A durable broadcast flow can look like this:

```txt
receive typed message
  -> build JsonMessage
  -> store.append(message)
  -> ws.broadcast_text_to_room(message.room, message.to_json_string())
```

This keeps the broadcast event replayable later.

## Error handling

Message stores can throw exceptions when storage fails.

Handle errors around persistence.

```cpp
try
{
  store.append(message);
}
catch (const std::exception &e)
{
  session.emit_error(e.what());
  session.close("message persistence failed");
}
```

For production systems, decide whether a failed store should:

- reject the message
- close the session
- retry later
- fall back to memory
- return an error message to the client

## Memory vs durable storage

Without a message store, realtime delivery is transient.

```txt
message sent
  -> broadcast
  -> gone after delivery
```

With a message store:

```txt
message sent
  -> append to store
  -> broadcast
  -> available for history and replay
```

Use durable storage when messages matter after the first delivery.

## Best practices

Store accepted messages before broadcasting.

Use typed messages for stored events.

Set a room when messages belong to a channel.

Use stable event type names.

Keep payloads small.

Use message ids for replay and pagination.

Use `list_by_room(...)` for room history.

Use `replay_from(...)` for reconnect recovery.

Handle persistence errors explicitly.

## Common mistakes

### Broadcasting before storing

Avoid this when durability matters:

```txt
broadcast
  -> append to store
```

Prefer:

```txt
append to store
  -> broadcast
```

### Storing huge payloads

Avoid storing large blobs in message payloads.

Store files separately and keep a reference in the payload.

### Missing room for room history

If the message should appear in a room history, set:

```cpp
message.room = "chat:general";
```

### No replay cursor

Reconnect recovery needs a cursor.

Track the last message id received by the client.

### Treating persistence as authorization

The message store only stores messages.

It does not decide who is allowed to read them.

Validate access before returning room history or replay data.

## Next steps

Continue with:

- [SQLite message store](./sqlite-message-store.md)
- [Messages](./messages.md)
- [Rooms and Broadcasting](./rooms-and-broadcasting.md)
- [Long polling](./long-polling.md)
- [API Reference](./api-reference.md)
