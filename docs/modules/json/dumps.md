# JSON Dumps Helpers

`vix/json/dumps.hpp` provides serialization helpers for Vix.cpp.

It covers two main use cases:

1) Serialize JSON to a string
2) Write JSON safely to disk

It is built on top of `nlohmann::json` and focuses on readability and safety.

---

## Include

```cpp
#include <vix/json/dumps.hpp>
using namespace vix::json;
```

---

## Types

```cpp
using Json = nlohmann::json;
namespace fs = std::filesystem;
```

---

# 1. Serialize JSON to string

## `dumps()`

Pretty-print JSON (multi-line, indented).

```cpp
Json j = {
  {"app", "Vix.cpp"},
  {"debug", true}
};

std::string s = dumps(j);
std::cout << s << "\n";
```

Default behavior:
- indent = 2 spaces
- ensure_ascii = false

### Custom indentation

```cpp
std::string s = dumps(j, 4); // 4 spaces
```

### Escape non-ASCII

```cpp
std::string s = dumps(j, 2, true);
```

Use this for:
- logs
- config files
- debugging output
- readable API responses

---

## `dumps_pretty()`

Explicit alias for `dumps()`.

```cpp
std::string s = dumps_pretty(j);
```

This exists to make intent clear in code.

---

## `dumps_compact()`

Compact single-line JSON.

```cpp
std::string s = dumps_compact(j);
```

Produces:

```json
{"app":"Vix.cpp","debug":true}
```

Use this for:
- network payloads
- compact storage
- performance-sensitive logs

---

# 2. Write JSON safely to disk

## `dump_file(path, json)`

Writes JSON using a temp-file + rename strategy.

```cpp
Json j = {
  {"app", "Vix.cpp"},
  {"debug", true}
};

dump_file("config.json", j);
```

---

## What makes it safe?

The function does:

1. Creates parent directories (best-effort)
2. Writes JSON to `config.json.tmp`
3. Renames `.tmp` to `config.json`
4. If rename fails, falls back to copy + remove

This reduces the risk of partially-written files if:
- the process crashes
- the machine shuts down
- an exception occurs mid-write

Important:
- Rename is atomic on most local filesystems when done within the same directory.
- Atomic behavior depends on filesystem semantics.

---

## Custom formatting

You can control indentation and ASCII escaping:

```cpp
dump_file("config.json", j, 4);        // 4-space indent
dump_file("config.json", j, 2, true);  // escape non-ASCII
```

---

## Overloads

Convenience overloads exist for:

```cpp
dump_file("file.json", j);
dump_file(std::string("file.json"), j);
dump_file(std::filesystem::path("file.json"), j);
```

---

# Error handling

`dump_file()` throws `std::runtime_error` if:

- temp file cannot be opened
- write fails
- final replacement fails

Example:

```cpp
try
{
  dump_file("config.json", j);
}
catch (const std::exception& e)
{
  std::cerr << "Failed to write JSON: " << e.what() << "\n";
}
```

---

# Practical patterns

## Writing application config

```cpp
Json cfg = {
  {"host", "127.0.0.1"},
  {"port", 8080},
  {"tls", false}
};

dump_file("config/app.json", cfg);
```

## Logging a JSON response

```cpp
Json res = {
  {"status", "ok"},
  {"count", 42}
};

std::cout << dumps(res) << "\n";
```

## Sending compact JSON over network

```cpp
std::string payload = dumps_compact(res);
// send(payload)
```

---

# When to use which

Use `dumps()` when:
- humans will read the output
- you store configuration files
- you want clean logs

Use `dumps_compact()` when:
- sending over network
- minimizing size
- performance matters

Use `dump_file()` when:
- writing config files
- persisting state
- generating fixtures safely

---

# Summary

`vix/json/dumps.hpp` provides:

- `dumps()` for pretty JSON
- `dumps_pretty()` explicit alias
- `dumps_compact()` for single-line output
- `dump_file()` for safe disk writes

It keeps serialization simple, readable, and safer by default.

