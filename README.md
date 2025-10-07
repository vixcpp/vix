# Vix.cpp

![C++](https://img.shields.io/badge/C++20-Standard-blue)
![License](https://img.shields.io/badge/License-MIT-green)

Vix.cpp is a modular C++ backend framework inspired by FastAPI, Vue.js, and React.js. It simplifies backend development while keeping high performance.

---

## Benchmarks & Examples

All tests were performed with [wrk](https://github.com/wg/wrk), 4 threads, 50 connections, for 10 seconds.

| Framework | Requests/sec | Avg Latency | Transfer/sec |
| --------- | ------------ | ----------- | ------------ |
| Vix.cpp   | 73612        | 2.71ms      | 12.43MB      |
| Go        | 81336        | 674.28µs    | 10.16MB      |
| Node.js   | 4220         | 16.00ms     | 0.97MB       |
| PHP       | 2804         | 16.87ms     | 498.38KB     |
| Crow      | 1149         | 41.60ms     | 358.95KB     |
| FastAPI   | 752          | 63.71ms     | 111.99KB     |

### Vix.cpp Example

```cpp
#include <vix/core.h>

int main()
{
    Vix::App app;

    app.get("/hello", [](auto &req, auto &res)
    { res.json({{"message", "Hello, World!"}}); });

    app.get("/users/{id}", [](auto &req, auto &res, auto &params)
    {
        std::string id = params["id"];
        res.json({{"user_id", id}});
    });

    app.run(8080);
}
```

Benchmark:

```bash
wrk -t8 -c200 -d10s --latency http://localhost:8080/hello >/dev/null \ && wrk -t8 -c200 -d30s --latency http://localhost:8080/hello
# 73612 req/sec, 12.43MB/sec
```

## Go Example

```go
package main

import (
    "fmt"
    "net/http"
)

func helloHandler(w http.ResponseWriter, r *http.Request) {
    fmt.Fprintln(w, "Hello, world!")
}

func main() {
    http.HandleFunc("/hello", helloHandler)
    fmt.Println("Server running on http://localhost:8001")
    http.ListenAndServe(":8001", nil)
}
```

## Benchmark

```bash
wrk -t4 -c50 -d10s http://localhost:8001/hello
# 81336 req/sec, 10.16MB/sec
```

# Node.js (Express) Example

```javascript
const express = require("express");
const app = express();
const PORT = 3000;

app.get("/hello", (req, res) => {
  res.send("Hello, world!");
});

app.listen(PORT, () => {
  console.log(`Server running on http://localhost:${PORT}`);
});
```

## Benchmark:

```bash
wrk -t4 -c50 -d10s http://localhost:3000/hello
# 4220 req/sec, 0.97MB/sec
```

# Crow (C++) Example

```cpp
#include <crow.h>

int main()
{
    crow::SimpleApp app;

    CROW_ROUTE(app, "/hello")([](){ return "Hello, world!"; });

    app.port(18080).concurrency(8).run();
}
```

## Benchmark:

```bash
wrk -t4 -c50 -d10s http://localhost:18080/hello
# 1149 req/sec, 358.95KB/sec
```

# FastAPI Example (Python)

```python
from fastapi import FastAPI

app = FastAPI()

@app.get("/hello")
def read_root():
    return {"message": "Hello, World!"}
```

## Benchmark:

```bash
wrk -t4 -c50 -d10s http://localhost:3001/hello
# 752 req/sec, 111.99KB/sec
```

## PHP Example

```php
<?php
header('Content-Type: application/json');

if ($_SERVER['REQUEST_METHOD'] === 'GET' && $_SERVER['REQUEST_URI'] === '/hello') {
    echo json_encode(["message" => "Hello, World!"]);
} else {
    http_response_code(404);
    echo json_encode(["error" => "Not Found"]);
}
```

## Benchmark:

```bash
wrk -t4 -c50 -d10s http://localhost:3002/hello
# 2804 req/sec, 498.38KB/sec
```

# Notes

Vix.cpp provides high throughput and low latency while remaining modular and flexible in C++.

Go achieves the highest raw performance in these benchmarks.

Latency is measured as the average time per request.

All tests were conducted on the same machine under identical conditions.

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

You need a C++20 compiler, CMake, and several libraries. Below are instructions for common platforms.

- C++20 compiler (GCC 12+, Clang 16+, MSVC 2022+)
- CMake 3.20+
- Boost libraries (asio, beast)
- nlohmann/json
- spdlog

1. C++20 Compiler

## Linux (Ubuntu/Debian):

```bash
sudo apt update
sudo apt install g++-12 clang-16 -y
```

## macOS (Homebrew):

```bash
brew install gcc
brew install llvm
```

## Windows:

Install MSVC 2022 via Visual Studio installer with Desktop C++ workload.

2. CMake 3.20+

## Linux/macOS:

```bash
sudo apt install cmake # Linux
brew install cmake # macOS
```

Windows: Install via CMake official site : https://cmake.org/download/

3. Boost Libraries (asio & beast)

# Linux:

```bash
sudo apt install libboost-all-dev
```

# macOS:

```bash
brew install boost
```

Windows: Use vcpkg or download from Boost.org : https://www.boost.org/

4. nlohmann/json

# Linux/macOS (via package manager):

```bash
sudo apt install nlohmann-json3-dev   # Linux
brew install nlohmann-json            # macOS
```

Windows: Install via vcpkg:

```bash
vcpkg install nlohmann-json
```

5. spdlog

# Linux/macOS:

```bash
sudo apt install libspdlog-dev   # Linux
brew install spdlog              # macOS
```

Windows: Install via vcpkg:

```bash
vcpkg install spdlog
```

### Build

Clone the repository, configure, and build all modules including examples:

```bash
# Clone the vix repository
git clone https://github.com/vixcpp/vix.git
cd vix

# Initialize and update submodules (core and others)
git submodule update --init --recursive

# Create a build directory and move into it
mkdir build && cd build

# Configure the project with CMake
cmake ..

# Compile all modules and examples using all CPU cores
make -j$(nproc)

```

## What this does:

Builds the core module as a library

Sets up the umbrella interface vix

Compiles all example executables in examples/:

main → general example

get_example → GET routes

post_example → POST routes

put_example → PUT routes

delete_example → DELETE routes

Copies config/config.json to the build directory automatically

## Run examples:

# From the build directory

./main
./get_example
./post_example
./put_example
./delete_example

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

## POST Example

```cpp
#include <vix/core.h>
#include <nlohmann/json.hpp>

int main()
{
    Vix::App app;

    app.get("/hello", [](auto &req, auto &res)
            { res.json(nlohmann::json{{"message", "Hello, World!"}}); });

    app.get("/users/{id}", [](auto &req, auto &res, auto &params)
            {
        std::string id = params["id"];
        res.json(nlohmann::json{{"user_id", id}}); });

    app.run(8080);
    return 0;
}
```

## Test with:

```bash
curl -X POST http://localhost:8080/users -d '{"name":"Alice"}' -H "Content-Type: application/json"
```

## PUT Example

```cpp
#include <vix/core.h>
#include <nlohmann/json.hpp>

int main()
{
    Vix::App app;

    app.put("/users/{id}", [](auto &req, auto &res, auto &params)
            {
        std::string id = params["id"];
        std::string name = "Jane";
        res.json(nlohmann::json{{"message", "User updated"}, {"id", id}, {"name", name}}); });

    app.run(8080);
    return 0;
}
```

## Test with:

```bash
curl -X PUT http://localhost:8080/users/1 -d '{"name":"Bob"}' -H "Content-Type: application/json"
```

## DELETE Example

```cpp
#include <vix/core.h>

int main() {
    Vix::App app;

    app.del("/users/{id}", [](auto &req, auto &res, auto &params) {
        std::string id = params["id"];
        res.json({{"status", "deleted"}, {"user_id", id}});
    });

    app.run(8080);
}
```

## Test with:

```bash
curl -X DELETE http://localhost:8080/users/1
```

## 🧩 Example: Minimal RESTful API with Vix.cpp

This example demonstrates how to build a complete CRUD REST API in pure C++ using Vix.cpp, with no external frameworks required.

It showcases how powerful and expressive the framework can be while remaining header-only, blazing fast, and thread-safe.

Uses the Vix::App HTTP server to define routes (GET, POST, PUT, DELETE).

Integrates JSON parsing via the built-in Vix::json module (wrapper around nlohmann/json).

Demonstrates validation, mutex-protected data, and in-memory storage via STL containers.

All responses are structured as JSON, with automatic status codes and validation errors.

👉 In just a few lines, you get a production-grade REST service written in C++, capable of handling 70 000+ req/s with minimal latency.

```cpp
#include <vix/core.h>
#include <vix/json/json.hpp>
#include <vix/utils/Validation.hpp>

#include <unordered_map>
#include <mutex>
#include <string>

namespace J = Vix::json;
using namespace Vix::utils;

struct User
{
    std::string id;
    std::string name;
    std::string email;
    int age{};
};

static std::mutex g_mtx;
static std::unordered_map<std::string, User> g_users;

static J::Json to_json(const User &u)
{
    return J::o("id", u.id, "name", u.name, "email", u.email, "age", u.age);
}

static std::string j_to_string(const nlohmann::json &j, const char *k)
{
    if (!j.contains(k))
        return {};
    const auto &v = j[k];
    if (v.is_string())
        return v.get<std::string>();
    if (v.is_number_integer())
        return std::to_string(v.get<long long>());
    if (v.is_number_unsigned())
        return std::to_string(v.get<unsigned long long>());
    if (v.is_number_float())
        return std::to_string(v.get<double>());
    if (v.is_boolean())
        return v.get<bool>() ? "true" : "false";
    return v.dump(); // objet/array → JSON string
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

int main()
{
    Vix::App app;

    // CREATE
    app.post("/users", [](auto &req, auto &res)
             {
        J::Json body;
        try {
            body = J::loads(req.body());
        } catch (...) {
            res.status(Vix::http::status::bad_request).json(J::o("error", "Invalid JSON"));
            return;
        }

        std::unordered_map<std::string, std::string> data{
            {"name",  j_to_string(body, "name")},
            {"email", j_to_string(body, "email")},
            {"age",   j_to_string(body, "age")}
        };

        Schema sch{
            {"name",  required("name")},
            {"age",   num_range(1, 150, "Age")},
            {"email", match(R"(^[^@\s]+@[^@\s]+\.[^@\s]+$)")}
        };

        auto r = validate_map(data, sch);
        if (r.is_err()) {
            J::Json e = J::o();
            for (const auto &kv : r.error()) e[kv.first] = kv.second;
            res.status(Vix::http::status::bad_request).json(J::o("errors", e));
            return;
        }

        User u;
        if (!parse_user(body, u)) {
            res.status(Vix::http::status::bad_request).json(J::o("error", "Invalid fields"));
            return;
        }

        u.id = std::to_string(std::hash<std::string>{}(u.email) & 0xFFFFFF);

        {
            std::lock_guard<std::mutex> lock(g_mtx);
            g_users[u.id] = u;
        }

        res.status(Vix::http::status::created).json(J::o("status", "created", "user", to_json(u))); });

    // READ
    app.get("/users/{id}", [](auto &req, auto &res, auto &params)
            {
        std::string id = params["id"];
        std::lock_guard<std::mutex> lock(g_mtx);
        auto it = g_users.find(id);
        if (it == g_users.end()) {
            res.status(Vix::http::status::not_found).json(J::o("error", "Not found"));
            return;
        }
        res.json(J::o("user", to_json(it->second))); });

    // UPDATE
    app.put("/users/{id}", [](auto &req, auto &res, auto &params)
            {
        std::string id = params["id"];

        J::Json body;
        try {
            body = J::loads(req.body());
        } catch (...) {
            res.status(Vix::http::status::bad_request).json(J::o("error", "Invalid JSON"));
            return;
        }

        std::lock_guard<std::mutex> lock(g_mtx);
        auto it = g_users.find(id);
        if (it == g_users.end()) {
            res.status(Vix::http::status::not_found).json(J::o("error", "Not found"));
            return;
        }

        if (body.contains("name"))  it->second.name  = body["name"].get<std::string>();
        if (body.contains("email")) it->second.email = body["email"].get<std::string>();
        if (body.contains("age")) {
            if (body["age"].is_string())             it->second.age = std::stoi(body["age"].get<std::string>());
            else if (body["age"].is_number_integer())   it->second.age = static_cast<int>(body["age"].get<long long>());
            else if (body["age"].is_number_unsigned())  it->second.age = static_cast<int>(body["age"].get<unsigned long long>());
            else if (body["age"].is_number_float())     it->second.age = static_cast<int>(body["age"].get<double>());
        }

        res.json(J::o("status", "updated", "user", to_json(it->second))); });

    // DELETE
    app.del("/users/{id}", [](auto &req, auto &res, auto &params)
            {
        std::string id = params["id"];
        std::lock_guard<std::mutex> lock(g_mtx);
        auto n = g_users.erase(id);
        if (!n) {
            res.status(Vix::http::status::not_found).json(J::o("error", "Not found"));
            return;
        }
        res.json(J::o("status", "deleted", "user_id", id)); });

    app.run(8080);
    return 0;
}
```

# Vix.cpp Examples

This folder contains example applications demonstrating how to use the Vix.cpp framework.

Each file shows how to create routes for different HTTP methods: GET, POST, PUT, DELETE.

---

## Example Files

| File                 | Description                                             |
| -------------------- | ------------------------------------------------------- |
| `get_example.cpp`    | Demonstrates GET routes, including parameterized paths. |
| `post_example.cpp`   | Demonstrates POST routes with JSON body handling.       |
| `put_example.cpp`    | Demonstrates PUT routes for updating resources.         |
| `delete_example.cpp` | Demonstrates DELETE routes for removing resources.      |

---

## Usage

1. Build the examples:

```bash
cd vix
mkdir -p build/examples && cd build/examples
cmake ../../
make -j$(nproc)
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

### Notes

Ensure vix.cpp core is built and running before testing.

Modify routes and port numbers in examples if needed.

High performance: C++ backend can handle tens of thousands of requests/sec as shown.

## Repository Structure

```

vix/
├─ modules/
│ ├─ core/
│ ├─ orm/
│ ├─ cli/
│ ├─ middleware/
│ ├─ websocket/
│ └─ devtools/
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
