# 01_minimal_ws

Minimal native WebSocket server using Vix.cpp.

## Goal

This example shows the smallest useful WebSocket runtime:

- start a Vix WebSocket server
- accept client connections
- echo raw text messages
- echo typed JSON messages

## Config

Copy:

```bash
cp .env.example .env
```

Default WebSocket port: `9090`

## Run

```bash
vix run examples/websocket/01_minimal_ws/main.cpp
```

## Typed message format

The server understands the Vix typed JSON convention:

```json
{
  "type": "chat.message",
  "payload": {
    "text": "hello"
  }
}
```

It will broadcast the same typed event back to connected clients.

## Raw text messages

If a client sends a plain text frame, the server broadcasts:

```json
{
  "type": "echo.raw",
  "payload": {
    "text": "..."
  }
}
```

## What this example demonstrates

- `vix::config::Config`
- `vix::websocket::Server`
- `on_open`
- `on_close`
- `on_error`
- `on_message`
- `on_typed_message`
- blocking runtime with `listen_blocking()`
