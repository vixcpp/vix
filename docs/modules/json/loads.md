# JSON Loads Guide

`vix/json/loads.hpp` provides simple and explicit JSON parsing helpers for Vix.cpp.

It is built on top of `nlohmann::json` and offers two clear styles:

- Throwing versions
- Safe optional versions

You always choose which behavior you want.

---

## Include

```cpp
#include <vix/json/loads.hpp>
using namespace vix::json;
```

---

# Overview

The API is intentionally small.

## From strings

- `loads()`
  Parse JSON and throw on error.

- `try_loads()`
  Parse JSON and return `std::nullopt` on failure.

## From files

- `load_file()`
  Load + parse JSON file, throw on error.

- `try_load_file()`
  Safe version returning `std::nullopt`.

---

# From strings

## `loads(std::string_view)`

Throwing version.

```cpp
Json j = loads(R"({"id": 1, "name": "Softadastra"})");
```

Behavior:
- Throws `nlohmann::json::parse_error` if input is invalid.

Use this when:
- invalid JSON is a programming error
- tests use static trusted JSON
- config must be valid

---

## `try_loads(std::string_view)`

Safe version. Never throws.

```cpp
if (auto maybe = try_loads(R"({"ok": true})"))
{
  std::cout << maybe->dump(2) << "\n";
}
else
{
  std::cout << "Invalid JSON\n";
}
```

Behavior:
- Returns `std::nullopt` on any failure.
- Catches all exceptions internally.

Use this when:
- parsing user input
- parsing external API payloads
- handling optional JSON fields

---

# From files

## `load_file(const fs::path&)`

Throwing version.

```cpp
Json cfg = load_file("config.json");
```

Behavior:
- Throws `std::runtime_error` if:
  - file cannot be opened
  - file is empty
- Throws `nlohmann::json::parse_error` if JSON is invalid.

Important:
- Entire file is read into memory.
- Intended for config and metadata files.
- Not for unbounded uploads.

---

## `try_load_file(const fs::path&)`

Safe version.

```cpp
if (auto cfg = try_load_file("optional.json"))
{
  std::cout << "Loaded config\n";
}
else
{
  std::cout << "Missing or invalid config\n";
}
```

Behavior:
- Never throws.
- Returns `std::nullopt` on any failure.

---

# Error model

Throwing functions propagate:

- `std::runtime_error` for I/O errors
- `nlohmann::json::parse_error` for invalid JSON

Safe versions:

- Catch everything
- Return `std::nullopt`

This keeps error handling explicit.

---

# When to use what?

| Situation | Recommended |
|-----------|------------|
| Required config file | `load_file()` |
| Optional config file | `try_load_file()` |
| User input | `try_loads()` |
| Tests / trusted literals | `loads()` |

---

# Examples

## Parse JSON from string

```cpp
Json j = loads(R"({"user": {"id": 42}})");

int id = j["user"]["id"];
```

## Safe parse user input

```cpp
std::string input = get_user_payload();

if (auto maybe = try_loads(input))
{
  // process JSON
}
else
{
  // return 400 Bad Request
}
```

## Load configuration

```cpp
try
{
  Json cfg = load_file("config/app.json");
}
catch (const std::exception& e)
{
  std::cerr << "Failed to load config: " << e.what() << "\n";
}
```

## Optional settings file

```cpp
if (auto s = try_load_file("settings.json"))
{
  std::cout << "Settings loaded\n";
}
else
{
  std::cout << "Using defaults\n";
}
```

---

# Convenience overloads

All helpers also support C-string overloads:

```cpp
Json j = loads("{"a":1}");

auto maybe = try_loads("{"a":1}");

Json cfg = load_file("config.json");

auto maybe_cfg = try_load_file("config.json");
```

---

# Design philosophy

This header avoids hidden behavior.

You always choose:

- strict (throwing)
- safe (optional)

There is no implicit global error mode.

This keeps Vix predictable and explicit.

---

# Summary

`vix/json/loads.hpp` provides:

- `loads()` for strict string parsing
- `try_loads()` for safe string parsing
- `load_file()` for strict file parsing
- `try_load_file()` for safe file parsing

It keeps JSON loading simple, explicit, and safe by design.

