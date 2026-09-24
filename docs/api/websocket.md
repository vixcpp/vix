# WebSocket API

The WebSocket API lets a Vix application keep a real-time connection open between a client and the server.

```txt
HTTP → request/response
WebSocket → bidirectional live connection
```

## Public headers

```cpp
#include <vix/websocket.hpp>
#include <vix.hpp>                              // for HTTP routes
#include <vix/websocket/AttachedRuntime.hpp>    // for combined HTTP + WebSocket
```

## Basic architecture

```txt
Config → RuntimeExecutor → App + WebSocket Server → run_http_and_ws(...)
```

## Minimal HTTP + WebSocket app

```cpp
#include <vix.hpp>
#include <vix/websocket.hpp>

int main()
{
  vix::config::Config cfg{".env"};
  auto executor = std::make_shared<vix::executor::RuntimeExecutor>(1u);
  vix::App app{executor};
  vix::websocket::Server ws{cfg, executor};

  app.get("/health", [](vix::Request &, vix::Response &res){
    res.json({
      "ok", true,
      "service", "websocket-api"
    });
  });

  ws.on_open([](vix::websocket::Session &) {
    std::cout << "client connected\n";
  });

  ws.on_close([](vix::websocket::Session &) {
    std::cout << "client disconnected\n";
  });

  ws.on_error([](vix::websocket::Session &, const std::string &e) {
    std::cout << "error: " << e << "\n";
  });

  vix::run_http_and_ws(app, ws, executor, cfg);
  return 0;
}
```

## .env for WebSocket

```dotenv
SERVER_PORT=8080
SERVER_TLS_ENABLED=false
WEBSOCKET_MAX_MESSAGE_SIZE=65536
WEBSOCKET_IDLE_TIMEOUT=60
WEBSOCKET_ENABLE_DEFLATE=true
WEBSOCKET_PING_INTERVAL=30
WEBSOCKET_AUTO_PING_PONG=true
```

## WebSocket lifecycle

```cpp
ws.on_open([&ws](vix::websocket::Session &session) {
  /* client connected */
});

ws.on_close([](vix::websocket::Session &session) {
  /* client disconnected */
});

ws.on_error([](vix::websocket::Session &session, const std::string &error) {
  /* error */
});
```

## Typed messages

Client sends:

```json
{ "type": "chat.message", "payload": { "user": "Ada", "text": "Hello" } }
```

Server handles:

```cpp
ws.on_typed_message(
  [&ws](vix::websocket::Session &session,
        const std::string &type,
        const vix::json::kvs &payload)
  {
    if (type == "app.ping")
    {
      ws.broadcast_json(
        "app.pong", {"status", "ok", "transport", "websocket"}
      );
      return;
    }

    if (type == "chat.message")
    {
      const std::string text = payload.get_string_or("text", "");
      if (text.empty())
      {
        ws.broadcast_json(
          "chat.error", {"error", "text is required"}
        );
        return;
      }

      ws.broadcast_json(
        "chat.message", payload
      );
      return;
    }

    ws.broadcast_json(
      "app.unknown", {"type", type, "message", "Unknown event type"
    });
});
```

## Broadcast messages

```cpp
ws.broadcast_json(
  "system.connected", {"message", "A client connected"}
);

ws.broadcast_json(
  "chat.message", payload
);
```

## Recommended event protocol

| Event | Direction | Purpose |
|-------|-----------|---------|
| `app.ping` | client → server | Health check |
| `app.pong` | server → client | Health response |
| `chat.join` | client → server | User joins |
| `chat.leave` | client → server | User leaves |
| `chat.message` | both | Chat message |
| `chat.error` | server → client | Chat error |
| `system.connected` | server → client | Client connected |
| `system.disconnected` | server → client | Client disconnected |

## Run HTTP and WebSocket together

```cpp
// Full control
vix::run_http_and_ws(app, ws, executor, cfg);

// Compact form
vix::serve_http_and_ws(".env", 8080, [](auto &app, auto &ws){
  app.get("/", [](auto &, auto &res) { res.json({{"framework", "Vix.cpp"}}); });
    ws.on_typed_message([&ws](auto &, const std::string &type, const vix::json::kvs &payload){
      if (type == "chat.message")
        ws.broadcast_json(
          "chat.message", payload
        );

      else
        ws.broadcast_json(
          "app.unknown", {"type", type}
        );
  });
});
```

## WebSocket config fields

| Variable | Purpose | Default |
|----------|---------|---------|
| `WEBSOCKET_MAX_MESSAGE_SIZE` | Max message size in bytes | 65536 |
| `WEBSOCKET_IDLE_TIMEOUT` | Idle timeout in seconds | 60 |
| `WEBSOCKET_ENABLE_DEFLATE` | Per-message deflate | true |
| `WEBSOCKET_PING_INTERVAL` | Ping interval in seconds | 30 |
| `WEBSOCKET_AUTO_PING_PONG` | Automatic ping/pong | true |

```cpp
vix::websocket::Config ws_cfg = vix::websocket::Config::from_core(core_cfg);
std::cout << ws_cfg.maxMessageSize << "\n";
```

## WebSocket behind Nginx

```nginx
location /ws/ {
    proxy_pass http://127.0.0.1:8080;
    proxy_http_version 1.1;
    proxy_set_header Host $host;
    proxy_set_header Upgrade $http_upgrade;
    proxy_set_header Connection "upgrade";
    proxy_read_timeout 3600;
    proxy_send_timeout 3600;
}
```

Without upgrade headers, WebSocket connections fail through Nginx.

## WebSocket vs HTTP vs P2P

| | HTTP | WebSocket | P2P |
|-|------|-----------|-----|
| Pattern | Request/response | Bidirectional | Node-to-node |
| Best for | CRUD APIs | Live UI updates | Distributed sync |
| Discovery | Manual | Manual | Auto (UDP) |

## Complete chat example

```cpp
static void register_ws_lifecycle(vix::websocket::Server &ws)
{
  ws.on_open([&ws](vix::websocket::Session &){
    ws.broadcast_json("system.connected", {"message", "A client connected"});
  });

  ws.on_close([&ws](vix::websocket::Session &){
    ws.broadcast_json("system.disconnected", {"message", "A client disconnected"});
  });

  ws.on_error([](vix::websocket::Session &, const std::string &e){
    std::cout << "websocket error: " << e << "\n";
  });
}

static void register_ws_protocol(vix::websocket::Server &ws)
{
  ws.on_typed_message([&ws](vix::websocket::Session &,
                      const std::string &type,
                      const vix::json::kvs &payload)
    {
      if (type == "app.ping") {
        ws.broadcast_json(
          "app.pong", {"status", "ok"}
        );
        return;
      }

      if (type == "chat.join") {
        ws.broadcast_json(
          "chat.join", payload
        );
        return;
      }

      if (type == "chat.leave") {
        ws.broadcast_json(
          "chat.leave", payload
        );
        return;
      }

      if (type == "chat.message")
      {
        if (payload.get_string_or("text", "").empty())
        {
          ws.broadcast_json(
            "chat.error", {"error", "text is required"}
          );
          return;
        }
        ws.broadcast_json(
          "chat.message", payload
        );
        return;
      }

      ws.broadcast_json(
        "app.unknown", {"type", type, "message", "Unknown event type"}
      );
});
}
```

## Production checklist

- Validate message type and payload fields
- Set message size limits
- Configure idle timeout and ping/pong
- Authenticate clients (token, cookie, or signed handshake)
- Never trust user-supplied identity from payload
- Avoid broadcasting private data
- Configure Nginx upgrade headers
- Use HTTPS/WSS publicly

## Common mistakes

### Broadcasting unvalidated payloads

Always validate before broadcasting.

### Using WebSocket for everything

Use HTTP for CRUD APIs. Use WebSocket for live events only.

### Forgetting /health

Even real-time apps need HTTP health checks.

### Not handling unknown event types

Always have a fallback returning `app.unknown`.

## What you should remember

```cpp
vix::config::Config cfg{".env"};
auto executor = std::make_shared<vix::executor::RuntimeExecutor>(1u);
vix::App app{executor};
vix::websocket::Server ws{cfg, executor};
vix::run_http_and_ws(app, ws, executor, cfg);
```

HTTP handles requests. WebSocket handles live events.

Next: [Async API](/api/async)
