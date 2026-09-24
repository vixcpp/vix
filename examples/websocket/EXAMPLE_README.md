# Vix.cpp – WebSocket Examples

This directory contains all **WebSocket-only** example applications for Vix.cpp.
They demonstrate:

- The low-level WebSocket server
- The typed-message protocol
- The WebSocket client API
- Full chat applications
- HTTP + WebSocket hybrid servers
- Progressive learning: **minimal → simple → advanced**

These examples are intentionally written to look familiar to users coming from **Node.js, Python, or Go**, while retaining the raw performance of **modern C++**.

---

## 📁 Directory Structure

```
websocket/
│
├── simple_server.cpp
├── simple_client.cpp
├── chat_room.cpp
│
├── simple/
│   ├── CMakeLists.txt
│   ├── config/
│   ├── public/
│   └── src/
│       ├── app_example.cpp
│       ├── minimal_ws_http_server.cpp
│       ├── server_ws_http.cpp
│       ├── simple_client.cpp
│       └── simple_server.cpp
│
├── advanced/
│   ├── CMakeLists.txt
│   ├── config/
│   └── src/
│       ├── client.cpp
│       └── server.cpp
│
└── CMakeLists.txt
```

---

## 🚀 1. Quick Start – Minimal WebSocket Server

File: **simple_server.cpp**

```cpp
#include <vix/websocket.hpp>

using vix::websocket::Server;

int main() {
    Server ws;

    ws.on_open([](auto& session) {
        session.send_json("chat.system", {"text", "Welcome 👋"});
    });

    ws.on_typed_message([](auto& session,
                           const std::string& type,
                           const vix::json::kvs& payload)
    {
        if (type == "chat.message")
            session.broadcast_json("chat.message", payload);
    });

    ws.listen_blocking();
}
```

Build:

```bash
vix run server.cpp
```

---

## 💬 2. Minimal WebSocket Client

File: **simple_client.cpp**

```cpp
auto client = Client::create("localhost", "9090", "/");

client->on_open([] {
    std::cout << "Connected!" << std::endl;
});

client->send("chat.message", {"text", "Hello world!"});
```

Run:

```bash
vix run client.cpp
```

---

## 🧩 3. Chat Room Example

- Multiple rooms
- Typed messages
- Group broadcasting
- Multi-client coordination
- Clean logging

---

## 🌐 4. simple/ – HTTP + WebSocket + Frontend Demo

```bash
simple/src/simple_server.cpp
simple/src/server_ws_http.cpp
simple/src/minimal_ws_http_server.cpp
simple/src/app_example.cpp
```

Run:

```bash
cd websocket/simple
mkdir build && cd build
cmake ..
make
./simple_server
```

Open: http://localhost:8080

---

## ⚡ 5. advanced/ – Production-Style Real-Time System

Files:

```bash
advanced/src/server.cpp
advanced/src/client.cpp
```

Build:

```bash
cd websocket/advanced
mkdir build && cd build
cmake ..
make
./server
./client
```

---

## 🔌 6. Typed Message Protocol

```json
{
  "type": "chat.message",
  "payload": { "text": "Hello", "user": "Alice" }
}
```

Send:

```cpp
session.send_json("chat.message", {"text", "Hello!"});
```

---

## ⚙️ 7. ThreadPool Executor

```cpp
auto exec = vix::experimental::make_threadpool_executor(4, 8, 0);
Server ws(cfg, std::move(exec));
```

---


## 📦 9. Build All WebSocket Examples

```bash
cd examples/websocket
mkdir build && cd build
cmake ..
make -j
```

---

## ❤️ Contributing

PRs welcome!

---

## 📣 Final Note

**Modern Web. C++ Performance. Beautiful Developer Experience.**
