# Vix WebSocket — Simple Examples

This document provides minimal, copy‑ready examples demonstrating how to use the
core components of the **Vix WebSocket Module**: WebSocket server, typed protocol,
rooms, SQLite storage, and the C++ client.

---

# ✅ Example 1 — Minimal WebSocket Echo Server

```cpp
#include <vix/websocket.hpp>
#include <vix/config/Config.hpp>

int main() {
    vix::config::Config cfg{"config/config.json"};
    auto exec = vix::experimental::make_threadpool_executor(2, 4, 0);

    vix::websocket::Server ws(cfg, exec);

    ws.on_message([](vix::websocket::Session& s, const std::string& text){
        s.send_text("echo: " + text);
    });

    ws.listen_blocking();
}
```

---

# ✅ Example 2 — Minimal Typed Handler

```cpp
ws.on_typed_message([](auto& session,
                       const std::string& type,
                       const vix::json::kvs& payload){
    if (type == "ping") {
        session.send_text(R"({"type":"pong"})");
    }
});
```

Test:

```
{"type":"ping"}
```

Response:

```
{"type":"pong"}
```

---

# ✅ Example 3 — Global Broadcast

```cpp
ws.on_typed_message([&ws](auto&, const std::string& type, const vix::json::kvs& kvs){
    if (type == "broadcast") {
        ws.broadcast_json("broadcast", kvs);
    }
});
```

All connected clients receive the same frame.

---

# ✅ Example 4 — Minimal Rooms

```cpp
ws.on_typed_message([&ws](auto& session, auto& type, auto& kvs){
    if (type == "join") {
        ws.join_room(session, kvs["room"]);
    }
    else if (type == "leave") {
        ws.leave_room(session, kvs["room"]);
    }
    else if (type == "msg") {
        ws.broadcast_room_json(
            kvs["room"],
            "msg",
            {"user", kvs["user"], "text", kvs["text"]}
        );
    }
});
```

Usage:

```
{"type":"join","payload":{"room":"africa"}}
{"type":"msg","payload":{"room":"africa","user":"Bob","text":"Hello"}}
```

---

# ✅ Example 5 — Minimal WebSocket Client

```cpp
#include <vix/websocket.hpp>
#include <iostream>

int main() {
    using vix::websocket::Client;

    auto client = Client::create("localhost", "9090", "/");

    client->on_open([&]{
        std::cout << "[client] connected\n";
        client->send("ping", {});
    });

    client->on_message([](const std::string& m){
        std::cout << "[recv] " << m << "\n";
    });

    client->connect();
    std::this_thread::sleep_for(std::chrono::hours(10));
}
```

---

# ✅ Example 6 — SQLite Storage (WAL)

```cpp
#include <vix/websocket/SqliteMessageStore.hpp>
#include <iostream>

int main() {
    vix::websocket::SqliteMessageStore store{"test.db"};

    vix::websocket::JsonMessage msg;
    msg.kind = "event";
    msg.type = "demo.hello";
    msg.room = "test";
    msg.payload = {"text", "Hello world"};

    store.append(msg);

    auto list = store.list_by_room("test", 10, std::nullopt);

    for (auto& m : list) {
        std::cout << m.id << " -> " << m.type << "\n";
    }
}
```

---

# ✅ Example 7 — High-Level API (ws::App)

```cpp
#include <vix/websocket/App.hpp>

int main() {
    vix::websocket::App app{"config/config.json"};

    app.ws("/chat", [](auto& session,
                       const std::string& type,
                       const vix::json::kvs& payload){
        if (type == "chat.message") {
            session.send_text(R"({"type":"ack"})");
        }
    });

    app.run_blocking();
}
```

---

# 📌 Notes

- All examples assume `config/config.json` defines the WebSocket port.
- These snippets are kept intentionally small for clarity.
- The full module provides: rooms, metrics, long‑polling bridge, persistent message store, and auto‑reconnect client behavior.

---

# 📄 License

MIT — part of the **Vix.cpp Runtime**.
