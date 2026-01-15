# 🧩 Vix.cpp — JSON Module

![C++](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
![License](https://img.shields.io/badge/License-MIT-green)
![Status](https://img.shields.io/badge/Status-Stable-success)
![JSON](https://img.shields.io/badge/JSON-nlohmann%2Fjson-orange)

> **vix.cpp/json** — A high-level JSON utility library built on top of [nlohmann/json](https://github.com/nlohmann/json).
> Provides expressive, concise, and safe helpers for working with JSON in C++.

---

## 🚀 Overview

The **Vix JSON module** offers a lightweight abstraction over `nlohmann::json`,
designed for simplicity and expressiveness. It adds helpers for:

- Fast JSON object/array construction (`o()`, `a()`, `kv()`)
- Human-readable dumps and safe file I/O (`dumps()`, `dump_file()`, `load_file()`)
- Path-based access and mutation (`jget()`, `jset()`, with dot/array syntax)

---

## ⚙️ Features

- 🧱 **Concise Builders** — `o()` for objects, `a()` for arrays, `kv()` for key-value initialization.
- 💾 **Safe File Operations** — Atomic writes with `dump_file()` using temporary `.tmp` suffix.
- 🧭 **JPath Navigation** — Access nested elements via `"user.profile.name"` or `"user.langs[2]"`.
- 🧩 **nlohmann/json Compatible** — Full interoperability with standard `json` objects.
- 🧠 **Readable Dumps** — `dumps(obj, 2)` for formatted pretty-printing.

---

## 🧩 Quick Start Example

```cpp
#include <vix/json/json.hpp>
#include <iostream>

using namespace vix::json;

int main() {
    auto user = o(
        "id", 42,
        "name", "Ada",
        "tags", a("pro", "admin")
    );

    std::cout << dumps(user, 2) << "\n";
}
```

**Output**

```json
{
  "id": 42,
  "name": "Ada",
  "tags": ["pro", "admin"]
}
```

---

## 🧱 JSON Builders

```cpp
#include <vix/json/json.hpp>
#include <iostream>
using namespace vix::json;

int main()
{
    Json obj1 = o("id", 1, "name", "Vix", "active", true);
    Json arr1 = a(1, 2, 3, 4);

    Json obj2 = kv({{"key1", "val1"}, {"key2", 2}});

    std::cout << dumps(obj1, 2) << "\n"
              << dumps(arr1, 2) << std::endl;
}
```

**Example Output**

```json
{
  "id": 1,
  "name": "Vix",
  "active": true
}
[1, 2, 3, 4]
```

---

## 💾 JSON File Operations

```cpp
#include <vix/json/json.hpp>
#include <iostream>
using namespace vix::json;

int main()
{
    Json j = loads(R"({"a":1,"b":[10,20]})");
    dump_file("out.json", j, 2);
    Json j2 = load_file("out.json");
    std::cout << dumps(j2, 2) << std::endl;
}
```
**Output**

```json
{
  "a": 1,
  "b": [
    10,
    20
  ]
}
```

**Features**

- `dump_file()` performs an atomic write (writes to `.tmp` then renames).
- `load_file()` safely loads JSON and throws on syntax errors.
- Ideal for config, caching, or persistence layers.

---

## 🧭 JPath Access

```cpp
#include <vix/json/json.hpp>
#include <iostream>

using namespace vix::json;

int main()
{
    Json j = obj();
    jset(j, "user.langs[2]", "cpp");
    jset(j, "user.profile.name", "Gaspard");
    jset(j, R"(user["display.name"])", "Ada L.");

    if (auto v = jget(j, "user.langs[2]"))
    {
        std::cout << v->get<std::string>() << "\n";
    }
    std::cout << dumps(j, 2) << "\n";
}
```

**Output**

```json
cpp
{
  "user": {
    "display.name": "Ada L.",
    "langs": [null, null, "cpp"],
    "profile": { "name": "Gaspard" }
  }
}
```

---

## 🧪 Demos Included

| File                        | Description                                        |
| --------------------------- | -------------------------------------------------- |
| `json_demo.cpp`             | Basic usage of `o()`, `a()`, `kv()`, and `dumps()` |
| `json_build_demo.cpp`       | Building structured objects and arrays             |
| `json_loads_dumps_demo.cpp` | Safe JSON parsing, writing, and reloading          |
| `json_jpath_demo.cpp`       | JSON path navigation (`jget`, `jset`)              |

---

## 🧰 Build & Run

```bash
git clone https://github.com/vixcpp/json.git
cd json
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/json_demo
```

---

## 🧩 Integration with Vix.cpp

This module is automatically included when you build the umbrella **Vix.cpp** project.

```cpp
#include <vix/json/json.hpp>
using namespace vix::json;

auto j = o("framework", "Vix.cpp", "version", "1.7.0");
```
**Output**

```json
{
  "framework": "Vix.cpp",
  "version": "1.7.0",
}
```

---

## 🧾 License

**MIT License** © [Gaspard Kirira](https://github.com/gkirira)
See [LICENSE](../../LICENSE) for details.
