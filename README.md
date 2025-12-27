# Vix.cpp

<p align="center" style="margin:0;">
  <img 
    src="https://res.cloudinary.com/dwjbed2xb/image/upload/v1762524350/vixcpp_etndhz.png" 
    alt="Vix.cpp Banner" 
    width="100%" 
    style="
      display:block;
      height:auto;
      max-width:900px;
      margin:auto;
      object-fit:cover;
      border-radius:8px;
    ">
</p>

<h1 align="center">Vix.cpp</h1>

<p align="center">
  <img src="https://img.shields.io/badge/C++20-Standard-blue">
  <img src="https://img.shields.io/badge/License-MIT-green">
</p>

---

# What is Vix?

**Vix** is a next-generation **offline-first, peer-to-peer, ultra-fast runtime for modern C++**.

[Japanese](README.ja.md)

Its goal is clear:

> **A runtime capable of running apps like Node / Deno / Bun —  
> but engineered for unstable, low-quality, real-world networks.**

Vix is more than a backend framework:  
it is a **modular runtime**, designed for distributed applications, edge systems, offline devices, and environments where traditional cloud frameworks fail.

Inspired by **FastAPI**, **Vue.js**, **React**, and modern runtimes — but rebuilt from scratch in C++20 for raw speed and full control.

---

# ⚡ Benchmarks (Updated — Dec 2025)

All benchmarks were executed using **wrk**  
`8 threads`, `800 connections`, for **30 seconds**, on the same machine:  
**Ubuntu 24.04 — Intel Xeon — C++20 optimized build — Logging disabled**

Results represent steady-state throughput on a simple `"OK"` endpoint.

---

## 🚀 Requests per second

| Framework                 | Requests/sec               | Avg Latency     | Transfer/sec   |
| ------------------------- | -------------------------- | --------------- | -------------- |
| ⭐ **Vix.cpp (v1.12.3)**  | **~98,942** _(pinned CPU)_ | **7.3–10.8 ms** | **~13.8 MB/s** |
| **Vix.cpp (default run)** | 81,300 – 81,400            | 9.7–10.8 ms     | ≈ 11.3 MB/s    |
| Go (Fiber)                | 81,336                     | 0.67 ms         | 10.16 MB/s     |
| **Deno**                  | ~48,868                    | 16.34 ms        | ~6.99 MB/s     |
| Node.js (Fastify)         | 4,220                      | 16.00 ms        | 0.97 MB/s      |
| PHP (Slim)                | 2,804                      | 16.87 ms        | 0.49 MB/s      |
| Crow (C++)                | 1,149                      | 41.60 ms        | 0.35 MB/s      |
| FastAPI (Python)          | 752                        | 63.71 ms        | 0.11 MB/s      |

> 🔥 **New record:** When pinned to a single core (`taskset -c 2`)  
> Vix.cpp reaches **~99k req/s**, surpassing Go and matching the fastest C++ microframeworks.

---

### ✔ Why Vix.cpp reaches Go-level performance

- zero-cost abstractions
- custom ThreadPool tuned for HTTP workloads
- optimized HTTP pipeline
- fast-path routing
- Beast-based IO
- minimal memory allocations
- predictable threading model

---

### ✔ Vix.cpp recommended benchmark mode

When benchmarking from inside the Vix.cpp repository (using the built-in example):

```bash
cd ~/vixcpp/vix
export VIX_LOG_LEVEL=critical
export VIX_LOG_ASYNC=false

# Run the optimized example server
vix run example main
```

Then, in another terminal:

```bash
wrk -t8 -c800 -d30s --latency http://127.0.0.1:8080/bench
```

If you want CPU pinning for more stable results:

```bash
taskset -c 2 ./build/main
wrk -t8 -c800 -d30s --latency http://127.0.0.1:8080/bench
```

#### 🏁 Result: ~98,942 req/s

✔ Fast-path routing gives +1–3%

Use /fastbench to bypass RequestHandler overhead.

---

# 🧭 Quick Example

```cpp
#include <vix.hpp>
using namespace Vix;

int main() {
    App app;

    app.get("/", [](Request&, Response& res){
        res.send("Hello from Vix.cpp 🚀");
    });

    app.run(8080);
}
```

## Route Parameters

```cpp
app.get("/users/{id}", [](Request req, Response res) {
    auto id = req.param("id");
    return json::o("user_id", id);
});
```

# 🧠 Vix REPL — Interactive Runtime Shell

## ▶️ Starting the REPL

```bash
vix
```

Example startup:

```
Vix.cpp v1.x (CLI) — Modern C++ backend runtime
[GCC 13.3.0] on linux
Exit: Ctrl+C / Ctrl+D | Clear: Ctrl+L | Type help for help
vix>
```

---

### Minimal HTTP + WebSocket Server

This example shows the **smallest fully working HTTP + WS hybrid server**.

### Features

- Basic GET route
- Simple WS connection handling
- Auto-start server

### Example (summary)

```cpp
#include <vix.hpp>
#include <vix/websocket/Runtime.hpp>

using namespace vix;

int main()
{
    vix::serve_http_and_ws([](auto &app, auto &ws){

        // HTTP
        app.get("/", [](auto&, auto& res)
        {
            res.send("HTTP + WebSocket");
        });

        // WebSocket
        ws.on_typed_message([&ws](auto&,
                    const std::string& type,
                    const vix::json::kvs& payload){
            if (type == "ping")
                ws.broadcast_json("pong", payload);
        });

    });
}
```

## Minimal WebSocket Client

```cpp
auto client = Client::create("localhost", "9090", "/");

client->on_open([] {
    std::cout << "Connected!" << std::endl;
});

client->send("chat.message", {"text", "Hello world!"});
```

---

# Why Vix Exists

Cloud-first frameworks assume:

- stable networks
- predictable latency
- always-online connectivity

**But in most of the world, this is not reality.**

Vix is built for:

### ✔ Offline-first

Applications continue functioning even without internet.

### ✔ Peer-to-Peer

Nodes sync and communicate locally without a central server.

### ✔ Ultra-Fast Native Execution

C++20 + Asio + zero-overhead abstractions.

---

## Key Features

- Offline-first runtime architecture
- Peer-to-peer–ready communication model
- Asynchronous HTTP server
- Expressive and composable routing
- ORM support for MySQL and SQLite
- Middleware system
- WebSocket engine
- Modular architecture
- Developer experience comparable to Node.js, Deno, and Bun
- High-performance runtime (80k+ requests/sec)

---

## 🚀 Getting Started

To build **Vix.cpp** from source:

## 🐧 Linux / Ubuntu

### Prerequisites

```bash
sudo apt update
sudo apt install -y \
  g++-12 cmake make git \                # Build tools
  libboost-all-dev \                     # Boost (asio, beast)
  nlohmann-json3-dev \                   # JSON (nlohmann/json)
  libspdlog-dev \                        # Logging (spdlog)
  zlib1g-dev \                           # gzip / ZLIB
  libmysqlcppconn-dev                   # Optional: MySQL Connector/C++ for ORM
```

Optional dependencies:

```bash
sudo apt install -y libmysqlcppconn-dev libsqlite3-dev
```

---

## 🍎 macOS

### Prerequisites

Install Homebrew first, then:

```bash
brew install cmake ninja llvm boost nlohmann-json spdlog fmt mysql sqlite3 zlib
export ZLIB_ROOT="$(brew --prefix zlib)"
```

### Build

```bash
git clone https://github.com/vixcpp/vix.git
cd vix
git submodule update --init --recursive
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
sudo cmake --install build --prefix /usr/local
```

> This builds the Vix runtime and CLI.  
> Make sure the resulting `vix` binary is available in your `PATH`.

---

## 🧰 Example (CLI Project)

Create and run a new Vix project using the CLI:

```bash
vix new myapp
cd myapp

vix build
vix run
```

Common development commands:

```bash
vix dev                # watch, rebuild, reload
vix run                # build (if needed) and run
vix check              # validate project or compile a single .cpp (no execution)
vix tests              # run project tests
vix orm migrate        # run ORM migrations
```

---

## Script Mode — Run `.cpp` Files Directly

Vix can execute a single `.cpp` file **like a script**, without creating a full project.

```bash
vix run main.cpp        # run once
vix dev main.cpp        # run + watch (hot reload)
```

---

### ▶Run once (`vix run`)

Runs the file once, then exits.

```bash
~/dev/scripts$ vix run main.cpp
Hello, world
```

---

### Watch mode (`vix dev`)

Runs the file **in watch mode**.  
Vix recompiles and restarts automatically when the file changes.

```bash
~/dev/scripts$ vix dev main.cpp
Starting Vix dev mode.
➜ Tip: use `Ctrl+C` to stop dev mode; edit your files and Vix will rebuild & restart automatically.
Watcher Process started (hot reload).
➜ Watching: /home/softadastra/dev/scripts/main.cpp
🏃 Script started (pid=125953)
Hello, world
```

---

### ✔ How Script Mode Works

When running a `.cpp` file directly, Vix:

- Creates a temporary build directory under:
  ```
  ./.vix-scripts/<filename>/
  ```
- Generates a minimal CMake project internally
- Compiles the file as a standalone executable
- Runs it immediately
- In `dev` mode:
  - Watches the source file
  - Rebuilds and restarts automatically on changes
- Stops cleanly on `Ctrl+C` (no noisy build output)

---

### Mental model

| Command            | Behavior                           |
| ------------------ | ---------------------------------- |
| `vix run main.cpp` | Compile → run once                 |
| `vix dev main.cpp` | Compile → run → watch & hot-reload |

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

## Module Documentation Index

- **Core Module** — [docs/modules/core.md](docs/modules/core.md)
- **WebSocket Module** — [docs/modules/websocket.md](docs/modules/websocket.md)
- **ORM Module** — [docs/modules/orm.md](docs/modules/orm.md)
- **JSON Module** — [docs/modules/json.md](docs/modules/json.md)
- **Utils Module** — [docs/modules/utils.md](docs/modules/utils.md)
- **CLI Module** — [docs/modules/cli.md](docs/modules/cli.md)

## Summary

Vix.cpp sits at the top of modern backend runtimes, matching or exceeding high-performance frameworks like Go Fiber, and outperforming Deno, Node, PHP, Python, and even several C++ frameworks like Crow.

Vix.cpp = the C++ runtime pushing boundaries.

---

## 🤝 Contributing

Contributions are welcome!  
Please read the [contributing guidelines](CONTRIBUTING.md).

---

## 🪪 License

Licensed under the **MIT License**.
