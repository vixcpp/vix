# Vix.cpp

![C++](https://img.shields.io/badge/C++20-Standard-blue)
![License](https://img.shields.io/badge/License-MIT-green)

Vix.cpp is a next-generation modular C++ backend framework — inspired by FastAPI, Vue.js, and React.js.
It aims to bring modern developer ergonomics and productivity to native C++,
while delivering extreme performance and full modularity.

---

## ⚡ Benchmarks

All benchmarks were executed with wrk
8 threads, 100 connections, for 30 seconds, on the same machine.

```markdown
| Framework | Requests/sec | Avg Latency | Transfer/sec |
| --------- | ------------ | ----------- | ------------ |
| Vix.cpp   | 73612        | 2.71ms      | 12.43MB      |
| Go        | 81336        | 674.28µs    | 10.16MB      |
| Node.js   | 4220         | 16.00ms     | 0.97MB       |
| PHP       | 2804         | 16.87ms     | 498.38KB     |
| Crow      | 1149         | 41.60ms     | 358.95KB     |
| FastAPI   | 752          | 63.71ms     | 111.99KB     |
```

🟢 Result:
Vix.cpp handles ~73K requests/sec with low latency (2.7ms) and
minimal memory footprint (12MB) — outperforming Node.js, FastAPI, and PHP.

# 🧭 Quick Example

```cpp
#include <vix/core.h>
#include <nlohmann/json.hpp>

int main() {
    Vix::App app;

    app.get("/hello", [](auto& req, auto& res) {
        res.json(nlohmann::json{{"message", "Hello, Vix!"}});
    });

    app.get("/users/{id}", [](auto& req, auto& res, auto& params) {
        res.json(nlohmann::json{{"user_id", params["id"]}});
    });

    app.run(8080);
}
```

## 🔬 Run benchmark locally

```bash
wrk -t8 -c200 -d30s --latency http://localhost:8080/hello
```

## Output:

```makefile
Requests/sec: 73612
Transfer/sec: 12.43MB
Latency:      2.71ms
```

# 🧩 Key Features

. ✅ Modern C++20 — clean syntax, type safety, and RAII
. ⚙️ Asynchronous HTTP server — built on Boost.Asio + Boost.Beast
. 🧭 Routing system — expressive path parameters (/users/{id})
. 💾 JSON-first design — integrates nlohmann::json and Vix::json helpers
. 🧰 Modular architecture — core, cli, utils, middleware, websocket
. 🧠 Middleware support — logging, validation, sessions
. ⚡ High performance — tens of thousands of requests per second
. 🪶 Header-only modules — portable and easy to embed in other projects

# 🧱 Repository Structure

```csharp
vix/
├─ modules/
│  ├─ core/          # HTTP server, router, request/response
│  ├─ utils/         # Logger, Validation, UUID, Env, Time
│  ├─ json/          # JSON helpers & builders
│  ├─ cli/           # Command-line interface
│  ├─ middleware/    # Future middlewares
│  ├─ websocket/     # WebSocket support (WIP)
│  └─ devtools/      # Developer utilities
│
├─ examples/         # Minimal REST demos (GET, POST, CRUD, Logger...)
├─ config/           # Runtime config (JSON)
├─ scripts/          # Helper scripts
├─ build/            # CMake build output
├─ LICENSE
├─ README.md
├─ CHANGELOG.md
└─ CMakeLists.txt
```

## Getting Started

### Prerequisites

You need a C++20 compiler, CMake, and several libraries. Below are instructions for common platforms.

- C++20 compiler (GCC 12+, Clang 16+, MSVC 2022+)
- CMake 3.20+
- Boost libraries (asio, beast)
- nlohmann/json
- spdlog

## 🧩 Build & Developer Setup

### Prerequisites

You need a **C++20** compiler, **CMake ≥ 3.20**, and the following libraries:

- Boost (asio, beast)
- nlohmann/json
- spdlog

#### Linux (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install g++-12 cmake libboost-all-dev nlohmann-json3-dev libspdlog-dev -y
```

### macOS

```bash
brew install llvm cmake boost nlohmann-json spdlog

```

### Windows (MSVC 2022 + CMake)

```bash
Use Visual Studio 2022 with Desktop C++ workload and install dependencies via vcpkg:
vcpkg install boost-asio boost-beast nlohmann-json spdlog
```

### 🏗️ Configure & Build

# Clone repository

```bash
git clone https://github.com/vixcpp/vix.git
cd vix

# Init and update submodules

git submodule update --init --recursive

# Configure and build (Release)

cmake -S . -B build-rel -DCMAKE_BUILD_TYPE=Release
cmake --build build-rel -j

# Or build in Debug with sanitizers (for developers)

cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DVIX_ENABLE_SANITIZERS=ON
cmake --build build -j
```

### ⚙️ Features Automatically Enabled

🪶 1. compile_commands.json Auto-Generation

CMake automatically exports build/compile_commands.json

It’s copied automatically to the project root (./compile_commands.json)
→ Perfect for VS Code IntelliSense and clang-tidy

VS Code setup:

In .vscode/c_cpp_properties.json:

```json
{
  "configurations": [
    {
      "name": "Linux",
      "includePath": ["${workspaceFolder}/**"],
      "compileCommands": "${workspaceFolder}/compile_commands.json",
      "cppStandard": "c++20",
      "intelliSenseMode": "linux-clang-x64"
    }
  ],
  "version": 4
}
```

This ensures IntelliSense, “Go to Definition”, and autocompletion work flawlessly.

### 🧪 2. Sanitizers Support (Optional)

You can enable runtime error detection using ASan + UBSan:

```bash
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DVIX_ENABLE_SANITIZERS=ON
cmake --build build-asan -j
```

This automatically adds:

```bash
-fsanitize=address,undefined
-O1 -g -fno-omit-frame-pointer
```

### 💡 Use this in Debug only.

For production, disable it (default).

### 🧱 3. Examples Build

All example .cpp files inside examples/ are built automatically when:

```bash
-DVIX_BUILD_EXAMPLES=ON   # (default)
```

### 🧪 Example Executables

Each example links against the umbrella interface target **`Vix::vix`**.

| **Example**                 | **Description**                             |
| --------------------------- | ------------------------------------------- |
| `main`                      | General example showcasing core usage.      |
| `get_example`               | Demonstrates handling of **GET** routes.    |
| `post_example`              | Demonstrates handling of **POST** routes.   |
| `put_example`               | Demonstrates handling of **PUT** routes.    |
| `delete_example`            | Demonstrates handling of **DELETE** routes. |
| `validation_user_create`    | Example of form validation logic.           |
| `logger_context_and_uuid`   | Logging with contextual data and UUIDs.     |
| `env_time_port`             | Environment variables and time demo.        |
| `full_crud_with_validation` | Full **CRUD** workflow with validation.     |

---

▶️ Running Examples

```bash
cd build-rel    # or build/
./main
./get_example
./post_example
./put_example
./delete_example
```

## Configuration file (config/config.json) is automatically copied to the build directory.

### 🧭 Summary of Key CMake Options

```markdown
## ⚙️ Build Options

Voici les options de configuration disponibles pour la compilation via CMake :

| **Option**                      | **Default** | **Description**                                                  |
| ------------------------------- | ----------- | ---------------------------------------------------------------- |
| `VIX_BUILD_EXAMPLES`            | `ON`        | Builds all example executables under `/examples`.                |
| `VIX_ENABLE_SANITIZERS`         | `OFF`       | Enables **ASan** + **UBSan** for runtime error checks.           |
| `CMAKE_EXPORT_COMPILE_COMMANDS` | `ON`        | Exports `compile_commands.json` for IDEs (VS Code, CLion, etc.). |
```

### 🧰 Developer Tips

Use cmake --build build -j to rebuild quickly.

Use ninja generator for faster builds: cmake -G Ninja -S . -B build.

Reset IntelliSense if VS Code shows stale errors:
Command Palette → “C/C++: Reset IntelliSense Database”

### ✅ With this setup, building Vix.cpp on any OS automatically:

generates compile_commands.json for VS Code,

compiles all modules (core, utils, json, cli),

and links examples ready to run.

## 🧩 Example: Minimal RESTful API with Vix.cpp

This example demonstrates how to build a complete CRUD REST API in pure C++ using Vix.cpp, with no external frameworks required.

It showcases how powerful and expressive the framework can be while remaining header-only, blazing fast, and thread-safe.

Uses the Vix::App HTTP server to define routes (GET, POST, PUT, DELETE).

Integrates JSON parsing via the built-in Vix::json module (wrapper around nlohmann/json).

Demonstrates validation, mutex-protected data, and in-memory storage via STL containers.

All responses are structured as JSON, with automatic status codes and validation errors.

👉 In just a few lines, you get a production-grade REST service written in C++, capable of handling 70 000+ req/s with minimal latency.

### 🧩 examples/main.cpp

```cpp
// ============================================================================
// main.cpp — Quick Example for Vix.cpp
// ----------------------------------------------------------------------------
// Minimal REST API demonstrating the simplicity of the Vix framework.
// ----------------------------------------------------------------------------
// Routes:
//   GET  /hello         → returns a JSON greeting
//   GET  /users/{id}    → returns a user ID as JSON
// ============================================================================

#include <vix/core.h>
#include <nlohmann/json.hpp>

int main()
{
    Vix::App app;

    // Simple JSON route
    app.get("/hello", [](auto &req, auto &res)
    {
        res.json(nlohmann::json{{"message", "Hello, Vix!"}});
    });

    // Example with path parameter
    app.get("/users/{id}", [](auto &req, auto &res, auto &params)
    {
        res.json(nlohmann::json{
            {"user_id", params["id"]},
            {"framework", "Vix.cpp"}
        });
    });

    app.run(8080);
    return 0;
}
```

### examples/get_example.cpp

```cpp
// GET example — simple read-only endpoints
#include <vix/core.h>
#include <nlohmann/json.hpp>
#include <string>

int main() {
    Vix::App app;

    // Simple health/hello
    app.get("/hello", [](auto& req, auto& res) {
        res.json(nlohmann::json{{"message", "Hello, World!"}});
    });

    // Read with path parameter
    app.get("/users/{id}", [](auto& req, auto& res, auto& params) {
        std::string id = params["id"];
        res.json(nlohmann::json{
            {"action", "read"},
            {"user_id", id}
        });
    });

    app.run(8080);
    return 0;
}

```

### examples/post_example.cpp

```cpp
// POST example — create a resource from JSON body
#include <vix/core.h>
#include <nlohmann/json.hpp>
#include <string>

int main() {
    Vix::App app;

    // Create user
    app.post("/users", [](auto& req, auto& res) {
        try {
            auto body = nlohmann::json::parse(req.body());

            // echo minimal "created" payload
            nlohmann::json out = {
                {"action", "create"},
                {"status", "created"},
                {"user", {
                    {"name",  body.value("name",  "")},
                    {"email", body.value("email", "")},
                    {"age",   body.value("age",   0)}
                }}
            };
            res.status(Vix::http::status::created).json(out);
        } catch (...) {
            res.status(Vix::http::status::bad_request)
               .json(nlohmann::json{{"error", "Invalid JSON"}});
        }
    });

    app.run(8080);
    return 0;
}
```

### examples/put_example.cpp

```cpp
// PUT example — update a resource by id
#include <vix/core.h>
#include <nlohmann/json.hpp>
#include <string>

int main() {
    Vix::App app;

    // Update user
    app.put("/users/{id}", [](auto& req, auto& res, auto& params) {
        const std::string id = params["id"];
        try {
            auto body = nlohmann::json::parse(req.body());

            // echo minimal "updated" payload
            nlohmann::json out = {
                {"action", "update"},
                {"status", "updated"},
                {"user", {
                    {"id",    id},
                    {"name",  body.value("name",  nlohmann::json())},
                    {"email", body.value("email", nlohmann::json())},
                    {"age",   body.value("age",   nlohmann::json())}
                }}
            };
            res.json(out);
        } catch (...) {
            res.status(Vix::http::status::bad_request)
               .json(nlohmann::json{{"error", "Invalid JSON"}});
        }
    });

    app.run(8080);
    return 0;
}
```

### examples/delete_example.cpp

```cpp
// DELETE example — delete a resource by id
#include <vix/core.h>
#include <nlohmann/json.hpp>
#include <string>

int main() {
    Vix::App app;

    // Delete user
    app.del("/users/{id}", [](auto& req, auto& res, auto& params) {
        const std::string id = params["id"];

        // In a real app you'd remove from DB/store here
        res.json(nlohmann::json{
            {"action", "delete"},
            {"status", "deleted"},
            {"user_id", id}
        });
    });

    app.run(8080);
    return 0;
}
```

### Build & run (rappel)

Ton CMake globe déjà examples/\*.cpp : juste recompiler suffit.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j


cd build
./get_example
# ./post_example
# ./put_example
# ./delete_example

```

```cpp
// ============================================================================
// full_crud_with_validation.cpp — Full CRUD + Validation (Vix.cpp)
// ----------------------------------------------------------------------------
// Complete demonstration of CRUD operations using Vix::App, with validation
// (Vix::utils::Validation) and JSON serialization (Vix::json).
//
// Routes:
//   POST   /users          → Create user (with validation)
//   GET    /users/{id}     → Read user
//   PUT    /users/{id}     → Update user
//   DELETE /users/{id}     → Delete user
//
// Thread-safe: std::mutex protects shared user map
// ============================================================================

#include <vix/core.h>
#include <vix/json/json.hpp>
#include <vix/utils/Validation.hpp>

#include <unordered_map>
#include <mutex>
#include <string>

namespace J = Vix::json;
using namespace Vix::utils;

// --------------------------- Data Model -------------------------------------
struct User
{
    std::string id;
    std::string name;
    std::string email;
    int age{};
};

static std::mutex g_mtx;
static std::unordered_map<std::string, User> g_users;

// --------------------------- JSON Helpers -----------------------------------
static J::Json to_json(const User &u)
{
    return J::o("id", u.id, "name", u.name, "email", u.email, "age", u.age);
}

static bool parse_user(const J::Json &j, User &out)
{
    try
    {
        out.name = j.value("name", std::string{});
        out.email = j.value("email", std::string{});

        if (j.contains("age"))
        {
            if (j["age"].is_string())
                out.age = std::stoi(j["age"].get<std::string>());
            else if (j["age"].is_number_integer())
                out.age = static_cast<int>(j["age"].get<long long>());
            else if (j["age"].is_number_unsigned())
                out.age = static_cast<int>(j["age"].get<unsigned long long>());
            else if (j["age"].is_number_float())
                out.age = static_cast<int>(j["age"].get<double>());
            else
                out.age = 0;
        }
        else
        {
            out.age = 0;
        }
        return true;
    }
    catch (...)
    {
        return false;
    }
}

// --------------------------- Main Application -------------------------------
int main()
{
    Vix::App app;

    // CREATE (POST /users)
    app.post("/users", [](auto &req, auto &res)
    {
        J::Json body;
        try { body = J::loads(req.body()); }
        catch (...) {
            res.status(Vix::http::status::bad_request)
               .json(J::o("error", "Invalid JSON"));
            return;
        }

        std::unordered_map<std::string, std::string> data{
            {"name",  body.value("name",  std::string{})},
            {"email", body.value("email", std::string{})},
            {"age",   body.value("age",   std::string{})}
        };

        Schema sch{
            {"name",  required("name")},
            {"age",   num_range(1, 150, "Age")},
            {"email", match(R"(^[^@\s]+@[^@\s]+\.[^@\s]+$)", "Invalid email")}
        };

        auto r = validate_map(data, sch);
        if (r.is_err())
        {
            J::Json e = J::o();
            for (const auto &kv : r.error())
                e[kv.first] = kv.second;

            res.status(Vix::http::status::bad_request)
               .json(J::o("errors", e));
            return;
        }

        User u;
        if (!parse_user(body, u))
        {
            res.status(Vix::http::status::bad_request)
               .json(J::o("error", "Invalid fields"));
            return;
        }

        // Generate ID (simple hash of email)
        u.id = std::to_string(std::hash<std::string>{}(u.email) & 0xFFFFFF);

        {
            std::lock_guard<std::mutex> lock(g_mtx);
            g_users[u.id] = u;
        }

        res.status(Vix::http::status::created)
           .json(J::o("status", "created", "user", to_json(u)));
    });

    // READ (GET /users/{id})
    app.get("/users/{id}", [](auto &req, auto &res, auto &params)
    {
        std::string id = params["id"];
        std::lock_guard<std::mutex> lock(g_mtx);
        auto it = g_users.find(id);
        if (it == g_users.end())
        {
            res.status(Vix::http::status::not_found)
               .json(J::o("error", "User not found"));
            return;
        }
        res.json(J::o("user", to_json(it->second)));
    });

    // UPDATE (PUT /users/{id})
    app.put("/users/{id}", [](auto &req, auto &res, auto &params)
    {
        std::string id = params["id"];
        J::Json body;
        try { body = J::loads(req.body()); }
        catch (...) {
            res.status(Vix::http::status::bad_request)
               .json(J::o("error", "Invalid JSON"));
            return;
        }

        std::lock_guard<std::mutex> lock(g_mtx);
        auto it = g_users.find(id);
        if (it == g_users.end())
        {
            res.status(Vix::http::status::not_found)
               .json(J::o("error", "User not found"));
            return;
        }

        if (body.contains("name"))  it->second.name  = body["name"].get<std::string>();
        if (body.contains("email")) it->second.email = body["email"].get<std::string>();
        if (body.contains("age"))
        {
            if (body["age"].is_string())
                it->second.age = std::stoi(body["age"].get<std::string>());
            else if (body["age"].is_number_integer())
                it->second.age = static_cast<int>(body["age"].get<long long>());
            else if (body["age"].is_number_unsigned())
                it->second.age = static_cast<int>(body["age"].get<unsigned long long>());
            else if (body["age"].is_number_float())
                it->second.age = static_cast<int>(body["age"].get<double>());
        }

        res.json(J::o("status", "updated", "user", to_json(it->second)));
    });

    // DELETE (DELETE /users/{id})
    app.del("/users/{id}", [](auto &req, auto &res, auto &params)
    {
        std::string id = params["id"];
        std::lock_guard<std::mutex> lock(g_mtx);
        auto n = g_users.erase(id);
        if (!n)
        {
            res.status(Vix::http::status::not_found)
               .json(J::o("error", "User not found"));
            return;
        }
        res.json(J::o("status", "deleted", "user_id", id));
    });

    // ------------------------------------------------------------------------
    app.run(8080);
    return 0;
}

```

```bash
# Create user
curl -X POST http://localhost:8080/users -H "Content-Type: application/json" \
     -d '{"name":"Alice","email":"alice@test.com","age":25}'

# Get user
curl http://localhost:8080/users/123456

# Update user
curl -X PUT http://localhost:8080/users/123456 -H "Content-Type: application/json" \
     -d '{"age":30}'

# Delete user
curl -X DELETE http://localhost:8080/users/123456
```

# Vix.cpp Examples

This folder contains example applications demonstrating how to use the Vix.cpp framework.

Each file shows how to create routes for different HTTP methods: GET, POST, PUT, DELETE.

---

## Example Files

| File                            | Description                                                      |
| ------------------------------- | ---------------------------------------------------------------- |
| `get_example.cpp`               | Demonstrates GET routes, including parameterized paths.          |
| `full_crud_with_validation.cpp` | Demonstrates POST,GET,PUT,DELETE routes with JSON body handling. |

---

## Usage

Build the examples:

```bash
cd build
./full_crud_with_validation
```

2. Run an example:

```bash
./get_example       # Runs GET routes
./post_example      # Runs POST routes
./put_example       # Runs PUT routes
./delete_example    # Runs DELETE routes
```

3. Test routes using curl or wrk:

# GET /hello

```bash
curl http://localhost:8080/hello
wrk -t4 -c50 -d10s http://localhost:8080/users/1
# {"message":"Hello, World!"}
```

# GET /users/{id}

```bash
wrk -t4 -c50 -d10s http://localhost:8080/users/1
# 49322 req/sec, 51.77MB read
```

# POST /users

```bash
curl -X POST http://localhost:8080/users \
 -d '{"name":"Alice"}' \
 -H "Content-Type: application/json"

# {"message":"User created","name":"Alice"}
```

# PUT /users/{id}

```bash
curl -X PUT http://localhost:8080/users/1 \
     -d '{"name":"Bob"}' \
     -H "Content-Type: application/json"
# {"id":"1","message":"User updated","name":"Bob"}
```

# DELETE /users/{id}

```bash
curl -X DELETE http://localhost:8080/users/1
# {"id":"1","message":"User deleted"}
```

# 🧩 Example: logger_context_and_uuid.cpp

📘 Description

Demonstrates how to use the Vix Logger with contextual information (request_id, module, etc.) and asynchronous logging.
Each incoming request gets a unique UUID for traceability.

Route Description
GET /trace Logs an incoming request with its method, path, and a generated request UUID.
🔍 Highlights

Uses Vix::utils::UUID to generate unique request IDs (uuid4()).

Context-aware logging with Logger::Context.

Async mode enabled for performance.

Output format customizable with setPattern() (spdlog-style).

### 🧠 Example Output

```bash
[2025-10-08 11:42:01.152] [info] Incoming request path=/trace method=GET
```

### 💡 JSON Response Example

```json
{
  "rid": "c8b4df4a-6e8a-4cf2-a0a6-5a3f5a6fbd99",
  "ok": true
}
```

# 📂 Source

```cpp
#include <vix/core.h>
#include <vix/utils/Logger.hpp>
#include <vix/utils/UUID.hpp>
#include <nlohmann/json.hpp>
#include <string>

using Vix::Logger;

int main()
{
    auto &log = Logger::getInstance();
    log.setPattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
    log.setLevel(Logger::Level::INFO);
    log.setAsync(true);

    Vix::App app;

    app.get("/trace", [](auto &req, auto &res)
    {
        Logger::Context cx;
        cx.request_id = Vix::utils::uuid4();
        cx.module = "trace_handler";
        Logger::getInstance().setContext(cx);

        std::string path(req.target().data(), req.target().size());
        std::string method(req.method_string().data(), req.method_string().size());

        Logger::getInstance().logf(
            Logger::Level::INFO,
            "Incoming request",
            "path", path.c_str(),
            "method", method.c_str());

        res.json(nlohmann::json{{"rid", cx.request_id}, {"ok", true}});
    });

    app.run(8080);
    return 0;
}
```

# ⏱ Example: env_time_port.cpp

#### 📘 Description

Demonstrates how to read environment variables and return current timestamps using Vix::utils::Env and Vix::utils::Time.

Route Description
GET /now Returns current system time in ISO 8601 format and milliseconds.
🔍 Highlights

Uses env_int("PORT", 8080) to read the server port from environment.

Uses iso8601_now() and now_ms() utilities for precise timestamps.

Simple structured logging via Vix::Logger.

#### 💡 JSON Response Example

```json
{
  "iso8601": "2025-10-08T11:45:22Z",
  "ms": 1738998322984
}
```

# 📂 Source

```cpp
#include <vix/core.h>
#include <vix/utils/Env.hpp>
#include <vix/utils/Time.hpp>
#include <vix/utils/Logger.hpp>

int main()
{
    auto &log = Vix::Logger::getInstance();
    log.setPattern("[%H:%M:%S.%e] [%^%l%$] %v");
    log.setLevel(Vix::Logger::Level::INFO);

    int port = Vix::utils::env_int("PORT", 8080);

    Vix::App app;

    app.get("/now", [](auto &req, auto &res)
    {
        res.json({
            {"iso8601", Vix::utils::iso8601_now()},
            {"ms", Vix::utils::now_ms()}
        });
    });

    log.log(Vix::Logger::Level::INFO, "Starting on port {}", port);

    app.run(port);
    return 0;
}
```

# examples/json_builders_routes.cpp

```cpp
// ============================================================================
// json_builders_routes.cpp — Minimal routes using Vix::json builders
// GET /hello          -> {"message":"Hello, World!"}
// GET /users/{id}     -> {"user":{"id":"<id>","active":true}}
// GET /roles          -> {"roles":["admin","editor","viewer"]}
// ============================================================================

#include <vix/core.h>
#include <vix/json/json.hpp>
#include <string>

namespace J = Vix::json;

int main() {
    Vix::App app;

    // GET /hello -> {"message": "Hello, World!"}
    app.get("/hello", [](auto& /*req*/, auto& res) {
        res.json(J::o("message", "Hello, World!"));
    });

    // GET /users/{id} -> {"user": {"id": "...", "active": true}}
    app.get("/users/{id}", [](auto& /*req*/, auto& res, auto& params) {
        const std::string id = params["id"];
        res.json(J::o("user", J::o("id", id, "active", true)));
    });

    // GET /roles -> {"roles": ["admin", "editor", "viewer"]}
    app.get("/roles", [](auto& /*req*/, auto& res) {
        res.json(J::o("roles", J::a("admin", "editor", "viewer")));
    });

    app.run(8080);
    return 0;
}
```

```bash
# depuis build/
./json_builders_routes
# tests
curl :8080/hello
curl :8080/users/42
curl :8080/roles
```

### Notes

Ensure vix.cpp core is built and running before testing.

Modify routes and port numbers in examples if needed.

High performance: C++ backend can handle tens of thousands of requests/sec as shown.

---

## Contributing

Contributions are welcome! Please read [CONTRIBUTING.md](./CONTRIBUTING.md) for guidelines.

---

## License

MIT License – see [LICENSE](./LICENSE) for details.
