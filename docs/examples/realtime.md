# Real-Time (HTTP + WebSocket)

This guide shows beginner-friendly real-time examples with Vix.cpp.

It focuses on:

- Running HTTP and WebSocket together
- A simple typed message protocol: `{ "type": "...", "payload": { ... } }`
- Broadcasting messages to all connected clients
- Testing quickly with tools you can install in 2 minutes

All examples are single-file and keep logic inside `main()`.

---

## What you need

- Vix.cpp installed and available in your PATH (`vix` command)
- A terminal

Optional but recommended:

- `curl` (usually already installed)
- `websocat` (for WebSocket testing)

### Check Vix is installed

```bash
vix --version
```

---

## 1) Minimal HTTP + WebSocket server (single file)

This is the smallest real-time server you can build:

- HTTP route: `GET /`
- WebSocket: echoes `chat.message` to everyone

Create a file `main.cpp`:

```cpp
#include <vix.hpp>
#include <vix/websocket/AttachedRuntime.hpp>

using namespace vix;

int main()
{
  vix::serve_http_and_ws([](App& app, auto& ws)
  {
    // HTTP: normal route
    app.get("/", [](Request&, Response& res)
    {
      res.json({
        "message", "Hello from Vix.cpp",
        "realtime", true
      });
    });

    // WebSocket: typed messages { "type": "...", "payload": {...} }
    ws.on_typed_message([&ws](auto&, const std::string& type, const vix::json::kvs& payload)
    {
      if (type == "chat.message")
      {
        // Broadcast to everyone connected
        ws.broadcast_json("chat.message", payload);
      }
    });
  });

  return 0;
}
```

### Run it

If your Vix install supports `vix run`:

```bash
vix run main.cpp
```

If you build with CMake in your project, compile and run your binary as usual. The example is still the same.

### Test HTTP

Open a new terminal:

```bash
curl http://127.0.0.1:8080/
```

Expected output is JSON similar to:

```json
{"message":"Hello from Vix.cpp","realtime":true}
```

---

## 2) WebSocket testing with websocat

### Install websocat

On Ubuntu/Debian:

```bash
sudo apt-get update
sudo apt-get install -y websocat
```

### Connect to the server

In a new terminal:

```bash
websocat ws://127.0.0.1:9090/
```

Now paste this message and press Enter:

```json
{"type":"chat.message","payload":{"user":"Alice","text":"Hello from WS"}}
```

If you open two websocat terminals connected at the same time, both will receive the broadcast.

---

## 3) The message format (typed protocol)

Vix WebSocket examples use a simple convention:

```json
{
  "type": "chat.message",
  "payload": {
    "user": "Alice",
    "text": "Hello!"
  }
}
```

Rules of thumb:

- `type` tells the server what kind of event it is
- `payload` is the data of the event
- Keep payload small and consistent
- Always handle unknown types (for debugging)

---

## 4) Add join + typing events (still minimal)

This example adds two extra message types:

- `chat.join` (someone joined)
- `chat.typing` (typing indicator)
- `chat.message` (real message)
- `chat.unknown` (debug fallback)

Create `main.cpp`:

```cpp
#include <vix.hpp>
#include <vix/websocket/AttachedRuntime.hpp>

using namespace vix;

int main()
{
  vix::serve_http_and_ws("config/config.json", 8080, [](App& app, auto& ws)
  {
    app.get("/health", [](Request&, Response& res)
    {
      res.json({ "ok", true, "service", "realtime" });
    });

    ws.on_open([&ws](auto&)
    {
      ws.broadcast_json("chat.system", {
        "text", "A new client connected"
      });
    });

    ws.on_typed_message([&ws](auto&, const std::string& type, const vix::json::kvs& payload)
    {
      if (type == "chat.join")
      {
        ws.broadcast_json("chat.system", payload);
        return;
      }

      if (type == "chat.typing")
      {
        ws.broadcast_json("chat.typing", payload);
        return;
      }

      if (type == "chat.message")
      {
        ws.broadcast_json("chat.message", payload);
        return;
      }

      ws.broadcast_json("chat.unknown", {
        "type", type
      });
    });
  });

  return 0;
}
```

### Run it

```bash
vix run main.cpp
```

### Test HTTP

```bash
curl http://127.0.0.1:8080/health
```

### Test WebSocket

```bash
websocat ws://127.0.0.1:9090/
```

Paste these lines (one by one):

```json
{"type":"chat.join","payload":{"user":"Alice"}}
{"type":"chat.typing","payload":{"user":"Alice"}}
{"type":"chat.message","payload":{"user":"Alice","text":"hello"}}
{"type":"something.weird","payload":{"debug":true}}
```

---

## 5) Config examples (config/config.json)

These examples cover common setups.

### A) One port for HTTP and WebSocket

```json
{
  "server": {
    "port": 8080,
    "request_timeout": 5000
  },
  "websocket": {
    "port": 8080,
    "max_message_size": 65536,
    "idle_timeout": 600
  }
}
```

### B) Separate ports (most common)

```json
{
  "server": {
    "port": 8080,
    "request_timeout": 5000
  },
  "websocket": {
    "port": 9090,
    "max_message_size": 65536,
    "idle_timeout": 600,
    "ping_interval": 30,
    "auto_ping_pong": true
  }
}
```

### C) Dev mode (smaller limits)

```json
{
  "server": {
    "port": 8080,
    "request_timeout": 3000
  },
  "websocket": {
    "port": 9090,
    "max_message_size": 16384,
    "idle_timeout": 120,
    "ping_interval": 15,
    "auto_ping_pong": true
  }
}
```

### D) Production style (bigger limits)

```json
{
  "server": {
    "port": 8080,
    "request_timeout": 10000
  },
  "websocket": {
    "port": 9090,
    "max_message_size": 262144,
    "idle_timeout": 1800,
    "ping_interval": 30,
    "enable_deflate": true,
    "auto_ping_pong": true
  }
}
```

---

## 6) Debug checklist

### HTTP works but WS does not

- Check `websocket.port` in your config
- Connect explicitly:
  - `websocat ws://127.0.0.1:9090/`

### WS connects but no messages appear

- Make sure you send the typed format with `type` and `payload`
- Start two clients to see broadcast behavior

### Port already in use

- Change `server.port` and/or `websocket.port`
- Or stop the process using the port:
  - `lsof -i :8080`
  - `kill <pid>`

---

## 7) Practical mental model

- Use HTTP for bootstrapping:
  - `/`, `/health`, `/me`, `/config`
- Use WebSocket for live events:
  - chat messages, presence, typing, notifications
- Keep the protocol small:
  - a few `type` values, consistent `payload` fields
- Start with broadcast, then add rooms, auth, and persistence later

