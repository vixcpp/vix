# Recovery

This guide shows how recovery works in Vix KV.

Use this page when you want to understand how durable values are restored after reopening a database.

## Public header

```cpp
#include <vix/kv/kv.hpp>
#include <vix/print.hpp>
```

## What recovery means

Recovery is the process of rebuilding the live database state from durable storage.

When a durable KV database is reopened, Vix KV reads persisted records and restores:

- live keys
- deleted keys
- sequence information
- recovery statistics

This allows data written before closing to be read again after reopening.

## Basic recovery flow

A normal recovery flow looks like this:

1. open durable database
2. write values
3. flush
4. close
5. open same database path again
6. read values back
7. inspect stats

## Write, close, and recover

```cpp
#include <vix/kv/kv.hpp>
#include <vix/print.hpp>

int main()
{
  {
    auto kv = vix::kv::open("data/recovery");

    kv.put("users/1/name", "Ada");
    kv.put("settings/theme", "dark");

    (void)kv.flush();
    (void)kv.close();
  }

  {
    auto kv = vix::kv::open("data/recovery");

    const auto name = kv.get("users/1/name");
    const auto theme = kv.get("settings/theme");

    if (!name.has_value())
    {
      vix::eprint("failed to recover users/1/name");
      return 1;
    }

    if (!theme.has_value())
    {
      vix::eprint("failed to recover settings/theme");
      return 1;
    }

    vix::print("users/1/name =", *name);
    vix::print("settings/theme =", *theme);

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
users/1/name = Ada
settings/theme = dark
```

## Recovery with structured keys

Structured keys also recover after reopening.

```cpp
#include <vix/kv/kv.hpp>
#include <vix/print.hpp>

int main()
{
  {
    auto opened = vix::kv::open_durable("data/structured-recovery");

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

    (void)kv.flush();
    (void)kv.close();
  }

  {
    auto opened = vix::kv::open_durable("data/structured-recovery");

    if (opened.is_err())
    {
      vix::eprint(opened.error().message());
      return 1;
    }

    auto kv = opened.move_value();

    auto value = kv.get({"users", "1", "email"});

    if (value.is_err())
    {
      vix::eprint(value.error().message());
      return 1;
    }

    vix::print("users/1/email =", value.value().to_string());

    (void)kv.close();
  }

  return 0;
}
```

Expected output:

```
users/1/email = ada@example.com
```

## Delete recovery

Deletes are recovered too.

If a key is deleted, flushed, closed, and reopened, the deleted key remains missing.

```cpp
#include <vix/kv/kv.hpp>
#include <vix/print.hpp>

int main()
{
  {
    auto kv = vix::kv::open("data/delete-recovery");

    kv.put("users/1/name", "Ada");
    kv.put("users/2/name", "Grace");

    (void)kv.flush();
    (void)kv.close();
  }

  {
    auto kv = vix::kv::open("data/delete-recovery");

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
    auto kv = vix::kv::open("data/delete-recovery");

    const auto user_1 = kv.get("users/1/name");
    const auto user_2 = kv.get("users/2/name");

    vix::print(
        "users/1/name =",
        user_1.has_value() ? *user_1 : "deleted");

    if (user_2.has_value())
    {
      vix::print("users/2/name =", *user_2);
    }

    const auto stats = kv.stats();

    vix::print("tombstones =", stats.tombstone_count);

    (void)kv.close();
  }

  return 0;
}
```

Expected output:

```
users/1/name = deleted
users/2/name = Grace
tombstones = 1
```

## Recovery stats

Use `stats()` after reopening to inspect recovery information.

```cpp
const auto stats = kv.stats();

vix::print("wal recovered =", stats.wal_records_recovered);
vix::print("last sequence =", stats.last_sequence);
vix::print("live keys =", stats.key_count);
vix::print("tombstones =", stats.tombstone_count);
```

Example:

```cpp
#include <vix/kv/kv.hpp>
#include <vix/print.hpp>

int main()
{
  {
    auto kv = vix::kv::open("data/recovery-stats");

    kv.put("a", "one");
    kv.put("b", "two");

    (void)kv.flush();
    (void)kv.close();
  }

  {
    auto kv = vix::kv::open("data/recovery-stats");

    const auto stats = kv.stats();

    vix::print("wal recovered =", stats.wal_records_recovered);
    vix::print("last sequence =", stats.last_sequence);
    vix::print("live keys =", stats.key_count);

    (void)kv.close();
  }

  return 0;
}
```

## WAL recovery

Vix KV uses durable records to rebuild state when a database is opened again.

The recovery process restores the latest live state from previously written records.

Conceptually:

```
records on disk
  -> read records
  -> apply puts
  -> apply deletes
  -> rebuild live key state
```

This means the final recovered state is based on the order of records. A later delete for a key hides the older value.

## Tombstones

A tombstone records a delete.

When you erase a key, Vix KV does not simply forget the delete. The delete must also survive recovery.

```cpp
auto erased = kv.erase(vix::kv::KeyPath{"users", "1", "name"});

if (erased.is_err())
{
  vix::eprint(erased.error().message());
  return 1;
}
```

After reopening, the key remains deleted.

Stats can show tombstones:

```cpp
const auto stats = kv.stats();

vix::print("tombstones =", stats.tombstone_count);
```

## Recovery and list

Recovered live entries can be listed. Deleted entries are not returned as live entries.

```cpp
auto entries = kv.list({"users"});

if (entries.is_err())
{
  vix::eprint(entries.error().message());
  return 1;
}

for (const auto &[key, value] : entries.value())
{
  vix::print(value.to_string());
}
```

Example:

```cpp
#include <vix/kv/kv.hpp>
#include <vix/print.hpp>

int main()
{
  {
    auto opened = vix::kv::open_durable("data/list-recovery");

    if (opened.is_err())
    {
      vix::eprint(opened.error().message());
      return 1;
    }

    auto kv = opened.move_value();

    (void)kv.set({"users", "1", "name"}, "Ada");
    (void)kv.set({"users", "2", "name"}, "Grace");
    (void)kv.set({"users", "3", "name"}, "Linus");

    auto erased = kv.erase(vix::kv::KeyPath{"users", "3", "name"});

    if (erased.is_err())
    {
      vix::eprint(erased.error().message());
      return 1;
    }

    (void)kv.flush();
    (void)kv.close();
  }

  {
    auto opened = vix::kv::open_durable("data/list-recovery");

    if (opened.is_err())
    {
      vix::eprint(opened.error().message());
      return 1;
    }

    auto kv = opened.move_value();

    auto users = kv.list({"users"});

    if (users.is_err())
    {
      vix::eprint(users.error().message());
      return 1;
    }

    vix::print("live users =", users.value().size());

    const auto stats = kv.stats();

    vix::print("tombstones =", stats.tombstone_count);

    (void)kv.close();
  }

  return 0;
}
```

Expected output:

```
live users = 2
tombstones = 1
```

## Recovery guarantees

For normal durable usage, Vix KV aims to recover the latest flushed state after reopening.

The common recovery rule is:

```
put key=value
flush
close
reopen
get key -> value
```

For deletes:

```
put key=value
flush
erase key
flush
close
reopen
get key -> missing
```

## Common workflows

### Recover one value

```cpp
auto kv = vix::kv::open("data/app");

const auto value = kv.get("name");

if (value.has_value())
{
  vix::print("name =", *value);
}

(void)kv.close();
```

### Recover structured data

```cpp
auto opened = vix::kv::open_durable("data/app");

if (opened.is_err())
{
  vix::eprint(opened.error().message());
  return 1;
}

auto kv = opened.move_value();

auto value = kv.get({"users", "1", "name"});

if (value.is_ok())
{
  vix::print("name =", value.value().to_string());
}

(void)kv.close();
```

### Verify delete recovery

```cpp
const auto value = kv.get("users/1/name");

if (!value.has_value())
{
  vix::print("deleted");
}
```

### Inspect recovery stats

```cpp
const auto stats = kv.stats();

vix::print("wal recovered =", stats.wal_records_recovered);
vix::print("last sequence =", stats.last_sequence);
vix::print("tombstones =", stats.tombstone_count);
```

## Common mistakes

### Reopening a different path

Wrong:

```cpp
auto first = vix::kv::open("data/app");
first.put("name", "Ada");
(void)first.flush();
(void)first.close();

auto second = vix::kv::open("data/other");
const auto value = second.get("name");
```

Correct:

```cpp
auto second = vix::kv::open("data/app");
const auto value = second.get("name");
```

### Forgetting to flush before testing recovery

For examples and tests, call `flush()` before closing.

```cpp
(void)kv.flush();
(void)kv.close();
```

### Expecting deleted values to reappear

Deletes are recovered.

```cpp
auto erased = kv.erase(vix::kv::KeyPath{"users", "1", "name"});
(void)kv.flush();
```

After reopen, the deleted key should remain missing.

### Confusing live keys and tombstones

`size()` and `stats().key_count` represent live keys. `tombstone_count` counts delete markers.

```cpp
const auto stats = kv.stats();

vix::print("live keys =", stats.key_count);
vix::print("tombstones =", stats.tombstone_count);
```

### Ignoring recovery errors in Result-style code

When using Result APIs, check errors before accessing values.

```cpp
auto value = kv.get({"users", "1", "name"});

if (value.is_err())
{
  vix::eprint(value.error().message());
  return 1;
}
```

## Best practices

Use the same database path across restarts. Call `flush()` before closing when demonstrating durable behavior. Use structured keys when you want to recover and list grouped data. Use `stats()` to debug recovered records, live keys, and tombstones. Treat deleted keys as part of durable state.

## Related pages

| Page | Purpose |
|---|---|
| [Opening a database](/modules/kv/opening) | Learn how to open memory and durable stores. |
| [Keys](/modules/kv/keys) | Learn how to structure keys and prefixes. |
| [Values](/modules/kv/values) | Learn how to write, read, and delete values. |
| [Persistence](/modules/kv/persistence) | Learn how values survive restart. |
| [Stats](/modules/kv/stats) | Learn how to inspect recovered state. |

## Next step

Continue with [stats](/modules/kv/stats).
