# Vix.cpp – Examples Overview

Welcome to the **Vix.cpp Examples Suite**, a complete collection of practical, real‑world demonstrations showing how to build modern backends using **HTTP**, **WebSockets**, **ORM**, and runtime features of Vix.cpp.

These examples are crafted to feel familiar to developers coming from **Node.js, Python (FastAPI/Flask), Go Fiber/Chi, Deno, Bun**, while giving you **C++ performance and full low‑level control**.

---

# 📁 Directory Structure

```
examples/
│
├── main.cpp                 ← Small "Hello Vix" example
│
├── hello_routes.cpp         ← Intro to routing
│
├── http/                    ← HTTP-only demos
│   ├── basic_get.cpp
│   ├── json_api.cpp
│   ├── router_params.cpp
│   ├── now_server.cpp
│   ├── json_builders_routes.cpp
│   └── trace_route.cpp
│
├── http_crud/               ← REST API + CRUD patterns
│   ├── users_crud.cpp
│   ├── users_crud_internal.cpp
│   ├── post_create_user.cpp
│   ├── put_update_user.cpp
│   ├── delete_user.cpp
│   ├── validation_user_create.cpp
│   ├── repository_crud_full.cpp
│   └── user_crud_with_validation.cpp
│
├── orm/                     ← ORM examples (CRUD + queries)
│
├── websocket/               ← WebSocket-only examples
│   ├── simple_server.cpp
│   ├── simple_client.cpp
│   ├── chat_room.cpp
│   ├── simple/
│   └── advanced/
│
├── http_ws/                 ← HTTP + WebSocket hybrid servers
│   ├── main_minimal.cpp
│   ├── main_basic.cpp
│   ├── main_chat.cpp
│   └── main_runtime.cpp
│
└── CMakeLists.txt
```

---

# 🚀 What You Will Learn

| Topic                   | Description                                                             |
| ----------------------- | ----------------------------------------------------------------------- |
| **HTTP Servers**        | Build REST APIs, JSON endpoints, route parameters, tracing.             |
| **CRUD Patterns**       | Full user CRUD flows with repositories, validation, and error handling. |
| **WebSocket Servers**   | Real-time messaging, typed protocols, broadcast, rooms.                 |
| **Hybrid HTTP+WS Apps** | Combine Express-style routing with WebSocket events in one server.      |
| **Runtime API**         | High-level server creation similar to `express()` or FastAPI.           |
| **ORM**                 | Database access patterns (select, insert, update, delete).              |

The examples grow from **beginner → intermediate → advanced**, giving you a clear learning path.

---

# 🧭 Recommended Learning Path

### **1. Basics**

Start here:

- `main.cpp`
- `hello_routes.cpp`
- `http/basic_get.cpp`
- `http/json_api.cpp`

Learn:

- JSON responses
- Routing
- Parameters
- Returning status codes

---

### **2. CRUD & API Design**

Move to:

- `http_crud/users_crud.cpp`
- `http_crud/repository_crud_full.cpp`
- `validation_user_create.cpp`

Learn:

- Repository pattern
- Validation
- Business logic separation
- Common API patterns

---

### **3. WebSockets**

Then explore real-time systems:

- `websocket/simple_server.cpp`
- `websocket/simple_client.cpp`
- `websocket/chat_room.cpp`
- `websocket/simple/*`
- `websocket/advanced/*`

Learn:

- Sessions
- Broadcast
- Rooms
- Typed JSON protocol
- Real-time dashboards & chat

---

### **4. HTTP + WebSocket Hybrid Servers**

End with the most powerful examples:

- `http_ws/main_minimal.cpp`
- `http_ws/main_basic.cpp`
- `http_ws/main_chat.cpp`
- `http_ws/main_runtime.cpp`

Learn:

- Express-style HTTP + WebSocket
- Unified runtime
- High-level patterns
- Zero-boilerplate server orchestration

---

# ⚙️ How to Build Everything

From the project root:

```
cd examples
mkdir build && cd build
cmake ..
make -j
```

All executables appear in:

```
examples/build/
```

---

# 🧱 Philosophy of These Examples

The examples are designed to:

- Be **small but realistic**
- Follow **clean architecture**
- Teach **best practices**
- Show **production-ready patterns**
- Feel **instantly intuitive**
- Run **blazingly fast** thanks to C++

They act as templates for:

- APIs
- microservices
- real-time chat
- dashboards
- multiplayer games
- IoT hubs

---

# ❤️ Contributing

You can contribute by:

- Adding new examples
- Improving existing demos
- Writing tutorials
- Adding more HTTP, WS, or ORM patterns
- Creating end-to-end projects

Pull Requests are welcome!

---

# ⭐ Final Note

The examples reflect the core philosophy of Vix.cpp:

> **Modern Web. Real-Time. C++ Performance. Developer Joy.**

Build powerful, beautiful, real-time systems — with the speed of C++ and the simplicity of high-level frameworks.
