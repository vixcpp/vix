# 🧩 Vix.cpp — Utils Module

![C++](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
![License](https://img.shields.io/badge/License-MIT-green)
![Status](https://img.shields.io/badge/Status-Stable-success)
![spdlog](https://img.shields.io/badge/Logging-spdlog-orange)

> **vix.cpp/utils** — Foundational utility layer for the **Vix.cpp** framework.  
> Includes environment management, logging, validation, UUIDs, timestamps, and build metadata.

---

## 🚀 Overview

The **Utils module** provides lightweight, reusable helpers used throughout Vix.cpp and standalone C++ apps.  
It is designed to simplify configuration, logging, and validation with zero runtime dependencies beyond `spdlog`.

---

## ⚙️ Features

- 🧩 **Env** — Retrieve typed environment variables with defaults (`env_or`, `env_bool`, `env_int`).
- ⏰ **Time** — Get current timestamps in ISO-8601, RFC-1123, or epoch milliseconds.
- 🪪 **UUID** — Generate RFC 4122–compliant UUIDv4 strings.
- 🧱 **Version** — Access framework version & build metadata.
- 🧾 **Logger** — Thread-safe, async, and colorized logging system.
- 🧪 **Validation** — Schema-based map validation for user input.

---

## 🧰 Quick Example

```cpp
#include <vix/utils/Env.hpp>
#include <vix/utils/Time.hpp>
#include <vix/utils/UUID.hpp>
#include <vix/utils/Version.hpp>
#include <iostream>
using namespace Vix::utils;

int main() {
    std::cout << "version=" << version() << "\n";
    std::cout << "build_info=" << build_info() << "\n";
    std::cout << "APP_ENV=" << env_or("APP_ENV", "dev") << "\n";
    std::cout << "uuid4=" << uuid4() << "\n";
}
```

**Output**

```
version=0.2.0
build_info=v0.2.0 (abcdef1, Oct 10 2025)
APP_ENV=dev
uuid4=550e8400-e29b-41d4-a716-446655440000
```

---

## 🧠 Logging Example

```cpp
#include <vix/utils/Logger.hpp>
#include <vix/utils/Env.hpp>
#include <vix/utils/UUID.hpp>
using namespace Vix::utils;
using Vix::Logger;

int main() {
    auto &log = Logger::getInstance();
    log.setAsync(env_bool("VIX_LOG_ASYNC", true));
    log.setPattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
    log.setLevel(Logger::Level::INFO);

    Logger::Context cx;
    cx.request_id = uuid4();
    cx.module = "utils_demo";
    cx.fields["env"] = env_or("APP_ENV", "dev");
    log.setContext(cx);

    log.log(Logger::Level::INFO, "Hello from utils!");
}
```

**Sample Output**

```
[2025-10-10 19:45:12.891] [info] Hello from utils/log_demo
[2025-10-10 19:45:12.891] [debug] Debug enabled = true
[2025-10-10 19:45:12.892] [info] Boot args | port=8080 async=true
[2025-10-10 19:45:12.892] [warn] This is a warning
```

---

## 🧪 Validation Example

```cpp
#include <vix/utils/Validation.hpp>
#include <vix/utils/Logger.hpp>
#include <unordered_map>
using namespace Vix::utils;
using Vix::Logger;

int main() {
    std::unordered_map<std::string, std::string> data{
        {"name", "Gaspard"}, {"age", "18"}, {"email", "softadastra@example.com"}};

    Schema sch{
        {"name", required("Name")},
        {"age", num_range(1, 150, "Age")},
        {"email", match(R"(^[^@\s]+@[^@\s]+\.[^@\s]+$)", "Email")}};

    auto &log = Logger::getInstance();
    const auto r = validate_map(data, sch);

    if (r.is_ok())
        log.log(Logger::Level::INFO, "Validation OK");
    else {
        log.log(Logger::Level::ERROR, "Validation FAILED:");
        for (auto &kv : r.error())
            log.log(Logger::Level::ERROR, " - {} -> {}", kv.first, kv.second);
    }
}
```

**Output**

```
[2025-10-10 19:04:12.512] [info] Validation OK
```

---

## 🧱 Components

| Header           | Description                             |
| ---------------- | --------------------------------------- |
| `Env.hpp`        | Environment variable access helpers     |
| `Time.hpp`       | Time and date utilities                 |
| `UUID.hpp`       | RFC 4122 UUIDv4 generator               |
| `Version.hpp`    | Build metadata and version info         |
| `Logger.hpp`     | Thread-safe async logger (spdlog-based) |
| `Validation.hpp` | Declarative schema validation           |

---

## 🏗️ Build & Installation

### Build standalone

```bash
git clone https://github.com/vixcpp/utils.git
cd utils
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

### Build as part of Vix.cpp umbrella

```bash
cd vixcpp/vix
cmake -B build -S .
cmake --build build -j$(nproc)
```

---

## 🧾 License

**MIT License** © [Gaspard Kirira](https://github.com/gkirira)  
See [LICENSE](../../LICENSE) for details.
