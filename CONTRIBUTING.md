# 🧭 Contributing to Vix.cpp

Thank you for your interest in contributing to **Vix.cpp** —  
a modern, high-performance C++ backend framework.

We welcome contributions of all kinds:

- 🧱 Core improvements (routing, HTTP, middleware, etc.)
- ⚙️ Utility modules (logging, validation, JSON, etc.)
- 🧩 Examples and documentation
- 🐛 Bug reports and performance tuning

---

## 🧠 Philosophy

Vix.cpp aims to bring **FastAPI-like developer experience** and **Go-like speed**  
to the C++ world — without sacrificing simplicity or readability.

Contributions should always follow these principles:

- 🧼 Clear and readable C++20 code
- 🧩 Modular, header-only where possible
- ⚡ High performance and low memory overhead
- 🧠 Self-documenting with concise comments

---

## ⚙️ Setup for Local Development

### 1. Clone the repository

```bash
git clone https://github.com/vixcpp/vix.git
cd vix
git submodule update --init --recursive
```

2. Build with CMake

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
```

You can enable examples for quick testing:

```bash
cmake -S . -B build -DVIX_BUILD_EXAMPLES=ON
```

## 🧪 Running Examples

To test your environment:

```bash
cd build
./main
```

##### Test the basic route:

```bash
curl http://localhost:8080/hello
```

### Expected output:

```json
{ "message": "Hello, Vix!" }
```

# 🧩 Code Style Guidelines

Follow C++ Core Guidelines and modern C++20 conventions.
Prefer auto only when the type is obvious.
Use namespaces to organize code (Vix::core, Vix::utils, etc.).
Avoid global state (use singletons or dependency injection when necessary).
Each .hpp file should have proper include guards or #pragma once.

## Example

```cpp
namespace Vix::utils {
class Logger {
public:
    static Logger& getInstance();
    void info(const std::string& msg);
private:
    Logger() = default;
};
} // namespace Vix::utils
```

# 🧱 CMake Conventions

Each module must define its own CMakeLists.txt:

```cmake
add_library(vix_core STATIC
    src/server/HTTPServer.cpp
    src/router/Router.cpp
)

target_include_directories(vix_core PUBLIC include)
target_link_libraries(vix_core PUBLIC Boost::asio nlohmann_json::nlohmann_json)
add_library(Vix::core ALIAS vix_core)
```

Use the Vix:: namespace for all exported targets.
Never use global include_directories() — use target_include_directories().

# 🧩 Submitting Changes

1. Fork the repository
2. Create a feature branch:

```bash
git checkout -b feat/my-awesome-feature
```

3. Make your changes and commit them:

```bash
git commit -m "feat(core): add new router handler"
```

4. Push your branch:

```bash
git push origin feat/my-awesome-feature
```

5. Open a Pull Request (PR) with:

. A clear description of your change
. Linked issues (if any)
. Example usage or benchmark results

# 🧪 Testing Guidelines

All modules must include basic unit tests under /tests or /unittests.

```bash
cd build
ctest --output-on-failure
```

Use GoogleTest for consistency:

```cpp
#include <gtest/gtest.h>

TEST(LoggerTest, BasicOutput) {
    auto& log = Vix::Logger::getInstance();
    EXPECT_NO_THROW(log.info("Hello"));
}
```

# ⚡ Performance Benchmarks

Before merging significant changes, run:

```bash
wrk -t8 -c100 -d10s http://localhost:8080/hello
```

Record your results (Requests/sec, Latency, Memory).

```markdown
## 🧾 Commit Convention

Suivez le format [Conventional Commits](https://www.conventionalcommits.org/) :

| Type        | Description                |
| ----------- | -------------------------- |
| `feat:`     | New feature or improvement |
| `fix:`      | Bug fix                    |
| `refactor:` | Code refactoring           |
| `test:`     | Adding or improving tests  |
| `docs:`     | Documentation updates      |
| `chore:`    | Maintenance tasks          |
```

# Example:

```bash
feat(core): add middleware chaining support
fix(utils): prevent null pointer in UUID generator
```

# 💬 Communication

Open issues for bugs or ideas.
Discuss architecture in PR comments or GitHub Discussions.
Be respectful and concise.

# 🪪 License

By contributing to Vix.cpp, you agree that your contributions
will be licensed under the same MIT License as the main project.

---

Vix.cpp — FastAPI simplicity, Go-like speed, C++ control.
