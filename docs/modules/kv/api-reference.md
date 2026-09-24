# API Reference

This page is a compact reference for the public Vix KV API.

Use it when you want to quickly find the main types, functions, and operations exposed by `vix::kv`.

## Public header

```cpp
#include <vix/kv/kv.hpp>
#include <vix/print.hpp>
```

## Namespace

```
vix::kv
```

## Main types

| Type | Purpose |
|---|---|
| `vix::kv::Kv` | Main database handle. |
| `vix::kv::KvOptions` | Opening and configuration options. |
| `vix::kv::KvValue` | Stored value type used by the Result API. |
| `vix::kv::KeyPath` | Structured key made of string segments. |
| `vix::kv::KvStats` | Runtime and recovery statistics. |
| `vix::kv::KvError` | Error object returned by Result APIs. |
| `vix::kv::KvErrorCode` | Stable error code enum. |

## Opening APIs

### open(path)

Opens a durable database and returns a `Kv` directly.

```cpp
auto kv = vix::kv::open("data/kv");
```

Use it for simple local durable storage.

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

### open_memory()

Opens a memory-only database and returns a Result.

```cpp
auto opened = vix::kv::open_memory();
```

Example:

```cpp
auto opened = vix::kv::open_memory();

if (opened.is_err())
{
  vix::eprint(opened.error().message());
  return 1;
}

auto kv = opened.move_value();
```

Memory-only databases do not persist after the process exits.

### open_durable(path)

Opens a durable database and returns a Result.

```cpp
auto opened = vix::kv::open_durable("data/kv");
```

Example:

```cpp
auto opened = vix::kv::open_durable("data/kv");

if (opened.is_err())
{
  vix::eprint(opened.error().message());
  return 1;
}

auto kv = opened.move_value();
```

Use this when you want explicit error handling while opening a durable database.

### open()

Opens a database with default configuration and returns a Result.

```cpp
auto opened = vix::kv::open();

if (opened.is_err())
{
  vix::eprint(opened.error().message());
  return 1;
}

auto kv = opened.move_value();
```

## Kv

`Kv` is the main database object.

| Method | Purpose |
|---|---|
| `put(key, value)` | Writes a simple string key/value pair. |
| `get(key)` | Reads a simple string key and returns an optional value. |
| `set(keyPath, value)` | Writes a structured key/value pair with Result error handling. |
| `get(keyPath)` | Reads a structured key and returns a Result. |
| `erase(keyPath)` | Deletes a structured key. |
| `contains(keyPath)` | Checks whether a structured key exists. |
| `list(prefix)` | Lists live entries under a prefix. |
| `list()` | Lists all live entries. |
| `flush()` | Flushes durable writes. |
| `close()` | Closes the database. |
| `is_open()` | Returns whether the database is open. |
| `empty()` | Returns whether the database has no live keys. |
| `size()` | Returns the number of live keys. |
| `stats()` | Returns database statistics. |

### put

Writes a simple string key/value pair.

```cpp
kv.put("hello", "world");
```

Example:

```cpp
auto kv = vix::kv::open("data/kv");

kv.put("hello", "world");

(void)kv.close();
```

### get with string key

Reads a simple string key.

```cpp
const auto value = kv.get("hello");
```

This returns an optional value.

```cpp
if (value.has_value())
{
  vix::print("hello =", *value);
}
```

Missing values are represented by an empty optional.

```cpp
const auto value = kv.get("missing");

if (!value.has_value())
{
  vix::print("missing");
}
```

### set

Writes a structured key.

```cpp
auto written = kv.set({"users", "1", "name"}, "Ada");
```

Check the Result before continuing.

```cpp
if (written.is_err())
{
  vix::eprint(written.error().message());
  return 1;
}
```

### get with KeyPath

Reads a structured key.

```cpp
auto value = kv.get({"users", "1", "name"});
```

Check the Result before accessing the value.

```cpp
if (value.is_err())
{
  vix::eprint(value.error().message());
  return 1;
}

vix::print("name =", value.value().to_string());
```

### erase

Deletes a structured key.

```cpp
auto erased = kv.erase(vix::kv::KeyPath{"users", "1", "name"});
```

Check the Result:

```cpp
if (erased.is_err())
{
  vix::eprint(erased.error().message());
  return 1;
}
```

### contains

Checks whether a structured key exists.

```cpp
if (kv.contains({"users", "1", "name"}))
{
  vix::print("user name exists");
}
```

### list(prefix)

Lists live entries under a prefix.

```cpp
auto users = kv.list({"users"});
```

Example:

```cpp
auto users = kv.list({"users"});

if (users.is_err())
{
  vix::eprint(users.error().message());
  return 1;
}

vix::print("users =", users.value().size());
```

### list()

Lists all live entries.

```cpp
auto all = kv.list();
```

Example:

```cpp
auto all = kv.list();

if (all.is_err())
{
  vix::eprint(all.error().message());
  return 1;
}

vix::print("entries =", all.value().size());
```

Deleted keys are not returned as live entries.

### flush

Flushes durable writes.

```cpp
auto flushed = kv.flush();

if (flushed.is_err())
{
  vix::eprint(flushed.error().message());
  return 1;
}
```

Short form:

```cpp
(void)kv.flush();
```

Use `flush()` before closing when demonstrating durable behavior.

### close

Closes the database.

```cpp
auto closed = kv.close();

if (closed.is_err())
{
  vix::eprint(closed.error().message());
  return 1;
}
```

Short form:

```cpp
(void)kv.close();
```

### is_open

Returns whether the database is currently open.

```cpp
vix::print("open =", kv.is_open() ? "yes" : "no");
```

### empty

Returns whether the database has no live keys.

```cpp
if (kv.empty())
{
  vix::print("empty database");
}
```

### size

Returns the number of live keys.

```cpp
vix::print("size =", kv.size());
```

Deleted keys are not counted as live keys.

### stats

Returns database statistics.

```cpp
const auto stats = kv.stats();

vix::print("open =", stats.open ? "yes" : "no");
vix::print("memory only =", stats.memory_only ? "yes" : "no");
vix::print("live keys =", stats.key_count);
vix::print("tombstones =", stats.tombstone_count);
vix::print("last sequence =", stats.last_sequence);
vix::print("wal recovered =", stats.wal_records_recovered);
```

## KeyPath

`KeyPath` represents a structured key.

```cpp
vix::kv::KeyPath key{"users", "1", "name"};
```

Use it when you want grouped data and prefix listing.

```cpp
kv.set({"users", "1", "name"}, "Ada");
kv.set({"users", "1", "email"}, "ada@example.com");
kv.set({"settings", "theme"}, "dark");
```

### KeyPath methods

```cpp
vix::kv::KeyPath key{"users", "1", "name"};

vix::print("size =", key.size());
vix::print("first =", key.at(0));
vix::print("second =", key.at(1));
vix::print("third =", key.at(2));
```

## KvValue

`KvValue` is the value type returned by the structured Result API.

```cpp
auto value = kv.get({"users", "1", "name"});

if (value.is_ok())
{
  vix::print(value.value().to_string());
}
```

Use `to_string()` when you want a string representation.

## KvStats

`KvStats` describes database state.

| Field | Meaning |
|---|---|
| `open` | Whether the database is open. |
| `memory_only` | Whether the database is memory-only. |
| `key_count` | Number of live keys. |
| `tombstone_count` | Number of delete markers. |
| `last_sequence` | Last applied operation sequence. |
| `wal_records_recovered` | Number of WAL records recovered after opening. |

Example:

```cpp
const auto stats = kv.stats();

vix::print("live keys =", stats.key_count);
vix::print("tombstones =", stats.tombstone_count);
```

## Result API

Several APIs return Result objects.

A Result can be checked with:

```cpp
result.is_ok()
result.is_err()
```

Read the value with:

```cpp
result.value()
```

Read the error with:

```cpp
result.error()
```

Move the value out with:

```cpp
result.move_value()
```

Example:

```cpp
auto opened = vix::kv::open_memory();

if (opened.is_err())
{
  vix::eprint(opened.error().message());
  return 1;
}

auto kv = opened.move_value();
```

## Error handling

Errors expose a message.

```cpp
if (result.is_err())
{
  vix::eprint(result.error().message());
  return 1;
}
```

Some tests also compare stable error codes through `KvErrorCode`.

```cpp
auto result = kv.set({}, "value");

if (result.is_err())
{
  vix::eprint(result.error().message());
}
```

## Common operation table

| Goal | API |
|---|---|
| Open durable store simply | `vix::kv::open("data/kv")` |
| Open memory store | `vix::kv::open_memory()` |
| Open durable store with Result | `vix::kv::open_durable("data/kv")` |
| Write simple value | `kv.put("hello", "world")` |
| Read simple value | `kv.get("hello")` |
| Write structured value | `kv.set({"users", "1", "name"}, "Ada")` |
| Read structured value | `kv.get({"users", "1", "name"})` |
| Check existence | `kv.contains({"users", "1", "name"})` |
| Delete value | `kv.erase(vix::kv::KeyPath{"users", "1", "name"})` |
| List prefix | `kv.list({"users"})` |
| List all | `kv.list()` |
| Flush durable writes | `kv.flush()` |
| Close database | `kv.close()` |
| Inspect state | `kv.stats()` |
| Live key count | `kv.size()` |

## Complete simple example

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

  (void)kv.flush();
  (void)kv.close();

  return 0;
}
```

## Complete Result API example

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

  const auto stats = kv.stats();

  vix::print("live keys =", stats.key_count);
  vix::print("last sequence =", stats.last_sequence);

  (void)kv.close();

  return 0;
}
```

## Common mistakes

### Accessing a Result value without checking it

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

### Confusing optional and Result APIs

Simple string get returns an optional:

```cpp
const auto value = kv.get("hello");

if (value.has_value())
{
  vix::print(*value);
}
```

Structured get returns a Result:

```cpp
auto value = kv.get({"hello"});

if (value.is_ok())
{
  vix::print(value.value().to_string());
}
```

### Forgetting `to_string()` on KvValue

Wrong:

```cpp
vix::print(value.value());
```

Correct:

```cpp
vix::print(value.value().to_string());
```

### Forgetting to flush persistence examples

For examples that demonstrate durable behavior:

```cpp
(void)kv.flush();
(void)kv.close();
```

### Expecting memory-only storage to persist

Memory-only storage is temporary.

```cpp
auto opened = vix::kv::open_memory();
```

Use durable storage when data must survive restart.

```cpp
auto kv = vix::kv::open("data/kv");
```

## Related pages

| Page | Purpose |
|:---|:---|
| [Overview](/modules/kv/) | Start with the KV overview. |
| [Opening a database](/modules/kv/opening) | Learn opening modes. |
| [Keys](/modules/kv/keys) | Learn simple and structured keys. |
| [Values](/modules/kv/values) | Learn read and write operations. |
| [Persistence](/modules/kv/persistence) | Learn durable storage. |
| [Recovery](/modules/kv/recovery) | Learn recovery and tombstones. |
| [Stats](/modules/kv/stats) | Learn observability fields. |
| [API Reference](/modules/kv/api-reference) | See the public KV API surface. |
