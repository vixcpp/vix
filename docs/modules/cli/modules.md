# Vix Modules

The `vix modules` command enables a strict, app-first module system for any CMake project.

The design is inspired by Go modules philosophy:
- Explicit dependencies
- Strict public/private boundaries
- No accidental header leakage
- Enforced architectural discipline

---

## Command Overview

```bash
vix modules <subcommand> [options]
```

### Subcommands

```text
init                 Initialize modules mode
add <name>           Create a module skeleton
check                Validate module safety rules
```

### Global Options

```text
-d, --dir <path>         Project root (default: current)
--project <name>         Override project name
--no-patch               Do not patch root CMakeLists.txt
--patch                  Patch root CMakeLists.txt (default)
--no-link                Do not auto-link module into main target
--link                   Auto-link module into main target (default)
-h, --help               Show help
```

---

# 1. Initialize Modules Mode

```bash
vix modules init
```

This creates:

```text
modules/
cmake/vix_modules.cmake
```

It can optionally patch your root `CMakeLists.txt`.

Example root patch:

```cmake
include(cmake/vix_modules.cmake)
vix_modules_enable()
```

---

# 2. Add a Module

```bash
vix modules add auth
```

Creates:

```text
modules/auth/
  include/auth/
  src/
  CMakeLists.txt
```

Public header example:

```cpp
// modules/auth/include/auth/api.hpp
#pragma once

namespace auth {
  void login();
}
```

Private implementation:

```cpp
// modules/auth/src/api.cpp
#include <auth/api.hpp>

namespace auth {
  void login() {}
}
```

Generated CMake target:

```cmake
add_library(<project>_auth)
add_library(<project>::auth ALIAS <project>_auth)

target_include_directories(<project>_auth
  PUBLIC modules/auth/include
  PRIVATE modules/auth/src
)
```

---

# 3. Explicit Cross-Module Dependency

If `products` depends on `auth`, you must declare it explicitly:

```cmake
target_link_libraries(<project>_products
  PUBLIC <project>::auth
)
```

No implicit dependency resolution is allowed.

---

# 4. Module Structure Contract

Each module must follow:

```text
modules/<m>/include/<m>/...   public headers
modules/<m>/src/...           private implementation
```

Public include style:

```cpp
#include <auth/api.hpp>
```

Never:

```cpp
#include "modules/auth/src/internal.hpp"
```

---

# 5. Validate Module Safety

```bash
vix modules check
```

This validates:

- Public headers do not include private headers
- Cross-module usage is explicitly linked
- Include structure follows contract
- No accidental boundary violations

---

# Design Philosophy

Vix modules enforce:

- Architectural clarity
- Dependency correctness
- Long-term maintainability
- Build-time safety

Everything must be explicit.
Nothing is automatic.
No hidden coupling.

---

# Minimal Workflow Example

```bash
vix modules init
vix modules add auth
vix modules add products
vix modules check
```

This results in a clean, explicit, Go-like module architecture for C++.

