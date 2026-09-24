# KV

`vix::kv` is a local key-value store for Vix applications.

Use it when you need simple local storage, structured keys, durable writes, and recovery after restart.

## Public header

```cpp
#include <vix/kv/kv.hpp>
#include <vix/print.hpp>
```

## What KV provides

Vix KV provides a small public API for storing and reading values by key.

It supports:

- memory-only databases
- durable databases stored on disk
- simple string keys
- structured keys with KeyPath
- put and get helpers
- Result-based APIs with explicit error handling
- deletes with tombstones
- listing by prefix
- stats for observability
- recovery after reopening a durable database

## Quick start

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

## Memory-only database

Use a memory-only database when you do not need persistence.

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

  auto written = kv.set({"users", "1", "name"}, "Ada");

  if (written.is_err())
  {
    vix::eprint(written.error().message());
    return 1;
  }

  auto value = kv.get({"users", "1", "name"});

  if (value.is_ok())
  {
    vix::print("users/1/name =", value.value().to_string());
  }

  (void)kv.close();

  return 0;
}
```

## Durable database

Use a durable database when values must survive after the process exits.

```cpp
#include <vix/kv/kv.hpp>
#include <vix/print.hpp>

int main()
{
  {
    auto kv = vix::kv::open("data/persistent");

    kv.put("name", "Ada");

    (void)kv.flush();
    (void)kv.close();
  }

  {
    auto kv = vix::kv::open("data/persistent");

    const auto value = kv.get("name");

    if (value.has_value())
    {
      vix::print("name =", *value);
    }

    (void)kv.close();
  }

  return 0;
}
```

## Simple API and Result API

Vix KV has two usage styles.

| Style | Use when |
|---|---|
| Simple API | You want fast put and get helpers with string keys. |
| Result API | You want explicit error handling and structured keys. |

**Simple API:**

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

**Result API:**

```cpp
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

if (value.is_ok())
{
  vix::print(value.value().to_string());
}

(void)kv.close();
```

## Structured keys

Structured keys are represented with `vix::kv::KeyPath`.

```cpp
kv.set({"users", "1", "name"}, "Ada");
kv.set({"users", "1", "email"}, "ada@example.com");
kv.set({"settings", "theme"}, "dark");
```

This makes it easy to group data by prefix.

```cpp
auto users = kv.list({"users"});
```

## Delete keys

Use `erase` to delete a key.

```cpp
auto erased = kv.erase(vix::kv::KeyPath{"session", "token"});

if (erased.is_err())
{
  vix::eprint(erased.error().message());
  return 1;
}
```

After a delete, `contains` returns false.

```cpp
vix::print(
    "exists =",
    kv.contains({"session", "token"}) ? "yes" : "no");
```

## List entries

Use `list` to read live entries.

```cpp
auto entries = kv.list({"users"});

if (entries.is_err())
{
  vix::eprint(entries.error().message());
  return 1;
}

for (const auto &[key, value] : entries.value())
{
  vix::print(key.size(), value.to_string());
}
```

Use `list()` without a prefix to list all live entries.

```cpp
auto all = kv.list();
```

## Stats

Use `stats()` to inspect the database state.

```cpp
const auto stats = kv.stats();

vix::print("open =", stats.open ? "yes" : "no");
vix::print("memory only =", stats.memory_only ? "yes" : "no");
vix::print("live keys =", stats.key_count);
vix::print("tombstones =", stats.tombstone_count);
vix::print("last sequence =", stats.last_sequence);
```

Stats are useful when debugging deletes, recovery, and persistent state.

## Common workflows

### Store a value

```cpp
auto kv = vix::kv::open("data/kv");

kv.put("hello", "world");

(void)kv.close();
```

### Read a value

```cpp
const auto value = kv.get("hello");

if (value.has_value())
{
  vix::print(*value);
}
```

### Store a structured value

```cpp
auto written = kv.set({"users", "1", "name"}, "Ada");

if (written.is_err())
{
  vix::eprint(written.error().message());
  return 1;
}
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

### Persist data before closing

```cpp
(void)kv.flush();
(void)kv.close();
```

## Common mistakes

### Forgetting to close the database

```cpp
auto kv = vix::kv::open("data/kv");

kv.put("hello", "world");

(void)kv.close();
```

### Forgetting to flush durable data

For durable examples, call `flush()` before closing when you want to force pending writes to disk.

```cpp
(void)kv.flush();
(void)kv.close();
```

### Ignoring Result errors

Wrong:

```cpp
auto opened = vix::kv::open_memory();
auto kv = opened.move_value();
```

Correct:

```cpp
auto opened = vix::kv::open_memory();

if (opened.is_err())
{
  vix::eprint(opened.error().message());
  return 1;
}

auto kv = opened.move_value();
```

### Confusing missing values and errors

The simple string get API returns an optional value.

```cpp
const auto value = kv.get("missing");

if (!value.has_value())
{
  vix::print("missing");
}
```

The Result API returns a result object.

```cpp
auto value = kv.get({"missing"});

if (value.is_err())
{
  vix::eprint(value.error().message());
}
```

## Related pages

| Page | Purpose |
|---|---|
| [Opening a database](/modules/kv/opening) | Learn how to open memory and durable stores. |
| [Keys](/modules/kv/keys) | Learn how to use string keys and KeyPath. |
| [Values](/modules/kv/values) | Learn how values are stored and read. |
| [Persistence](/modules/kv/persistence) | Learn how data survives restart. |
| [Recovery](/modules/kv/recovery) | Learn how WAL recovery works. |
| [Stats](/modules/kv/stats) | Learn how to inspect KV state. |
| [API Reference](/modules/kv/api-reference) | See the public API surface. |

## Next step

Continue with [opening a database](/modules/kv/opening).
