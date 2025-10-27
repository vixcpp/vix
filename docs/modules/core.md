# vixcpp/core

![C++](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
![License](https://img.shields.io/badge/License-MIT-green)
![Status](https://img.shields.io/badge/Status-Stable-success)
![Performance](https://img.shields.io/badge/Throughput-80k%2B%20req%2Fs-orange)

> **vix.cpp/core** — The foundational module of the [**Vix.cpp**](https://github.com/vixcpp/vix) framework.  
> Provides the high-performance HTTP server, router, middleware system, and base runtime.  
> Every other Vix module builds on top of this layer.

---

## 🚀 Key Features

- **⚡ High-Performance HTTP Server** — Async I/O using **Boost.Asio** & **Beast**
- **🧭 Router System** — Supports `/users/{id}` syntax & HTTP method matching
- **🧩 Middleware Pipeline** — Extend with logging, auth, rate limiting, etc.
- **📦 JSON Utilities** — Built-in [nlohmann/json](https://github.com/nlohmann/json) (optional & pluggable)
- **🧵 Thread-Safe Lifecycle** — Graceful shutdown via signal handlers
- **🧠 Optimized Runtime** — Low allocations, `std::string_view` routing, fast parameter parsing

---

## 🧱 Architecture

```text
+----------------------+
|        App           |
|  - Manages routes    |
|  - Runs HTTP server  |
+----------+-----------+
           |
           v
+---------------------- +
|       Router         |
|  - Matches paths     |
|  - Resolves params   |
+----------+-----------+
           |
           v
+---------------------- +
|   RequestHandler<T>  |
|  - Executes handler  |
|  - Builds response   |
+----------+-----------+
           |
           v
+---------------------- +
|   ResponseWrapper    |
|  - JSON / Text       |
|  - Status helpers    |
+----------------------+
```

---

## ⚙️ Installation

```bash
git clone https://github.com/vixcpp/core.git
cd core
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

**Requirements**

- C++20 compiler
- CMake ≥ 3.20
- Boost ≥ 1.78 (`filesystem`, `system`)
- Optional: OpenSSL, nlohmann/json

---

## 💻 Quick Example

```cpp
#include <vix/core.h>

int main() {
    Vix::App app;

    app.get("/hello", [](auto&, auto& res) {
        res.json({{"message", "Hello, World!"}});
    });

    app.get("/users/{id}", [](auto&, auto& res, auto& params) {
        res.json({{"user_id", params["id"]}});
    });

    app.run(8080);
}
```

✅ Supports `GET`, `POST`, `PUT`, `DELETE`  
✅ Automatic path parameter extraction  
✅ Helper methods: `res.json()`, `res.text()`, `res.status()`

---

## 📊 Performance

**Command**

```bash
wrk -t8 -c200 -d30s --latency http://localhost:8080/
```

**Results**

```
Requests/sec: 80,083.48
Latency: p50=2.21ms, p99=3.27ms
Transfer/sec: 18.25MB
```

**Benchmark Environment**

- 8 threads · 200 connections · 30 seconds
- C++20 (GCC 13 / Clang 17) · Boost 1.84
- Ubuntu 24.04 x64
- Build type: Release (LTO enabled)

> 🧠 _Optimized for cache locality, async I/O, and minimal overhead._

---

## 🧩 Configuration Options

| Option                  | Default | Description                      |
| ----------------------- | ------- | -------------------------------- |
| `VIX_CORE_WITH_OPENSSL` | ON      | Enable HTTPS/TLS support         |
| `VIX_CORE_WITH_MYSQL`   | OFF     | Link MySQL C++ Connector         |
| `VIX_CORE_WITH_JSON`    | AUTO    | AUTO / ON / OFF                  |
| `VIX_CORE_FETCH_UTILS`  | ON      | Auto-fetch Vix::utils if missing |

---

## 🤝 Contributing

1. Fork this repo
2. Create a new branch `feature/...`
3. Commit and open a PR
4. Follow the [Vix coding style](https://github.com/vixcpp/vix/wiki/Code-Style)

---

## 🧾 License

**MIT License** © [Gaspard Kirira](https://github.com/gkirira)  
See [LICENSE](../../LICENSE) for details.
