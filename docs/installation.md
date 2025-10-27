# Installation — Vix.cpp

## 🚀 Getting Started

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

This guide explains how to build and install Vix.cpp on Linux, macOS, and Windows.

---

## 🐧 Linux / Ubuntu

### Prerequisites

```bash
sudo apt update
sudo apt install -y \ g++-12 cmake make git \                            # Build tools
  libboost-all-dev \                                 # Boost (includes asio, beast)
  nlohmann-json3-dev \                               # JSON (nlohmann/json)
  libspdlog-dev \                                    # Logging (spdlog)
  libmysqlcppconn-dev                                # Optional: MySQL Connector/C++ for ORM
```

Optional dependencies:

```bash
sudo apt install -y libmysqlcppconn-dev libsqlite3-dev
```

### Build

```bash
git clone https://github.com/vixcpp/vix.git
cd vix
git submodule update --init --recursive
cmake -S . -B build-rel -DCMAKE_BUILD_TYPE=Release
cmake --build build-rel -j
sudo cmake --install build-rel --prefix /usr/local
```

---

## 🍎 macOS

### Prerequisites

Install Homebrew first, then:

```bash
brew install cmake ninja llvm boost nlohmann-json spdlog fmt mysql sqlite3
```

### Build

```bash
cmake -S . -B build-rel -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-rel -j
sudo cmake --install build-rel
```

---

## 🪟 Windows

### Requirements

- Visual Studio 2022 (with C++ Desktop workload)
- [vcpkg](https://github.com/microsoft/vcpkg)
- Git Bash or PowerShell

### Build

```bash
git clone https://github.com/vixcpp/vix.git
cd vix
git submodule update --init --recursive

cmake -S . -B build-rel -G "Ninja" ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake

cmake --build build-rel -j
```

---

## ⚙️ Verify Installation

```bash
vix --version
```

Output example:

```
Vix.cpp v1.9.0 (C++20, GCC 13, Ubuntu 24.04)
```

If `vix` is not found, ensure `/usr/local/bin` (or your custom prefix) is in your PATH.

---

## 🧩 Troubleshooting

- **Missing headers** — Ensure all dependencies are installed (Boost, fmt, spdlog).
- **CMake errors** — Check `CMAKE_CXX_STANDARD` (must be ≥ 20).
- **Link errors on Linux** — Use `lld` (`-fuse-ld=lld`) for faster and cleaner linking.
- **Permission denied on install** — Add `sudo` to the `cmake --install` command.

---

## ✅ Next Steps

- [Quick Start](./quick-start.md)
- [Build & Packaging](./build.md) _(optional)_
- [Benchmarks](./benchmarks.md)
