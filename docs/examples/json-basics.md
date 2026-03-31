# Simple JSON Model Guide

`vix/json/Simple.hpp` defines a minimal JSON-like value model for lightweight internal Vix APIs.

It is:
- header-only
- dependency-free (does not depend on `nlohmann::json`)
- designed for structured data exchange between modules without parsing text

If you want to parse or serialize JSON text, use the regular Vix JSON helpers based on `nlohmann::json`.

---

## Include

```cpp
#include <vix/json/Simple.hpp>
using namespace vix::json;
```

---

## What is Simple?

Simple is a tiny DOM-like value system that can represent:

- null
- bool
- integer (stored as `int64`)
- floating point (stored as `double`)
- string
- arrays (`array_t`)
- objects (`kvs`)

It is meant for:
- internal payloads between modules
- in-memory structured data
- building small trees of values for adapters and bridges

It is not meant for:
- large untrusted JSON text parsing
- full JSON schema validation
- streaming or incremental parsing

---

## Core types

### `token`

A `token` is a tagged variant that stores either:
- a primitive value, or
- a `shared_ptr<array_t>` / `shared_ptr<kvs>` for recursion

Internally:

```cpp
using value_t = std::variant<
  std::monostate,           // null
  bool,                     // boolean
  std::int64_t,             // integer
  double,                   // floating point
  std::string,              // string
  std::shared_ptr<array_t>, // array
  std::shared_ptr<kvs>      // object
>;
```

Default token is null.

### `array_t`

A JSON-like array:

- stored as `std::vector<token>`
- provides convenience methods (`push_*`, `ensure(idx)`, `erase_at`, etc.)

### `kvs`

A JSON-like object.

Important design choice:
- objects are stored as a flat vector: `key0, value0, key1, value1, ...`
- keys are typically string tokens
- this layout keeps the model minimal and predictable

---

## Quick example

```cpp
using namespace vix::json;

kvs user = obj({
  "name", "Alice",
  "age", 30,
  "skills", array({"C++", "Networking"})
});

token root = user;

root.ensure_object().set_string("country", "UG");
```

---

## Creating values

### Null

```cpp
token a;
token b = nullptr;
```

### Bool

```cpp
token t = true;
```

### Integers

All integral types (except bool) are accepted and stored as `int64`.

```cpp
token a = 42;
token b = std::int64_t(42);
```

### Floating point

```cpp
token t = 3.14;
```

### Strings

```cpp
token a = "hello";
token b = std::string("hello");
```

### Arrays

Using initializer list:

```cpp
array_t xs = array({1, 2, 3, "hi"});
token t = xs;
```

Using vector:

```cpp
std::vector<token> v = {1, 2, 3};
array_t xs = array(v);
array_t ys = array(std::move(v));
```

### Objects

Objects are built with `obj({ ... })` where the initializer list is flattened key/value order:

```cpp
kvs cfg = obj({
  "host", "127.0.0.1",
  "port", 8080,
  "tls", false
});

token t = cfg;
```

Important:
- the initializer list is not nested pairs
- it is: key, value, key, value, ...

---

## Reading values

### Type checks

```cpp
token t = 42;

t.is_null();
t.is_bool();
t.is_i64();
t.is_f64();
t.is_string();
t.is_array();
t.is_object();
```

### Raw getters (pointer or nullptr)

```cpp
if (const std::int64_t* x = t.as_i64())
{
  // *x is available
}
```

Available raw getters:
- `as_bool()`
- `as_i64()`
- `as_f64()`
- `as_string()`
- `as_array_ptr()`
- `as_object_ptr()`

### Convenience getters with default

```cpp
bool ok = t.as_bool_or(false);
std::int64_t n = t.as_i64_or(0);
double d = t.as_f64_or(0.0);
std::string s = t.as_string_or("default");
```

---

## Mutating values

### Setters

```cpp
token t;

t.set_null();
t.set_bool(true);
t.set_i64(123);
t.set_int(123);
t.set_ll(123LL);
t.set_ull(123ULL);
t.set_f64(3.14);
t.set_string("hi");
t.set_cstr("hi");
```

### Ensure array/object

These helpers replace the token with an empty array/object if needed, then return a mutable reference.

```cpp
token t;

// becomes array if not already
array_t& xs = t.ensure_array();
xs.push_int(1);
xs.push_string("two");

// becomes object if not already
kvs& o = t.ensure_object();
o.set_string("name", "Ada");
o.set_int("age", 42);
```

---

## `array_t` guide

### Append elements

```cpp
array_t xs;

xs.push_null();
xs.push_bool(true);
xs.push_int(1);
xs.push_i64(2);
xs.push_f64(3.14);
xs.push_cstr("hi");
xs.push_string("hello");
xs.push_back(obj({"k", "v"})); // token from object
```

### Ensure index

`ensure(idx)` grows the array with nulls if needed, then returns element `idx`.

```cpp
array_t xs;

token& slot = xs.ensure(3); // size becomes 4, filled with null
slot = "value";
```

### Remove at index

```cpp
xs.erase_at(1);
```

---

## `kvs` guide

### Find and contains

```cpp
kvs o = obj({"a", 1, "b", 2});

o.contains("a");
std::size_t i = o.find_key_index("b"); // raw index in flat storage
```

### Get pointer

```cpp
if (const token* p = o.get_ptr("a"))
{
  // p is value token
}
```

### Get or create

`operator[]` returns a reference to a value token. If missing, it creates key with null value.

```cpp
kvs o;
o["x"].set_int(123);
```

### Set (insert or replace)

```cpp
o.set("x", 123);
o.set("name", "Ada");
```

### Typed getters

```cpp
auto name = o.get_string("name");  // optional<string>
auto age  = o.get_i64("age");      // optional<int64_t>
auto ok   = o.get_bool("active");  // optional<bool>
auto pi   = o.get_f64("pi");       // optional<double>
```

### Typed getters with defaults

```cpp
std::string name = o.get_string_or("name", "unknown");
std::int64_t age = o.get_i64_or("age", 0);
bool active      = o.get_bool_or("active", false);
double pi        = o.get_f64_or("pi", 0.0);
```

### Typed setters

```cpp
o.set_string("name", "Ada");
o.set_bool("active", true);
o.set_f64("pi", 3.14);
o.set_i64("age", 42);
o.set_int("count", 7);
o.set_ll("big", 999LL);
o.set_ull("ubig", 999ULL);
```

### Ensure nested array/object

```cpp
kvs root;

kvs& user = root.ensure_object("user");
user.set_string("name", "Ada");

array_t& roles = user.ensure_array("roles");
roles.push_cstr("admin");
```

---

## Builders and aliases

### Builders (existing API)

- `array(...)` builds `array_t`
- `obj(...)` builds `kvs`

These are the primary entry points.

### Explicit aliases

When you also include other JSON helpers, the names `obj` and `array` may feel ambiguous.

These aliases keep intent explicit:

- `simple_obj(...)`
- `simple_array(...)`

They do not change behavior, they only forward to `obj()` and `array()`.

---

## Important fix: integer ambiguity

Some platforms define `std::int64_t` as `long` (not `long long`).

In those cases, passing a `long long` into `token` inside an `initializer_list` could become ambiguous.

This header adds explicit support for:
- `long long`
- `unsigned long long`

So call sites like this remain stable and unambiguous:

```cpp
kvs o = obj({
  "big", 123LL,
  "ubig", 123ULL
});
```

---

## When to use Simple vs nlohmann::json

Use Simple when:
- you need lightweight in-memory payloads between modules
- you want a minimal value model without external dependency
- you want predictable structure and cheap copying

Use `nlohmann::json` when:
- you parse or serialize JSON text
- you need integration with external JSON tooling
- you want richer conversion utilities

---

## Summary

`vix/json/Simple.hpp` provides a minimal JSON-like model for internal use:

- `token` for values (primitive or recursive)
- `array_t` for arrays
- `kvs` for objects (flat storage)
- builders: `array(...)` and `obj(...)`
- explicit aliases: `simple_array(...)`, `simple_obj(...)`

It keeps internal structured data easy, dependency-free, and predictable.


