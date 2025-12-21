# 🧩 Vix.cpp — CLI Module

### Modern C++ Runtime Tooling • Zero-Friction Development • Fast Web Apps

![C++](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
![License](https://img.shields.io/badge/License-MIT-green)
![Status](https://img.shields.io/badge/Status-Stable-success)
![Platform](https://img.shields.io/badge/Platform-Linux%20|%20macOS%20|%20Windows-lightgrey)
![Runtime](https://img.shields.io/badge/Runtime-Vix.cpp%201.x-orange)

> **Vix CLI** is the official command-line interface for  
> **Vix.cpp** — the modern C++ backend runtime.
>
> It provides a **professional, modern, runtime-like developer experience**
> for C++, comparable to **Python**, **Node.js**, **Deno**, or **Bun**.

---

# 🚀 Overview

The Vix CLI (`vix`) brings modern runtime ergonomics to C++:

- Instant project creation
- Smart CMake-based builds
- Friendly compiler diagnostics
- Sanitizer-first validation
- Script-like execution of `.cpp` files
- Packaging & artifact verification
- Built-in interactive REPL (**default**)

Running `vix` with no arguments launches the **interactive REPL**.

---

# ⚙️ Features

## 🧠 Built-in REPL (default)

```bash
vix
```

- Variables & expressions
- JSON literals
- Math evaluation
- Runtime APIs (`Vix.cwd()`, `Vix.env()`, etc.)
- Script-like exploration

Explicit mode:

```bash
vix repl
```

---

## 🏗️ Project scaffolding

```bash
vix new blog
```

Creates:

- CMake-based project
- Modern C++20 structure
- Ready-to-run Vix app

---

## ⚡ Smart build system

```bash
vix build
```

- Uses CMake presets automatically
- Parallel builds
- Colored logs & spinners
- Clean Ctrl+C handling

---

## 🚀 Run applications

```bash
vix run
```

- Auto-build if required
- Real-time logs
- Runtime log-level injection

Script mode:

```bash
vix run demo.cpp
```

---

## 🧪 Check & Tests (Sanitizers ready)

Compile-only validation:

```bash
vix check
vix check demo.cpp
```

With sanitizers:

```bash
vix check demo.cpp --san
vix check demo.cpp --asan
vix check demo.cpp --ubsan
vix check demo.cpp --tsan
```

Run tests:

```bash
vix tests
vix tests --san
```

---

## 📦 Packaging & Verification

Create a distribution artifact:

```bash
vix pack --name blog --version 1.0.0
```

Verify artifacts:

```bash
vix verify dist/blog@1.0.0
vix verify dist/blog@1.0.0 --require-signature
```

---

## 🧠 ErrorHandler — your C++ teacher

- Explains template & overload errors
- Detects missing includes
- Highlights the _first real error_
- Provides actionable hints

---

# 🧰 Commands

```bash
vix <command> [options]
```

| Command                   | Description                  |
| ------------------------- | ---------------------------- |
| `vix`                     | Start REPL (default)         |
| `vix repl`                | Start REPL explicitly        |
| `vix new <name>`          | Create a new project         |
| `vix build [name]`        | Configure + build            |
| `vix run [name] [--args]` | Build and run                |
| `vix dev [name]`          | Dev mode (watch & reload)    |
| `vix check [path]`        | Compile-only validation      |
| `vix tests [path]`        | Run tests                    |
| `vix pack [options]`      | Create distribution artifact |
| `vix verify [options]`    | Verify artifact              |
| `vix orm <subcommand>`    | ORM tooling                  |
| `vix help [command]`      | Show help                    |
| `vix version`             | Show version                 |

---

# 🧪 Usage Examples

```bash
vix
vix new api
cd api
vix dev
vix check --san
vix tests
vix pack --name api --version 1.0.0
vix verify dist/api@1.0.0
```

---

# 🧩 Architecture

The CLI is built around a command dispatcher:

```cpp
std::unordered_map<std::string, CommandHandler> commands;
```

### Main components

| Path                             | Responsibility       |
| -------------------------------- | -------------------- |
| `include/vix/cli/CLI.hpp`        | CLI entry & parsing  |
| `src/CLI.cpp`                    | Command routing      |
| `src/ErrorHandler.cpp`           | Compiler diagnostics |
| `src/commands/ReplCommand.cpp`   | Interactive REPL     |
| `src/commands/CheckCommand.cpp`  | Validation           |
| `src/commands/PackCommand.cpp`   | Packaging            |
| `src/commands/VerifyCommand.cpp` | Verification         |

---

# 🔧 Build & Installation

### Standalone CLI build

```bash
git clone https://github.com/vixcpp/vix.git
cd vix/modules/cli
cmake -B build -S .
cmake --build build -j$(nproc)
```

Binary:

```bash
./build/vix
```

---

### Full Vix build

```bash
cd vix
cmake -B build -S .
cmake --build build
```

---

# ⚙️ Configuration

### Environment variables

| Variable              | Description               |
| --------------------- | ------------------------- |
| `VIX_LOG_LEVEL`       | Runtime log level         |
| `VIX_STDOUT_MODE`     | `line` for real-time logs |
| `VIX_MINISIGN_SECKEY` | Secret key for `pack`     |
| `VIX_MINISIGN_PUBKEY` | Public key for `verify`   |

---

# 📦 CLI Help Output

```sql
Vix.cpp — Modern C++ backend runtime
Version: v1.x.x

Usage:
  vix [GLOBAL OPTIONS] <COMMAND> [ARGS...]
  vix help <command>

Quick start:
  vix new api
  cd api && vix dev
  vix pack --name api --version 1.0.0 && vix verify

Commands:
  Project:
    new <name>               Create a new Vix project
    build [name]             Configure + build
    run   [name]             Build and run
    dev   [name]             Dev mode
    check [path]             Compile-only validation
    tests [path]             Run tests

  Packaging & security:
    pack                     Create distribution artifact
    verify                   Verify artifact or package

  REPL:
    repl                     Start interactive REPL
    (default)                Run `vix` to start the REPL

Global options:
  --verbose
  -q, --quiet
  --log-level <level>
  -h, --help
  -v, --version

```

---

# 🧾 License

**MIT License** © [Gaspard Kirira](https://github.com/gkirira)  
See [`LICENSE`](../../LICENSE) for details.
