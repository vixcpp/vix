<p align="center">
  <img
    src="https://res.cloudinary.com/dwjbed2xb/image/upload/v1762524350/vixcpp_etndhz.png"
    alt="Vix.cpp Banner"
    width="50%"
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

**Vix.cpp** is a modern **C++ runtime** designed as a serious alternative to **Node.js, Deno, and Bun**
but engineered **from day one** for:

- unstable networks
- offline-first environments
- peer-to-peer systems
- extreme native performance

> **Run applications like Node/Deno/Bun
> with C++ speed, control, and predictability.**

Vix is not just a backend framework.
It is a **runtime layer** for real-world distributed systems.

---

## Who is Vix.cpp for?

Vix.cpp is built for developers who:

- Build backend systems in **modern C++**
- Need **predictable performance** (no GC pauses)
- Target **offline-first or unreliable networks**
- Work on **edge, local, or P2P systems**
- Want a **Node/Deno-like DX**, but native

If you’ve ever thought _“I wish Node was faster and more reliable”_
Vix is for you.

---

## Why Vix exists

Most modern runtimes assume:

- stable internet
- cloud-first infrastructure
- predictable latency
- always-online connectivity

That is **not reality** for much of the world.

**Vix.cpp is built for real conditions first.**

---

## Performance is not a feature it’s a requirement

Vix.cpp is designed to remove overhead, unpredictability, and GC pauses.

### ⚡ Benchmarks (Dec 2025)

| Framework                   | Requests/sec | Avg Latency |
| --------------------------- | ------------ | ----------- |
| ⭐ **Vix.cpp (pinned CPU)** | **~99,000**  | 7–10 ms     |
| Vix.cpp (default)           | ~81,400      | 9–11 ms     |
| Go (Fiber)                  | ~81,300      | ~0.6 ms     |
| Deno                        | ~48,800      | ~16 ms      |
| Node.js (Fastify)           | ~4,200       | ~16 ms      |
| PHP (Slim)                  | ~2,800       | ~17 ms      |
| FastAPI (Python)            | ~750         | ~64 ms      |

---

## It really is this simple

```cpp
#include <vix.hpp>
using namespace vix;

int main() {
    App app;

    app.get("/", [](Request&, Response& res){
        res.send("Hello from Vix.cpp 🚀");
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

  ws.on_typed_message(
    [](auto& session,
    const std::string& type,
    const vix::json::kvs& payload){
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

    ws.on_typed_message(
      [&ws](auto& session,
      const std::string& type,
      const vix::json::kvs& payload){
      (void)session;

      if (type == "chat.message") {
        ws.broadcast_json("chat.message", payload);
      }
    });
  });

  return 0;
}
```

### P2P control plane (p2p_http)

```cpp
#include <vix.hpp>
#include <vix/console.hpp>
#include <vix/p2p/Node.hpp>
#include <vix/p2p/P2P.hpp>

using namespace vix;

int main()
{
  vix::p2p::NodeConfig cfg;
  cfg.node_id = "node-A";
  cfg.listen_port = 9001;

  auto node = vix::p2p::make_tcp_node(cfg);
  vix::p2p::P2PRuntime runtime(node);

  runtime.start();

  console.info("Node A running on port 9001");
  runtime.wait(); // blocks

  return 0;
}
```

---

## Script mode Run C++ like a script

```bash
vix run main.cpp
vix dev main.cpp
```

## Included Runtimes & Modules

Vix.cpp ships as an **umbrella runtime** composed of multiple modules:

* **HTTP Runtime** : REST APIs and control plane
* **WebSocket Runtime** : real-time messaging and synchronization
* **P2P Runtime** : peer-to-peer networking and transport
* **p2p_http** : HTTP control plane for P2P introspection
* **ORM** : native C++ ORM with prepared statements
* **CLI** : Node-like developer experience
* **Cache, Middleware, Utils** : core building blocks

---

## Documentation

- [Introduction](docs/introduction.md)
- [Quick Start](docs/quick-start.md)
- [Architecture & Modules](docs/architecture.md)
- [ORM Overview](docs/orm/overview.md)
- [Benchmarks](docs/benchmarks.md)
- [Examples](docs/examples/overview.md)
- [Build & Installation](docs/build.md)
- [CLI Options](docs/options.md)
- [CLI Reference](docs/vix-cli-help.md)

## ⭐ Support the project

If you believe in modern C++ tooling, offline-first systems, and native performance,
please consider starring the repository.

MIT License

