# 04_chat_persistent

Persistent HTTP + WebSocket chat example using Vix.cpp.

## Goal

This example extends the room-based chat by adding SQLite-backed message history.

It demonstrates:

- HTTP + WebSocket runtime
- room membership in memory
- persistent message history in SQLite
- room history endpoints
- replay endpoint

## Run

```bash
cp .env.example .env
mkdir -p storage
vix run examples/websocket/04_chat_persistent/main.cpp --with-sqlite
```

Default ports:

- HTTP: `8080`
- WebSocket: `9090`

## SQLite database

The example stores messages in:

```
storage/chat_rooms.db
```

The store enables WAL mode automatically.

## HTTP endpoints

#### `GET /`
Basic info.

#### `GET /health`
Health check.

#### `GET /rooms`
List active in-memory rooms.

#### `GET /rooms/{room}`
List users currently tracked in one room.

#### `GET /rooms/{room}/messages?limit=50&before_id=...`
Read room history from SQLite.

#### `GET /replay/{start_id}?limit=50`
Replay messages from a message id cursor.

#### `GET /stats`
Returns counts for active rooms and members.

## WebSocket endpoint

Connect to:

```
ws://127.0.0.1:9090/
```

## Typed protocol

#### Join a room

```json
{
  "type": "chat.join",
  "payload": {
    "user": "Ada",
    "room": "general"
  }
}
```

#### Send a message

```json
{
  "type": "chat.message",
  "payload": {
    "text": "Hello everyone"
  }
}
```

#### Typing event

```json
{
  "type": "chat.typing",
  "payload": {}
}
```

#### Leave the room

```json
{
  "type": "chat.leave",
  "payload": {}
}
```

## Notes

This example is intentionally balanced:

- room membership is in memory
- message history is durable in SQLite
- events are still broadcast globally for simplicity
- room filtering can be hardened further in a later example
