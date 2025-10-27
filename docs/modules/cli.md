# 🧩 Vix.cpp — CLI Module

![C++](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
![License](https://img.shields.io/badge/License-MIT-green)
![Status](https://img.shields.io/badge/Status-Stable-success)
![Platform](https://img.shields.io/badge/Platform-Linux%20|%20macOS%20|%20Windows-lightgrey)

> **vix.cpp/cli** — The official command-line interface for the [Vix.cpp](https://github.com/vixcpp/vix) framework.  
> Designed for speed and simplicity, it lets you **create**, **build**, and **run** modern C++ web applications with a single command.

---

## 🚀 Overview

The **Vix CLI** (`vix`) provides a clean developer experience similar to **FastAPI**, **Vue CLI**, or **Cargo** — but for **modern C++20**.  
It manages your projects, builds them efficiently, and runs them instantly.

---

## ⚙️ Features

- 🏗️ **Project scaffolding** — `vix new <name>` creates a complete C++ app structure.
- ⚡ **Unified build system** — `vix build` compiles using CMake presets and caching.
- 🚀 **Instant execution** — `vix run` builds (if needed) and launches automatically.
- 📦 **Portable binary** — works as a standalone executable or within the full Vix suite.
- 🧠 **Minimal dependencies** — written in clean, modern C++20.
- 🎨 **Rich logging** — colorized output powered by `Vix::Logger`.

---

## 🧰 Commands

```bash
vix <command> [options]
```

| Command                   | Description                                        |
| ------------------------- | -------------------------------------------------- |
| `vix new <name>`          | Create a new Vix project in the current directory. |
| `vix build [name]`        | Build a Vix project or app (uses CMake).           |
| `vix run [name] [--args]` | Run the specified project or the root app.         |
| `vix version`             | Show the current CLI version.                      |
| `vix help`                | Display available commands and usage examples.     |

---

## 🧪 Examples

```bash
# Create a new project
vix new blog

# Build the current project
vix build

# Run the app on port 8080
vix run -- --port 8080
```

Sample output:

```
[vix][info] Building project 'blog'...
[vix][success] Build complete (1.2s)
[vix][info] Running ./build/blog ...
[vix][ready] Listening on http://localhost:8080 🚀
```

---

## 🧩 Architecture

The CLI follows a **command registry** pattern for extensibility:

```cpp
std::unordered_map<std::string, std::function<int(const std::vector<std::string>&)>> commands_;
```

Each command (`new`, `build`, `run`) is implemented as a separate handler under `src/commands/`.

| File                        | Description                                         |
| --------------------------- | --------------------------------------------------- |
| `include/vix/cli/CLI.hpp`   | Core CLI class definition                           |
| `src/CLI.cpp`               | Command dispatcher & entrypoint                     |
| `commands/NewCommand.cpp`   | Handles project creation                            |
| `commands/BuildCommand.cpp` | Handles CMake builds                                |
| `commands/RunCommand.cpp`   | Runs the built executable                           |
| `CMakeLists.txt`            | Build config (supports LTO, sanitizers, standalone) |

---

## ⚙️ Build & Installation

### Build Standalone

```bash
git clone https://github.com/vixcpp/vix.git
cd vix/modules/cli
cmake -B build -S .
cmake --build build -j$(nproc)
```

Binary output:

```bash
./build/vix
```

### Build with the Full Framework

```bash
cd vix
cmake -B build -S .
cmake --build build -j$(nproc)
```

The CLI (`vix`) is built and installed automatically as part of the umbrella project.

---

## 🧰 Development Flags

| Flag                    | Default | Description                      |
| ----------------------- | ------- | -------------------------------- |
| `VIX_ENABLE_SANITIZERS` | OFF     | Enables AddressSanitizer + UBSan |
| `VIX_ENABLE_LTO`        | OFF     | Enables Link-Time Optimization   |

Example:

```bash
cmake -B build -S . -DVIX_ENABLE_SANITIZERS=ON
```

Run:

```bash
./vix help
```

---

## 📦 Output

```bash
✨ Vix.cpp CLI - Developer Commands ✨
----------------------------------------
  new <name>          Create a new Vix project
  build [name]        Build a project (root or app)
  run [name] [--args] Run a project or app
  version             Show CLI version
  help                Show this help message

Examples:
  vix new blog
  vix build blog --config Debug
  vix run blog -- --port 8080
```

---

## 🧾 License

**MIT License** © [Gaspard Kirira](https://github.com/gkirira)  
See [LICENSE](../../LICENSE) for details.
