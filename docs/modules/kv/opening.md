# Opening a database

This guide shows how to open a Vix KV database.

Use this page when you want to choose between memory-only storage, durable storage, and simple direct access.

## Public header

```cpp
#include <vix/kv/kv.hpp>
#include <vix/print.hpp>
```

## Opening modes

Vix KV provides three common opening modes.

| API | Purpose |
|---|---|
| `vix::kv::open_memory()` | Opens a memory-only database. |
| `vix::kv::open(path)` | Opens a durable database at a filesystem path. |
| `vix::kv::open_durable(path)` | Opens a durable database with explicit Result-style error handling. |

## Memory-only database

Use `open_memory()` when you want temporary data.

The data is lost when the database is closed or when the process exits.

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

  auto written = kv.set({"session", "token"}, "abc123");

  if (written.is_err())
  {
    vix::eprint(written.error().message());
    return 1;
  }

  auto value = kv.get({"session", "token"});

  if (value.is_ok())
  {
    vix::print("session/token =", value.value().to_string());
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
session/token = abc123
```

## Durable database

Use `open(path)` when values must survive after restart.

```cpp
#include <vix/kv/kv.hpp>
#include <vix/print.hpp>

int main()
{
  auto kv = vix::kv::open("data/app");

  kv.put("hello", "world");

  (void)kv.flush();
  (void)kv.close();

  return 0;
}
```

The database files are stored under the directory you pass to `open`.

```
data/app
```

## Reopen a durable database

A durable database can be reopened later.

```cpp
#include <vix/kv/kv.hpp>
#include <vix/print.hpp>

int main()
{
  {
    auto kv = vix::kv::open("data/app");

    kv.put("name", "Ada");

    (void)kv.flush();
    (void)kv.close();
  }

  {
    auto kv = vix::kv::open("data/app");

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

Expected output:

```
name = Ada
```

## Result-style durable opening

Use `open_durable(path)` when you want explicit error handling while opening a durable database.

```cpp
#include <vix/kv/kv.hpp>
#include <vix/print.hpp>

int main()
{
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

  return 0;
}
```

## Default opening

`vix::kv::open()` opens a database with default configuration.

```cpp
#include <vix/kv/kv.hpp>
#include <vix/print.hpp>

int main()
{
  auto opened = vix::kv::open();

  if (opened.is_err())
  {
    vix::eprint(opened.error().message());
    return 1;
  }

  auto kv = opened.move_value();

  vix::print("open =", kv.is_open() ? "yes" : "no");
  vix::print("empty =", kv.empty() ? "yes" : "no");
  vix::print("size =", kv.size());

  (void)kv.close();

  return 0;
}
```

## Direct API vs Result API

There are two common opening styles.

### Direct style

Use direct style when you want a compact local database API.

```cpp
auto kv = vix::kv::open("data/app");

kv.put("hello", "world");

const auto value = kv.get("hello");

if (value.has_value())
{
  vix::print("hello =", *value);
}

(void)kv.close();
```

This style is simple and useful for small examples.

### Result style

Use Result style when you want to handle errors explicitly.

```cpp
auto opened = vix::kv::open_memory();

if (opened.is_err())
{
  vix::eprint(opened.error().message());
  return 1;
}

auto kv = opened.move_value();

auto written = kv.set({"hello"}, "world");

if (written.is_err())
{
  vix::eprint(written.error().message());
  return 1;
}

auto value = kv.get({"hello"});

if (value.is_ok())
{
  vix::print("hello =", value.value().to_string());
}

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

For simple examples, this is also accepted:

```cpp
(void)kv.close();
```

## Flush durable writes

Use `flush()` when you want to force pending durable writes to disk.

```cpp
auto flushed = kv.flush();

if (flushed.is_err())
{
  vix::eprint(flushed.error().message());
  return 1;
}
```

Typical durable flow:

```cpp
auto kv = vix::kv::open("data/app");

kv.put("name", "Ada");

(void)kv.flush();
(void)kv.close();
```

## Check database state

After opening a database, you can inspect its state.

```cpp
vix::print("open =", kv.is_open() ? "yes" : "no");
vix::print("empty =", kv.empty() ? "yes" : "no");
vix::print("size =", kv.size());
```

For more details, use `stats()`.

```cpp
const auto stats = kv.stats();

vix::print("memory only =", stats.memory_only ? "yes" : "no");
vix::print("live keys =", stats.key_count);
vix::print("last sequence =", stats.last_sequence);
```

## Common workflows

### Open a temporary store

```cpp
auto opened = vix::kv::open_memory();

if (opened.is_err())
{
  vix::eprint(opened.error().message());
  return 1;
}

auto kv = opened.move_value();
```

### Open a durable store

```cpp
auto kv = vix::kv::open("data/app");
```

### Open a durable store with explicit errors

```cpp
auto opened = vix::kv::open_durable("data/app");

if (opened.is_err())
{
  vix::eprint(opened.error().message());
  return 1;
}

auto kv = opened.move_value();
```

### Write, flush, and close

```cpp
kv.put("hello", "world");

(void)kv.flush();
(void)kv.close();
```

### Reopen and read

```cpp
auto kv = vix::kv::open("data/app");

const auto value = kv.get("hello");

if (value.has_value())
{
  vix::print("hello =", *value);
}

(void)kv.close();
```

## Common mistakes

### Using memory storage when data must persist

Memory-only data does not survive restart.

```cpp
auto opened = vix::kv::open_memory();
```

Use durable storage instead:

```cpp
auto kv = vix::kv::open("data/app");
```

### Forgetting to check open errors

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

### Forgetting to close the database

```cpp
auto kv = vix::kv::open("data/app");

kv.put("hello", "world");

(void)kv.close();
```

### Forgetting to flush durable writes in examples

When writing durable examples, call `flush()` before `close()`.

```cpp
(void)kv.flush();
(void)kv.close();
```

### Confusing direct open and Result open

This returns a database directly:

```cpp
auto kv = vix::kv::open("data/app");
```

This returns a Result:

```cpp
auto opened = vix::kv::open_memory();
auto opened = vix::kv::open_durable("data/app");
```

## Related pages

| Page | Purpose |
|---|---|
| [Keys](/modules/kv/keys) | Learn how to structure keys. |
| [Values](/modules/kv/values) | Learn how to store and read values. |
| [Persistence](/modules/kv/persistence) | Learn how data survives restart. |
| [Stats](/modules/kv/stats) | Learn how to inspect database state. |

## Next step

Continue with [keys](/modules/kv/keys).
