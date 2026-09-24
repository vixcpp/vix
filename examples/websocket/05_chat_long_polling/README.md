# 05_chat_long_polling

Room-based chat with HTTP long-polling fallback.

## Goal

This example shows how to keep one shared event model for:

- WebSocket clients
- HTTP long-polling clients

## Run

```bash
cp .env.example .env
vix run examples/websocket/05_chat_long_polling/main.cpp
```

Default ports:

- HTTP: `8080`
- WebSocket: `9090`

## HTTP endpoints

#### `GET /`
Basic info.

#### `GET /health`
Health check.

#### `GET /rooms`
List active rooms.

#### `GET /rooms/{room}`
List active users and the corresponding long-poll session id.

#### `GET /ws/poll?session_id=room:general&max=50`
Drain queued long-poll messages for one session id.

#### `POST /ws/send`
Push a typed event through HTTP.

Example body:

```json
{
  "session_id": "room:general",
  "room": "general",
  "type": "chat.message",
  "payload": {
    "user": "http-bot",
    "text": "hello from HTTP"
  }
}
```

## WebSocket endpoint

```
ws://127.0.0.1:9090/
```

## Typed protocol

#### Join

```json
{
  "type": "chat.join",
  "payload": {
    "user": "Ada",
    "room": "general"
  }
}
```

#### Message

```json
{
  "type": "chat.message",
  "payload": {
    "text": "Hello"
  }
}
```

#### Typing

```json
{
  "type": "chat.typing",
  "payload": {}
}
```

#### Leave

```json
{
  "type": "chat.leave",
  "payload": {}
}
```

## Long-poll session ids

This example uses room-based session ids:

- `room:general`
- `room:support`
- `broadcast`

A long-poll client interested in the `general` room should poll:

```
GET /ws/poll?session_id=room:general
```

## Why this example matters

It shows that Vix can expose one realtime protocol over two transports:

- WebSocket when available
- long-polling when WebSocket is not available
