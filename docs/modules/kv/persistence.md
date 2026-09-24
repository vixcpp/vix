# Persistence

This guide shows how durable storage works in Vix KV.

Use this page when you want values to survive after the process exits and after the database is reopened.

## Public header

```cpp
#include <vix/kv/kv.hpp>
#include <vix/print.hpp>
```

## What persistence means

Persistence means data is written to disk and can be read again later.

A memory-only database is temporary:

```cpp
auto opened = vix::kv::open_memory();
```

A durable database is backed by a filesystem path:

```cpp
auto kv = vix::kv::open("data/app");
```

Use durable storage when the data must survive restart.

## Durable database

Open a durable database with `open(path)`.

```cpp
#include <vix/kv/kv.hpp>
#include <vix/print.hpp>

int main()
{
  auto kv = vix::kv::open("data/app");

  kv.put("name", "Ada");

  (void)kv.flush();
  (void)kv.close();

  return 0;
}
```

The path is the database directory.

```
data/app
```

## Write and reopen

This example writes a value, closes the database, opens it again, and reads the value back.

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

    if (!value.has_value())
    {
      vix::eprint("failed to recover key: name");
      return 1;
    }

    vix::print("name =", *value);

    (void)kv.close();
  }

  return 0;
}
```

Run:

```
vix run main.cpp
```

Expected output:

```
name = Ada
```

## Persistent structured keys

Structured keys can also be persisted.

```cpp
#include <vix/kv/kv.hpp>
#include <vix/print.hpp>

int main()
{
  {
    auto opened = vix::kv::open_durable("data/users");

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

    (void)kv.flush();
    (void)kv.close();
  }

  {
    auto opened = vix::kv::open_durable("data/users");

    if (opened.is_err())
    {
      vix::eprint(opened.error().message());
      return 1;
    }

    auto kv = opened.move_value();

    auto value = kv.get({"users", "1", "name"});

    if (value.is_err())
    {
      vix::eprint(value.error().message());
      return 1;
    }

    vix::print("users/1/name =", value.value().to_string());

    (void)kv.close();
  }

  return 0;
}
```

Expected output:

```
users/1/name = Ada
```

## Flush writes

Use `flush()` when you want to force pending durable writes to disk.

```cpp
auto flushed = kv.flush();

if (flushed.is_err())
{
  vix::eprint(flushed.error().message());
  return 1;
}
```

For simple examples, this compact form is also common:

```cpp
(void)kv.flush();
(void)kv.close();
```

A normal durable write flow is:

```cpp
auto kv = vix::kv::open("data/app");

kv.put("hello", "world");

(void)kv.flush();
(void)kv.close();
```

## Close the database

Close the database when you are done.

```cpp
auto closed = kv.close();

if (closed.is_err())
{
  vix::eprint(closed.error().message());
  return 1;
}
```

For short examples:

```cpp
(void)kv.close();
```

## Delete persistence

Deletes are persisted too.

If a key is deleted, it should remain deleted after reopen.

```cpp
#include <vix/kv/kv.hpp>
#include <vix/print.hpp>

int main()
{
  {
    auto kv = vix::kv::open("data/delete-example");

    kv.put("users/1/name", "Ada");
    kv.put("users/2/name", "Grace");

    (void)kv.flush();
    (void)kv.close();
  }

  {
    auto kv = vix::kv::open("data/delete-example");

    auto erased = kv.erase(vix::kv::KeyPath{"users", "1", "name"});

    if (erased.is_err())
    {
      vix::eprint(erased.error().message());
      return 1;
    }

    (void)kv.flush();
    (void)kv.close();
  }

  {
    auto kv = vix::kv::open("data/delete-example");

    const auto user_1 = kv.get("users/1/name");
    const auto user_2 = kv.get("users/2/name");

    vix::print(
        "users/1/name =",
        user_1.has_value() ? *user_1 : "deleted");

    if (user_2.has_value())
    {
      vix::print("users/2/name =", *user_2);
    }

    (void)kv.close();
  }

  return 0;
}
```

Expected output:

```
users/1/name = deleted
users/2/name = Grace
```

## Persistence and stats

After reopening a durable database, `stats()` can show recovery-related information.

```cpp
const auto stats = kv.stats();

vix::print("live keys =", stats.key_count);
vix::print("tombstones =", stats.tombstone_count);
vix::print("wal recovered =", stats.wal_records_recovered);
vix::print("last sequence =", stats.last_sequence);
```

Example:

```cpp
#include <vix/kv/kv.hpp>
#include <vix/print.hpp>

int main()
{
  {
    auto kv = vix::kv::open("data/stats");

    kv.put("settings/theme", "dark");

    (void)kv.flush();
    (void)kv.close();
  }

  {
    auto kv = vix::kv::open("data/stats");

    const auto value = kv.get("settings/theme");

    if (value.has_value())
    {
      vix::print("settings/theme =", *value);
    }

    const auto stats = kv.stats();

    vix::print("wal recovered =", stats.wal_records_recovered);
    vix::print("last sequence =", stats.last_sequence);

    (void)kv.close();
  }

  return 0;
}
```

## Memory-only vs durable storage

| Mode | API | Survives restart | Use for |
|---|---|---|---|
| Memory-only | `open_memory()` | no | temporary state, tests, short-lived data |
| Durable | `open(path)` | yes | local app data, cache, settings, persistent state |
| Durable Result API | `open_durable(path)` | yes | durable storage with explicit open errors |

## Recommended durable layout

Use a stable directory for the database.

```
data/
└── kv/
```

Example:

```cpp
auto kv = vix::kv::open("data/kv");
```

For apps, use a path that belongs to the application runtime directory.

```
runtime/
└── kv/
```

Example:

```cpp
auto kv = vix::kv::open("runtime/kv");
```

## Common workflows

### Write durable data

```cpp
auto kv = vix::kv::open("data/app");

kv.put("name", "Ada");

(void)kv.flush();
(void)kv.close();
```

### Reopen durable data

```cpp
auto kv = vix::kv::open("data/app");

const auto value = kv.get("name");

if (value.has_value())
{
  vix::print("name =", *value);
}

(void)kv.close();
```

### Write structured durable data

```cpp
auto opened = vix::kv::open_durable("data/app");

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

(void)kv.flush();
(void)kv.close();
```

### Delete durable data

```cpp
auto erased = kv.erase(vix::kv::KeyPath{"users", "1", "name"});

if (erased.is_err())
{
  vix::eprint(erased.error().message());
  return 1;
}

(void)kv.flush();
```

### Inspect durable state

```cpp
const auto stats = kv.stats();

vix::print("live keys =", stats.key_count);
vix::print("tombstones =", stats.tombstone_count);
vix::print("last sequence =", stats.last_sequence);
```

## Common mistakes

### Using memory-only storage for persistent data

Wrong:

```cpp
auto opened = vix::kv::open_memory();
```

Correct:

```cpp
auto kv = vix::kv::open("data/app");
```

### Forgetting to flush in persistence examples

For durable examples, flush before closing.

```cpp
(void)kv.flush();
(void)kv.close();
```

### Writing to one path and reading from another

Wrong:

```cpp
auto kv = vix::kv::open("data/app");
kv.put("name", "Ada");
(void)kv.close();

auto other = vix::kv::open("data/other");
const auto value = other.get("name");
```

Correct:

```cpp
auto kv = vix::kv::open("data/app");
kv.put("name", "Ada");
(void)kv.close();

auto same = vix::kv::open("data/app");
const auto value = same.get("name");
```

### Ignoring open errors with Result-style durable storage

Wrong:

```cpp
auto opened = vix::kv::open_durable("data/app");
auto kv = opened.move_value();
```

Correct:

```cpp
auto opened = vix::kv::open_durable("data/app");

if (opened.is_err())
{
  vix::eprint(opened.error().message());
  return 1;
}

auto kv = opened.move_value();
```

### Expecting deleted values to come back after reopen

Deletes are durable.

```cpp
auto erased = kv.erase(vix::kv::KeyPath{"users", "1", "name"});
(void)kv.flush();
```

After reopening, the key should remain deleted.

## Best practices

Use durable storage for data that belongs to the app state. Use memory-only storage for tests, temporary sessions, and examples that do not need disk state. Call `flush()` in examples where you want to demonstrate persistence clearly. Call `close()` when the store is no longer needed. Keep the database path stable across restarts. Use `stats()` when debugging persistence and recovery.

## Related pages

| Page | Purpose |
|---|---|
| [Opening a database](/modules/kv/opening) | Learn how to open memory and durable stores. |
| [Keys](/modules/kv/keys) | Learn how to structure keys. |
| [Values](/modules/kv/values) | Learn how to store and read values. |
| [Recovery](/modules/kv/recovery) | Learn how WAL recovery works after reopen. |
| [Stats](/modules/kv/stats) | Learn how to inspect recovered records and tombstones. |

## Next step

Continue with [recovery](/modules/kv/recovery).
