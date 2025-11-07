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

**Vix.cpp** is a next-generation **modular C++ backend framework** — inspired by **FastAPI**, **Vue.js**, and **React.js**.  
It brings modern developer ergonomics, productivity, and extreme performance to native C++ — with a fully modular design.

---

## ⚡ Benchmarks

All benchmarks were executed using **wrk**  
`8 threads`, `200 connections`, for `30 seconds`, on the same machine (Ubuntu 24.04, Intel Xeon, C++20 build).

| Framework             | Requests/sec | Avg Latency | Transfer/sec   |
| --------------------- | ------------ | ----------- | -------------- |
| **Vix.cpp (v1.10.1)** | **88,973**   | **2.21 ms** | **18.25 MB/s** |
| Go (Fiber)            | 81,336       | 0.67 ms     | 10.16 MB/s     |
| Node.js (Fastify)     | 4,220        | 16.00 ms    | 0.97 MB/s      |
| PHP (Slim)            | 2,804        | 16.87 ms    | 0.49 MB/s      |
| Crow (C++)            | 1,149        | 41.60 ms    | 0.35 MB/s      |
| FastAPI (Python)      | 752          | 63.71 ms    | 0.11 MB/s      |

⚙️ Tested with: `wrk -t8 -c200 -d30s http://localhost:8080/`  
🧩 Environment: Ubuntu 24.04, GCC 13, Asio standalone (no Boost).

---

## 🧭 Quick Example

```cpp
#include <vix.hpp>
using namespace Vix;

int main() {
    App app;

    // GET /
    app.get("/", [](auto&, auto& res) {
        res.json({
            "message", "Hello world"
        });
    });

    app.run(8080);
}
```

Run benchmark locally:

```bash
wrk -t8 -c200 -d30s --latency http://localhost:8080/
```

---

## 🧩 Key Features

- ✅ **Modern C++20** — clean syntax, type safety, RAII
- ⚙️ **Async HTTP server** — powered by standalone Asio
- 🧭 **Routing system** — expressive path parameters (`/users/{id}`)
- 💾 **ORM for MySQL/SQLite** — repository & query builder patterns
- 🧠 **Middleware support** — logging, validation, sessions
- 💡 **JSON-first design** — integrates `nlohmann::json` with helpers
- 🧰 **Modular architecture** — core, cli, utils, orm, middleware, websocket
- ⚡ **High performance** — 80k+ requests/sec on commodity hardware
- 🪶 **Header-only modules** — easy to embed in any C++ project

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

> For full installation steps (Windows, Linux, macOS):  
> 📘 [docs/installation.md](./docs/installation.md)

> For build flags, packaging, and sanitizers:  
> ⚙️ [docs/build.md](./docs/build.md)

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

## 📚 Documentation

- 🧭 [Introduction](./docs/introduction.md)
- ⚡ [Quick Start](./docs/quick-start.md)
- 🧱 [Architecture & Modules](./docs/architecture.md)
- 💾 [ORM Overview](./docs/orm/overview.md)
- 📈 [Benchmarks](./docs/benchmarks.md)
- 🧰 [Examples](./docs/examples/overview.md)

---

## 🤝 Contributing

Contributions are welcome!  
See [CONTRIBUTING.md](./CONTRIBUTING.md) for details.

---

## 🪪 License

Licensed under the **MIT License**.  
See [LICENSE](./LICENSE) for more information.
