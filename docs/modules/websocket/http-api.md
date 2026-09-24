# HTTP API

This page explains the HTTP helper API provided by the Vix WebSocket module.

Use it when you want to expose WebSocket-compatible fallback endpoints over HTTP, especially for long-polling clients.

## Header

```cpp
#include <vix/websocket/HttpApi.hpp>
```

Or use the umbrella header:

```cpp
#include <vix/websocket.hpp>
```

## What the HTTP API provides

The HTTP API connects the WebSocket runtime with normal HTTP routes.

It is mainly used for long-polling fallback.

It provides helpers for endpoints such as:

```txt
GET  /ws/poll
POST /ws/send
```

These endpoints allow HTTP-only clients to interact with the same message model used by WebSocket clients.

## Basic model

The model is:

```txt
WebSocket clients
  -> persistent WebSocket connection

HTTP fallback clients
  -> GET /ws/poll
  -> POST /ws/send

LongPollingBridge
  -> shared message bridge
```

This lets one application support both realtime WebSocket clients and HTTP fallback clients.

## Common endpoints

| Endpoint        | Purpose                             |
| --------------- | ----------------------------------- |
| `GET /ws/poll`  | Poll queued messages for a session. |
| `POST /ws/send` | Send a typed message through HTTP.  |

These endpoints are also documented by the WebSocket OpenAPI helper.

## GET /ws/poll

`GET /ws/poll` lets an HTTP client pull queued messages.

Example request:

```txt
GET /ws/poll?session_id=room:general&max=50
```

The endpoint reads:

| Query parameter | Purpose                               |
| --------------- | ------------------------------------- |
| `session_id`    | Long-polling session or channel id.   |
| `max`           | Maximum number of messages to return. |

Example response:

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

## POST /ws/send

`POST /ws/send` lets an HTTP client push a typed message.

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

The endpoint can push this message into the long-polling bridge.

If configured, it can also forward the message to WebSocket clients.

## Message shape

The HTTP API uses the same `JsonMessage` model as the WebSocket layer.

A message can contain:

```txt
session_id
id
kind
room
type
ts
payload
```

The most important field is:

```txt
type
```

A minimal HTTP message is:

```json
{
  "type": "chat.message",
  "payload": {
    "text": "Hello"
  }
}
```

## session_id

`session_id` identifies the long-polling buffer.

Example:

```json
{
  "session_id": "room:general",
  "type": "chat.message",
  "payload": {
    "text": "Hello"
  }
}
```

If your application uses room-based fallback, a common convention is:

```txt
room:<room-name>
```

Example:

```txt
room:general
room:support
room:project:42
```

## room

`room` identifies the logical WebSocket room or channel.

Example:

```json
{
  "room": "general",
  "type": "chat.message",
  "payload": {
    "text": "Hello"
  }
}
```

The long-polling bridge can use this room to resolve the fallback session id.

By default:

```txt
room = general
  -> session_id = room:general
```

## type

`type` identifies the application event.

Example:

```json
{
  "type": "notification.created",
  "payload": {
    "title": "New order"
  }
}
```

Use clear event names:

```txt
chat.message
room.join
room.leave
presence.update
notification.created
order.updated
```

## payload

`payload` contains the event data.

Example:

```json
{
  "type": "chat.message",
  "payload": {
    "text": "Hello",
    "user_id": "42"
  }
}
```

Keep payloads small.

For large uploads or downloads, use normal HTTP file endpoints instead of WebSocket or long-polling messages.

## Query parsing helpers

The HTTP API includes helpers for reading query parameters from request targets.

It supports percent-decoding for URL query components.

Example:

```txt
/ws/poll?session_id=room%3Ageneral&max=50
```

The decoded `session_id` becomes:

```txt
room:general
```

The helper supports both Vix HTTP requests and Beast-like request targets.

## Long-polling bridge requirement

The HTTP API needs a `LongPollingBridge` to serve fallback messages.

Conceptually:

```txt
GET /ws/poll
  -> bridge.poll(session_id, max)

POST /ws/send
  -> parse JsonMessage
  -> bridge.send_from_http(session_id, message)
```

If the bridge is not attached, HTTP endpoints should return a service error.

Common response:

```txt
503 Service Unavailable
```

## Poll flow

The poll flow is:

```txt
HTTP client
  -> GET /ws/poll?session_id=room:general&max=50
  -> HTTP API reads query params
  -> LongPollingBridge::poll(...)
  -> LongPollingManager drains messages
  -> response JSON array
```

This lets HTTP clients receive queued realtime messages.

## Send flow

The send flow is:

```txt
HTTP client
  -> POST /ws/send
  -> HTTP API parses JSON body
  -> creates JsonMessage
  -> LongPollingBridge::send_from_http(...)
  -> message enters fallback buffer
  -> optional WebSocket forward callback runs
```

This lets HTTP clients push messages into the realtime system.

## Browser poll example

```js
async function poll() {
  const response = await fetch("/ws/poll?session_id=room:general&max=50");

  if (!response.ok) {
    console.error("poll failed:", response.status);
    setTimeout(poll, 1000);
    return;
  }

  const messages = await response.json();

  for (const message of messages) {
    console.log("message:", message);
  }

  setTimeout(poll, 1000);
}

poll();
```

## Browser send example

```js
await fetch("/ws/send", {
  method: "POST",
  headers: {
    "Content-Type": "application/json",
  },
  body: JSON.stringify({
    session_id: "room:general",
    room: "general",
    type: "chat.message",
    payload: {
      text: "Hello from HTTP",
    },
  }),
});
```

## C++ bridge example

```cpp
vix::websocket::WebSocketMetrics metrics;

vix::websocket::LongPollingBridge bridge{
    &metrics,
    std::chrono::seconds{60},
    256};
```

Then attach it to the WebSocket server:

```cpp
ws.attach_long_polling_bridge(&bridge);
```

The WebSocket server can now forward parsed typed WebSocket messages into the bridge.

## HTTP to WebSocket forwarding

The bridge can also forward HTTP-originated messages to WebSocket clients.

This is done with an optional callback:

```cpp
using HttpToWsForward =
    std::function<void(const JsonMessage &)>;
```

Conceptually:

```txt
POST /ws/send
  -> bridge.send_from_http(...)
  -> enqueue for long-polling
  -> forward to WebSocket broadcast
```

This allows HTTP fallback clients and WebSocket clients to participate in the same realtime channel.

## Example forwarding callback

```cpp
vix::websocket::LongPollingBridge bridge{
    manager,
    {},
    [&ws](const vix::websocket::JsonMessage &message)
    {
      if (!message.room.empty())
      {
        ws.broadcast_text_to_room(message.room, message.to_json_string());
        return;
      }

      ws.broadcast_text(message.to_json_string());
    }};
```

This pattern lets `/ws/send` update both HTTP polling clients and WebSocket clients.

## HTTP API and OpenAPI

The OpenAPI helper documents the HTTP API endpoints.

Documented routes include:

```txt
GET  /ws/poll
POST /ws/send
```

The send endpoint documents a JSON body with fields such as:

```txt
session_id
room
type
kind
id
ts
payload
```

The required field is:

```txt
type
```

## Response codes

Common response codes:

| Status | Meaning                                       |
| ------ | --------------------------------------------- |
| `200`  | Poll returned successfully.                   |
| `202`  | Message accepted by `/ws/send`.               |
| `400`  | Invalid JSON body or missing required fields. |
| `503`  | Long-polling bridge is not attached.          |

## Metrics

HTTP polling can update long-polling metrics.

Relevant metrics include:

```txt
lp_polls_total
lp_sessions_total
lp_sessions_active
lp_messages_buffered
lp_messages_enqueued_total
lp_messages_drained_total
```

These metrics are stored in:

```cpp
vix::websocket::WebSocketMetrics
```

and can be exported through:

```txt
GET /metrics
```

## Security considerations

The HTTP API is a transport bridge.

It does not automatically replace authentication or authorization.

Before using it in production, make sure your app validates:

- who can poll a session
- who can send messages
- which rooms a user can access
- maximum body size
- allowed message types
- payload shape

Rooms and session ids are delivery mechanisms.

They are not security boundaries by themselves.

## Recommended usage

Use the HTTP API as a fallback layer.

The preferred model is:

```txt
try WebSocket first
  -> if unavailable, use HTTP long-polling
```

Use the same typed message shape for both transports:

```json
{
  "type": "domain.action",
  "room": "optional-room",
  "payload": {
    "key": "value"
  }
}
```

This keeps client logic and server logic consistent.

## Best practices

Use `session_id` consistently.

Use `room:<name>` for room-based fallback buffers.

Keep payloads small.

Validate message types.

Validate room access before sending or polling.

Return clear HTTP status codes.

Use metrics to observe fallback usage.

Use WebSocket as the primary transport and HTTP API as the fallback.

## Common mistakes

### Exposing fallback endpoints without authorization

Do not let any client poll any session id.

Validate access before returning messages.

### Confusing room and session id

A room is the logical channel.

A session id is the long-polling buffer key.

The default bridge maps:

```txt
room = general
  -> session_id = room:general
```

### Sending huge payloads

Avoid large JSON bodies through `/ws/send`.

Use normal HTTP upload for large data.

### Treating long-polling as the main realtime path

Long-polling is useful, but WebSocket should remain the primary transport when available.

### Forgetting bridge attachment

If no bridge is attached, `/ws/poll` and `/ws/send` cannot work.

Attach a `LongPollingBridge` before exposing these endpoints.

## Next steps

Continue with:

- [Attached runtime](./attached-runtime.md)
- [Long polling](./long-polling.md)
- [Messages](./messages.md)
- [OpenAPI](./openapi.md)
- [Metrics](./metrics.md)
