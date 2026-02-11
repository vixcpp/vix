# Contributing to Vix.cpp

Thank you for your interest in contributing to **Vix.cpp**,  
a modern, high-performance C++ backend framework.

This document explains **how to contribute effectively**, what kinds of contributions are welcome, and the standards expected when submitting changes.

---

## Project Scope & Contribution Areas

We welcome contributions in the following areas:

- Core runtime improvements (HTTP server, routing, middleware, async model)
- Utility modules (logging, validation, JSON, configuration)
- CLI tooling (commands, UX, diagnostics, dev workflow)
- ORM and database integrations
- WebSocket and real-time components
- Documentation, guides, and examples
- Bug fixes, performance improvements, and benchmarks

If you are unsure where to start, look for issues labeled **`good first issue`** or **`devx`**.

---

## Project Philosophy

Vix.cpp aims to bring:

- FastAPI-like developer experience
- Go-like performance characteristics
- C++20/23 correctness and control

All contributions should follow these core principles:

- Clear, readable, modern C++ (C++20 minimum)
- Modular design with minimal coupling
- High performance and predictable memory usage
- Explicit behavior and well-defined ownership
- Concise, meaningful comments where needed

Simplicity and correctness are preferred over cleverness.

---

## Local Development Setup

### 1. Clone the repository

```bash
git clone https://github.com/vixcpp/vix.git
cd vix
git submodule update --init --recursive
```

### 2. Configure and build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
```

To enable examples:

```bash
cmake -S . -B build -DVIX_BUILD_EXAMPLES=ON
```

---

## Running Examples

To verify your setup:

```bash
cd build
./main
```

Test the basic route:

```bash
curl http://localhost:8080/hello
```

Expected output:

```json
{ "message": "Hello, Vix!" }
```

---

## Code Style Guidelines

Vix.cpp follows modern C++ best practices:

- Follow the C++ Core Guidelines
- Prefer explicit types over excessive `auto`
- Use namespaces consistently (`Vix::core`, `Vix::utils`, etc.)
- Avoid global state; use dependency injection or scoped singletons
- All headers must use `#pragma once` or include guards
- Header files should not include unnecessary dependencies

### Example

```cpp
namespace Vix::utils {

class Logger {
public:
    static Logger& instance();
    void info(const std::string& message);

private:
    Logger() = default;
};

} // namespace Vix::utils
```

---

## CMake Conventions

Each module must define its own `CMakeLists.txt`.

```cmake
add_library(vix_core STATIC
    src/server/HTTPServer.cpp
    src/router/Router.cpp
)

target_include_directories(vix_core PUBLIC include)
target_link_libraries(vix_core PUBLIC Boost::asio nlohmann_json::nlohmann_json)
add_library(Vix::core ALIAS vix_core)
```

Rules:

- Use the `Vix::` namespace for all exported targets
- Never use global `include_directories()`
- Prefer `target_*` commands exclusively

---

## Branch Protection and Maintainer Workflow

The `main` branch is protected and does not allow direct pushes.

All changes to `main` must be made through Pull Requests (PRs).

### Maintainer Fix Workflow

When applying fixes or updates, maintainers should follow this workflow:

1. Create a new branch from `main`
2. Apply the fix or change in the new branch
3. Open a Pull Request targeting `main`
4. Review and merge the Pull Request

This ensures code quality, reviewability, and a consistent project history.
---

## Submitting Changes

1. Fork the repository
2. Create a feature branch:

```bash
git checkout -b feat/my-feature
```

3. Make your changes and commit them:

```bash
git commit -m "feat(core): add new router handler"
```

4. Push your branch:

```bash
git push origin feat/my-feature
```

5. Open a Pull Request including:
   - A clear description of the change
   - Motivation and design decisions
   - Linked issues (if applicable)
   - Benchmarks or usage examples when relevant

---

## Testing Guidelines

All new features and fixes must include tests.

- Place unit tests under `tests/` or `unittests/`
- Use GoogleTest for consistency

Run tests with:

```bash
cd build
ctest --output-on-failure
```

Example test:

```cpp
#include <gtest/gtest.h>

TEST(LoggerTest, BasicOutput) {
    auto& logger = Vix::utils::Logger::instance();
    EXPECT_NO_THROW(logger.info("Hello"));
}
```

---

## Performance Considerations

For performance-sensitive changes, run benchmarks before submitting:

```bash
wrk -t8 -c100 -d10s http://localhost:8080/hello
```

Include results when relevant:

- Requests/sec
- Latency (avg / p99)
- Memory impact

---

## Commit Convention

Vix.cpp follows **Conventional Commits**:

| Type      | Description                    |
| --------- | ------------------------------ |
| feat:     | New feature or improvement     |
| fix:      | Bug fix                        |
| refactor: | Internal code refactoring      |
| test:     | Adding or improving tests      |
| docs:     | Documentation updates          |
| chore:    | Maintenance or tooling changes |

Examples:

```bash
feat(core): add middleware chaining support
fix(utils): prevent null pointer in UUID generator
```

---

## Communication

- Use GitHub Issues for bugs and feature proposals
- Use Pull Request discussions for design decisions
- Keep discussions technical, respectful, and focused

---

## License

By contributing to Vix.cpp, you agree that your contributions
will be licensed under the **MIT License**, the same as the main project.

---

Vix.cpp — FastAPI simplicity, Go-like speed, C++ control.
