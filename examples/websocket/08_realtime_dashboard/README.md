# 08_realtime_dashboard

Realtime dashboard example using HTTP + WebSocket.

## Goal

This example demonstrates a small production-like app built with Vix:

- static dashboard UI over HTTP
- live updates over WebSocket
- JSON stats endpoint
- Prometheus metrics endpoint

## Run

```bash
cp .env.example .env
vix run examples/websocket/08_realtime_dashboard/main.cpp
```

Default ports:

- HTTP: `8080`
- WebSocket: `9090`

Open in browser:

```
http://127.0.0.1:8080/
```

## HTTP endpoints

#### `GET /`
Dashboard UI.

#### `GET /health`
Health check.

#### `GET /stats`
JSON snapshot of the current dashboard state.

#### `GET /metrics`
Prometheus-style metrics.

#### `GET /api/features`
Feature description.

## WebSocket endpoint

```
ws://127.0.0.1:9090/
```

## Typed events

#### Refresh snapshot

```json
{
  "type": "dashboard.refresh",
  "payload": {}
}
```

#### Check status

```json
{
  "type": "dashboard.status",
  "payload": {}
}
```

## Why this example matters

This is the most application-like example in the WebSocket series. It shows how Vix can serve:

- backend routes
- static frontend assets
- realtime push updates
- metrics
