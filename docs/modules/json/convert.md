# JSON Convert Helpers

`vix/json/convert.hpp` provides safe JSON accessors and converters for Vix.cpp, built on `nlohmann::json`.

It removes repetitive boilerplate like:
- `contains()` + `is_*()` checks everywhere
- try/catch around `get<T>()`
- manual handling of missing keys and out-of-range indexes

It also includes helpers to convert Vix Simple JSON tokens into `nlohmann::json`.

---

## Include

```cpp
#include <vix/json/convert.hpp>
using namespace vix::json;
```

---

## Strictness levels

This header gives you four levels of strictness. Choose based on how much you trust the JSON.

Rule of thumb:
- External/user input: prefer `get_opt()` or `get_or()`
- Internal/trusted data: prefer `ensure()`

### 1) `ptr()`

Returns a pointer to an element, or `nullptr` if missing or invalid.

- `ptr(obj, "key")` -> `const Json*`
- `ptr(arr, idx)` -> `const Json*`

Use this when you want to branch without allocating, copying, or throwing.

### 2) `get_opt<T>()`

Returns `std::optional<T>`. It never throws.

It catches `nlohmann::json::exception` internally and returns `std::nullopt` if conversion fails.

### 3) `get_or<T>()`

Returns a value or a default when missing/invalid.

This is the most convenient option for external input when you have sensible defaults.

### 4) `ensure<T>()`

Strict conversion. It throws when:
- the JSON is not the expected shape
- a key is missing
- the type does not match

Use it when failure should be loud and immediate.

---

## Types

This header uses the same core type alias as other JSON helpers:

```cpp
using Json = nlohmann::json;
```

---

## `ptr()` reference

### `ptr(const Json& j, std::string_view key)`

Returns a pointer to `j[key]` if `j` is an object and the key exists.

```cpp
Json j = R"({"user":{"id":42}})"_json;

if (const Json* p = ptr(j, "user"))
{
  // p points to {"id":42}
}
```

Notes:
- Returns `nullptr` when `j` is not an object
- Returns `nullptr` when key does not exist
- This implementation allocates a temporary `std::string` for the key for compatibility across `nlohmann::json` versions

### `ptr(const Json& j, std::size_t idx)`

Returns a pointer to `j[idx]` if `j` is an array and `idx` is within bounds.

```cpp
Json j = R"([10, 20, 30])"_json;

if (const Json* p = ptr(j, 1))
{
  // *p is 20
}
```

---

## `get_opt<T>()` reference

### Convert a value

```cpp
Json j = R"({"id":42, "name":"Ada"})"_json;

auto id   = get_opt<int>(j["id"]);          // optional<int>(42)
auto name = get_opt<std::string>(j["name"]); // optional<string>("Ada")
```

### Convert from a pointer

```cpp
const Json* p = ptr(j, "id");
auto id = get_opt<int>(p);
```

### Convert an object member by key

```cpp
auto id = get_opt<int>(j, "id");
```

### Convert an array element by index

```cpp
Json arr = R"([1,2,3])"_json;
auto v = get_opt<int>(arr, 2); // 3
```

Behavior:
- returns `std::nullopt` if `j` is null/discarded
- returns `std::nullopt` if `get<T>()` throws

---

## `get_or<T>()` reference

### Convert a value with a default

```cpp
Json j = R"({"debug":"true"})"_json;

bool debug = get_or<bool>(j["debug"], false); // false (type mismatch)
```

### Convert an object member with a default

```cpp
Json j = R"({"user":{"id":42}})"_json;

int id = get_or<int>(j["user"], "id", -1); // 42
int age = get_or<int>(j["user"], "age", 0); // 0 (missing key)
```

### Convert an array element with a default

```cpp
Json xs = R"([10,20])"_json;

int v0 = get_or<int>(xs, 0, -1); // 10
int v2 = get_or<int>(xs, 2, -1); // -1 (out of bounds)
```

---

## `ensure<T>()` reference

### Strict conversion of a value

```cpp
Json j = R"({"id":42})"_json;

int id = ensure<int>(j["id"]); // ok
```

### Strict conversion of an object member

`ensure(obj, key)` provides clearer errors:
- throws if `obj` is not an object
- throws if key is missing
- throws with a message that includes the key on type mismatch

```cpp
Json j = R"({"user":{"id":42}})"_json;

int id = ensure<int>(j["user"], "id"); // ok
```

Example failure cases you should expect:
- `ensure: not an object`
- `ensure: missing key 'id'`
- `ensure: type error for key 'id': ...`

Tip: this is ideal for trusted config files or internal data where you want hard failures.

---

## Converting Vix Simple JSON to nlohmann::json

This header also converts Vix Simple tokens into `nlohmann::json`.

Supported conversions:
- null -> null
- bool -> bool
- int64 -> integer
- double -> float
- string -> string
- array/object -> deep conversion

### `simple_to_json(token|kvs|array_t)`

```cpp
#include <vix/json/Simple.hpp>
#include <vix/json/convert.hpp>

using namespace vix::json;

// Suppose you parsed with the Simple parser:
vix::json::token t = /* ... */;

Json j = simple_to_json(t);
std::cout << j.dump(2) << "\n";
```

### `to_json(...)` convenience overloads

Aliases to keep naming consistent:

```cpp
Json j1 = to_json(t);

vix::json::kvs obj = /* ... */;
Json j2 = to_json(obj);

vix::json::array_t arr = /* ... */;
Json j3 = to_json(arr);
```

---

## Practical patterns

### Parsing external payload safely

```cpp
Json body = /* request json */;

auto id   = get_opt<int>(body, "id");
auto name = get_or<std::string>(body, "name", "anonymous");

if (!id)
{
  // return 400: missing/invalid id
}
```

### Trusted config parsing strictly

```cpp
Json cfg = /* config json */;

std::string host = ensure<std::string>(cfg, "host");
int port         = ensure<int>(cfg, "port");
bool tls         = get_or<bool>(cfg, "tls", false);
```

---

## Summary

`vix/json/convert.hpp` gives you predictable, low-boilerplate JSON access:

- `ptr()` for pointer-style access without throwing
- `get_opt<T>()` for safe optional conversion
- `get_or<T>()` for defaults on missing/invalid data
- `ensure<T>()` for strict conversions with clear errors
- `simple_to_json()` and `to_json()` for converting Vix Simple JSON into `nlohmann::json`

