# CLI Overview

The Vix CLI is the central interface for building, running, packaging, and managing Vix projects.

It is designed to be:

- Explicit
- Predictable
- Modular
- Production-oriented

---

## Version Example

```bash
vix -h
```

Example output:

```
Vix.cpp — Modern C++ backend runtime
Version: v1.x.x
```

---

## Core Philosophy

The CLI is structured around:

- Project lifecycle
- Dependency management
- Packaging and security
- Network runtime
- Database migrations
- Developer tooling

Everything is explicit.
No hidden background behavior.

---

# Command Categories

## 1. Project

```
vix new <name>
vix build
vix run
vix dev
vix check
vix tests
vix repl
```

Used to create and manage applications.

---

## 2. Project Structure (Modules)

```
vix modules <subcommand>
```

Opt-in modular system for adding and validating Vix modules.

---

## 3. Network

```
vix p2p
```

Run a P2P node with TCP transport and discovery.

---

## 4. Registry

```
vix registry
vix add <pkg>@<version>
vix search <query>
vix remove <pkg>
vix list
vix store
vix publish <version>
vix deps
```

Manages dependencies via a git-based registry model.

Fully offline search supported.

---

## 5. Packaging & Security

```
vix pack
vix verify
vix install
```

Secure artifact generation and verification:

- SHA256
- Minisign support
- Versioned distributions

---

## 6. Database (ORM)

```
vix orm <subcommand>
```

Manage migrations, status, rollback, and schema operations.

---

## 7. Info

```
vix help
vix version
```

Show help and version information.

---

# Global Options

```
--verbose
--quiet
--log-level <level>
-h / --help
-v / --version
```

Supported log levels:

```
trace | debug | info | warn | error | critical
```

---

# Quick Start Example

```bash
vix new api
cd api
vix dev
```

Package and verify:

```bash
vix pack --version 1.0.0
vix verify
```

---

# Design Goals

- Zero hidden dependency resolution
- Reproducible builds
- Explicit version pinning
- Secure artifact validation
- Clean project structure
- Developer-first experience

---

The CLI is the backbone of Vix workflows.

It connects development, packaging, registry, networking, and runtime in one consistent interface.

