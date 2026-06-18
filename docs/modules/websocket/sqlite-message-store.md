# SQLite message store

This page explains `vix::websocket::SqliteMessageStore`.

Use it when you want to persist WebSocket `JsonMessage` values in SQLite, keep room history, replay messages, or build a durable local message backend.

## Header

```cpp
#include <vix/websocket/SqliteMessageStore.hpp>
```

Or use the umbrella header:

```cpp
#include <vix/websocket.hpp>
```

## What SqliteMessageStore provides

`SqliteMessageStore` is the built-in SQLite implementation of:

```cpp
vix::websocket::IMessageStore
```

It stores WebSocket messages in a SQLite database.

It supports:

- appending messages
- listing messages by room
- replaying messages from a cursor
- automatic schema initialization
- automatic id generation when missing
- automatic timestamp generation when missing
- SQLite WAL mode
- durable local message history

## Basic model

The model is:

```txt
JsonMessage
  -> SqliteMessageStore::append(...)
  -> SQLite database
  -> messages table
```

Then messages can be queried:

```txt
room
  -> list_by_room(...)
  -> room history
```

Or replayed:

```txt
start_id
  -> replay_from(...)
  -> message replay
```

## Create a store

```cpp
vix::websocket::SqliteMessageStore store{"messages.db"};
```

If the database does not exist, SQLite creates it.

The store initializes the message schema automatically.

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

## Constructor

```cpp
explicit SqliteMessageStore(const std::string &db_path);
```

The constructor opens or creates the SQLite database.

```cpp
vix::websocket::SqliteMessageStore store{"messages.db"};
```

During construction, the store:

```txt
1. opens the SQLite database
2. enables SQLite WAL mode
3. initializes the messages table
```

The implementation opens SQLite with `sqlite3_open(...)`, enables `PRAGMA journal_mode=WAL`, then calls `init_schema()`. :contentReference[oaicite:0]{index=0}

## Destructor

```cpp
~SqliteMessageStore() override;
```

The destructor closes the SQLite database handle.

```txt
~SqliteMessageStore
  -> sqlite3_close
  -> db_ = nullptr
```

## Schema

The store creates a table named:

```txt
messages
```

The schema contains:

```txt
id
kind
room
type
ts
payload_json
```

Conceptually:

```sql
CREATE TABLE IF NOT EXISTS messages (
  id           TEXT PRIMARY KEY,
  kind         TEXT NOT NULL,
  room         TEXT,
  type         TEXT NOT NULL,
  ts           TEXT NOT NULL,
  payload_json TEXT NOT NULL
);
```

The table is created automatically if it does not exist. :contentReference[oaicite:1]{index=1}

## Stored fields

| Field          | Purpose                              |
| -------------- | ------------------------------------ |
| `id`           | Unique message id and replay cursor. |
| `kind`         | Message category, usually `event`.   |
| `room`         | Optional room or channel id.         |
| `type`         | Application event type.              |
| `ts`           | Timestamp.                           |
| `payload_json` | Serialized JSON payload.             |

## append

Use `append(...)` to persist a message.

```cpp
store.append(message);
```

Signature:

```cpp
void append(const JsonMessage &msg) override;
```

Example:

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

## Automatic id

If `message.id` is empty, the SQLite store generates an id.

```cpp
vix::websocket::JsonMessage message;
message.id = "";
message.type = "chat.message";

store.append(message);
```

The current implementation generates an id based on the current timestamp in microseconds and formats it as a padded string. :contentReference[oaicite:2]{index=2}

## Automatic timestamp

If `message.ts` is empty, the SQLite store generates a UTC timestamp.

```cpp
vix::websocket::JsonMessage message;
message.ts = "";
message.type = "chat.message";

store.append(message);
```

The timestamp shape is:

```txt
YYYY-MM-DDTHH:MM:SSZ
```

The implementation fills `ts` before inserting when the field is missing. :contentReference[oaicite:3]{index=3}

## Default kind

If `message.kind` is empty, the store uses:

```txt
event
```

Example:

```cpp
vix::websocket::JsonMessage message;
message.kind = "";
message.type = "chat.message";

store.append(message);
```

After normalization, the message kind becomes:

```txt
event
```

## Payload serialization

The store serializes `message.payload` to JSON before insertion.

```txt
vix::json::kvs
  -> nlohmann::json
  -> payload_json text
```

This allows structured payloads to be stored in one SQLite text column.

## Insert behavior

The SQLite store uses insert-or-replace behavior.

Conceptually:

```sql
INSERT OR REPLACE INTO messages
(id, kind, room, type, ts, payload_json)
VALUES (?, ?, ?, ?, ?, ?);
```

This means a message with an existing id can replace the previous row.

## list_by_room

Use `list_by_room(...)` to load messages for one room.

```cpp
auto messages = store.list_by_room("chat:general", 50);
```

Signature:

```cpp
std::vector<JsonMessage> list_by_room(
    const std::string &room,
    std::size_t limit,
    const std::optional<std::string> &before_id = std::nullopt) override;
```

Arguments:

| Argument    | Purpose                               |
| ----------- | ------------------------------------- |
| `room`      | Room identifier.                      |
| `limit`     | Maximum number of messages to return. |
| `before_id` | Optional pagination cursor.           |

## List room history

```cpp
auto history = store.list_by_room("general", 50);

for (const auto &message : history)
{
  // send or inspect message
}
```

Use this when:

- a user opens a chat room
- a dashboard loads previous events
- a client reconnects
- a room timeline must be restored

## Pagination with before_id

Use `before_id` to load messages before a cursor.

```cpp
auto older = store.list_by_room(
    "chat:general",
    50,
    "00000000000000001000");
```

This is useful for paginated history.

Example flow:

```txt
load latest 50 messages
  -> user scrolls up
  -> request older messages before oldest id
  -> list_by_room(room, 50, before_id)
```

## replay_from

Use `replay_from(...)` to replay messages from a cursor.

```cpp
auto messages = store.replay_from("00000000000000000001", 100);
```

Signature:

```cpp
std::vector<JsonMessage> replay_from(
    const std::string &start_id,
    std::size_t limit) override;
```

Arguments:

| Argument   | Purpose                               |
| ---------- | ------------------------------------- |
| `start_id` | Cursor id to start from.              |
| `limit`    | Maximum number of messages to return. |

## Replay after reconnect

A reconnect flow can look like this:

```txt
client reconnects
  -> client sends last_seen_id
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

## Store before broadcast

For durable realtime behavior, store messages before broadcasting them.

```cpp
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
    message.room = "chat:general";
    message.type = type;
    message.payload = payload;

    store.append(message);

    ws.broadcast_text_to_room(
        "chat:general",
        message.to_json_string());
  });
```

The flow is:

```txt
receive typed message
  -> validate
  -> append to SQLite
  -> broadcast to room
```

This makes the event durable before clients are notified.

## Chat room example

```cpp
vix::websocket::SqliteMessageStore store{"chat.db"};

ws.on_open([&ws](vix::websocket::Session &session)
{
  ws.join_room(session.shared_from_this(), "chat:general");

  auto history = store.list_by_room("chat:general", 50);

  for (const auto &message : history)
  {
    session.send_text(message.to_json_string());
  }
});

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
    message.room = "chat:general";
    message.type = type;
    message.payload = payload;

    store.append(message);

    ws.broadcast_text_to_room(
        "chat:general",
        message.to_json_string());
  });
```

## Database file location

You can store the SQLite database wherever your application has write access.

```cpp
vix::websocket::SqliteMessageStore store{"data/messages.db"};
```

For local development:

```txt
messages.db
chat.db
.vix/messages.db
```

For production:

```txt
/var/lib/myapp/messages.db
/opt/myapp/data/messages.db
```

Make sure the application user can write to the directory.

## WAL mode

The SQLite store enables WAL mode.

```sql
PRAGMA journal_mode=WAL;
```

WAL mode is useful because it improves durability and supports better concurrent read behavior.

This fits WebSocket applications where writes and reads can happen frequently.

## Error handling

SQLite operations can throw exceptions when they fail.

Handle persistence errors in your application.

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

Use this when persistence is required for correctness.

## Required fields

Before storing, make sure a message has at least:

```txt
type
payload
```

The store can fill:

```txt
id
ts
kind
```

But the application should set meaningful values for:

```txt
room
type
payload
```

when it wants useful history and replay.

## Room field

Set `room` when messages belong to a room.

```cpp
message.room = "chat:general";
```

Without a room, `list_by_room(...)` cannot return the message for a room history.

## Type field

Set `type` to describe the application event.

```cpp
message.type = "chat.message";
```

Use clear event names:

```txt
chat.message
notification.created
presence.update
order.updated
```

## Payload field

Set `payload` to carry event data.

```cpp
message.payload = {
  {"text", "Hello"},
  {"user_id", "42"}
};
```

Keep payloads small.

For files, store a file URL or id instead of embedding large data.

## Id field

Set `id` manually only if your application needs deterministic ids.

```cpp
message.id = "00000000000000000042";
```

Otherwise, let the store generate it.

## Timestamp field

Set `ts` manually only if your application already has a trusted timestamp.

```cpp
message.ts = "2026-05-17T10:00:00Z";
```

Otherwise, let the store generate it.

## Kind field

Use `kind` to classify messages.

Common default:

```txt
event
```

Examples:

```txt
event
command
system
notification
presence
```

Keep this field stable across your application.

## SQLite store vs custom store

Use `SqliteMessageStore` when:

- you need a simple local durable store
- the application is small or medium-sized
- you want embedded persistence
- you want local development history
- you want WAL-friendly message storage

Use a custom `IMessageStore` implementation when:

- you need PostgreSQL
- you need MySQL
- you need Redis Streams
- you need distributed storage
- you need cloud storage
- you need application-specific indexing

## Performance notes

SQLite is a good default for local durable message history.

Keep these rules in mind:

- keep payloads small
- use room ids consistently
- use limits for queries
- avoid loading unlimited history
- use pagination
- avoid storing large binary blobs
- store references to files instead of file content

## Production notes

For production use:

- store the database in a persistent directory
- back up the database
- monitor disk usage
- keep message limits reasonable
- avoid unbounded history growth
- consider retention policies
- protect history endpoints with authorization

## Retention strategy

The current interface focuses on append, room listing, and replay.

If your application needs retention, add a cleanup policy around the store.

Examples:

```txt
delete messages older than 30 days
keep last 10,000 messages per room
archive old messages to object storage
compact system events periodically
```

## Security notes

Message storage does not provide authorization.

Before returning room history or replayed messages, validate that the client is allowed to read them.

```txt
client asks for room history
  -> authenticate client
  -> authorize room access
  -> list_by_room
  -> send messages
```

Do not expose stored message history without access checks.

## Best practices

Store messages before broadcasting when durability matters.

Set `room` for room history.

Set clear `type` values.

Keep payloads small.

Use `limit` for all history queries.

Use `before_id` for pagination.

Use `replay_from(...)` for reconnect recovery.

Handle SQLite exceptions.

Store the database in a persistent path.

Protect history and replay endpoints with authorization.

## Common mistakes

### Missing room

Avoid:

```cpp
message.type = "chat.message";
store.append(message);
```

For room history, set:

```cpp
message.room = "chat:general";
```

### Loading unlimited history

Avoid APIs that return all messages.

Prefer:

```cpp
store.list_by_room("chat:general", 50);
```

### Broadcasting before append

Avoid:

```txt
broadcast
  -> append
```

Prefer:

```txt
append
  -> broadcast
```

### Storing large files in payload_json

Do not store large binary payloads in `payload_json`.

Store a file reference instead.

### Ignoring storage errors

Do not silently ignore append failures when durability matters.

Handle exceptions.

## Next steps

Continue with:

- [OpenAPI](./openapi.md)
- [Message store](./message-store.md)
- [Messages](./messages.md)
- [Rooms and Broadcasting](./rooms-and-broadcasting.md)
- [Shutdown](./shutdown.md)
