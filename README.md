# Vix.cpp

[![Build](https://img.shields.io/github/actions/workflow/status/vixcpp/vix/cmake.yml?branch=main)](https://github.com/vixcpp/vix/actions)
[![License](https://img.shields.io/github/license/vixcpp/vix)](./LICENSE)
[![Coverage](https://img.shields.io/badge/coverage-unknown-lightgrey)](./)

Vix.cpp is a modular C++ backend framework inspired by FastAPI, Vue.js, and React.js. It simplifies backend development while keeping high performance.

---

## Features

- Asynchronous HTTP server (Boost.Asio & Boost.Beast)
- Flexible routing with path parameters
- JSON serialization (nlohmann::json)
- Middleware system
- WebSocket support
- High-performance: tens of thousands of requests/sec

---

## Getting Started

### Prerequisites

- C++20 compiler (GCC 12+, Clang 16+, MSVC 2022+)
- CMake 3.20+
- Boost libraries (asio, beast)
- nlohmann/json
- spdlog

### Build

```bash
git clone https://github.com/vixcpp/vix.git
cd vix
mkdir build && cd build
cmake ..
make -j$(nproc)
```

---

## Quick Example

```cpp
#include <vix/core.h>

int main() {
    Vix::App app;

    app.get("/hello", [](auto &req, auto &res) {
        res.json({{"message", "Hello, World!"}});
    });

    app.get("/users/{id}", [](auto &req, auto &res, auto &params) {
        std::string id = params["id"];
        res.json({{"user_id", id}});
    });

    app.run(8080);
}
```

---

## Performance Testing

```bash
wrk -t8 -c100 -d30s http://localhost:8080/hello
wrk -t8 -c100 -d30s http://localhost:8080/users/1
```

---

## Repository Structure

```
vix/
├─ modules/
│  ├─ core/
│  ├─ orm/
│  ├─ cli/
│  ├─ middleware/
│  ├─ websocket/
│  └─ devtools/
├─ examples/
├─ config/
├─ scripts/
├─ build/
├─ LICENSE
├─ README.md
├─ CHANGELOG.md
└─ CMakeLists.txt
```

---

## Contributing

Contributions are welcome! Please read [CONTRIBUTING.md](./CONTRIBUTING.md) for guidelines.

---

## License

MIT License – see [LICENSE](./LICENSE) for details.
