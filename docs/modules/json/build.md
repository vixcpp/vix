# JSON Build Helpers

`vix/json/build.hpp` provides tiny, header-only helpers to construct JSON values with less boilerplate.

It is built on top of `nlohmann::json` and is meant for:
- API responses
- tests and fixtures
- configuration generation

The main goal is readability without losing practicality.

---

## Include

```cpp
#include <vix/json/build.hpp>
using namespace vix::json;
```

This header only depends on `nlohmann::json`.

---

## Types

### `vix::json::Json`

Alias for the primary JSON type used across Vix.cpp:

```cpp
using Json = nlohmann::json;
```

Use it when you do not care about key ordering.

### `vix::json::OrderedJson`

Alias for a deterministic key-order JSON object:

```cpp
using OrderedJson = nlohmann::ordered_json;
```

This is useful for:
- stable snapshots in tests
- deterministic API output when you want consistent ordering

Note: JSON objects are conceptually unordered, but deterministic order is very useful for logs, fixtures, and tests.

---

## Builders

### `o(k1, v1, k2, v2, ...)` -> `OrderedJson`

Build an ordered JSON object from alternating (key, value) pairs.

Rules:
- Keys must be convertible to `std::string_view`
- Argument count must be even, otherwise you get a compile-time error

Example:

```cpp
auto user = o(
  "id", 42,
  "name", "Gaspard",
  "active", true
);

std::cout << user.dump(2) << "\n";
```

#### Nested example

::: raw
```cpp
auto res = o(
  "status", "ok",
  "data", o(
    "user", o("id", 42, "name", "Gaspard"),
    "skills", a("C++", "Networking", "Systems")
  )
);
```
:::


#### Why ordered

`o()` returns `OrderedJson` to keep output stable across runs. This helps when you:
- compare JSON dumps in tests
- generate fixtures
- log responses and want consistent diffs

---

### `a(v1, v2, v3, ...)` -> `Json`

Build a JSON array from values.

Example:

```cpp
auto xs = a(1, 2, 3, 4);
std::cout << xs.dump() << "\n";
```

#### Nested arrays and objects

```cpp
auto j = o(
  "items", a(
    o("id", 1, "title", "A"),
    o("id", 2, "title", "B")
  )
);
```

---


Build a JSON object from a list of pairs.

This is the helper you use when pairs are not known at compile time.

Example:

```cpp
Json j = kv({
  {"version", Json("1.0.0")},
  {"debug", Json(true)}
});
```

#### Dynamic pairs example

```cpp
#include <vector>

std::vector<std::pair<std::string_view, Json>> pairs;

pairs.push_back({"name", Json("vix")});
pairs.push_back({"stars", Json(213)});

Json out = Json::object();
for (const auto& [k, v] : pairs) out[std::string(k)] = v;

// Or if you already have an initializer_list at the call site:
Json out2 = kv({
  {"name", Json("vix")},
  {"stars", Json(213)}
});
```

Tip: If you can write the object inline, prefer `o()` because it is simpler.

---

## Common mistakes

### 1) Odd number of arguments to `o()`

This fails at compile time:

```cpp
auto bad = o("a", 1, "b"); // missing value
```

You will get an error similar to:

- `json::o requires an even number of args: (k1,v1,k2,v2,...)`

Fix:

```cpp
auto ok = o("a", 1, "b", 2);
```

### 2) Using `kv({{"a", 1}})`

This is invalid because `kv()` requires the value type to be `Json`:

```cpp
auto bad = kv({{"a", 1}}); // wrong
```

Fix:

```cpp
auto ok1 = kv({{"a", Json(1)}});
auto ok2 = o("a", 1); // usually the best
```

### 3) Keys that are not key-like

Keys passed to `o()` must be convertible to `std::string_view`.

Good:
- string literals `"id"`
- `std::string`
- `std::string_view`

Bad:
- integers
- custom types without conversion

---

## Patterns you can copy

### API response envelope

```cpp
OrderedJson ok(Json data)
{
  return o(
    "status", "ok",
    "data", std::move(data)
  );
}

OrderedJson err(int code, std::string_view message)
{
  return o(
    "status", "error",
    "error", o(
      "code", code,
      "message", message
    )
  );
}
```

### Pagination response

```cpp
OrderedJson page(Json items, int page, int limit, int total)
{
  return o(
    "status", "ok",
    "data", o(
      "items", std::move(items),
      "page", page,
      "limit", limit,
      "total", total
    )
  );
}
```

---

## Notes on conversions

`nlohmann::json` can serialize many common types automatically. For custom types, prefer explicit conversion.

A common pattern is to write a function that returns `Json`:

```cpp
struct User
{
  int id;
  std::string name;
};

Json to_json_value(const User& u)
{
  return o("id", u.id, "name", u.name);
}
```

---

## When to use which helper

Use `o()` when:
- you can write pairs inline
- you want stable key order
- you want the cleanest syntax

Use `a()` when:
- you need arrays with minimal boilerplate

Use `kv()` when:
- keys and values are produced dynamically
- you already have pairs as data

---

## Summary

`vix/json/build.hpp` keeps JSON construction short and readable:

- `o(...)` builds ordered objects
- `a(...)` builds arrays
- `kv(...)` builds objects from (key, Json) pairs

It is ideal for API responses, tests, and fixtures.

