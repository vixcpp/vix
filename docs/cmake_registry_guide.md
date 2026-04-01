# CMake Principles and Vix Registry Strategy

Deterministic. Composable. Registry-safe.

---

## Overview

Vix uses CMake as the integration layer between packages, local projects, and generated build scripts.

This document explains:

- CMake principles for package authors
- how the Vix registry resolves dependencies safely

Goal:

- packages must be easy to consume
- builds must be deterministic
- registry installs must behave consistently across:
  - vix install
  - vix build
  - vix run

---

## Why this matters

A package in the Vix registry is never built in isolation.

It becomes part of a dependency graph.

Therefore it must be:

- composable
- non-invasive
- predictable
- safe to include

A package that works alone but pollutes CMake globally is NOT registry-safe.

---

## Core Principles

### 1. Packages must be dependency-safe

A package must work in all contexts:

- standalone
- add_subdirectory(...)
- vix_deps.cmake
- vix run generated CMake

Avoid global side effects.

---

### 2. Tests and examples must be OFF by default

```cmake
option(MY_PACKAGE_BUILD_TESTS "Build tests" OFF)
option(MY_PACKAGE_BUILD_EXAMPLES "Build examples" OFF)
```

---

### 3. Example targets must be prefixed

Bad:

```cmake
add_executable(server main.cpp)
```

Good:

```cmake
add_executable(cnerium_server_example_basic main.cpp)
```

---

### 4. Libraries must expose stable target names

```cmake
add_library(cnerium_http INTERFACE)
add_library(cnerium::http ALIAS cnerium_http)
```

---

### 5. Header-only packages must behave like real packages

```cmake
add_library(cnerium_router INTERFACE)
add_library(cnerium::router ALIAS cnerium_router)

target_include_directories(cnerium_router INTERFACE include)
target_link_libraries(cnerium_router INTERFACE cnerium::http)
```

---

### 6. Dependency resolution must be tolerant

```cmake
if(TARGET cnerium::http)
elseif(TARGET cnerium_http)
elseif(TARGET http::http)
elseif(TARGET http)
endif()
```

---

### 7. Packages should fail clearly

```cmake
message(FATAL_ERROR
  "cnerium http target not found.
   Run:
   vix add @cnerium/http
   vix install")
```

---

### 8. Use helper functions

```cmake
function(cnerium_add_example target file)
  add_executable(${target} ${file})
  target_link_libraries(${target} PRIVATE cnerium::server)
endfunction()
```

---

## Vix Registry Strategy

### What vix install does

- reads vix.lock
- resolves dependencies
- builds graph
- sorts packages
- generates `.vix/vix_deps.cmake`

---

### Dependency order

Correct:

json → http → router

---

### Metadata

```json
{
  "dependencies": {
    "cnerium/http": "0.7.0"
  }
}
```

---

### vix_deps.cmake

- loads packages in order
- disables extras
- bridges aliases

---

### Alias bridging

Supports:

- cnerium::http
- cnerium_http
- http::http
- http

---

## Checklist

- tests OFF
- examples OFF
- canonical alias
- no collisions
- clear errors

---

## Rule

A package is ready when it integrates cleanly into another project.

---

## License

MIT License
Copyright (c) Gaspard Kirira

