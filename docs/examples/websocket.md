# WebSocket

Build a realtime HTTP + WebSocket application.

```txt
GET /          → app info
GET /health    → health check
WebSocket      → realtime typed events
```

## What you will build

WebSocket events:

```txt
app.ping     → server replies with app.pong
chat.join    → broadcast user joined
chat.message → broadcast chat message
chat.leave   → broadcast user left
```

## Create a workspace

```bash
mkdir -p ~/tmp/vix-examples/websocket
cd ~/tmp/vix-examples/websocket
touch main.cpp .env
```

## .env

```dotenv
SERVER_PORT=8080
SERVER_TLS_ENABLED=false
WEBSOCKET_MAX_MESSAGE_SIZE=65536
WEBSOCKET_IDLE_TIMEOUT=60
WEBSOCKET_ENABLE_DEFLATE=true
WEBSOCKET_PING_INTERVAL=30
WEBSOCKET_AUTO_PING_PONG=true
```

## Full code

```cpp
#include <iostream>
#include <memory>
#include <string>
#include <vix.hpp>
#include <vix/websocket.hpp>

static void register_http_routes(vix::App &app)
{
  app.get("/", [](vix::Request &, vix::Response &res){
    res.json({
      "message", "Vix WebSocket example",
      "realtime", true
    });
  });

  app.get("/health", [](vix::Request &, vix::Response &res){
    res.json({
      "ok", true,
      "service", "websocket-example",
      "http", "enabled",
      "websocket", "enabled"
      });
  });
}

static void register_ws_lifecycle(vix::websocket::Server &ws)
{
  ws.on_open([&ws](vix::websocket::Session &){
    std::cout << "[ws] client connected\n";

    ws.broadcast_json(
      "system.connected", {"message", "A client connected"}
    );
  });

  ws.on_close([&ws](vix::websocket::Session &){
    std::cout << "[ws] client disconnected\n";

    ws.broadcast_json(
      "system.disconnected", {"message", "A client disconnected"}
    );
  });

  ws.on_error([](vix::websocket::Session &, const std::string &e){
    std::cout << "[ws] error: " << e << "\n";
  });
}

static void register_ws_protocol(vix::websocket::Server &ws)
{
  ws.on_typed_message(
    [&ws](vix::websocket::Session &, const std::string &type, const vix::json::kvs &payload)
    {
      if (type == "app.ping")
      {
        ws.broadcast_json(
          "app.pong", {"ok", true, "transport", "websocket"}
        );
        return;
      }

      if (type == "chat.join")
      {
        const std::string user = payload.get_string_or("user", "");
        if (user.empty()) {
          ws.broadcast_json("chat.error", {"error", "user is required"});
          return;
        }

        ws.broadcast_json(
          "chat.join", {"user", user, "message", user + " joined"}
        );
        return;
      }

      if (type == "chat.message")
      {
        const std::string user = payload.get_string_or("user", "");
        const std::string text = payload.get_string_or("text", "");

        if (user.empty()) {
          ws.broadcast_json(
            "chat.error", {"error", "user is required"}
          );
          return;
        }

        if (text.empty()) {
          ws.broadcast_json(
            "chat.error", {"error", "text is required"}
          );
          return;
        }

        ws.broadcast_json(
          "chat.message", {"user", user, "text", text}
        );
        return;
      }

      if (type == "chat.leave")
      {
        const std::string user = payload.get_string_or("user", "unknown");

        ws.broadcast_json(
          "chat.leave", {"user", user, "message", user + " left"}
        );
        return;
      }

      ws.broadcast_json(
        "app.unknown", {"type", type, "message", "Unknown event type"}
      );

    });
}

int main()
{
  vix::config::Config cfg{".env"};
  auto executor = std::make_shared<vix::executor::RuntimeExecutor>(1u);

  vix::App app{executor};
  vix::websocket::Server ws{cfg, executor};

  register_http_routes(app);
  register_ws_lifecycle(ws);
  register_ws_protocol(ws);

  vix::run_http_and_ws(app, ws, executor, cfg);

  return 0;
}
```

## Run and test

```bash
vix run main.cpp
```

```bash
curl -i http://127.0.0.1:8080/health

# WebSocket with websocat
websocat ws://127.0.0.1:8080
```

```
{"type":"app.ping","payload":{} }
{"type":"chat.join","payload": {"user":"Ada"} }
{"type":"chat.message","payload":{"user":"Ada","text":"Hello from Vix"} }
{"type":"chat.leave","payload": {"user":"Ada"} }
{"type":"something.else","payload": {} }
```

## Event protocol

| Event | Direction | Purpose |
|-------|-----------|---------|
| `app.ping` | client → server | Health check |
| `app.pong` | server → client | Health response |
| `chat.join` | client → server | User joins |
| `chat.leave` | client → server | User leaves |
| `chat.message` | both | Chat message |
| `chat.error` | server → client | Validation error |
| `system.connected` | server → client | Client connected |
| `system.disconnected` | server → client | Client disconnected |
| `app.unknown` | server → client | Unknown event type |

## WebSocket config fields

| Variable | Purpose | Default |
|----------|---------|---------|
| `WEBSOCKET_MAX_MESSAGE_SIZE` | Max message size | 65536 |
| `WEBSOCKET_IDLE_TIMEOUT` | Idle timeout seconds | 60 |
| `WEBSOCKET_ENABLE_DEFLATE` | Compression | true |
| `WEBSOCKET_PING_INTERVAL` | Ping interval seconds | 30 |
| `WEBSOCKET_AUTO_PING_PONG` | Auto ping/pong | true |

## Nginx for WebSocket

```nginx
location / {
    proxy_pass http://127.0.0.1:8080;
    proxy_http_version 1.1;
    proxy_set_header Host $host;
    proxy_set_header Upgrade $http_upgrade;
    proxy_set_header Connection "upgrade";
    proxy_read_timeout 3600;
}
```

## Common mistakes

### Not validating payloads

```cpp
// Wrong
if (type == "chat.message") {
  ws.broadcast_json(
    "chat.message", payload
  );
}

// Correct
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
}
```

### Not handling unknown event types

Always add a fallback returning `app.unknown`.

### Using WebSocket for everything

Use HTTP for CRUD APIs. Use WebSocket for live events only.

## What you should remember

```cpp
vix::config::Config cfg{".env"};
auto executor = std::make_shared<vix::executor::RuntimeExecutor>(1u);
vix::App app{executor};
vix::websocket::Server ws{cfg, executor};
vix::run_http_and_ws(app, ws, executor, cfg);
```

The core idea: **HTTP handles requests — WebSocket handles live events.**

Next: [Cache](/examples/cache)
