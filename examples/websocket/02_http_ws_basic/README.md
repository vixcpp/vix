# 02_http_ws_basic

Basic HTTP + WebSocket example using Vix.cpp.

## Goal

This example shows how to run:

- a Vix HTTP application
- a Vix WebSocket server
- a shared runtime executor

at the same time.

## What it demonstrates

### HTTP

- `GET /`
- `GET /health`
- `GET /hello/{name}`

### WebSocket

- connection lifecycle handlers
- typed message handling
- simple protocol:
  - `app.ping`
  - `chat.message`

## Config

Copy:

```bash
cp .env.example .env
```

Default ports:

- HTTP: `8080`
- WebSocket: `9090`

## Run

```bash
vix run examples/websocket/02_http_ws_basic/main.cpp
```

## Test the HTTP side

```bash
curl http://127.0.0.1:8080/
curl http://127.0.0.1:8080/health
curl http://127.0.0.1:8080/hello/Ada
```

## Test the WebSocket side

Connect a client to:

```
ws://127.0.0.1:9090/
```

Send a typed message:

```json
{
  "type": "app.ping",
  "payload": {}
}
```

The server responds with:

```json
{
  "type": "app.pong",
  "payload": {
    "status": "ok",
    "transport": "websocket"
  }
}
```

Send a chat message:

```json
{
  "type": "chat.message",
  "payload": {
    "user": "Ada",
    "text": "Hello"
  }
}
```

The server broadcasts the same `chat.message` event back.

## Why this example exists

This file is the first real mixed runtime example:

- HTTP routes are registered through `vix::App`
- WebSocket events are registered through `vix::websocket::Server`
- both runtimes are started together through the attached runtime helpers
