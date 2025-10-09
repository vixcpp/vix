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

````markdown
| Framework            | Requests/sec | Avg Latency | Transfer/sec   |
| -------------------- | ------------ | ----------- | -------------- |
| **Vix.cpp (v1.3.0)** | **84,582**   | **2.38 ms** | **19.28 MB/s** |
| Go (Fiber)           | 81,336       | 0.67 ms     | 10.16 MB/s     |
| Node.js (Fastify)    | 4,220        | 16.00 ms    | 0.97 MB/s      |
| PHP (Slim)           | 2,804        | 16.87 ms    | 0.49 MB/s      |
| Crow (C++)           | 1,149        | 41.60 ms    | 0.35 MB/s      |
| FastAPI (Python)     | 752          | 63.71 ms    | 0.11 MB/s      |

🟢 Result
Vix.cpp (v1.3.0) reaches ~84 K requests/sec with an average latency of 2.3 ms —
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
            "framework", "Vix.cpp",
            "message", "Welcome to the future of C++ web development 🚀"
        });
    });

    // GET /hello/{name}
    app.get("/hello/{name}", [](auto&, auto& res, auto& params) {
        res.json({
            "greeting", "Hello " + params["name"] + " 👋",
            "powered_by", "Vix.cpp"
        });
    });

    // Start server
    app.run(8080);
    return 0;
}
```
````

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
| `hello_routes`              | Demonstrates handling of **GET** routes.    |
| `post_user`                 | Demonstrates handling of **POST** routes.   |
| `put_example`               | Demonstrates handling of **PUT** routes.    |
| `delete_user`               | Demonstrates handling of **DELETE** routes. |
| `validation_user_create`    | Example of form validation logic.           |
| `trace_route`               | Logging with contextual data and UUIDs.     |
| `now_server`                | Environment variables and time demo.        |
| `user_crud_with_validation` | Full **CRUD** workflow with validation.     |

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
