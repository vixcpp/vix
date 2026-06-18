# Values

This guide shows how to write, read, and delete values with Vix KV.

Use this page when you want to store strings, read them back, and understand the difference between the simple API and the Result API.

## Public header

```cpp
#include <vix/kv/kv.hpp>
#include <vix/print.hpp>
```

## Basic value storage

The simplest way to store a value is `put`.

```cpp
#include <vix/kv/kv.hpp>
#include <vix/print.hpp>

int main()
{
  auto kv = vix::kv::open("data/kv");

  kv.put("hello", "world");

  const auto value = kv.get("hello");

  if (value.has_value())
  {
    vix::print("hello =", *value);
  }

  (void)kv.close();

  return 0;
}
```

Run:

```
vix run main.cpp
```

Expected output:

```
hello = world
```

## Simple API

The simple API uses string keys and optional values.

```cpp
auto kv = vix::kv::open("data/kv");

kv.put("hello", "world");

const auto value = kv.get("hello");

if (value.has_value())
{
  vix::print(*value);
}

(void)kv.close();
```

Use this style when you want a compact local key-value API.

## Missing values

When a key does not exist, `get` returns an empty optional.

```cpp
const auto value = kv.get("missing");

if (!value.has_value())
{
  vix::print("missing");
}
```

This is not an error. It only means the key is not present.

## Result API

The Result API uses structured keys and explicit error handling.

```cpp
#include <vix/kv/kv.hpp>
#include <vix/print.hpp>

int main()
{
  auto opened = vix::kv::open_memory();

  if (opened.is_err())
  {
    vix::eprint(opened.error().message());
    return 1;
  }

  auto kv = opened.move_value();

  auto written = kv.set({"users", "1", "email"}, "ada@example.com");

  if (written.is_err())
  {
    vix::eprint(written.error().message());
    return 1;
  }

  auto value = kv.get({"users", "1", "email"});

  if (value.is_err())
  {
    vix::eprint(value.error().message());
    return 1;
  }

  vix::print("users/1/email =", value.value().to_string());

  (void)kv.close();

  return 0;
}
```

Expected output:

```
users/1/email = ada@example.com
```

## KvValue

The Result API returns `KvValue`.

Use `to_string()` when you want a string representation.

```cpp
auto value = kv.get({"users", "1", "name"});

if (value.is_ok())
{
  vix::print("name =", value.value().to_string());
}
```

## Write values with set

Use `set` with structured keys.

```cpp
auto written = kv.set({"settings", "theme"}, "dark");

if (written.is_err())
{
  vix::eprint(written.error().message());
  return 1;
}
```

Structured keys are useful for grouped data.

```cpp
kv.set({"users", "1", "name"}, "Ada");
kv.set({"users", "1", "email"}, "ada@example.com");
kv.set({"users", "2", "name"}, "Grace");
```

## Check if a value exists

Use `contains`.

```cpp
if (kv.contains({"users", "1", "email"}))
{
  vix::print("email exists");
}
```

Example:

```cpp
#include <vix/kv/kv.hpp>
#include <vix/print.hpp>

int main()
{
  auto opened = vix::kv::open_memory();

  if (opened.is_err())
  {
    vix::eprint(opened.error().message());
    return 1;
  }

  auto kv = opened.move_value();

  (void)kv.set({"session", "token"}, "abc123");

  vix::print(
      "contains =",
      kv.contains({"session", "token"}) ? "yes" : "no");

  (void)kv.close();

  return 0;
}
```

Expected output:

```
contains = yes
```

## Delete values

Use `erase` to delete a value.

```cpp
auto erased = kv.erase(vix::kv::KeyPath{"session", "token"});

if (erased.is_err())
{
  vix::eprint(erased.error().message());
  return 1;
}
```

Complete example:

```cpp
#include <vix/kv/kv.hpp>
#include <vix/print.hpp>

int main()
{
  auto opened = vix::kv::open_memory();

  if (opened.is_err())
  {
    vix::eprint(opened.error().message());
    return 1;
  }

  auto kv = opened.move_value();

  (void)kv.set({"session", "token"}, "abc123");

  vix::print(
      "before erase =",
      kv.contains({"session", "token"}) ? "exists" : "missing");

  auto erased = kv.erase(vix::kv::KeyPath{"session", "token"});

  if (erased.is_err())
  {
    vix::eprint(erased.error().message());
    return 1;
  }

  vix::print(
      "after erase =",
      kv.contains({"session", "token"}) ? "exists" : "missing");

  (void)kv.close();

  return 0;
}
```

Expected output:

```
before erase = exists
after erase = missing
```

## List values

Use `list` to read live values.

```cpp
auto entries = kv.list({"users"});

if (entries.is_err())
{
  vix::eprint(entries.error().message());
  return 1;
}
```

Example:

```cpp
#include <vix/kv/kv.hpp>
#include <vix/print.hpp>

static void print_key(const vix::kv::KeyPath &key)
{
  for (std::size_t i = 0; i < key.size(); ++i)
  {
    if (i > 0)
    {
      vix::print_inline("/");
    }

    vix::print_inline(key.at(i));
  }

  vix::print();
}

int main()
{
  auto opened = vix::kv::open_memory();

  if (opened.is_err())
  {
    vix::eprint(opened.error().message());
    return 1;
  }

  auto kv = opened.move_value();

  (void)kv.set({"users", "1", "name"}, "Ada");
  (void)kv.set({"users", "2", "name"}, "Grace");
  (void)kv.set({"settings", "theme"}, "dark");

  auto users = kv.list({"users"});

  if (users.is_err())
  {
    vix::eprint(users.error().message());
    return 1;
  }

  for (const auto &[key, value] : users.value())
  {
    vix::print_inline("key = ");
    print_key(key);

    vix::print("value =", value.to_string());
  }

  (void)kv.close();

  return 0;
}
```

## List all live values

Call `list()` without a prefix to read all live entries.

```cpp
auto all = kv.list();

if (all.is_err())
{
  vix::eprint(all.error().message());
  return 1;
}

for (const auto &[key, value] : all.value())
{
  vix::print(value.to_string());
}
```

Deleted keys are not returned as live values.

## Value workflow

A normal value workflow looks like this:

```cpp
auto opened = vix::kv::open_memory();

if (opened.is_err())
{
  vix::eprint(opened.error().message());
  return 1;
}

auto kv = opened.move_value();

auto written = kv.set({"users", "1", "name"}, "Ada");

if (written.is_err())
{
  vix::eprint(written.error().message());
  return 1;
}

auto value = kv.get({"users", "1", "name"});

if (value.is_ok())
{
  vix::print(value.value().to_string());
}

(void)kv.close();
```

## Simple API vs Result API

| Operation | Simple API | Result API |
|---|---|---|
| Write | `kv.put("hello", "world")` | `kv.set({"hello"}, "world")` |
| Read | `kv.get("hello")` | `kv.get({"hello"})` |
| Missing value | empty `std::optional` | Result value/error model |
| Structured keys | string path | KeyPath |
| Error handling | compact | explicit |

## Common workflows

### Store and read a string

```cpp
kv.put("hello", "world");

const auto value = kv.get("hello");

if (value.has_value())
{
  vix::print(*value);
}
```

### Store and read a structured value

```cpp
auto written = kv.set({"users", "1", "name"}, "Ada");

if (written.is_err())
{
  vix::eprint(written.error().message());
  return 1;
}

auto value = kv.get({"users", "1", "name"});

if (value.is_ok())
{
  vix::print(value.value().to_string());
}
```

### Check existence

```cpp
vix::print(
    "exists =",
    kv.contains({"users", "1", "name"}) ? "yes" : "no");
```

### Delete a value

```cpp
auto erased = kv.erase(vix::kv::KeyPath{"users", "1", "name"});

if (erased.is_err())
{
  vix::eprint(erased.error().message());
  return 1;
}
```

### List a prefix

```cpp
auto users = kv.list({"users"});

if (users.is_ok())
{
  vix::print("count =", users.value().size());
}
```

## Common mistakes

### Treating a missing simple value as an error

A missing value from the simple API is represented by an empty optional.

```cpp
const auto value = kv.get("missing");

if (!value.has_value())
{
  vix::print("not found");
}
```

### Forgetting to check Result errors

Wrong:

```cpp
auto value = kv.get({"users", "1", "name"});
vix::print(value.value().to_string());
```

Correct:

```cpp
auto value = kv.get({"users", "1", "name"});

if (value.is_err())
{
  vix::eprint(value.error().message());
  return 1;
}

vix::print(value.value().to_string());
```

### Forgetting `to_string()`

Wrong:

```cpp
vix::print(value.value());
```

Correct:

```cpp
vix::print(value.value().to_string());
```

### Deleting with the wrong key shape

If you wrote with:

```cpp
kv.set({"session", "token"}, "abc123");
```

Delete with the same key path:

```cpp
kv.erase(vix::kv::KeyPath{"session", "token"});
```

### Expecting deleted values to appear in list

Deleted keys are tombstoned internally and are not returned as live entries by `list`.

Use `stats()` if you want to inspect tombstones.

```cpp
const auto stats = kv.stats();

vix::print("tombstones =", stats.tombstone_count);
```

## Related pages

| Page | Purpose |
|---|---|
| [Opening a database](/modules/kv/opening) | Learn how to open memory and durable stores. |
| [Keys](/modules/kv/keys) | Learn how to structure keys. |
| [Persistence](/modules/kv/persistence) | Learn how values survive restart. |
| [Recovery](/modules/kv/recovery) | Learn how deletes and WAL recovery work. |
| [Stats](/modules/kv/stats) | Learn how to inspect live keys and tombstones. |

## Next step

Continue with [keys](/modules/kv/keys).
