# Keys

This guide shows how keys work in Vix KV.

Use this page when you want to organize values with simple string keys, structured keys, and prefixes.

## Public header

```cpp
#include <vix/kv/kv.hpp>
#include <vix/print.hpp>
```

## Key styles

Vix KV supports two common key styles.

| Style | Example | Use when |
|---|---|---|
| String key | `"hello"` | You want a simple key for the direct API. |
| Structured key | `{"users", "1", "name"}` | You want grouped data and prefix listing. |

## Simple string keys

The direct API uses string keys.

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

## Structured keys

Structured keys use `vix::kv::KeyPath`.

```cpp
kv.set({"users", "1", "name"}, "Ada");
kv.set({"users", "1", "email"}, "ada@example.com");
kv.set({"settings", "theme"}, "dark");
```

A structured key is made of segments.

```
users / 1 / name
```

In code:

```cpp
vix::kv::KeyPath key{"users", "1", "name"};
```

## Write with a structured key

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

Expected output:

```
users/1/name = Ada
```

## Read with the same key

The read key must match the write key.

```cpp
kv.set({"users", "1", "name"}, "Ada");

auto value = kv.get({"users", "1", "name"});

if (value.is_ok())
{
  vix::print(value.value().to_string());
}
```

This is a different key:

```cpp
kv.get({"users", "name", "1"});
```

Key segment order matters.

## Prefix organization

Structured keys are useful for grouping related values.

```cpp
kv.set({"users", "1", "name"}, "Ada");
kv.set({"users", "1", "email"}, "ada@example.com");
kv.set({"users", "2", "name"}, "Grace");
kv.set({"settings", "theme"}, "dark");
```

This creates a logical layout:

```
users/
  1/
    name
    email
  2/
    name

settings/
  theme
```

## List by prefix

Use `list(prefix)` to read all live entries under a prefix.

```cpp
auto users = kv.list({"users"});

if (users.is_err())
{
  vix::eprint(users.error().message());
  return 1;
}
```

Complete example:

```cpp
#include <vix/kv/kv.hpp>
#include <vix/print.hpp>

static std::string key_to_string(const vix::kv::KeyPath &key)
{
  std::string out;

  for (std::size_t i = 0; i < key.size(); ++i)
  {
    if (i > 0)
    {
      out += "/";
    }

    out += key.at(i);
  }

  return out;
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
    vix::print(key_to_string(key), "=", value.to_string());
  }

  (void)kv.close();

  return 0;
}
```

Example output:

```
users/1/name = Ada
users/2/name = Grace
```

## List all keys

Use `list()` without a prefix to read all live entries.

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

## Check if a key exists

Use `contains`.

```cpp
if (kv.contains({"users", "1", "name"}))
{
  vix::print("user name exists");
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

  (void)kv.set({"users", "1", "name"}, "Ada");

  vix::print(
      "contains =",
      kv.contains({"users", "1", "name"}) ? "yes" : "no");

  (void)kv.close();

  return 0;
}
```

Expected output:

```
contains = yes
```

## Delete a key

Use `erase` with the same key path.

```cpp
auto erased = kv.erase(vix::kv::KeyPath{"users", "1", "name"});

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

## KeyPath basics

A `KeyPath` is a sequence of string segments.

```cpp
vix::kv::KeyPath key{"users", "1", "profile", "name"};
```

You can inspect its size:

```cpp
vix::print("segments =", key.size());
```

You can access individual segments:

```cpp
vix::print("first =", key.at(0));
vix::print("last =", key.at(key.size() - 1));
```

Example:

```cpp
#include <vix/kv/kv.hpp>
#include <vix/print.hpp>

int main()
{
  vix::kv::KeyPath key{"users", "1", "name"};

  vix::print("size =", key.size());
  vix::print("first =", key.at(0));
  vix::print("second =", key.at(1));
  vix::print("third =", key.at(2));

  return 0;
}
```

Expected output:

```
size = 3
first = users
second = 1
third = name
```

## Recommended key shape

Use stable, predictable segments.

Good:

```cpp
kv.set({"users", "1", "name"}, "Ada");
kv.set({"users", "1", "email"}, "ada@example.com");
kv.set({"orders", "1001", "status"}, "paid");
kv.set({"settings", "theme"}, "dark");
```

Avoid putting everything in one flat string when you want prefix listing.

Less useful:

```cpp
kv.put("users/1/name", "Ada");
```

Better for structured queries:

```cpp
kv.set({"users", "1", "name"}, "Ada");
```

## Prefix examples

### Users

```cpp
kv.set({"users", "1", "name"}, "Ada");
kv.set({"users", "1", "role"}, "admin");
kv.set({"users", "2", "name"}, "Grace");
```

List all users-related entries:

```cpp
auto users = kv.list({"users"});
```

List a single user:

```cpp
auto user_1 = kv.list({"users", "1"});
```

### Settings

```cpp
kv.set({"settings", "theme"}, "dark");
kv.set({"settings", "language"}, "en");
```

List settings:

```cpp
auto settings = kv.list({"settings"});
```

### Sessions

```cpp
kv.set({"sessions", "abc123", "user_id"}, "1");
kv.set({"sessions", "abc123", "expires_at"}, "2026-01-01");
```

Delete one session value:

```cpp
auto erased = kv.erase(vix::kv::KeyPath{"sessions", "abc123", "user_id"});
```

## Common workflows

### Store user fields

```cpp
kv.set({"users", "1", "name"}, "Ada");
kv.set({"users", "1", "email"}, "ada@example.com");
kv.set({"users", "1", "role"}, "admin");
```

### Read one field

```cpp
auto name = kv.get({"users", "1", "name"});

if (name.is_ok())
{
  vix::print("name =", name.value().to_string());
}
```

### Check existence

```cpp
if (kv.contains({"users", "1", "email"}))
{
  vix::print("email exists");
}
```

### List a prefix

```cpp
auto entries = kv.list({"users", "1"});

if (entries.is_ok())
{
  vix::print("fields =", entries.value().size());
}
```

### Delete one field

```cpp
auto erased = kv.erase(vix::kv::KeyPath{"users", "1", "role"});

if (erased.is_err())
{
  vix::eprint(erased.error().message());
  return 1;
}
```

## Common mistakes

### Changing segment order

Wrong:

```cpp
kv.set({"users", "1", "name"}, "Ada");

auto value = kv.get({"users", "name", "1"});
```

Correct:

```cpp
auto value = kv.get({"users", "1", "name"});
```

### Mixing string keys and structured keys accidentally

This direct API key:

```cpp
kv.put("users/1/name", "Ada");
```

is not the same API style as:

```cpp
kv.set({"users", "1", "name"}, "Ada");
```

Use one style consistently in the same part of your app.

### Using empty keys

Avoid empty key paths.

```cpp
vix::kv::KeyPath key{};
```

A useful key has at least one segment.

```cpp
vix::kv::KeyPath key{"settings", "theme"};
```

### Forgetting that deletes use the same key path

If the value was written with:

```cpp
kv.set({"session", "token"}, "abc123");
```

Delete it with:

```cpp
kv.erase(vix::kv::KeyPath{"session", "token"});
```

### Expecting deleted keys in list results

Deleted keys are not returned as live entries.

```cpp
auto entries = kv.list({"users"});
```

Use stats to inspect tombstones.

```cpp
const auto stats = kv.stats();

vix::print("tombstones =", stats.tombstone_count);
```

## Best practices

Use plural resource names for grouped data.

```cpp
{"users", "1", "name"}
{"orders", "1001", "status"}
{"sessions", "abc123", "user_id"}
```

Keep keys stable. Avoid changing key shapes between versions unless you also migrate old data.

Use structured keys when you need prefix listing. Use direct string keys for very small examples or simple standalone values.

## Related pages

| Page | Purpose |
|---|---|
| [Opening a database](/modules/kv/opening) | Learn how to open memory and durable stores. |
| [Values](/modules/kv/values) | Learn how to write, read, and delete values. |
| [Persistence](/modules/kv/persistence) | Learn how keys and values survive restart. |
| [Recovery](/modules/kv/recovery) | Learn how deletes and WAL recovery work. |
| [Stats](/modules/kv/stats) | Learn how to inspect live keys and tombstones. |

## Next step

Continue with [persistence](/modules/kv/persistence).
