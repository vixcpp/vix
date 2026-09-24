# Stats

This guide shows how to inspect the state of a Vix KV database.

Use this page when you want to check live keys, tombstones, memory mode, recovery information, and sequence state.

## Public header

```cpp
#include <vix/kv/kv.hpp>
#include <vix/print.hpp>
```

## What stats are for

Stats help you observe what is happening inside a KV database.

Use `stats()` when you want to know:

- whether the database is open
- whether it is memory-only
- how many live keys exist
- how many tombstones exist
- the last sequence number
- how many WAL records were recovered after reopen

## Basic stats

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

  (void)kv.set({"a"}, "one");
  (void)kv.set({"b"}, "two");
  (void)kv.erase(vix::kv::KeyPath{"a"});

  const auto stats = kv.stats();

  vix::print("open =", stats.open ? "yes" : "no");
  vix::print("memory only =", stats.memory_only ? "yes" : "no");
  vix::print("live keys =", stats.key_count);
  vix::print("tombstones =", stats.tombstone_count);
  vix::print("last sequence =", stats.last_sequence);

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
open = yes
memory only = yes
live keys = 1
tombstones = 1
last sequence = 3
```

## Stats fields

| Field | Meaning |
|---|---|
| `open` | Whether the database is currently open. |
| `memory_only` | Whether the database is memory-only. |
| `key_count` | Number of live keys. |
| `tombstone_count` | Number of delete markers. |
| `last_sequence` | Last applied operation sequence. |
| `wal_records_recovered` | Number of WAL records recovered when opening a durable database. |

## Live keys

Live keys are values currently visible to reads and lists.

```cpp
const auto stats = kv.stats();

vix::print("live keys =", stats.key_count);
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

  (void)kv.set({"users", "1", "name"}, "Ada");
  (void)kv.set({"users", "2", "name"}, "Grace");

  const auto stats = kv.stats();

  vix::print("live keys =", stats.key_count);
  vix::print("size =", kv.size());

  (void)kv.close();

  return 0;
}
```

Expected output:

```
live keys = 2
size = 2
```

## Tombstones

A tombstone represents a delete.

When you erase a key, the live key count decreases and the tombstone count increases.

```cpp
auto erased = kv.erase(vix::kv::KeyPath{"users", "1", "name"});

if (erased.is_err())
{
  vix::eprint(erased.error().message());
  return 1;
}
```

Then inspect stats:

```cpp
const auto stats = kv.stats();

vix::print("live keys =", stats.key_count);
vix::print("tombstones =", stats.tombstone_count);
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

  (void)kv.set({"users", "1", "name"}, "Ada");
  (void)kv.set({"users", "2", "name"}, "Grace");

  auto erased = kv.erase(vix::kv::KeyPath{"users", "1", "name"});

  if (erased.is_err())
  {
    vix::eprint(erased.error().message());
    return 1;
  }

  const auto stats = kv.stats();

  vix::print("live keys =", stats.key_count);
  vix::print("tombstones =", stats.tombstone_count);

  (void)kv.close();

  return 0;
}
```

Expected output:

```
live keys = 1
tombstones = 1
```

## Open state

Use `stats.open` or `kv.is_open()` to check whether the database is open.

```cpp
const auto stats = kv.stats();

vix::print("open =", stats.open ? "yes" : "no");
```

You can also use:

```cpp
vix::print("open =", kv.is_open() ? "yes" : "no");
```

## Memory-only state

Use `stats.memory_only` to know whether the database is temporary.

```cpp
const auto stats = kv.stats();

vix::print("memory only =", stats.memory_only ? "yes" : "no");
```

Memory-only databases are useful for tests and temporary state.

```cpp
auto opened = vix::kv::open_memory();
```

Durable databases are backed by a filesystem path.

```cpp
auto kv = vix::kv::open("data/app");
```

## Sequence state

`last_sequence` tracks the last applied operation sequence.

Each write or delete advances the operation history.

```cpp
const auto stats = kv.stats();

vix::print("last sequence =", stats.last_sequence);
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

  (void)kv.set({"a"}, "one");
  (void)kv.set({"b"}, "two");
  (void)kv.erase(vix::kv::KeyPath{"a"});

  const auto stats = kv.stats();

  vix::print("last sequence =", stats.last_sequence);

  (void)kv.close();

  return 0;
}
```

Expected output:

```
last sequence = 3
```

## Recovery stats

For durable databases, stats can show recovery information after reopening.

```cpp
const auto stats = kv.stats();

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
    auto kv = vix::kv::open("data/recovery-stats");

    kv.put("name", "Ada");
    kv.put("theme", "dark");

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

Example output:

```
wal recovered = 2
last sequence = 2
live keys = 2
```

## Stats with list

`list()` returns live entries. `stats()` tells you about live entries and tombstones.

```cpp
auto entries = kv.list({"users"});

if (entries.is_err())
{
  vix::eprint(entries.error().message());
  return 1;
}

const auto stats = kv.stats();

vix::print("listed users =", entries.value().size());
vix::print("live keys =", stats.key_count);
vix::print("tombstones =", stats.tombstone_count);
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

  (void)kv.set({"users", "1", "name"}, "Ada");
  (void)kv.set({"users", "2", "name"}, "Grace");
  (void)kv.set({"users", "3", "name"}, "Linus");

  auto erased = kv.erase(vix::kv::KeyPath{"users", "3", "name"});

  if (erased.is_err())
  {
    vix::eprint(erased.error().message());
    return 1;
  }

  auto users = kv.list({"users"});

  if (users.is_err())
  {
    vix::eprint(users.error().message());
    return 1;
  }

  const auto stats = kv.stats();

  vix::print("listed users =", users.value().size());
  vix::print("live keys =", stats.key_count);
  vix::print("tombstones =", stats.tombstone_count);

  (void)kv.close();

  return 0;
}
```

Expected output:

```
listed users = 2
live keys = 2
tombstones = 1
```

## Stats and size

`kv.size()` returns the number of live keys. `stats.key_count` also represents live keys.

```cpp
vix::print("size =", kv.size());

const auto stats = kv.stats();

vix::print("live keys =", stats.key_count);
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

  (void)kv.set({"a"}, "one");
  (void)kv.set({"b"}, "two");

  const auto stats = kv.stats();

  vix::print("size =", kv.size());
  vix::print("live keys =", stats.key_count);

  (void)kv.close();

  return 0;
}
```

Expected output:

```
size = 2
live keys = 2
```

## Common workflows

### Print basic stats

```cpp
const auto stats = kv.stats();

vix::print("open =", stats.open ? "yes" : "no");
vix::print("memory only =", stats.memory_only ? "yes" : "no");
vix::print("live keys =", stats.key_count);
vix::print("tombstones =", stats.tombstone_count);
vix::print("last sequence =", stats.last_sequence);
```

### Check live key count

```cpp
const auto stats = kv.stats();

if (stats.key_count == 0)
{
  vix::print("database is empty");
}
```

### Check delete markers

```cpp
const auto stats = kv.stats();

if (stats.tombstone_count > 0)
{
  vix::print("database has tombstones");
}
```

### Check recovery

```cpp
const auto stats = kv.stats();

vix::print("wal recovered =", stats.wal_records_recovered);
```

### Compare list and stats

```cpp
auto users = kv.list({"users"});

if (users.is_ok())
{
  const auto stats = kv.stats();

  vix::print("users prefix =", users.value().size());
  vix::print("all live keys =", stats.key_count);
}
```

## Common mistakes

### Confusing live keys and tombstones

Live keys are readable values. Tombstones are delete markers.

```cpp
const auto stats = kv.stats();

vix::print("live keys =", stats.key_count);
vix::print("tombstones =", stats.tombstone_count);
```

### Expecting deleted keys to appear in size

Deleted keys are not live keys.

```cpp
vix::print("size =", kv.size());
```

Use tombstones when you want to observe deletes.

```cpp
const auto stats = kv.stats();

vix::print("tombstones =", stats.tombstone_count);
```

### Expecting memory-only stores to recover WAL records

Memory-only stores are temporary.

```cpp
auto opened = vix::kv::open_memory();
```

Recovery stats are most useful with durable databases.

```cpp
auto kv = vix::kv::open("data/app");
```

### Reading stats before operations

Stats reflect the current state. If you read stats before writing, counts may be zero.

```cpp
const auto before = kv.stats();

(void)kv.set({"a"}, "one");

const auto after = kv.stats();
```

### Treating stats as application data

Stats are for observability. Store application data with `put` or `set`.

```cpp
kv.put("settings/theme", "dark");
```

## Best practices

Use `stats()` when debugging persistence and recovery. Use `stats()` after deletes to confirm tombstones. Use `kv.size()` when you only need the live key count. Use `list(prefix)` when you need the actual entries. Use `wal_records_recovered` to inspect durable reopen behavior.

## Related pages

| Page | Purpose |
|---|---|
| [Opening a database](/modules/kv/opening) | Learn how to open memory and durable stores. |
| [Keys](/modules/kv/keys) | Learn how to structure keys and prefixes. |
| [Values](/modules/kv/values) | Learn how to write, read, and delete values. |
| [Persistence](/modules/kv/persistence) | Learn how values survive restart. |
| [Recovery](/modules/kv/recovery) | Learn how recovered records and tombstones work. |
| [API Reference](/modules/kv/api-reference) | See the public KV API surface. |

## Next step

Continue with the [API reference](/modules/kv/api-reference).
