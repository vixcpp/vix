# Vix WebSocket Module

High-Performance • Async • Typed Protocol • Rooms • Persistent Storage • Prometheus Metrics  
Part of the **Vix.cpp Runtime** — offline-first, real-time, P2P-friendly.

The Vix WebSocket module provides a modern, production-ready WebSocket stack for C++20.  
It is built for high-performance backend runtimes, chat systems, IoT, dashboards, and
offline-first applications such as **Softadastra Chat**.

---

# ✨ Features

## 🔌 High-Performance WebSocket Server (Asio + Beast)

- Fully asynchronous server
- Uses the Vix ThreadPool executor
- Optimized dispatch loop
- Tunable message limits, ping interval, idle timeout
- Transparent backpressure handling

---

## 🏠 Room-Based Messaging (join/leave/broadcast)

- `join_room(session, "room")`
- `leave_room(session, "room")`
- `broadcast_room_json(room, type, payload)`
- Automatic membership tracking
- Supports millions of messages per channel

---

## 💾 Persistent Storage (SQLite + WAL)

### Abstract API

```cpp
struct MessageStore {
    virtual ~MessageStore() = default;

    virtual std::string append(
        const std::string& kind,
        const std::string& room,
        const std::string& type,
        const nlohmann::json& payload) = 0;

    virtual std::vector<StoredMessage> list_by_room(
        const std::string& room,
        size_t limit,
        const std::string& before_id) = 0;

    virtual std::vector<StoredMessage> replay_from(
        const std::string& id) = 0;
};
```

### SQLite Implementation

- WAL journaling for crash-safe durability
- Ordered message IDs
- Fast replay after reconnect
- Chat history persistence

```cpp
vix::websocket::SqliteMessageStore store{"chat_messages.db"};
store.append(message);
auto history = store.list_by_room("africa", 50, std::nullopt);
```

---

## 📊 Prometheus Metrics

Built-in `/metrics` endpoint:

- Active WebSocket sessions
- Total connections
- Messages in/out
- Errors

Example:

```cpp
WebSocketMetrics metrics;

std::thread([&]{
    run_metrics_server(metrics, "0.0.0.0", 9100);
}).detach();
```

Then scrape:

```
GET /metrics
```

---

## 🔧 Typed JSON Protocol

Every frame uses:

```json
{
  "type": "chat.message",
  "payload": {
    "user": "Gaspard",
    "room": "africa",
    "text": "Hello!"
  }
}
```

Helpers:

- `JsonMessage::serialize(type, kvs)`
- `ws_kvs_to_nlohmann()` for KVS → JSON
- Consistent server/client protocol

---

# 📦 Installation

This module is included inside the Vix umbrella:

```
vix/modules/websocket/
```

Enable it in CMake:

```cmake
add_subdirectory(modules/websocket websocket_build)
target_link_libraries(vix INTERFACE vix::websocket)
```

Include in code:

```cpp
#include <vix/websocket.hpp>
```

---

# 🚀 Basic Usage

## 1) Start the WebSocket server

```cpp
vix::websocket::Server ws(cfg, executor);
ws.listen_blocking();
```

## 2) Handle connection open

```cpp
ws.on_open([](Session &s) {
    s.send_text("Welcome!");
});
```

## 3) Handle typed events

```cpp
ws.on_typed_message([](Session&, const std::string &type, const kvs &payload) {
    if (type == "chat.message") {
        // process
    }
});
```

## 4) Join a room

```cpp
ws.join_room(session, "africa");
```

## 5) Broadcast JSON frame

```cpp
ws.broadcast_room_json("africa", "chat.message", {"text", "Hello!"});
```

---

# 🧱 Architecture

```
modules/websocket/
│
├─ include/vix/websocket/
│   ├─ server.hpp
│   ├─ client.hpp
│   ├─ session.hpp
│   ├─ router.hpp
│   ├─ protocol.hpp
│   ├─ config.hpp
│   ├─ MessageStore.hpp
│   ├─ SqliteMessageStore.hpp
│   └─ websocket.hpp     # aggregator
│
└── src/
    ├─ server.cpp
    ├─ session.cpp
    ├─ router.cpp
    ├─ SqliteMessageStore.cpp
    └─ ...
```

---

# 📡 Minimal Chat Example

```cpp
#include <vix/websocket.hpp>
#include <vix/config/Config.hpp>
#include <nlohmann/json.hpp>

int main() {
    vix::config::Config cfg{"config/config.json"};
    auto exec = vix::experimental::make_threadpool_executor(4, 8, 0);

    vix::websocket::Server ws(cfg, exec);
    vix::websocket::SqliteMessageStore store{"chat_messages.db"};

    ws.on_open([](auto& session) {
        session.send_text(JsonMessage::serialize(
            "chat.system",
            {"user", "server", "text", "Welcome!"}
        ));
    });

    ws.on_typed_message([&](auto& session, auto& type, auto& kvs) {
        auto j = ws_kvs_to_nlohmann(kvs);

        if (type == "chat.join") {
            std::string room = j.value("room", "");
            ws.join_room(session, room);

            ws.broadcast_room_json(room, "chat.system",
                {"text", j["user"].get<std::string>() + " joined"});
        }
    });

    ws.listen_blocking();
}
```

---

# 🧪 SQLite Inspection Cheatsheet

```bash
sqlite3 chat_messages.db
.tables
.schema ws_messages
SELECT id, room, type, substr(payload_json,1,80)
FROM ws_messages LIMIT 5;
```

---

# ⚙️ Configuration Example (`config.json`)

```json
{
  "websocket": {
    "port": 9090,
    "maxMessageSize": 65536,
    "idleTimeout": 600,
    "pingInterval": 30
  }
}
```

---

# 📂 Directory Layout Summary

```
modules/websocket/
│
├─ include/vix/websocket/
│   ├─ client.hpp
│   ├─ server.hpp
│   ├─ session.hpp
│   ├─ router.hpp
│   ├─ protocol.hpp
│   ├─ MessageStore.hpp
│   ├─ SqliteMessageStore.hpp
│   └─ websocket.hpp
│
├─ src/
│   ├─ server.cpp
│   ├─ session.cpp
│   ├─ router.cpp
│   ├─ SqliteMessageStore.cpp
│
└─ examples/
    ├─ simple_server.cpp
    ├─ simple_client.cpp
```

---

# 🛣 Roadmap

| Feature                      | Status  |
| ---------------------------- | ------- |
| Dedicated WebSocket server   | ✅      |
| Typed JSON protocol          | ✅      |
| Rooms (join/leave/broadcast) | ✅      |
| SQLite storage (WAL)         | ✅      |
| Replay by ID                 | ✅      |
| Prometheus /metrics          | ✅      |
| Presence (online/offline)    | Planned |
| Auto-reconnect client        | ✅      |
| Binary frames                | Planned |
| Encrypted channels           | Planned |
| Batch messages               | Planned |

# 📚 Documentation

The WebSocket module ships with complete, structured documentation inside the `docs/` folder:

- **[ws/EXAMPLES.md](ws/EXAMPLES.md)**  
  Simple & advanced examples, including rooms, history replay, and persistence.

- **[ws/CLIENT_GUIDE.md](ws/CLIENT_GUIDE.md)**  
  Guide for building clients (C++ console + Browser UI).

- **[ws/LONGPOLLING.md](ws/LONGPOLLING.md)**  
  Explains the offline-first architecture with WebSocket → LongPolling fallback.

- **[ws/TECHNICAL.md](ws/TECHNICAL.md)**  
  Full expanded documentation, architecture, diagrams, protocol details.

- **[ws/vix_websocket_examples.md](ws/vix_websocket_examples.md)**  
  Extra examples generated for quick testing and integration.

Documentation directory:

---

# 📝 License

**MIT License**  
Included inside the Vix.cpp repository.

---

# 🔥 Summary

The **Vix WebSocket Module** is a modern, room-based, typed, persistent and metrics-aware WebSocket runtime for C++20, built for real-time backends, chat systems, IoT, and offline-first apps.

Fast • Reliable • Protocol-Driven • Production-Ready  
Part of **Vix.cpp — the Offline-First, P2P-Ready C++ Backend Runtime**.
