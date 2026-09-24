# OpenAPI

This page explains the OpenAPI integration provided by the Vix WebSocket module.

Use it when you want WebSocket-related endpoints to appear in the Vix OpenAPI registry and documentation.

## Header

```cpp
#include <vix/websocket/openapi_docs.hpp>
```

Or use the umbrella header:

```cpp
#include <vix/websocket.hpp>
```

## What OpenAPI integration provides

The WebSocket module can register documentation for WebSocket and fallback HTTP endpoints.

It documents:

```txt
GET  /ws
GET  /ws/poll
POST /ws/send
GET  /metrics
```

These routes describe:

- the WebSocket upgrade endpoint
- the long-polling pull endpoint
- the long-polling push endpoint
- the metrics endpoint

The helper registers route metadata into the global Vix OpenAPI registry.

## Main function

The main helper is:

```cpp
vix::websocket::openapi::register_ws_docs(...)
```

Default signature shape:

```cpp
register_ws_docs(
    std::string ws_upgrade_path = "/ws",
    std::string lp_poll_path = "/ws/poll",
    std::string lp_send_path = "/ws/send",
    std::string metrics_path = "/metrics");
```

Use it to register WebSocket documentation once during app setup.

## Basic usage

```cpp
#include <vix.hpp>
#include <vix/websocket.hpp>

int main()
{
  vix::App app;

  vix::websocket::openapi::register_ws_docs();

  app.run(8080);

  return 0;
}
```

This registers documentation for the default WebSocket-related paths.

## Default documented routes

| Method | Path       | Purpose                     |
| ------ | ---------- | --------------------------- |
| `GET`  | `/ws`      | WebSocket upgrade endpoint. |
| `GET`  | `/ws/poll` | Long-polling pull endpoint. |
| `POST` | `/ws/send` | Long-polling push endpoint. |
| `GET`  | `/metrics` | WebSocket metrics endpoint. |

## WebSocket upgrade route

The WebSocket route is documented as:

```txt
GET /ws
```

This route represents the WebSocket upgrade endpoint.

A WebSocket client connects to it using a URL such as:

```txt
ws://localhost:9090/ws
```

or, depending on your server setup:

```txt
ws://localhost:9090/
```

The OpenAPI documentation describes the endpoint, but normal Swagger UI cannot fully execute a WebSocket upgrade.

## Why Swagger UI cannot fully test WebSocket

WebSocket starts as an HTTP request, then upgrades the connection using:

```txt
101 Switching Protocols
```

OpenAPI documentation can describe the route, but Swagger UI is mainly designed for request-response HTTP APIs.

So the WebSocket route is documented for discovery and client understanding, not as a fully interactive Swagger operation.

## WebSocket responses

The WebSocket upgrade route documents responses such as:

| Status | Meaning                                               |
| ------ | ----------------------------------------------------- |
| `101`  | Switching Protocols. The WebSocket upgrade succeeded. |
| `426`  | Upgrade Required. The route was called as plain HTTP. |

## WebSocket vendor extensions

The WebSocket route can include OpenAPI extension fields.

Conceptually:

```txt
x-ws-upgrade = true
x-ws-url = ws://<host>:<ws_port>/
```

These fields help signal that the route is a WebSocket endpoint, not a normal HTTP endpoint.

## Long-polling pull route

The long-polling pull route is:

```txt
GET /ws/poll
```

It lets HTTP fallback clients pull queued messages.

Example request:

```txt
GET /ws/poll?session_id=room:general&max=50
```

Query parameters:

| Parameter    | Purpose                               |
| ------------ | ------------------------------------- |
| `session_id` | Long-polling session or channel id.   |
| `max`        | Maximum number of messages to return. |

## Long-polling pull responses

Common responses:

| Status | Meaning                              |
| ------ | ------------------------------------ |
| `200`  | Messages returned successfully.      |
| `503`  | Long-polling bridge is not attached. |

Example response shape:

```json
[
  {
    "kind": "event",
    "room": "general",
    "type": "chat.message",
    "payload": {
      "text": "Hello"
    }
  }
]
```

## Long-polling push route

The long-polling push route is:

```txt
POST /ws/send
```

It lets HTTP clients send a typed message into the realtime system.

Example request:

```http
POST /ws/send
Content-Type: application/json
```

Body:

```json
{
  "session_id": "room:general",
  "room": "general",
  "type": "chat.message",
  "payload": {
    "text": "Hello from HTTP"
  }
}
```

## Long-polling push request body

The request body can contain:

| Field        | Required | Purpose                         |
| ------------ | -------- | ------------------------------- |
| `type`       | Yes      | Application event type.         |
| `payload`    | No       | Event data.                     |
| `session_id` | No       | Long-polling target session id. |
| `room`       | No       | Logical room or channel.        |
| `kind`       | No       | Message category.               |
| `id`         | No       | Message id.                     |
| `ts`         | No       | Timestamp.                      |

The only required field is:

```txt
type
```

## Long-polling push responses

Common responses:

| Status | Meaning                              |
| ------ | ------------------------------------ |
| `202`  | Message accepted and queued.         |
| `400`  | Invalid JSON body or missing fields. |
| `503`  | Long-polling bridge is not attached. |

## Metrics route

The metrics route is:

```txt
GET /metrics
```

It exposes WebSocket metrics in Prometheus text format when configured.

Example response shape:

```txt
# HELP vix_ws_connections_total Total WebSocket connections.
# TYPE vix_ws_connections_total counter
vix_ws_connections_total 12
```

Common responses:

| Status | Meaning                                         |
| ------ | ----------------------------------------------- |
| `200`  | Metrics returned successfully.                  |
| `501`  | Metrics endpoint is not configured in this app. |

## Register custom paths

You can override the default paths.

```cpp
vix::websocket::openapi::register_ws_docs(
    "/realtime",
    "/realtime/poll",
    "/realtime/send",
    "/realtime/metrics");
```

This registers:

```txt
GET  /realtime
GET  /realtime/poll
POST /realtime/send
GET  /realtime/metrics
```

Use custom paths when your application uses a different API structure.

## Example with attached runtime

```cpp
#include <memory>

#include <vix.hpp>
#include <vix/websocket.hpp>

int main()
{
  vix::App app;

  auto executor =
      std::make_shared<vix::executor::RuntimeExecutor>(4);

  vix::websocket::Server ws{app.config(), executor};

  ws.on_message(
    [](vix::websocket::Session &session, const std::string &message)
    {
      session.send_text("echo: " + message);
    });

  vix::websocket::openapi::register_ws_docs();

  vix::websocket::AttachedRuntime runtime{app, ws, executor};

  app.run(8080);

  return 0;
}
```

## Example with custom API paths

```cpp
#include <vix.hpp>
#include <vix/websocket.hpp>

int main()
{
  vix::App app;

  vix::websocket::openapi::register_ws_docs(
      "/api/ws",
      "/api/ws/poll",
      "/api/ws/send",
      "/api/ws/metrics");

  app.run(8080);

  return 0;
}
```

This documents:

```txt
GET  /api/ws
GET  /api/ws/poll
POST /api/ws/send
GET  /api/ws/metrics
```

## Relationship with HTTP API

The OpenAPI helper documents the HTTP-facing parts of the WebSocket module.

It does not implement the endpoints by itself.

The implementation still comes from:

```cpp
#include <vix/websocket/HttpApi.hpp>
```

and from how your application attaches a `LongPollingBridge`.

Think of it like this:

```txt
HttpApi
  -> implements fallback behavior

openapi_docs
  -> documents fallback behavior
```

## Relationship with WebSocket server

The WebSocket server runs the actual realtime connection.

```cpp
vix::websocket::Server ws{config, executor};
```

The OpenAPI route only documents how clients connect.

It does not create the WebSocket server.

## Relationship with long-polling

The documented routes:

```txt
GET  /ws/poll
POST /ws/send
```

belong to the long-polling fallback model.

They are useful when WebSocket is unavailable and clients need HTTP-compatible realtime delivery.

## Relationship with metrics

The documented route:

```txt
GET /metrics
```

describes the metrics endpoint.

The metrics endpoint can be served by:

```cpp
vix::websocket::run_metrics_http_exporter(metrics);
```

or by your main HTTP app if you expose metrics there.

## Documentation tags

The WebSocket OpenAPI helper uses tags such as:

```txt
ws
long-poll
metrics
```

These tags help group WebSocket-related endpoints in generated documentation.

## Recommended setup

For a full HTTP + WebSocket app, register docs during startup.

```cpp
vix::App app;

vix::websocket::openapi::register_ws_docs();

app.run(8080);
```

If your application uses custom paths, pass those paths explicitly.

```cpp
vix::websocket::openapi::register_ws_docs(
    "/api/ws",
    "/api/ws/poll",
    "/api/ws/send",
    "/api/ws/metrics");
```

## When to use OpenAPI docs

Use OpenAPI registration when:

- you expose `/ws/poll`
- you expose `/ws/send`
- you expose `/metrics`
- you want clients to discover the WebSocket endpoint
- you want Swagger or docs pages to show fallback APIs
- you want HTTP and realtime endpoints documented together

## When OpenAPI is not enough

OpenAPI cannot fully describe every runtime behavior of WebSocket.

It does not fully express:

- persistent bidirectional streams
- ongoing message exchange
- room subscriptions
- arbitrary realtime events
- full WebSocket protocol lifecycle

Use the WebSocket documentation pages for protocol behavior and use OpenAPI for discoverability.

## Message body example

The OpenAPI docs include an example request body for `/ws/send`.

```json
{
  "type": "chat.message",
  "room": "general",
  "payload": {
    "text": "Hello"
  }
}
```

This is the recommended basic shape for HTTP fallback push messages.

## Best practices

Register WebSocket docs once during startup.

Use paths that match your real endpoints.

Document WebSocket upgrade routes even if Swagger cannot execute them.

Use `/ws/poll` and `/ws/send` for long-polling fallback.

Keep message examples simple.

Use clear tags for realtime endpoints.

Keep OpenAPI docs aligned with your actual HTTP API.

## Common mistakes

### Expecting Swagger UI to open WebSocket streams

Swagger UI can show the route, but it does not behave like a WebSocket client.

Use a browser WebSocket client, JavaScript, or the Vix WebSocket client to test the stream.

### Registering docs without implementing endpoints

OpenAPI registration only documents routes.

Make sure the actual HTTP API is implemented and connected to a `LongPollingBridge`.

### Using different paths in docs and code

Avoid documenting:

```txt
/ws/poll
```

while implementing:

```txt
/api/ws/poll
```

Pass custom paths to `register_ws_docs(...)` if needed.

### Forgetting the metrics endpoint

If you expose metrics, document it.

If you do not expose metrics, either leave it documented as not configured or use custom docs that match your app.

## Next steps

Continue with:

- [Shutdown](./shutdown.md)
- [HTTP API](./http-api.md)
- [Long polling](./long-polling.md)
- [Metrics](./metrics.md)
- [API Reference](./api-reference.md)
