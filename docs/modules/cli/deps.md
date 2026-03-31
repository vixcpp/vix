# Dependencies

The `vix deps` command installs project dependencies defined in `vix.lock`
into a local directory and generates a CMake include file.

This system is:

- Deterministic (lockfile-based)
- Local (no global state)
- CMake-native
- Explicit

---

## Command

```bash
vix deps
```

---

## What it does

When executed, Vix:

1. Reads `vix.lock`
2. Downloads required packages
3. Installs them into:

```text
./.vix/deps
```

4. Generates:

```text
./.vix/vix_deps.cmake
```

You must include this file in your root `CMakeLists.txt`.

---

## Typical workflow

Add dependencies:

```bash
vix add gaspardkirira/tree@0.4.0
vix add gaspardkirira/binary_search@0.1.1
```

Install them:

```bash
vix deps
```

---

## CMake integration

In your root `CMakeLists.txt`:

```cmake
include(${CMAKE_SOURCE_DIR}/.vix/vix_deps.cmake)
```

This makes dependency targets available to your project.

Example usage:

```cmake
target_link_libraries(my_app
  PRIVATE
    tree
    binary_search
)
```

---

## vix.lock

The `vix.lock` file ensures:

- Exact version resolution
- Reproducible builds
- No surprise upgrades
- Same dependency graph across machines

Never edit this file manually.

---

## Design principles

- No global install directory
- No implicit include paths
- No hidden transitive linkage
- Fully explicit linking in CMake

This keeps builds predictable and safe.

---

## Summary

`vix deps` is the install step of the Vix dependency system.

It transforms:

```text
vix.lock
```

into:

```text
./.vix/deps
./.vix/vix_deps.cmake
```

Simple. Deterministic. CMake-native.

