# 06_metrics_runtime

HTTP + WebSocket runtime with Prometheus-style metrics.

## Goal

This example demonstrates observability for a mixed Vix runtime.

It includes:

- HTTP routes
- WebSocket typed events
- in-memory counters
- Prometheus text exposition on `/metrics`

## Run

```bash
cp .env.example .env
vix run examples/websocket/06_metrics_runtime/main.cpp
```

Default ports:

- HTTP: `8080`
- WebSocket: `9090`

## HTTP endpoints

#### `GET /`
Basic info.

#### `GET /health`
Health check.

#### `GET /features`
Lists the exposed runtime features.

#### `GET /stats`
Returns a JSON snapshot of the main counters.

#### `GET /metrics`
Returns Prometheus-style metrics text.

## WebSocket endpoint

```
ws://127.0.0.1:9090/
```

## Typed events

#### Ping

```json
{
  "type": "app.ping",
  "payload": {}
}
```

#### Chat message

```json
{
  "type": "chat.message",
  "payload": {
    "user": "Ada",
    "text": "Hello"
  }
}
```

#### Metrics snapshot request

```json
{
  "type": "system.metrics",
  "payload": {}
}
```

This emits `system.metrics.snapshot`.

## Why this example matters

This example shows how to make a Vix realtime runtime observable without adding external dependencies.
