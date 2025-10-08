# 🧪 Vix.cpp — Security Testing Guide

This document describes how to **audit, test, and validate** the security of **Vix.cpp** and its modules.  
It is primarily intended for **maintainers** and **core contributors**.

---

## 🧭 Overview

Vix.cpp includes several layers of protection:

| Layer                   | Description                                                             |
| ----------------------- | ----------------------------------------------------------------------- |
| 🔒 **Memory safety**    | Use of smart pointers, RAII, and optional sanitizers                    |
| 🧰 **Input validation** | Validation utilities under `Vix::utils::Validation`                     |
| 🌐 **HTTP safety**      | Controlled header parsing and path sanitization                         |
| ⚙️ **Runtime checks**   | Optional AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan) |
| 🧠 **Static analysis**  | Support for clang-tidy, cppcheck, and Coverity                          |

Maintainers are encouraged to periodically **run full sanitizer builds** and **fuzz endpoints** before major releases.

---

## 🧩 1. Sanitizer Testing (ASan + UBSan)

Sanitizers detect memory leaks, invalid reads/writes, and undefined behavior at runtime.

### 🔧 Build with Sanitizers

```bash
cmake -S . -B build-sanitize \
  -DCMAKE_BUILD_TYPE=Debug \
  -DVIX_ENABLE_SANITIZERS=ON
cmake --build build-sanitize -j
```

# ▶️ Run examples with ASan/UBSan enabled

```bash
cd build-sanitize
ASAN_OPTIONS=detect_leaks=1 ./main
```

# Typical output (no errors):

```sql
==12345== All heap blocks were freed -- no leaks are possible
```

. 💡 Tip: Use -fsanitize=address,undefined in your own modules for deep inspection.

# 🔬 2. Fuzz Testing

Fuzz testing helps uncover unexpected behavior under random or malformed input.

## ⚙️ Install dependencies (Linux/macOS)

```bash
sudo apt install clang llvm
```

## 🧠 Run a simple fuzz test

Create tests/fuzz_http.cpp:

```cpp
#include <vix/core.h>
#include <string>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    try {
        std::string input(reinterpret_cast<const char*>(data), size);
        // Simulate route parsing
        Vix::Router r;
        r.add("/users/{id}", [](auto&, auto&, auto&){});
        r.match(input);
    } catch (...) {
        // ignore malformed input
    }
    return 0;
}
```

Build and run:

```bash
clang++ -g -fsanitize=fuzzer,address,undefined \
  -Iinclude -o fuzz_http tests/fuzz_http.cpp
./fuzz_http
```

. Fuzzer will mutate inputs and test your route-matching logic for memory or logic errors.

# 🧱 3. Static Analysis

## 🔍 Run clang-tidy

```bash
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -B build
clang-tidy -p build $(find modules -name '*.cpp')
```

## 🧮 Run cppcheck

```bash
cppcheck --enable=all --std=c++20 --inconclusive --quiet modules/
```

## 🧠 Optional (Advanced): Coverity Scan

If integrated with Coverity:

```bash
cov-build --dir cov-int cmake --build build
tar czvf vix-cov.tgz cov-int
```

## 🔐 4. Dependency Audit

📦 Check system libraries

```bash
ldd build/vix_cli | grep -E 'boost|ssl|crypto'
```

## 🧩 Update dependencies

Ensure system dependencies are up to date:

```bash
sudo apt update && sudo apt upgrade libboost-all-dev nlohmann-json3-dev
```

## ⚙️ Verify header integrity

```bash
sha256sum modules/json/include/vix/json/json.hpp
```

# 🧰 5. Security Unit Tests

Write test cases for:
. Request parameter sanitization

. Validation rules (regex injection, XSS, overflow)

. JSON parser boundaries

## Example:

```cpp
#include <gtest/gtest.h>
#include <vix/utils/Validation.hpp>

using namespace Vix::utils;

TEST(ValidationTest, RejectsInvalidEmail) {
    Schema sch{{"email", match(R"(^[^@\s]+@[^@\s]+\.[^@\s]+$)")}};
    auto data = std::unordered_map<std::string, std::string>{{"email", "not-an-email"}};
    auto res = validate_map(data, sch);
    EXPECT_TRUE(res.is_err());
}

```

# Run:

```bash
ctest --output-on-failure
```

# 🧪 6. Runtime Hardening Flags

Always compile releases with secure flags:

```bash
-ffunction-sections -fdata-sections -fstack-protector-strong -D_FORTIFY_SOURCE=2
```

On Linux:

```bash
cmake -DCMAKE_CXX_FLAGS="-O2 -fstack-protector-strong -D_FORTIFY_SOURCE=2" -B build-rel
```

# ⚡ 7. Optional: Valgrind Leak Check

```bash
valgrind --leak-check=full --show-leak-kinds=all ./build/main
```

Example clean output:

```yaml
HEAP SUMMARY:
  definitely lost: 0 bytes in 0 blocks
  indirectly lost: 0 bytes in 0 blocks
```

```markdown
## 📊 8. Recommended Frequency

Fréquences recommandées pour les différents types de tests et audits :

| **Test Type**    | **Recommended Frequency**        |
| ---------------- | -------------------------------- |
| Unit tests       | Every commit / PR                |
| Sanitizers       | Before each release              |
| Fuzz tests       | Weekly or before large refactors |
| Static analysis  | Monthly or automated via CI      |
| Dependency audit | Quarterly                        |
```

```markdown
## 🧾 Summary

Résumé des outils recommandés pour assurer la qualité du code :

| **Tool**    | **Purpose**             | **Command**                         |
| ----------- | ----------------------- | ----------------------------------- |
| ASan/UBSan  | Detect memory/UB errors | `-DVIX_ENABLE_SANITIZERS=ON`        |
| Clang-Tidy  | Static analysis         | `clang-tidy -p build ...`           |
| Cppcheck    | Light static check      | `cppcheck --enable=all modules/`    |
| Valgrind    | Leak detection          | `valgrind --leak-check=full ./main` |
| LLVM Fuzzer | Fuzzing                 | `clang++ -fsanitize=fuzzer,address` |
```

🧠 Goal:
Every release of Vix.cpp must pass sanitizer, fuzzing, and static analysis tests
to ensure long-term stability and security of the framework.

Maintained by the Vix.cpp Security Team
📧 mailto:vixcpp.security@gmail.com
