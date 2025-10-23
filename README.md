# Vix.cpp

![C++](https://img.shields.io/badge/C++20-Standard-blue)
![License](https://img.shields.io/badge/License-MIT-green)

Vix.cpp is a next-generation modular C++ backend framework — inspired by FastAPI, Vue.js, and React.js.
It aims to bring modern developer ergonomics and productivity to native C++,
while delivering extreme performance and full modularity.

---

## ⚡ Benchmarks

All benchmarks were executed using **wrk**  
`8 threads`, `200 connections`, for `30 seconds`, on the same machine (Ubuntu 24.04, Intel Xeon, C++20 build).

```markdown
| Framework            | Requests/sec | Avg Latency | Transfer/sec   |
| -------------------- | ------------ | ----------- | -------------- |
| **Vix.cpp (v1.7.0)** | **88,973**   | **2.27 ms** | **14.85 MB/s** |
| Go (Fiber)           | 81,336       | 0.67 ms     | 10.16 MB/s     |
| Node.js (Fastify)    | 4,220        | 16.00 ms    | 0.97 MB/s      |
| PHP (Slim)           | 2,804        | 16.87 ms    | 0.49 MB/s      |
| Crow (C++)           | 1,149        | 41.60 ms    | 0.35 MB/s      |
| FastAPI (Python)     | 752          | 63.71 ms    | 0.11 MB/s      |
```

🟢 Result
Vix.cpp (v1.7.0) reaches ~84 K requests/sec with an average latency of 2.3 ms —
matching Go in throughput while maintaining pure native C++ performance and low memory usage.

⚙️ Tested with: wrk -t8 -c200 -d30s http://localhost:8080/

🧩 Environment: Ubuntu 24.04, C++20 (GCC 13), Asio standalone, no Boost.

# 🧭 Quick Example

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

## 🔬 Run benchmark locally

```bash
wrk -t8 -c200 -d30s --latency http://localhost:8080/hello
```

## Output:

```makefile
Requests/sec: 88,973
Transfer/sec: 14.85MB
Latency:      2.27ms
```

# 🧩 Key Features

- ✅ **Modern C++20** — clean syntax, type safety, and RAII
- ⚙️ **Asynchronous HTTP server** — built on Boost.Asio + Boost.Beast
- 🧭 **Routing system** — expressive path parameters (`/users/{id}`)
- 💾 **ORM for MySQL / SQLite** — high-level data access with QueryBuilder & Repository pattern
- 🧠 **Middleware support** — logging, validation, and sessions
- 💡 **JSON-first design** — integrates `nlohmann::json` and `Vix::json` helpers
- 🧰 **Modular architecture** — core, cli, utils, orm, middleware, websocket
- ⚡ **High performance** — tens of thousands of requests per second
- 🪶 **Header-only modules** — portable and easy to embed in other projects

---

# 🧱 Repository Structure

```csharp
vix/
├─ modules/
│  ├─ core/          # HTTP server, router, request/response
│  ├─ utils/         # Logger, Validation, UUID, Env, Time
│  ├─ json/          # JSON helpers & builders
│  ├─ orm/           # ORM layer (MySQL / SQLite, QueryBuilder, Repository)
│  ├─ cli/           # Command-line interface
│  ├─ middleware/    # Future middlewares
│  ├─ websocket/     # WebSocket support (WIP)
│  └─ devtools/      # Developer utilities
│
├─ examples/         # Minimal REST + ORM demos (GET, POST, CRUD, Transactions...)
├─ config/           # Runtime config (JSON)
├─ scripts/          # Helper scripts (e.g. submodules-sync.sh)
├─ build/            # CMake build output
├─ LICENSE
├─ README.md
├─ CHANGELOG.md
└─ CMakeLists.txt
```

# 🚀 Getting Started

## 🧩 Overview

Vix.cpp is a high-performance C++20 web framework inspired by FastAPI, Vue.js, and React.
It’s modular by design — each component (core, utils, json, orm, cli) can be built independently or together under the umbrella project.pdlog

## 🧩 Build & Developer Setup

### 🧱 Prerequisites

You’ll need the following tools and libraries depending on your platform:

| **Component**       | **Minimum Version**              | **Purpose**              |
| ------------------- | -------------------------------- | ------------------------ |
| C++ Compiler        | GCC 12+ / Clang 16+ / MSVC 2022+ | C++20 support            |
| CMake               | ≥ 3.20                           | Build system             |
| Boost               | asio, beast                      | Networking (core module) |
| nlohmann/json       | ≥ 3.11                           | JSON serialization       |
| spdlog              | ≥ 1.10                           | Logging                  |
| MySQL Connector/C++ | _optional_                       | ORM (database driver)    |

#### Linux (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install -y \ g++-12 cmake make git \                            # Build tools
  libboost-all-dev \                                 # Boost (includes asio, beast)
  nlohmann-json3-dev \                               # JSON (nlohmann/json)
  libspdlog-dev \                                    # Logging (spdlog)
  libmysqlcppconn-dev                                # Optional: MySQL Connector/C++ for ORM
```

If you need SQLite for testing ORM:

```bash
sudo apt install -y libsqlite3-dev
```

### 🍎 macOS (Homebrew)

```bash
brew install llvm cmake boost nlohmann-json spdlog
# optional for ORM:
brew install mysql-connector-c++
```

💡 If you’re using LLVM via Homebrew, you may want to prepend its toolchain:

```bash
export CC=/usr/local/opt/llvm/bin/clang
export CXX=/usr/local/opt/llvm/bin/clang++
```

### 🪟 Windows (Visual Studio 2022 + vcpkg)

1. Install Visual Studio 2022 with the “Desktop C++” workload.
2. Use vcpkg to install dependencies:

```bash
vcpkg install boost-asio boost-beast nlohmann-json spdlog
# optional for ORM:
vcpkg install mysql-connector-cpp
```

Then integrate vcpkg with CMake:

```bash
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
```

### 🏗️ Configure & Build

# Clone repository

```bash
git clone https://github.com/vixcpp/vix.git
cd vix

# Initialize and update submodules

git submodule update --init --recursive
```

# ⚙️ Dev local (sans packaging)

```bash
cmake -S . -B build-rel -DCMAKE_BUILD_TYPE=Release -DVIX_ENABLE_INSTALL=OFF
cmake --build build-rel -j
```

Builds all umbrella modules (core, utils, json, orm, cli) locally without installation.

# 📦 Packaging / Installation

```bash
cmake -S . -B build-pkg -DCMAKE_BUILD_TYPE=Release -DVIX_ENABLE_INSTALL=ON
cmake --build build-pkg -j
sudo cmake --install build-pkg --prefix /usr/local
```

Installs the umbrella package (with all modules) system-wide in /usr/local.

# 🔁 Rebuild & Reinstall Quickly

```bash
cmake -S . -B build-pkg -DCMAKE_BUILD_TYPE=Release -DVIX_ENABLE_INSTALL=ON
cmake --build build-pkg -j
sudo cmake --install build-pkg --prefix /usr/local
```

# 🧪 Debug Build (with Sanitizers)

For developers who want to debug memory or undefined behavior:

```bash
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug -DVIX_ENABLE_SANITIZERS=ON
cmake --build build-debug -j
```

# Configure and build (Release)

```bash
cmake -S . -B build-rel -DCMAKE_BUILD_TYPE=Release
cmake --build build-rel -j
```

# 🧰 Example (CLI Project)

Once installed, you can generate a new Vix project using the CLI:

```bash
vix new myapp
cd vix/myapp
cmake -S . -B build
cmake --build build -j
./build/myapp
```

You should see:

```csharp
[GET] / → {"message": "Hello world"}
```

# 🧠 Tips

To rebuild all modules after updates:

```bash
git submodule update --remote --merge
cmake --build build-pkg -j
sudo cmake --install build-pkg --prefix /usr/local
```

# To check installed files:

```bash
cmake --install build-pkg --prefix /usr/local --dry-run
```

This builds all umbrella modules (core, utils, json, orm, cli).

### ⚙️ Optional Build Flags

```markdown
| **Flag**                     | **Description**                                                     |
| ---------------------------- | ------------------------------------------------------------------- |
| `-DVIX_BUILD_EXAMPLES=ON`    | Build all umbrella examples in `/examples`                          |
| `-DVIX_BUILD_TESTS=ON`       | Build unit tests (requires GoogleTest)                              |
| `-DVIX_ENABLE_SANITIZERS=ON` | Enable AddressSanitizer and UndefinedBehaviorSanitizer              |
| `-DVIX_ENABLE_LTO=ON`        | Enable Link-Time Optimization (applies to Release builds only)      |
| `-DVIX_ORM_USE_MYSQL=ON`     | Enable MySQL backend in the ORM module                              |
| `-DVIX_ORM_REQUIRE_MYSQL=ON` | Fail the build if MySQL is requested but not found                  |
| `-DVIX_ENABLE_CLANG_TIDY=ON` | Enable `clang-tidy` static analysis during build                    |
| `-DVIX_ENABLE_CPPCHECK=ON`   | Enable `cppcheck` static analysis                                   |
| `-DVIX_ENABLE_COVERAGE=ON`   | Enable code coverage instrumentation (applies to Debug builds only) |
```

# 🧩 Example: Build ORM only

```bash
cd modules/orm
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DVIX_ORM_USE_MYSQL=ON \
  -DVIX_ORM_BUILD_EXAMPLES=ON
cmake --build build -j
./build/vix_orm_users
```

# ✅ Verifying

After a successful build, you’ll see:

If you only want to build the ORM module:

```less
-- [vix_orm] MySQL: ON (target: 1)
-- [vix_orm] SQLite: OFF (found: )
-- [vix_orm] spdlog: 1
[100%] Built target vix_orm_users
```

Run an example from the root:

```bash
./build-rel/hello_routes
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

[![Build](https://github.com/vixcpp/vix/actions/workflows/build.yml/badge.svg)](https://github.com/vixcpp/vix/actions/workflows/build.yml)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg?style=flat&logo=c%2B%2B)](https://en.cppreference.com/w/cpp/20)
[![CMake](https://img.shields.io/badge/CMake-3.20+-064F8C.svg?style=flat&logo=cmake)](https://cmake.org)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

The following examples demonstrate core features of **Vix.cpp** — from routing and ORM integration to validation, transactions, and migrations.

```markdown
| **Example**                 | **Description**                                                           |
| --------------------------- | ------------------------------------------------------------------------- |
| `main`                      | Entry point / smoke test example for the umbrella build.                  |
| `hello_routes`              | Demonstrates defining and handling **GET** routes.                        |
| `post_create_user`          | Example showing how to create a user with a **POST** request.             |
| `put_update_user`           | Demonstrates updating user data with a **PUT** request.                   |
| `delete_user`               | Demonstrates handling of **DELETE** routes.                               |
| `trace_route`               | Shows advanced request tracing with contextual logs and UUIDs.            |
| `validation_user_create`    | Implements server-side **validation** for user creation forms.            |
| `user_crud_with_validation` | Full **CRUD** workflow example with integrated validation logic.          |
| `users_crud`                | Basic CRUD operations example (Create, Read, Update, Delete).             |
| `users_crud_internal`       | Internal shared code used by other CRUD examples (no `main()`).           |
| `repository_crud_full`      | Demonstrates the complete **Repository pattern** for data management.     |
| `batch_insert_tx`           | Example of batch inserts within a **transaction** context.                |
| `tx_unit_of_work`           | Demonstrates the **Unit of Work** pattern in transactional operations.    |
| `migrate_init`              | Example showing how to initialize and run database migrations.            |
| `querybuilder_update`       | Demonstrates building complex SQL queries with **QueryBuilder**.          |
| `demo_with_orm`             | End-to-end demo using the ORM for database interaction.                   |
| `error_handling`            | Demonstrates exception and error management using `DBError`.              |
| `json_builders_routes`      | Shows JSON builders and route composition using `Vix::json`.              |
| `now_server`                | Example of dynamic responses using system time and environment variables. |
```

---

# 💡 Run any example after building:

```bash
./build/<example_name>
```

---

▶️ Running Examples

```bash
cd build-rel    # or build/
./main
./hello_routes
./post_user
./put_user
./delete_user
```

## Configuration file (config/config.json) is automatically copied to the build directory.

### 🧭 Summary of Key CMake Options

## ⚙️ Build Options

Voici les options de configuration disponibles pour la compilation via **CMake** :

| **Option**                      | **Default** | **Description**                                                              |
| ------------------------------- | ----------- | ---------------------------------------------------------------------------- |
| `VIX_BUILD_EXAMPLES`            | `ON`        | Builds all umbrella examples under `/examples`.                              |
| `VIX_BUILD_TESTS`               | `ON`        | Builds and runs unit tests with **GoogleTest**.                              |
| `VIX_ENABLE_SANITIZERS`         | `OFF`       | Enables **ASan** + **UBSan** for runtime error detection.                    |
| `VIX_ENABLE_LTO`                | `OFF`       | Enables Link Time Optimization (for Release builds).                         |
| `CMAKE_EXPORT_COMPILE_COMMANDS` | `ON`        | Exports `compile_commands.json` for IDEs (VS Code, CLion, etc.).             |
| `VIX_ORM_USE_MYSQL`             | `ON`        | Enables **MySQL Connector/C++** support for ORM.                             |
| `VIX_ORM_USE_SQLITE`            | `OFF`       | Enables **SQLite3** backend (if available).                                  |
| `VIX_ORM_BUILD_EXAMPLES`        | `OFF`       | Builds ORM-specific examples like `users_crud`, `repository_crud_full`, etc. |
| `VIX_ORM_BUILD_TESTS`           | `OFF`       | Builds ORM test suites (disabled by default).                                |

---

### 🧰 Developer Tips

- Rebuild quickly:

```bash
  cmake --build build -j
```

Use the Ninja generator for faster incremental builds:

```bash
cmake -G Ninja -S . -B build
```

- Reset IntelliSense if VS Code shows stale errors:
  .Command Palette → “C/C++: Reset IntelliSense Database”

### ✅ With this setup, building Vix.cpp on any OS automatically:

- generates compile_commands.json for IDE integration
- compiles all modules (core, utils, json, cli, orm)
- links all examples and test executables, ready to run

🧩 Example: Minimal RESTful API with Vix.cpp

This example demonstrates how to build a complete CRUD REST API in pure C++ using Vix.cpp, with no external frameworks required.
It showcases the expressive and high-performance design of the framework while remaining header-only, blazing fast, and thread-safe.
Uses Vix::App as the asynchronous HTTP server.
Defines clean REST routes: GET, POST, PUT, DELETE.
Integrates JSON parsing via Vix::json (wrapper around nlohmann/json).
Demonstrates input validation, mutex-protected data, and STL-based storage.
Outputs consistent JSON responses with proper status codes and error handling.
👉 In just a few lines, you get a production-grade C++ REST API capable of handling
70 000+ requests per second with minimal latency — and now, optionally, full ORM persistence for real databases.

### 🧩 examples/main.cpp

```cpp
// ============================================================================
// main.cpp — Quick Start Example (Vix.cpp)
// ---------------------------------------------------------------------------
// A minimal HTTP server built with Vix.cpp.
// Run →  ./main
// Then visit →  http://localhost:8080/hello
// ============================================================================

#include <vix.hpp>
using namespace Vix;

int main()
{
        App app;

        // GET /
        app.get("/", [](auto &, auto &res)
                { res.json({"framework", "Vix.cpp",
                            "message", "Welcome to the future of C++ web development 🚀"}); });

        // GET /hello/{name}
        app.get("/hello/{name}", [](auto &, auto &res, auto &params)
                { res.json({"greeting", "Hello " + params["name"] + " 👋",
                            "powered_by", "Vix.cpp"}); });

        // Start server
        app.run(8080);
        return 0;
}
```

### examples/hello_routes.cpp

```cpp
// ============================================================================
// hello_routes.cpp — Minimal example (new Vix.cpp API)
// GET /hello        → {"message":"Hello, Vix!"}
// GET /user         → {"name":"Ada","tags":["c++","net","http"],"profile":{"id":42,"vip":true}}
// GET /users/{id}   → 404 {"error":"User not found"}
// ============================================================================

#include <vix.hpp>
using namespace Vix;
namespace J = Vix::json;

int main()
{
        App app;

        // Simple hello route
        app.get("/hello", [](auto &, auto &res)
                { res.json({"message", "Hello, Vix!"}); });

        // Nested JSON using builders
        app.get("/user", [](auto &, auto &res)
                {
        using namespace J;
        res.json({
            "name", "Ada",
            "tags", array({ "c++", "net", "http" }),
            "profile", obj({ "id", 42, "vip", true })
        }); });

        // Example with path param
        app.get("/users/{id}", [](auto &, auto &res)
                { res.status(http::status::not_found).json({"error", "User not found"}); });

        app.run(8080);
        return 0;
}
```

### examples/post_example.cpp

```cpp
// ============================================================================
// post_create_user.cpp — POST example (new Vix.cpp API)
// POST /users -> {"action":"create","status":"created","user":{...}}
// ============================================================================

#include <vix.hpp>
#include <vix/json/Simple.hpp>
#include <string>

using namespace Vix;
namespace J = Vix::json;

int main()
{
    App app;

    // POST /users
    app.post("/users", [](auto &req, auto &res)
             {
        try {
            // Parse body as nlohmann::json for simplicity (still supported)
            auto body = nlohmann::json::parse(req.body());

            const std::string name  = body.value("name",  "");
            const std::string email = body.value("email", "");
            const int age           = body.value("age",   0);

            res.status(http::status::created).json({
                "action", "create",
                "status", "created",
                "user", J::obj({
                    "name",  name,
                    "email", email,
                    "age",   static_cast<long long>(age)
                })
            });
        }
        catch (...) {
            res.status(http::status::bad_request).json({
                "error", "Invalid JSON"
            });
        } });

    app.run(8080);
    return 0;
}
```

### examples/put_example.cpp

```cpp
// ============================================================================
// put_update_user.cpp — PUT example (new Vix.cpp API)
// PUT /users/{id} -> {"action":"update","status":"updated","user":{...}}
// ============================================================================

#include <vix.hpp>
#include <vix/json/Simple.hpp>
#include <string>

using namespace Vix;
namespace J = Vix::json;

int main()
{
    App app;

    // PUT /users/{id}
    app.put("/users/{id}", [](auto &req, auto &res, auto &params)
            {
        const std::string id = params["id"];

        try {
            // Parsing with nlohmann::json for input is fine (Vix supports it internally)
            auto body = nlohmann::json::parse(req.body());

            const std::string name  = body.value("name",  "");
            const std::string email = body.value("email", "");
            const int age           = body.value("age",   0);

            res.json({
                "action", "update",
                "status", "updated",
                "user", J::obj({
                    "id",    id,
                    "name",  name,
                    "email", email,
                    "age",   static_cast<long long>(age)
                })
            });
        }
        catch (...) {
            res.status(http::status::bad_request).json({
                "error", "Invalid JSON"
            });
        } });

    app.run(8080);
    return 0;
}
```

### examples/delete_example.cpp

```cpp
// ============================================================================
// delete_user.cpp — DELETE example (new Vix.cpp API)
// DELETE /users/{id} -> {"action":"delete","status":"deleted","user_id":"<id>"}
// ============================================================================

#include <vix.hpp>
#include <vix/json/Simple.hpp>
#include <string>

using namespace Vix;
namespace J = Vix::json;

int main()
{
    App app;

    // DELETE /users/{id}
    app.del("/users/{id}", [](auto &, auto &res, auto &params)
            {
        const std::string id = params["id"];

        // In a real app you'd remove the resource from DB or memory here
        res.json({
            "action",  "delete",
            "status",  "deleted",
            "user_id", id
        }); });

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
./hello_routes
# ./post_example
# ./put_example
# ./delete_example

```

```cpp
// ============================================================================
// user_crud_with_validation.cpp — Full CRUD + Validation (Vix.cpp, nouvelle API)
// ----------------------------------------------------------------------------
// Routes:
//   POST   /users          → Create user (with validation)
//   GET    /users/{id}     → Read user
//   PUT    /users/{id}     → Update user
//   DELETE /users/{id}     → Delete user
// ============================================================================

#include <vix.hpp>                  // App, http, ResponseWrapper, etc.
#include <vix/json/Simple.hpp>      // Vix::json::token, obj(), array()
#include <vix/utils/Validation.hpp> // required(), num_range(), match(), validate_map
#include <nlohmann/json.hpp>

#include <unordered_map>
#include <mutex>
#include <string>
#include <optional>
#include <sstream>
#include <vector>

using namespace Vix;
namespace J = Vix::json;
using njson = nlohmann::json;
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

// --------------------------- Helpers ----------------------------------------
static J::kvs user_to_kvs(const User &u)
{
    return J::obj({"id", u.id,
                   "name", u.name,
                   "email", u.email,
                   "age", static_cast<long long>(u.age)});
}

static std::string to_string_safe(const njson &j)
{
    if (j.is_string())
        return j.get<std::string>();
    if (j.is_number_integer())
        return std::to_string(j.get<long long>());
    if (j.is_number_unsigned())
        return std::to_string(j.get<unsigned long long>());
    if (j.is_number_float())
        return std::to_string(j.get<double>());
    if (j.is_boolean())
        return j.get<bool>() ? "true" : "false";
    return {};
}

static bool parse_user(const njson &j, User &out)
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

static std::string gen_id_from_email(const std::string &email)
{
    const auto h = std::hash<std::string>{}(email) & 0xFFFFFFull;
    std::ostringstream oss;
    oss << h;
    return oss.str();
}

// --------------------------- Main -------------------------------------------
int main()
{
    App app;

    // CREATE (POST /users)
    app.post("/users", [](auto &req, auto &res)
             {
        njson body;
        try {
            body = njson::parse(req.body());
        } catch (...) {
            res.status(http::status::bad_request).json({
                "error", "Invalid JSON"
            });
            return;
        }

        // Prépare les champs pour la validation (map<string,string>)
        std::unordered_map<std::string, std::string> data{
            {"name",  body.value("name",  std::string{})},
            {"email", body.value("email", std::string{})},
            {"age",   body.contains("age") ? to_string_safe(body["age"]) : std::string{}}
        };

        Schema sch{
            {"name",  required("name")},
            {"age",   num_range(1, 150, "Age")},
            {"email", match(R"(^[^@\s]+@[^@\s]+\.[^@\s]+$)", "Invalid email")}
        };

        auto r = validate_map(data, sch);
        if (r.is_err()) {
            // Construire {"errors": { field: message, ... }} SANS nlohmann
            std::vector<J::token> flat;
            flat.reserve(r.error().size() * 2);
            for (const auto& kv : r.error()) {
                flat.emplace_back(kv.first);   // clé
                flat.emplace_back(kv.second);  // valeur
            }

            res.status(http::status::bad_request).json({
                "errors", J::obj(std::move(flat))
            });
            return;
        }

        User u;
        if (!parse_user(body, u)) {
            res.status(http::status::bad_request).json({
                "error", "Invalid fields"
            });
            return;
        }

        u.id = gen_id_from_email(u.email);

        {
            std::lock_guard<std::mutex> lock(g_mtx);
            g_users[u.id] = u;
        }

        res.status(http::status::created).json({
            "status", "created",
            "user",   user_to_kvs(u)
        }); });

    // READ (GET /users/{id})
    app.get("/users/{id}", [](auto & /*req*/, auto &res, auto &params)
            {
        const std::string id = params["id"];
        std::lock_guard<std::mutex> lock(g_mtx);
        auto it = g_users.find(id);
        if (it == g_users.end()) {
            res.status(http::status::not_found).json({
                "error", "User not found"
            });
            return;
        }
        res.json({
            "user", user_to_kvs(it->second)
        }); });

    // UPDATE (PUT /users/{id})
    app.put("/users/{id}", [](auto &req, auto &res, auto &params)
            {
        const std::string id = params["id"];

        njson body;
        try {
            body = njson::parse(req.body());
        } catch (...) {
            res.status(http::status::bad_request).json({
                "error", "Invalid JSON"
            });
            return;
        }

        std::lock_guard<std::mutex> lock(g_mtx);
        auto it = g_users.find(id);
        if (it == g_users.end()) {
            res.status(http::status::not_found).json({
                "error", "User not found"
            });
            return;
        }

        if (body.contains("name"))  it->second.name  = body["name"].get<std::string>();
        if (body.contains("email")) it->second.email = body["email"].get<std::string>();
        if (body.contains("age")) {
            if      (body["age"].is_string())          it->second.age = std::stoi(body["age"].get<std::string>());
            else if (body["age"].is_number_integer())  it->second.age = static_cast<int>(body["age"].get<long long>());
            else if (body["age"].is_number_unsigned()) it->second.age = static_cast<int>(body["age"].get<unsigned long long>());
            else if (body["age"].is_number_float())    it->second.age = static_cast<int>(body["age"].get<double>());
        }

        res.json({
            "status", "updated",
            "user",   user_to_kvs(it->second)
        }); });

    // DELETE (DELETE /users/{id})
    app.del("/users/{id}", [](auto & /*req*/, auto &res, auto &params)
            {
        const std::string id = params["id"];
        std::lock_guard<std::mutex> lock(g_mtx);
        const auto n = g_users.erase(id);
        if (!n) {
            res.status(http::status::not_found).json({
                "error", "User not found"
            });
            return;
        }
        res.json({
            "status",  "deleted",
            "user_id", id
        }); });

    // Lancement
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

## 🧩 ORM Overview

The **Vix ORM (Object–Relational Mapper)** is a modern, lightweight database layer built in pure C++20.  
It provides a clean abstraction over SQL databases (MySQL and SQLite), enabling developers to interact with data using expressive and type-safe C++ interfaces.

The ORM follows a **Repository + Unit of Work** pattern inspired by enterprise frameworks — but optimized for high performance and simplicity.

---

### 🧠 Core Components

```markdown
| **Class / Module**               | **Purpose**                                                                                                |
| -------------------------------- | ---------------------------------------------------------------------------------------------------------- |
| `ConnectionPool`                 | Manages a pool of reusable database connections for maximum throughput and thread safety.                  |
| `MySQLDriver` / `SQLiteDriver`   | Implements low-level drivers using **MySQL Connector/C++** or **SQLite3** APIs.                            |
| `QueryBuilder`                   | Fluent, chainable interface to build SQL queries safely (`select()`, `where()`, `insert()`, `update()` …). |
| `Repository<T>`                  | Provides CRUD operations for a given entity type, hiding SQL syntax.                                       |
| `Entity`                         | Base class or concept representing a table row — supports serialization, primary keys, and mapping.        |
| `Mapper`                         | Maps entities to database columns and handles conversions between C++ objects and SQL result sets.         |
| `Transaction`                    | RAII wrapper for transactional consistency (commit/rollback automatically handled).                        |
| `UnitOfWork`                     | Tracks pending changes across multiple repositories and commits them atomically.                           |
| `Migration` / `MigrationsRunner` | Handles versioned schema migrations with up/down scripts to evolve database structure over time.           |
| `DBError`                        | Custom exception type providing detailed SQL error diagnostics (driver, code, message).                    |
```

---

### ⚙️ Supported Drivers

```markdown
| **Driver** | **Status**  | **Library Dependency**        | **Notes**                                     |
| ---------- | ----------- | ----------------------------- | --------------------------------------------- |
| MySQL      | ✅ Stable   | `mysql-connector-cpp`         | Full CRUD, prepared statements, transactions. |
| SQLite3    | ⚙️ Optional | `libsqlite3-dev` (if enabled) | Lightweight local database (experimental).    |
```

Enable drivers in CMake:

```bash
# MySQL only (default)
cmake -S . -B build -DVIX_ORM_USE_MYSQL=ON -DVIX_ORM_USE_SQLITE=OFF

# MySQL + SQLite
cmake -S . -B build -DVIX_ORM_USE_MYSQL=ON -DVIX_ORM_USE_SQLITE=ON
```

# 🧩 Example — Simple Repository Pattern

```cpp
#include <vix/orm/orm.hpp>
using namespace Vix::orm;

struct User {
    int id;
    std::string name;
    std::string email;
};

// Example CRUD
int main() {
    auto pool = ConnectionPool::create_mysql("tcp://127.0.0.1:3306", "root", "pass", "vixdb");

    Repository<User> repo(pool);

    // Create
    User u{0, "Gaspard", "gaspardkirira@outlook.com"};
    repo.insert(u);

    // Read
    auto users = repo.find_all();
    for (auto &u : users)
        std::cout << u.name << " <" << u.email << ">\n";

    // Update
    u.email = "gaspardkirira9@gmail.com";
    repo.update(u);

    // Delete
    repo.remove(u.id);
}
```

# 🚀 Highlights

- Fully thread-safe via internal connection pooling
- Type-safe query builder — no string concatenation or unsafe SQL
- Automatic transactions (RAII-style rollback on exceptions)
- Unified interface for MySQL and SQLite
- Composable design — can be used standalone or as part of the full Vix.cpp stack

### 🧩 Example ORM Demos

```markdown
| **Example File**           | **Description**                                        |
| -------------------------- | ------------------------------------------------------ |
| `users_crud.cpp`           | Full CRUD workflow using the Repository pattern.       |
| `repository_crud_full.cpp` | Demonstrates bulk operations and entity relationships. |
| `tx_unit_of_work.cpp`      | Example of Transaction and Unit-of-Work patterns.      |
| `querybuilder_update.cpp`  | Fluent construction of dynamic SQL `UPDATE` queries.   |
| `migrate_init.cpp`         | Initialization and execution of schema migrations.     |
| `error_handling.cpp`       | Robust error handling using `DBError` exception types. |
```

# Vix.cpp Examples

This folder contains example applications demonstrating how to use the Vix.cpp framework.

Each file shows how to create routes for different HTTP methods: GET, POST, PUT, DELETE.

---

## Example Files

| File                            | Description                                                      |
| ------------------------------- | ---------------------------------------------------------------- |
| `hello_routes.cpp`              | Demonstrates GET routes, including parameterized paths.          |
| `user_crud_with_validation.cpp` | Demonstrates POST,GET,PUT,DELETE routes with JSON body handling. |

---

## Usage

Build the examples:

```bash
cd build
./user_crud_with_validation
```

2. Run an example:

```bash
./hello_routes       # Runs GET routes
./post_user          # Runs POST routes
./put_user           # Runs PUT routes
./delete_user        # Runs DELETE routes
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
// ============================================================================
// trace_route.cpp — contextual logging + request tracing (new Vix.cpp API)
// GET /trace -> {"rid":"<uuid>","ok":true}
// ============================================================================

#include <vix.hpp>
#include <vix/utils/Logger.hpp>
#include <vix/utils/UUID.hpp>
#include <string>

using namespace Vix;

int main() {
    auto& log = Logger::getInstance();
    log.setPattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
    log.setLevel(Logger::Level::INFO);
    log.setAsync(true);

    App app;

    app.get("/trace", [](auto& req, auto& res) {
        Logger::Context cx;
        cx.request_id = utils::uuid4();
        cx.module     = "trace_handler";
        Logger::getInstance().setContext(cx);

        std::string path(req.target().data(), req.target().size());
        std::string method(req.method_string().data(), req.method_string().size());

        Logger::getInstance().logf(
            Logger::Level::INFO,
            "Incoming request",
            "path",   path.c_str(),
            "method", method.c_str(),
            "rid",    cx.request_id.c_str()
        );

        res.json({
            "rid", cx.request_id,
            "ok",  true
        });
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
// ============================================================================
// now_server.cpp — Demo route: current time in ISO 8601 and milliseconds
// GET /now -> {"iso8601":"2025-10-09T10:34:12.123Z","ms":1696848852123}
// ============================================================================

#include <vix.hpp>
#include <vix/utils/Env.hpp>
#include <vix/utils/Time.hpp>
#include <vix/utils/Logger.hpp>

using namespace Vix;

int main()
{
    auto &log = Logger::getInstance();
    log.setPattern("[%H:%M:%S.%e] [%^%l%$] %v");
    log.setLevel(Logger::Level::INFO);

    const int port = utils::env_int("PORT", 8080);

    App app;

    // GET /now → returns current ISO 8601 timestamp and epoch ms
    app.get("/now", [](auto &, auto &res)
            { res.json({"iso8601", utils::iso8601_now(),
                        "ms", static_cast<long long>(utils::now_ms())}); });

    log.log(Logger::Level::INFO, "Starting server on port {}", port);

    app.run(port);
    return 0;
}


```

# examples/json_builders_routes.cpp

```cpp
// ============================================================================
// json_builders_routes.cpp — Minimal routes using Vix::json builders (new API)
// GET /hello          -> {"message":"Hello, World!"}
// GET /users/{id}     -> {"user":{"id":"<id>","active":true}}
// GET /roles          -> {"roles":["admin","editor","viewer"]}
// ============================================================================

#include <vix.hpp>
#include <vix/json/Simple.hpp>

using namespace Vix;
namespace J = Vix::json;

int main()
{
        App app;

        // GET /hello -> {"message": "Hello, World!"}
        app.get("/hello", [](auto &, auto &res)
                { res.json({"message", "Hello, World!"}); });

        // GET /users/{id} -> {"user": {"id": "...", "active": true}}
        app.get("/users/{id}", [](auto &, auto &res, auto &params)
                {
        const std::string id = params["id"];
        res.json({
            "user", J::obj({
                "id",     id,
                "active", true
            })
        }); });

        // GET /roles -> {"roles": ["admin", "editor", "viewer"]}
        app.get("/roles", [](auto &, auto &res)
                { res.json({"roles", J::array({"admin", "editor", "viewer"})}); });

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
