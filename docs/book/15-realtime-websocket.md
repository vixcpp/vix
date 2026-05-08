# Real-time WebSocket

In the previous chapter, you connected a Vix app to a database.
Now you will learn WebSocket.

HTTP is request/response — the connection ends.
WebSocket stays open for real-time communication.

```txt
client connects → connection stays open → client sends events → server sends events → many clients receive updates
```

## When to use WebSocket

Use HTTP for:
CRUD APIs,
page loading,
authentication,
normal JSON APIs.

Use WebSocket for:
live messages,
dashboards,
presence,
streaming events,
notifications.

## Public headers

```cpp
#include <vix.hpp>
#include <vix/websocket.hpp>
#include <vix/websocket/AttachedRuntime.hpp>  // for HTTP + WebSocket together
```

## Vix typed message model

```json
{ "type": "chat.message", "payload": { "user": "Ada", "text": "Hello" } }
```

## Minimal WebSocket server

```cpp
#include <memory>
#include <vix.hpp>
#include <vix/websocket.hpp>

int main()
{
  vix::config::Config config{".env"};
  auto executor = std::make_shared<vix::executor::RuntimeExecutor>(1u);
  vix::websocket::Server ws{config, executor};

  ws.on_open([&ws](vix::websocket::Session &session){
    ws.broadcast_json(
      "system.connected",
      {"message", "A client connected"
    });
  });

  ws.on_message([&ws](vix::websocket::Session &session, const std::string &payload){
    ws.broadcast_json(
      "echo.raw",
      {"text", payload});
  });

  ws.on_typed_message([&ws](vix::websocket::Session &session,
                            const std::string &type,
                            const vix::json::kvs &payload){
    ws.broadcast_json(type, payload);
  });

  ws.listen_blocking();
  return 0;
}
```

## HTTP + WebSocket together

```cpp
struct BasicRuntime
{
  vix::config::Config config{".env"};
  std::shared_ptr<vix::executor::RuntimeExecutor> executor{
      std::make_shared<vix::executor::RuntimeExecutor>(1u)};
  vix::App app{executor};
  vix::websocket::Server ws{config, executor};
};
```

### Register HTTP routes

```cpp
static void register_http_routes(vix::App &app)
{
  app.get("/", [](vix::Request &, vix::Response &res){
    res.json({"name", "Vix HTTP + WebSocket"});
  });

  app.get("/health", [](vix::Request &, vix::Response &res){
    res.json({ "status", "ok", "service", "http-ws" });
  });
}
```

### Register WebSocket protocol

```cpp
static void register_ws_protocol(vix::websocket::Server &ws)
{
  ws.on_typed_message(
      [&ws](vix::websocket::Session &session,
            const std::string &type,
            const vix::json::kvs &payload)
    {
      if (type == "app.ping")
      {
        ws.broadcast_json("app.pong", {"status", "ok", "transport", "websocket"});
        return;
      }
      if (type == "chat.message")
      {
        ws.broadcast_json("chat.message", payload);
        return;
      }
      ws.broadcast_json("app.unknown", {"type", type, "message", "Unknown event type"});
  });
}
```

### Run together

```cpp
vix::run_http_and_ws(runtime.app, runtime.ws, runtime.executor, runtime.config.getServerPort());
```

## Compact alternative: `serve_http_and_ws`

```cpp
vix::serve_http_and_ws(".env", 8080, [](auto &app, auto &ws) {

  app.get("/", [](auto &, auto &res) {
    res.json({"framework", "Vix.cpp"});
  });

  ws.on_typed_message([&ws](auto &, const std::string &type, const vix::json::kvs &payload) {
    if (type == "chat.message") ws.broadcast_json("chat.message", payload);
  });

});
```

## Recommended event protocol

| Event                 | Direction        | Purpose                           |
|-----------------------|------------------|-----------------------------------|
| `chat.join`           | Client -> server | Joins a chat room or session.     |
| `chat.leave`          | Client -> server | Leaves a chat room or session.    |
| `chat.message`        | Both directions  | Sends or receives chat content.   |
| `chat.error`          | Server -> client | Reports a chat-related error.     |
| `app.ping`            | Client -> server | Sends a health check request.     |
| `app.pong`            | Server -> client | Returns a health check response.  |
| `system.connected`    | Server -> client | Confirms the client connected.    |
| `system.disconnected` | Server -> client | Confirms the client disconnected. |

## Validate WebSocket payloads

```cpp
if (type == "chat.message")
{
  const std::string text = payload.get_string_or("text", "");
  if (text.empty())
  {
    ws.broadcast_json(
      "chat.error", {
        "error", "message_required",
        "message", "Message text is required"
      }
    );
    return;
  }
  ws.broadcast_json("chat.message", payload);
}
```

## WebSocket and Nginx

```nginx
proxy_set_header Upgrade $http_upgrade;
proxy_set_header Connection "upgrade";
proxy_read_timeout 3600;
proxy_send_timeout 3600;
```

## Common mistakes

### Using WebSocket for normal CRUD

Use HTTP for create/read/update/delete.
Use WebSocket for live events only.

### Broadcasting unvalidated payloads

Validate payloads before broadcasting.

### Forgetting Nginx upgrade headers

Without upgrade headers, WebSocket fails through Nginx.

### Not handling unknown event types

Always have a fallback for unknown types returning `app.unknown`.

## What you should remember

HTTP is request/response.
WebSocket is a long-lived real-time connection.

Vix WebSocket uses:
`Server`,
`Session`,
`on_open`,
`on_close`,
`on_error`,
`on_message`,
`on_typed_message`,
`broadcast_json`.

The core idea:
use HTTP for normal API requests and WebSocket for live events that must reach clients immediately.

## Next chapter

[Next: Async runtime](/book/16-async-runtime)
