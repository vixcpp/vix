# Quick Start — Vix.cpp

This guide helps you get a Vix.cpp application running in minutes.

---

## 1️⃣ Clone the Repository

```bash
git clone https://github.com/vixcpp/vix.git
cd vix
git submodule update --init --recursive
```

---

## 2️⃣ Build the Framework

### Linux / macOS

```bash
cmake -S . -B build-rel -DCMAKE_BUILD_TYPE=Release
cmake --build build-rel -j
```

### Windows (Visual Studio + vcpkg)

```bash
cmake -S . -B build-rel -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build-rel -j
```

---

## 3️⃣ Run Your First Example

```bash
./build-rel/hello_routes
# or
./build-rel/main
```

Expected output:

```bash
[GET] / → {"message": "Hello world"}
```

Visit <http://localhost:8080/> to see your first JSON response.

---

## 4️⃣ Create a New App with the CLI

Once Vix.cpp is installed system-wide:

```bash
vix new myapp
cd myapp
vix build
vix run
```

The CLI automatically:

- Configures your project
- Builds using CMake
- Launches the HTTP server

---

## 5️⃣ Verify Installation

```bash
vix --version
```

Should print something like:

```bash
Vix.cpp v1.9.0  (C++20, GCC 13, Ubuntu 24.04)
```

---

## 6️⃣ Explore Examples

See [docs/examples/overview.md](./examples/overview.md) for CRUD, ORM, JSON, Logger, and Time demos.

---

## 7️⃣ Next Steps

- Learn the architecture → [docs/architecture.md](./architecture.md)
- See all build flags → [docs/options.md](./options.md)
- Benchmark results → [docs/benchmarks.md](./benchmarks.md)
