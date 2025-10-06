# Vix.cpp

![C++](https://img.shields.io/badge/C++20-Standard-blue)
![License](https://img.shields.io/badge/License-MIT-green)

Vix.cpp is a modular C++ backend framework inspired by FastAPI, Vue.js, and React.js. It simplifies backend development while keeping high performance.

---

## Benchmarks & Examples

All tests were performed with [wrk](https://github.com/wg/wrk), 4 threads, 50 connections, for 10 seconds.

| Framework | Requests/sec | Avg Latency | Transfer/sec |
| --------- | ------------ | ----------- | ------------ |
| Go        | 81336        | 674.28µs    | 10.16MB      |
| Vix.cpp   | 40447        | 1.18ms      | 4.71MB       |
| Node.js   | 4220         | 16.00ms     | 0.97MB       |
| PHP       | 2804         | 16.87ms     | 498.38KB     |
| Crow      | 1149         | 41.60ms     | 358.95KB     |
| FastAPI   | 752          | 63.71ms     | 111.99KB     |

## Requests/sec Comparison

Go [:████████████████████] 81336
Vix.cpp [:███████████████·····] 40447
Node.js [:██··················] 4220
PHP [:█···················] 2804
Crow [:····················] 1149
FastAPI [:····················] 752

- Each `█` block roughly represents ~4000 req/sec.
- `·` is used as a filler to preserve alignment.
- Helps quickly see relative throughput.

---

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
wrk -t4 -c50 -d10s http://localhost:8080/hello
# 40447 req/sec, 4.71MB/sec

wrk -t4 -c50 -d10s http://localhost:8080/users/1
# 49322 req/sec, 5.17MB/sec
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

int main() {
    Vix::App app;

    app.post("/users", [](auto &req, auto &res) {
        auto body = req.body();
        res.json({{"status", "created"}, {"data", body}});
    });

    app.run(8080);
}
```

## Test with:

```bash
curl -X POST http://localhost:8080/users -d '{"name":"Alice"}' -H "Content-Type: application/json"
```

## PUT Example

```cpp
#include <vix/core.h>

int main() {
    Vix::App app;

    app.put("/users/{id}", [](auto &req, auto &res, auto &params) {
        std::string id = params["id"];
        auto body = req.body();
        res.json({{"status", "updated"}, {"user_id", id}, {"data", body}});
    });

    app.run(8080);
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
