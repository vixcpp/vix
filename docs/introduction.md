# Introduction to Vix.cpp

Vix.cpp is a next‑generation **C++20** web backend framework focused on **speed**, **modularity**, and **developer ergonomics**.
Inspired by ideas from **FastAPI**, **Vue.js**, and **React**, it brings a clean, expressive style to native C++ while retaining zero‑overhead abstractions and low‑level control.

---

## Why Vix.cpp?

- **Extreme performance** — event‑driven I/O and careful memory locality yield tens of thousands of req/s on commodity hardware.
- **Modern C++20 design** — strong typing, RAII, concepts (where useful), and readable APIs.
- **Modular architecture** — use only what you need: `core`, `utils`, `json`, `orm`, `cli`, `websocket`, `devtools`.
- **Header‑first philosophy** — many pieces are header‑only for easy embedding and fast iteration.
- **Developer experience** — a friendly CLI (`vix new`, `vix build`, `vix run`) and pragmatic defaults.

> The web doesn’t have to be slow — Vix.cpp aims to prove it with a clear, minimal API and a focus on correctness and speed.

---

## Core Ideas

1. **Small, sharp core**
   The core (`App`, router, request/response, HTTP server) stays tiny and predictable. Everything else is opt‑in.

2. **Simple routing**
   Declarative routes with path parameters: `app.get("/users/{id}", handler);`

3. **JSON‑first**
   Seamless helpers around _nlohmann/json_ via `Vix::json` (builders, small utilities, safe conversions).

4. **Composability**
   Middleware, utilities (Logger, UUID, Time, Env), and an optional ORM layer (MySQL / SQLite) integrate without tight coupling.

5. **Pragmatism**
   Clean, incremental APIs; clear error messages; predictable defaults; portable builds (CMake).

---

## Quick Glance

```cpp
#include <vix.hpp>
using namespace Vix;

int main() {
    App app;
    app.get("/", [](Request&, Response& res) {
        res.send("message", "Hello world");
    });
    app.run(8080);
}
```

- One header include (`<vix.hpp>`) to start.
- Minimal boilerplate.
- JSON response helpers out of the box.

---

## Modules Overview

- **core** — HTTP server, router, request/response, status codes.
- **utils** — Logger (sync/async), UUID, Time, Env, Validation.
- **json** — Light wrappers and builders around _nlohmann/json_.
- **orm** _(optional)_ — Repository/Unit‑of‑Work, QueryBuilder, connection pool, MySQL/SQLite drivers.
- **cli** — `vix new`, `vix build`, `vix run` for fast iteration.
- **websocket** _(WIP)_ — Real‑time channels and messaging.
- **devtools** — Helpers and scripts for local development.

You can consume modules independently or via the umbrella project.

---

## Performance & Benchmarks (Overview)

Vix.cpp targets **ultra‑low overhead** per request by combining:

- event‑driven networking,
- efficient string/JSON handling paths,
- careful thread‑pooling and lock boundaries,
- and predictable memory usage patterns (favoring locality).

See complete methodology, hardware specs, and raw outputs in **[docs/benchmarks.md](./benchmarks.md)**.

---

## Getting Started

1. **Clone & submodules**

```bash
git clone https://github.com/vixcpp/vix.git
cd vix
git submodule update --init --recursive
```

2. **Build (Release)**

```bash
cmake -S . -B build-rel -DCMAKE_BUILD_TYPE=Release
cmake --build build-rel -j
```

3. **Run an example**

```bash
./build-rel/hello_routes
```

For platform‑specific setup (Linux/macOS/Windows), see **[Installation](./installation.md)**.
For packaging, sanitizers, and compile_commands.json, see **[Build & Packaging](./build.md)**.

---

## Philosophy & API Design

- **Clarity over cleverness** — APIs are readable and unsurprising.
- **Minimal global state** — lifetimes are explicit; resources use RAII.
- **Opt‑in features** — only pay for what you use.
- **Stable routes first, meta later** — routing & JSON are first‑class; ORMs, WebSockets, etc. are optional.
- **Tooling matters** — fast feedback loops via CLI and simple CMake presets.

---

## Roadmap (High‑Level)

- WebSocket engine (channels, rooms, backpressure).
- CLI templates for production scaffolds (Dockerfiles, systemd, Nginx).
- ORM query planner refinements and driver adapters.
- More middlewares (auth, rate‑limit, CORS presets).
- Devtools: profiling hooks and trace exporters.

For details and status, see **[Architecture](./architecture.md)** and module pages under **[docs/modules](./modules/)**.

---

## Where to Next?

- **Quick Start** — **[docs/quick-start.md](./quick-start.md)**
- **Installation** — **[docs/installation.md](./installation.md)**
- **Build & Packaging** — **[docs/build.md](./build.md)**
- **CMake Options** — **[docs/options.md](./options.md)**
- **Architecture** — **[docs/architecture.md](./architecture.md)**
- **Examples** — **[docs/examples/overview.md](./examples/overview.md)**
- **ORM Overview** — **[docs/orm/overview.md](./orm/overview.md)**
- **Benchmarks** — **[docs/benchmarks.md](./benchmarks.md)**

---

## Contributing & License

Contributions are welcome! Please read **CONTRIBUTING.md**.
Licensed under **MIT** — see **LICENSE**.
