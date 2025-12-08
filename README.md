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

# 🌍 What is Vix?

**Vix** is a next-generation **offline-first, peer-to-peer, ultra-fast runtime for modern C++**.

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
| ⭐ **Vix.cpp (v1.12.0)**  | **~98,942** _(pinned CPU)_ | **7.3–10.8 ms** | **~13.8 MB/s** |
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

## 📝 Notes

### ✔ Why Vix.cpp reaches Go-level performance

- zero-cost abstractions
- custom ThreadPool tuned for HTTP workloads
- optimized HTTP pipeline
- fast-path routing
- Beast-based IO
- minimal memory allocations
- predictable threading model

---

## 🦕 Deno benchmark (reference)

```bash
$ wrk -t8 -c800 -d30s --latency http://127.0.0.1:8000
Requests/sec: 48,868.73
```

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

    app.get("/", [](auto&, auto& res) {
        res.json({ "message", "Hello world" });
    });

    app.run(8080);
}
```

---

# 🧱 Why Vix Exists

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

# 🧩 Key Features

- 🌍 Offline-first runtime
- 🔗 P2P-ready communication model
- ⚙️ Async HTTP server
- 🧭 Expressive routing
- 💾 ORM for MySQL/SQLite
- 🧠 Middleware system
- 📡 WebSocket engine
- 🧰 Modular design
- 🚀 Developer experience similar to Node/Deno/Bun
- ⚡ 80k+ requests/sec performance

---

## 🚀 Getting Started

To set up Vix.cpp on your system:

```bash
git clone https://github.com/vixcpp/vix.git
cd vix
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/hello_routes
```

---

## 🧰 Example (CLI Project)

Once installed, you can generate a new project using the CLI:

```bash
vix new myapp
cd myapp
vix build
vix run
```

Expected output:

```bash
[GET] / → {"message": "Hello world"}
```

---

# 🎯 Script Mode — Run `.cpp` Files Directly

Vix can execute a single `.cpp` file **like a script**, without creating a full project.

```bash
vix run file.cpp
```

### ✔ How it works

- Generates a temporary CMake project under:  
  `./.vix-scripts/<filename>/`
- Compiles the `.cpp` file as a standalone executable
- Runs it immediately
- Stops cleanly on Ctrl+C (no gmake noise)

### Example:

```bash
~/myapp/test$ vix run server.cpp
Script mode: compiling server.cpp
Using script build directory:
  • .vix-scripts/server

✔ Build succeeded
[I] Server running on port 8080
^C
ℹ Server interrupted by user (SIGINT)
```

---

## 📚 Documentation

- 🧭 [Introduction](docs/introduction.md)
- ⚡ [Quick Start](docs/quick-start.md)
- 🧱 [Architecture & Modules](docs/architecture.md)
- 💾 [ORM Overview](docs/orm/overview.md)
- 📈 [Benchmarks](docs/benchmarks.md)
- 🧰 [Examples](docs/examples)
- 🛠️ [Build & Installation](docs/build.md)
- ⚙️ [CLI Options](docs/options.md)

## 📦 Module Documentation Index

- 🧩 **Core Module** — [docs/modules/core.md](docs/modules/core.md)
- 📡 **WebSocket Module** — [docs/modules/websocket.md](docs/modules/websocket.md)
- 🗃️ **ORM Module** — [docs/modules/orm.md](docs/modules/orm.md)
- 🔧 **JSON Module** — [docs/modules/json.md](docs/modules/json.md)
- 🛠️ **Utils Module** — [docs/modules/utils.md](docs/modules/utils.md)
- 🧰 **CLI Module** — [docs/modules/cli.md](docs/modules/cli.md)
- ⚙️ **Rix Library (Essential C++ utilities)** — [docs/modules/rix.md](docs/modules/rix.md)

📊 Summary

Vix.cpp sits at the top of modern backend runtimes, matching or exceeding high-performance frameworks like Go Fiber, and outperforming Deno, Node, PHP, Python, and even several C++ frameworks like Crow.

Vix.cpp = the C++ runtime pushing boundaries.

---

## 🤝 Contributing

Contributions are welcome!  
Please read the contributing guidelines.

---

## 🪪 License

Licensed under the **MIT License**.
