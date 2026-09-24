# 07_client_runtime

Native Vix WebSocket client example.

## Goal

This example demonstrates the Vix native WebSocket client API.

It shows:

- client creation
- lifecycle callbacks
- auto reconnect
- heartbeat
- typed JSON messages

## Run

Default target:

- host: `127.0.0.1`
- port: `9090`
- target: `/`

```bash
vix run examples/websocket/07_client_runtime/main.cpp
```

## Environment overrides

You can customize the target with environment variables:

```bash
export VIX_WS_HOST=127.0.0.1
export VIX_WS_PORT=9090
export VIX_WS_TARGET=/
export VIX_WS_USER=ada
export VIX_WS_ROOM=general
```

Then run:

```bash
vix run examples/websocket/07_client_runtime/main.cpp
```

## What it sends

The client sends:

- `chat.join`
- `app.ping`
- `chat.message`
- `chat.typing`
- `chat.leave`

## Example server targets

This client works well against the previous websocket examples, especially:

- `03_chat_rooms`
- `04_chat_persistent`
- `05_chat_long_polling`
- `06_metrics_runtime`

## Why this example matters

It proves that Vix provides both sides of realtime systems:

- server runtime
- native client runtime
