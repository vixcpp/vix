# Vix CLI — Help Reference

Modern **C++ backend runtime & CLI** designed for performance, clarity, and developer experience.

---

## 🧭 Overview

```text
Vix.cpp — Modern C++ backend runtime
Version: v1.11.0
```

Vix provides a fast, ergonomic CLI for building, running, testing, and packaging modern C++ applications.

---

## 📦 Usage

```bash
vix <command> [options] [args...]
vix help <command>
```

---

## ⚡ Quick Start

```bash
vix new api
cd api && vix dev
vix pack --version 1.0.0 && vix verify
```

---

## 🧰 Commands

### Project

```text
new <name>               Create a new Vix project in ./<name>
build [name]             Configure + build (root project or app)
run   [name] [--args]    Build (if needed) then run
dev   [name]             Dev mode (watch, rebuild, reload)
check [path]             Validate a project or compile a single .cpp (no execution)
tests [path]             Run project tests (alias of check --tests)
```

---

### Packaging & Security

```text
pack   [options]         Create dist/<name>@<version> (+ optional .vixpkg)
verify [options]         Verify dist/<name>@<version> or a .vixpkg artifact
```

---

### Database (ORM)

```text
orm <subcommand>         Migrations / status / rollback
```

---

### Info

```text
help [command]           Show help for CLI or a specific command
version                  Show version information
```

---

## 🌍 Global Options

```text
--verbose                Enable debug logs (equivalent to --log-level debug)
-q, --quiet              Only show warnings and errors
--log-level <level>      trace | debug | info | warn | error | critical
-h, --help               Show CLI help (or: vix help)
-v, --version            Show version info
```

---

## 🔐 Environment Variables

```text
VIX_LOG_LEVEL=level      Default log level (if --log-level not provided)
VIX_MINISIGN_SECKEY=path Secret key used by `vix pack` to sign payload.digest
VIX_MINISIGN_PUBKEY=path Public key used by `vix verify` if --pubkey not provided
```

---

## 🧪 Examples

```bash
vix pack --name blog --version 1.0.0
vix pack --verbose
vix verify --require-signature
vix help verify
```

---

## 🔗 Links

- GitHub: https://github.com/vixcpp/vix

---

## ✨ Notes

- `vix tests` is the preferred way to run project tests.
- `vix run <file.cpp>` supports **script mode** (no project required).
- Designed for clean output, predictable exit codes, and high performance.

---

© Vix.cpp — Modern C++ Runtime
