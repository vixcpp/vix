# WebSocket

Vix supports real-time messaging with a simple typed protocol.

A typed message has this shape:

``` json
{
  "type": "chat.message",
  "payload": { "room": "general", "text": "hello" }
}
```

In C++, Vix gives you:

-   `on_open(...)` to handle new connections
-   `on_typed_message(...)` to handle `{type, payload}` messages
-   `broadcast_json(...)` to broadcast to all sessions
-   optional: room broadcasting (`broadcast_room_json(...)`)
-   optional: Long Polling bridge for environments where WebSocket is
    blocked

This guide shows two minimal setups:

1)  One process serving HTTP + WebSocket (with long polling bridge)
2)  A standalone WebSocket server

All examples are minimal and keep everything in `main()`.

------------------------------------------------------------------------

## 1) HTTP + WS in one process (with long polling bridge)

This is useful when you want a single binary that serves:

-   normal HTTP routes
-   WebSocket events
-   fallback long polling endpoints

``` cpp
#include <vix.hpp>

#include <vix/websocket/AttachedRuntime.hpp>
#include <vix/websocket/HttpApi.hpp>
#include <vix/websocket/LongPollingBridge.hpp>

#include <chrono>
#include <memory>
#include <string>

using namespace vix;

// Small helper to read a string from vix::json::kvs payload
static std::string token_to_string(const vix::json::token& t)
{
  if (auto s = std::get_if<std::string>(&t.v))
    return *s;
  return {};
}

static std::string kvs_get_string(const vix::json::kvs& obj, const std::string& key)
{
  const auto& f = obj.flat;
  for (std::size_t i = 0; i + 1 < f.size(); i += 2)
  {
    const std::string k = token_to_string(f[i]);
    if (k == key)
      return token_to_string(f[i + 1]);
  }
  return {};
}

int main()
{
  vix::serve_http_and_ws([](auto& app, auto& ws)
  {
    static vix::websocket::WebSocketMetrics metrics;

    auto bridge = std::make_shared<vix::websocket::LongPollingBridge>(
      &metrics,
      std::chrono::seconds{60}, // session ttl
      256                        // max messages buffered
    );

    ws.attach_long_polling_bridge(bridge);

    // HTTP route
    app.get("/", [](auto&, auto& res)
    {
      res.json({"message", "Hello from Vix.cpp"});
    });

    // Long polling HTTP endpoints (client fallback)
    app.get("/ws/poll", [&ws](auto& req, auto& res)
    {
      vix::websocket::http::handle_ws_poll(req, res, ws);
    });

    app.post("/ws/send", [&ws](auto& req, auto& res)
    {
      vix::websocket::http::handle_ws_send(req, res, ws);
    });

    app.get("/ws/status", [&ws](auto&, auto& res)
    {
      auto b = ws.long_polling_bridge();
      res.json(vix::json::kv({
        {"bridge_attached", static_cast<bool>(b)},
        {"sessions", b ? static_cast<int>(b->session_count()) : 0}
      }));
    });

    // Typed message handler
    ws.on_typed_message([&ws](auto&, const std::string& type, const vix::json::kvs& payload)
    {
      if (type != "chat.message")
        return;

      const std::string room = kvs_get_string(payload, "room");

      if (!room.empty())
        ws.broadcast_room_json(room, "chat.message", payload);
      else
        ws.broadcast_json("chat.message", payload);
    });
  });

  return 0;
}
```

What this provides:

-   `/` normal HTTP
-   `/ws/poll` long polling receive
-   `/ws/send` long polling send
-   `/ws/status` bridge status

------------------------------------------------------------------------

## 2) Standalone WebSocket server

This is useful when you want a dedicated WS service.

``` cpp
#include <vix/config/Config.hpp>
#include <vix/experimental/ThreadPoolExecutor.hpp>
#include <vix/websocket.hpp>

#include <string>

int main()
{
  using vix::websocket::Server;

  // Load config (example path)
  vix::config::Config cfg{"config/config.json"};

  // Thread pool executor
  auto exec = vix::experimental::make_threadpool_executor(
    4, // min threads
    8, // max threads
    0  // default priority
  );

  Server ws(cfg, std::move(exec));

  ws.on_open([&ws](auto& session)
  {
    (void)session;

    ws.broadcast_json("chat.system", {
      "user", "server",
      "text", "Welcome to the WebSocket server"
    });
  });

  ws.on_typed_message([&ws](auto& session, const std::string& type, const vix::json::kvs& payload)
  {
    (void)session;

    if (type == "chat.message")
    {
      ws.broadcast_json("chat.message", payload);
      return;
    }

    ws.broadcast_json("chat.unknown", {
      "type", type,
      "info", "Unknown message type"
    });
  });

  ws.listen_blocking();

  return 0;
}
```

------------------------------------------------------------------------

## Minimal client message

Any client (browser, Node.js, mobile) can send a typed message:

``` json
{
  "type": "chat.message",
  "payload": {
    "room": "general",
    "user": "Alice",
    "text": "Hello!"
  }
}
```

------------------------------------------------------------------------

## Notes

-   Keep handlers small: parse, validate, broadcast.
-   Use the long polling bridge when WebSocket is blocked by networks or
    proxies.
-   For multi-room chat, include a `room` field in payload and broadcast
    to rooms.


