# 03_chat_rooms

HTTP + WebSocket chat rooms example using Vix.cpp.

## Goal

This example introduces a simple room-based chat on top of the Vix mixed runtime.

It demonstrates:

- HTTP routes
- WebSocket typed events
- in-memory room membership
- join / leave / typing / message flows

## Run

```bash
cp .env.example .env
vix run examples/websocket/03_chat_rooms/main.cpp
```

Default ports:

- HTTP: `8080`
- WebSocket: `9090`

## HTTP endpoints

#### `GET /`
Basic info about the example.

#### `GET /health`
Health check.

#### `GET /rooms`
List active rooms.

#### `GET /rooms/{room}`
List users currently present in one room.

#### `GET /stats`
Returns the number of active rooms and connected members known by the registry.

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

This example uses an in-memory room registry. It is intentionally simple:

- no persistence
- no authentication
- no per-room transport isolation
- no history replay

Those capabilities come in later examples.
