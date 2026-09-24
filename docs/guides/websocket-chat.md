# WebSocket chat

This guide shows how to build a small real-time chat system with Vix WebSocket.

## What you will build

HTTP routes: `GET /`, `GET /health`, `GET /rooms`

WebSocket events: `chat.message`, `chat.join`, `chat.leave`, `app.ping`

## Public headers

```cpp
#include <vix.hpp>
#include <vix/websocket.hpp>
#include <vix/websocket/AttachedRuntime.hpp>
```

## Setup

```bash
vix new websocket-chat
cd websocket-chat
```

## The Vix WebSocket model

Typed messages:

```json
{ "type": "chat.message", "payload": { "user": "Ada", "text": "Hello" } }
```

## Runtime struct

```cpp
struct ChatRuntime
{
  vix::config::Config config{".env"};
  std::shared_ptr<vix::executor::RuntimeExecutor> executor{
      std::make_shared<vix::executor::RuntimeExecutor>(1u)};
  vix::App app{executor};
  vix::websocket::Server ws{config, executor};
};
```

## Register lifecycle handlers

```cpp
static void register_ws_lifecycle(vix::websocket::Server &ws)
{
  ws.on_open([&ws](vix::websocket::Session &session)
             { ws.broadcast_json("system.connected", {"message", "A client connected"}); });

  ws.on_close([&ws](vix::websocket::Session &session)
              { ws.broadcast_json("system.disconnected", {"message", "A client disconnected"}); });

  ws.on_error([](vix::websocket::Session &session, const std::string &error) {});
}
```

## Register the chat protocol

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
        if (type == "chat.message" || type == "chat.join" || type == "chat.leave")
        {
          ws.broadcast_json(type, payload);
          return;
        }
        ws.broadcast_json("app.unknown", {"type", type, "message", "Unknown event type"});
      });
}
```

## Run HTTP and WebSocket together

```cpp
static void run(ChatRuntime &runtime)
{
  const int http_port = runtime.config.getServerPort();
  vix::run_http_and_ws(runtime.app, runtime.ws, runtime.executor, http_port);
}

int main()
{
  ChatRuntime runtime;
  configure(runtime);
  run(runtime);
  return 0;
}
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

## Add .env

```dotenv
SERVER_PORT=8080
WEBSOCKET_PORT=9090
SERVER_TLS_ENABLED=false
```

## Common mistakes

### Not validating incoming payloads

```cpp
const std::string text = payload.get_string_or("text", "");
if (text.empty()) {
  ws.broadcast_json(
    "chat.error",
    {"error", "text is required"}
  );

  return;
}
ws.broadcast_json("chat.message", payload);
```

### Trusting the username from the payload

For production, use the authenticated user from the server, not user-supplied names.

### Broadcasting private data to everyone

Use rooms or targeted sends for private chats.

## Production checklist

- Use HTTPS and WSS
- Authenticate WebSocket clients
- Validate all payloads
- Use rooms for private chat
- Persist important messages
- Run behind Nginx
- Run under systemd

## What to use next

- [Static files guide](/guides/static-files)
- [Templates guide](/guides/templates)
- [Production Nginx + systemd guide](/guides/production-nginx-systemd)
