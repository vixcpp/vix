# Vix.cpp

<p align="center">
  <img
    src="https://res.cloudinary.com/dwjbed2xb/image/upload/v1762524350/vixcpp_etndhz.png"
    alt="Vix.cpp Banner"
    width="100%"
    style="max-width:900px;border-radius:8px;"
  />
</p>

<h1 align="center">Vix.cpp</h1>

<p align="center">
  ⚡ Offline-first · Peer-to-peer · Ultra-fast C++ runtime
</p>

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-20-blue" />
  <img src="https://img.shields.io/badge/License-MIT-green" />
  <img src="https://img.shields.io/github/actions/workflow/status/vixcpp/vix/SECURITY_CI.yml?label=CI" />
  <img src="https://img.shields.io/github/stars/vixcpp/vix?style=flat" />
  <img src="https://img.shields.io/github/forks/vixcpp/vix?style=flat" />
  <img src="https://img.shields.io/github/issues/vixcpp/vix" />
  <img src="https://img.shields.io/badge/Status-Active-success" />
  <img src="https://img.shields.io/github/v/tag/vixcpp/vix" />
</p>

<p align="center">
⭐ If this vision resonates with you, consider starring the project.
</p>

---

## What is Vix.cpp?

**Vix.cpp** is a modern **C++ runtime** designed as a serious alternative to **Node.js, Deno, and Bun**,
but engineered **from day one** for real-world constraints:

* unstable or unreliable networks
* offline-first environments
* peer-to-peer systems
* extreme native performance

> Run applications like Node/Deno/Bun
> with C++ speed, control, and predictability.

Vix is not just a backend framework.
It is a **runtime layer** for distributed, edge, and offline-capable systems.

---

## The Vix Runtime Model

Vix.cpp is **not a single server**.
It is a **multi-plane runtime** composed of **three specialized servers**,
each with a clearly defined responsibility.

Together, they form a coherent, offline-first and P2P-ready execution model.

---

### 1) HTTP Server — Control & APIs

The HTTP server provides:

* REST APIs
* configuration endpoints
* management and orchestration
* integration with CLIs, dashboards, and web frontends

It is designed to be:

* fast
* stateless
* predictable
* middleware-driven

This server acts as the **control plane** of the runtime.

---

### 2) WebSocket Server — Real-time & State Synchronization

The WebSocket server handles:

* real-time messaging
* rooms and channels
* presence and events
* durable message storage (SQLite + WAL)
* offline-friendly reconnection and replay

It is optimized for:

* chat systems
* dashboards
* IoT streams
* live collaboration

This server is the **real-time plane** of the runtime.

---

### 3) P2P Runtime — Transport & Distributed Systems

The P2P runtime is responsible for:

* peer discovery
* secure handshakes
* connection lifecycle management
* transport abstraction (TCP, QUIC, …)
* offline-first and edge-friendly networking

It does **not** rely on HTTP or WebSocket for data transport.

This is the **transport plane** of the runtime.

---

### How the planes fit together

```
          ┌───────────────┐
          │   HTTP API    │   ← Control plane
          │ (REST, Admin) │
          └───────┬───────┘
                  │
          ┌───────▼───────┐
          │  WebSocket    │   ← Real-time plane
          │ (Events, RT)  │
          └───────┬───────┘
                  │
          ┌───────▼───────┐
          │     P2P       │   ← Transport plane
          │ (Peers, Mesh) │
          └───────────────┘
```

Each plane is:

* independent
* non-blocking
* explicitly scoped

They can be combined to build:

* offline-first backends
* distributed runtimes
* edge-native applications

---

## Who is Vix.cpp for?

Vix.cpp is built for developers who:

* build backend systems in **modern C++**
* need **predictable performance** (no GC pauses)
* target **offline-first or unreliable networks**
* work on **edge, local, or P2P systems**
* want a **Node/Deno-like DX**, but native

If you have ever thought:

> “I wish Node was faster, more predictable, and worked offline”

Vix.cpp is for you.

---

## Why Vix exists

Most modern runtimes assume:

* stable internet
* cloud-first infrastructure
* predictable latency
* always-online connectivity

That is **not reality** for much of the world.

**Vix.cpp is built for real conditions first.**

Offline is not a fallback.
It is a first-class design constraint.

---

## Performance is not a feature — it is a requirement

Vix.cpp is designed to remove overhead, unpredictability, and garbage-collection pauses.

### Benchmarks (Dec 2025)

| Framework                | Requests/sec | Avg Latency |
| ------------------------ | ------------ | ----------- |
| **Vix.cpp (pinned CPU)** | ~99,000      | 7–10 ms     |
| Vix.cpp (default)        | ~81,400      | 9–11 ms     |
| Go (Fiber)               | ~81,300      | ~0.6 ms     |
| Deno                     | ~48,800      | ~16 ms      |
| Node.js (Fastify)        | ~4,200       | ~16 ms      |
| PHP (Slim)               | ~2,800       | ~17 ms      |
| FastAPI (Python)         | ~750         | ~64 ms      |

---

## It really is this simple

### Minimal HTTP server

```cpp
#include <vix.hpp>
using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request&, Response& res) {
    res.send("Hello from Vix.cpp");
  });

  app.run(8080);
}
```

### Minimal WebSocket server

```cpp
#include <vix/websocket.hpp>

using vix::websocket::Server;

int main()
{
  Server ws;

  ws.on_open([](auto& session) {
    session.send_json("chat.system", {"text", "Welcome"});
  });

  ws.on_typed_message([](auto& session,
                         const std::string& type,
                         const vix::json::kvs& payload)
  {
    (void)session;

    if (type == "chat.message") {
      session.broadcast_json("chat.message", payload);
    }
  });

  ws.listen_blocking();
}
```

### HTTP + WebSocket together

```cpp
#include <vix.hpp>
#include <vix/websocket/AttachedRuntime.hpp>

using namespace vix;

int main()
{
  vix::serve_http_and_ws([](auto& app, auto& ws) {
    app.get("/", [](auto&, auto& res) {
      res.json({
        "message",   "Hello from Vix.cpp minimal example",
        "framework", "Vix.cpp"
      });
    });

    ws.on_typed_message([&ws](auto& session,
                              const std::string& type,
                              const vix::json::kvs& payload)
    {
      (void)session;

      if (type == "chat.message") {
        ws.broadcast_json("chat.message", payload);
      }
    });
  });

  return 0;
}
```

### Minimal P2P control plane (HTTP)

```cpp
#include <vix.hpp>
#include <vix/p2p_http/P2PHttp.hpp>

using namespace vix;

int main()
{
  App app;

  P2PHttpOptions opt;
  opt.enable_peers = true;

  install_p2p_http(app, opt);

  app.listen(5178, [](const vix::utils::ServerReadyInfo &info){
    console.info("UI API listening on", info.port);
  });

  app.wait();
}
```

---

## Script mode — run C++ like a script

```bash
vix run main.cpp
vix dev main.cpp
```

---

## Included Runtimes & Modules

Vix.cpp ships as an **umbrella runtime** composed of multiple modules:

* **HTTP Runtime** — REST APIs and control plane
* **WebSocket Runtime** — real-time messaging and synchronization
* **P2P Runtime** — peer-to-peer networking and transport
* **p2p_http** — HTTP control plane for P2P introspection
* **ORM** — native C++ ORM with prepared statements
* **CLI** — Node-like developer experience
* **Cache, Middleware, Utils** — core building blocks

Each module is optional and explicitly linked.

---

## Documentation

* Introduction — `docs/introduction.md`
* Quick Start — `docs/quick-start.md`
* Architecture & Modules — `docs/architecture.md`
* HTTP Runtime — `docs/http/overview.md`
* WebSocket Runtime — `docs/websocket/overview.md`
* P2P Runtime — `docs/p2p/overview.md`
* ORM Overview — `docs/orm/overview.md`
* Benchmarks — `docs/benchmarks.md`
* Examples — `docs/examples/overview.md`
* Build & Installation — `docs/build.md`
* CLI Options — `docs/options.md`
* CLI Reference — `docs/vix-cli-help.md`
* Vix Console — `docs/console.md`

---

## Support the project

If you believe in:

* modern C++ tooling
* offline-first systems
* peer-to-peer infrastructure
* native performance without compromise

please consider starring the repository.

---

## License

MIT License

Copyright (c) Vix.cpp contributors
