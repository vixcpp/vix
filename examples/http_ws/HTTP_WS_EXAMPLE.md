# Vix.cpp – HTTP + WebSocket (http_ws) Examples

The **http_ws/** directory contains hybrid examples that combine:

- HTTP server
- WebSocket server
- Shared routing logic
- High‑level runtime API
- Minimal, basic, chat, and full runtime demos

These examples show how to build **modern real‑time backends** using Vix.cpp with **both HTTP and WebSockets** in the same application.

---

## 📁 Directory Structure

```
http_ws/
│
├── main_minimal.cpp     ← Smallest HTTP + WS server example
├── main_basic.cpp       ← Basic API routes + WebSocket events
├── main_chat.cpp        ← Real-time chat example (HTTP + WS)
└── main_runtime.cpp     ← High-level “Node.js-like” runtime
```

---

# 🚀 1. main_minimal.cpp

### Minimal HTTP + WebSocket Server

This example shows the **smallest fully working HTTP + WS hybrid server**.

### Features

- Basic GET route
- Simple WS connection handling
- Auto-start server

### Example (summary)

```cpp
#include <vix.hpp>
#include <vix/websocket/AttachedRuntime.hpp>

using namespace vix;

int main()
{
    auto bundle = vix::make_http_and_ws("config/config.json");
    auto &[app, ws] = bundle;

    app.get("/", [](auto &, auto &res)
            { res.json({"framework", "Vix.cpp",
                        "message", "HTTP + WebSocket example (basic) 🚀"}); });

    ws.on_open([&ws](auto &session)
               {
        (void)session;

        ws.broadcast_json("chat.system", {
            "user", "server",
            "text", "Welcome to Vix WebSocket! 👋"
        }); });

    vix::run_http_and_ws(app, ws, 8080);

    return 0;
}
```

Run:

```bash
vix run maim_minimal.cpp
```

---

# ⚙️ 2. main_basic.cpp

### Basic REST API + WebSocket Interaction

This version adds:

- Multiple REST routes
- WebSocket broadcast
- JSON communication
- Event handlers

Useful for learning how HTTP and WS complement each other.

### Example features

- GET `/now`
- WS event: `ws.on_json(...)`
- Server timestamp pushes
- JSON responses everywhere

---

# 💬 3. main_chat.cpp

### HTTP + WebSocket Chat Example

This example demonstrates building a small real‑time chat service:

### Included:

- `/` returns a JSON welcome message
- Every WS client receives join/leave notifications
- Broadcast chat messages
- Structured message types
- Clean logging

### Flow:

1. Client connects
2. Server broadcasts `"chat.join"` to all users
3. Messages are relayed to everyone
4. On disconnect, `"chat.leave"` is broadcast

Perfect base for:

- chat systems
- classroom apps
- multiplayer game lobby
- collaborative tools

---

# 🧱 4. main_runtime.cpp

### High‑Level Runtime (Express.js style)

This is the most powerful example in the folder.

It uses the high‑level **runtime API**:

```cpp
auto [app, ws] = vix::make_http_and_ws("config.json");
vix::run_http_and_ws(app, ws, 8080);
```

### Features

- Loads config file
- Auto-configured thread pool
- HTTP and WS run in a single call
- Very similar to Node.js `express + ws`
- Perfect template for real apps

### Example routing

```cpp
app.get("/", [](auto&, auto& res){
    res.json({"message", "Hello from Vix.cpp"});
});

ws.on_typed_message([](auto& s, std::string type, auto payload){
    if (type == "chat.message")
        s.broadcast_json("chat.message", payload);
});
```

This example is ideal for:

- building APIs with real-time messages
- dashboards
- monitoring tools
- SaaS apps
- multiplayer prototypes

---

# 📦 5. Build All http_ws Examples

From the root:

```
cd examples/http_ws
mkdir build && cd build
cmake ..
make -j
```

Executables appear in `build/`.

---

# 🧭 Recommended Learning Order

1. **main_minimal.cpp** – understand the basics
2. **main_basic.cpp** – learn HTTP+WS interaction
3. **main_chat.cpp** – add structured real‑time features
4. **main_runtime.cpp** – production‑ready architecture

---

# ❤️ Contributing

You can contribute by:

- adding new hybrid examples
- improving runtime presets
- documenting more real‑time patterns
- submitting patches

---

# 📣 Final Note

These examples represent the philosophy of **Vix.cpp**:

**Modern Web. Real-Time. C++ Performance. Developer Joy.**

Build next‑generation real‑time systems with clarity and power.
