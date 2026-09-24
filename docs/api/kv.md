# Vix KV API

Vix KV is a small durable key-value engine for Vix applications.

It gives you a simple public API:

```cpp
auto kv = vix::kv::open("data/app");

kv.put("hello", "world");

auto value = kv.get("hello");
```

The goal is simple:

- simple API for application code
- durable storage when a path is used
- memory-only mode for tests and temporary data
- structured keys for nested data
- predictable errors for advanced usage
- optional direct API for fast prototyping

Vix KV is designed for local-first and offline-first systems where local writes must be safe, observable, and recoverable.

## Include

Use the main public header:

```cpp
#include <vix/kv/kv.hpp>
```

This header exposes the public API:

- `vix::kv::Kv`
- `vix::kv::KvOptions`
- `vix::kv::KvValue`
- `vix::kv::KeyPath`
- `vix::kv::KvError`
- `vix::kv::KvErrorCode`
- `vix::kv::KvStats`
- `vix::kv::KvResult<T>`

## Quick example

```cpp
#include <vix/kv/kv.hpp>

#include <iostream>

int main()
{
  auto kv = vix::kv::open("data/app");

  kv.put("hello", "world");

  auto value = kv.get("hello");

  if (value.has_value())
  {
    std::cout << *value << '\n';
  }

  kv.close();

  return 0;
}
```

Output:

```
world
```

## Two API styles

Vix KV exposes two public styles.

The first style is the simple direct API:

```cpp
auto kv = vix::kv::open("data/app");

kv.put("hello", "world");

auto value = kv.get("hello");
```

This style is useful for application code, examples, scripts, and fast prototyping.

The second style is the explicit result API:

```cpp
auto opened = vix::kv::open_memory();

if (opened.is_err())
{
  std::cerr << opened.error().message() << '\n';
  return 1;
}

auto kv = opened.move_value();

auto written = kv.set({"users", "1", "name"}, "Ada");

if (written.is_err())
{
  std::cerr << written.error().message() << '\n';
  return 1;
}

auto value = kv.get({"users", "1", "name"});

if (value.is_ok())
{
  std::cout << value.value().to_string() << '\n';
}
```

This style is useful for libraries, tests, backend systems, and code that needs explicit error handling.

## Opening a database

### Durable database

```cpp
auto kv = vix::kv::open("data/app");
```

This opens a durable KV database at `data/app`.

This direct overload returns a `vix::kv::Kv` handle directly. If opening fails, it throws `std::runtime_error`.

Use it when you want a very simple API:

```cpp
auto kv = vix::kv::open("data/app");

kv.put("name", "Ada");

auto name = kv.get("name");
```

### Durable database with explicit result

```cpp
auto opened = vix::kv::open_durable("data/app");

if (opened.is_err())
{
  std::cerr << opened.error().message() << '\n';
  return 1;
}

auto kv = opened.move_value();
```

This returns:

```
vix::kv::KvResult<vix::kv::Kv>
```

Use it when you do not want exceptions.

### Memory-only database

```cpp
auto opened = vix::kv::open_memory();

if (opened.is_ok())
{
  auto kv = opened.move_value();
}
```

Memory-only mode does not persist data after the handle is closed.

It is useful for:

- unit tests
- temporary data
- examples
- benchmarks
- in-memory application state

Example:

```cpp
auto opened = vix::kv::open_memory();

if (opened.is_err())
{
  return 1;
}

auto kv = opened.move_value();

kv.set({"session", "token"}, "abc123");
```

### Fast durable database

```cpp
auto opened = vix::kv::open_fast("data/cache");
```

Fast mode keeps durable storage enabled, but disables automatic flush.

This is useful when you want to control when data is flushed:

```cpp
auto opened = vix::kv::open_fast("data/cache");

if (opened.is_err())
{
  return 1;
}

auto kv = opened.move_value();

kv.set({"a"}, "one");
kv.set({"b"}, "two");

kv.flush();
```

### Opening with options

Use `KvOptions` when you want full control.

```cpp
auto options = vix::kv::KvOptions::durable("data/app");

options.auto_flush = false;

auto opened = vix::kv::open(options);

if (opened.is_err())
{
  std::cerr << opened.error().message() << '\n';
  return 1;
}

auto kv = opened.move_value();
```

## KvOptions

`KvOptions` describes how the database should be opened.

### Memory-only options

```cpp
auto options = vix::kv::KvOptions::memory_only();
```

Memory-only options create a database that does not write to disk.

```cpp
auto opened = vix::kv::open(options);
```

### Durable options

```cpp
auto options = vix::kv::KvOptions::durable("data/app");
```

Durable options create a database that can recover data after restart.

```cpp
auto opened = vix::kv::open(options);
```

### Fast options

```cpp
auto options = vix::kv::KvOptions::fast("data/cache");
```

Fast options use durable storage but disable automatic flush.

```cpp
auto opened = vix::kv::open(options);
```

## Public handle: vix::kv::Kv

`vix::kv::Kv` is the public database handle.

```cpp
vix::kv::Kv kv;
```

Most users should create a handle with:

```cpp
auto kv = vix::kv::open("data/app");
```

or:

```cpp
auto opened = vix::kv::open_memory();
auto kv = opened.move_value();
```

## Simple direct API

The direct API is designed for simple usage.

### put

```cpp
kv.put("hello", "world");
```

Stores a string value.

```cpp
auto kv = vix::kv::open("data/app");

kv.put("hello", "world");
kv.put("users/1/name", "Ada");
```

The direct API throws `std::runtime_error` when the operation fails.

### get

```cpp
auto value = kv.get("hello");
```

Returns:

```
std::optional<std::string>
```

Example:

```cpp
auto value = kv.get("hello");

if (value.has_value())
{
  std::cout << *value << '\n';
}
else
{
  std::cout << "not found\n";
}
```

### Missing key

```cpp
auto value = kv.get("missing");

if (!value.has_value())
{
  std::cout << "missing\n";
}
```

Missing keys return `std::nullopt`.

### Slash keys

The direct API supports slash-separated keys:

```cpp
kv.put("users/1/name", "Ada");
```

This behaves like a structured key:

```cpp
kv.set({"users", "1", "name"}, "Ada");
```

So this works:

```cpp
kv.put("users/1/name", "Ada");

auto value = kv.get("users/1/name");
```

And this also works:

```cpp
auto value = kv.get({"users", "1", "name"});
```

## Explicit result API

The explicit API returns `KvResult<T>`.

It does not throw for normal errors.

### set

```cpp
auto result = kv.set({"hello"}, "world");
```

Returns:

```
vix::kv::KvResult<void>
```

Example:

```cpp
auto result = kv.set({"users", "1", "name"}, "Ada");

if (result.is_err())
{
  std::cerr << result.error().message() << '\n';
}
```

### get

```cpp
auto value = kv.get({"hello"});
```

Returns:

```
vix::kv::KvResult<vix::kv::KvValue>
```

Example:

```cpp
auto value = kv.get({"hello"});

if (value.is_ok())
{
  std::cout << value.value().to_string() << '\n';
}
else
{
  std::cerr << value.error().message() << '\n';
}
```

### erase

```cpp
auto result = kv.erase(vix::kv::KeyPath{"hello"});
```

Returns:

```
vix::kv::KvResult<void>
```

Example:

```cpp
auto result = kv.erase(vix::kv::KeyPath{"users", "1", "name"});

if (result.is_err())
{
  std::cerr << result.error().message() << '\n';
}
```

### contains

```cpp
bool exists = kv.contains({"hello"});
```

Example:

```cpp
if (kv.contains({"users", "1", "name"}))
{
  std::cout << "exists\n";
}
```

### list

```cpp
auto entries = kv.list({"users"});
```

Returns all live entries matching the prefix.

Example:

```cpp
auto entries = kv.list({"users"});

if (entries.is_ok())
{
  for (const auto &[key, value] : entries.value())
  {
    std::cout << key.at(0) << " = "
              << value.to_string()
              << '\n';
  }
}
```

### List all entries

```cpp
auto entries = kv.list();
```

This lists all live entries.

```cpp
auto entries = kv.list();

if (entries.is_ok())
{
  std::cout << entries.value().size() << '\n';
}
```

## Keys

Vix KV supports structured keys.

A key is represented by:

```
vix::kv::KeyPath
```

Example:

```cpp
vix::kv::KeyPath key{"users", "1", "name"};
```

This key has three segments:

- `users`
- `1`
- `name`

### Creating keys

#### Initializer list

```cpp
vix::kv::KeyPath key{"users", "1", "name"};
```

#### Single segment

```cpp
auto key = vix::kv::KeyPath::from("hello");
```

#### Append segments

```cpp
vix::kv::KeyPath key;

key.push_back("users");
key.push_back("1");
key.push_back("name");
```

#### Append another path

```cpp
vix::kv::KeyPath base{"users", "1"};
vix::kv::KeyPath field{"profile", "name"};

base.append(field);
```

Result:

```
users/1/profile/name
```

### Key rules

A valid key should:

- contain at least one segment
- not contain empty segments
- not exceed the configured key limits
- be valid according to the key validator

Example of an invalid key:

```cpp
vix::kv::KeyPath key{};
```

Example of another invalid key:

```cpp
vix::kv::KeyPath key{"users", "", "name"};
```

## Values

A value is represented by:

```
vix::kv::KvValue
```

The simple API uses strings:

```cpp
kv.put("hello", "world");
```

The explicit API uses `KvValue` internally:

```cpp
auto value = vix::kv::KvValue::from_string("world");

kv.set({"hello"}, value);
```

### Creating values

#### From string

```cpp
auto value = vix::kv::KvValue::from_string("hello");
```

#### Empty value

Empty values are allowed:

```cpp
kv.set({"empty"}, "");
```

Reading it returns an empty string:

```cpp
auto value = kv.get({"empty"});

if (value.is_ok())
{
  std::cout << value.value().to_string() << '\n';
}
```

## Persistence

When you open Vix KV with a path, data is durable.

```cpp
{
  auto kv = vix::kv::open("data/app");

  kv.put("hello", "world");

  kv.flush();

  kv.close();
}

{
  auto kv = vix::kv::open("data/app");

  auto value = kv.get("hello");

  if (value.has_value())
  {
    std::cout << *value << '\n';
  }

  kv.close();
}
```

Output:

```
world
```

## Recovery

Vix KV uses a WAL internally for durable writes.

When a durable database is opened again, Vix KV can recover previous writes from the WAL.

Example:

```cpp
const auto path = "data/app";

{
  auto kv = vix::kv::open(path);

  kv.put("users/1/name", "Ada");
  kv.put("users/2/name", "Grace");

  kv.flush();
  kv.close();
}

{
  auto kv = vix::kv::open(path);

  auto user1 = kv.get("users/1/name");
  auto user2 = kv.get("users/2/name");

  if (user1.has_value())
  {
    std::cout << *user1 << '\n';
  }

  if (user2.has_value())
  {
    std::cout << *user2 << '\n';
  }

  kv.close();
}
```

Output:

```
Ada
Grace
```

### Delete persistence

Deletes are also durable.

```cpp
const auto path = "data/app";

{
  auto kv = vix::kv::open(path);

  kv.put("hello", "world");

  kv.erase(vix::kv::KeyPath{"hello"});

  kv.flush();
  kv.close();
}

{
  auto kv = vix::kv::open(path);

  auto value = kv.get("hello");

  if (!value.has_value())
  {
    std::cout << "deleted\n";
  }

  kv.close();
}
```

Output:

```
deleted
```

### Put after delete

A later write can restore a deleted key.

```cpp
const auto path = "data/app";

{
  auto kv = vix::kv::open(path);

  kv.put("hello", "old");

  kv.erase(vix::kv::KeyPath{"hello"});

  kv.put("hello", "new");

  kv.flush();
  kv.close();
}

{
  auto kv = vix::kv::open(path);

  auto value = kv.get("hello");

  if (value.has_value())
  {
    std::cout << *value << '\n';
  }

  kv.close();
}
```

Output:

```
new
```

## Flush

`flush()` forces pending durable data to be flushed.

```cpp
auto result = kv.flush();

if (result.is_err())
{
  std::cerr << result.error().message() << '\n';
}
```

In memory-only mode, `flush()` succeeds as a no-op.

In durable mode, it flushes durable state.

In fast mode, it is important because automatic flush is disabled.

## Close

`close()` closes the database handle.

```cpp
auto result = kv.close();

if (result.is_err())
{
  std::cerr << result.error().message() << '\n';
}
```

Calling `close()` more than once is safe.

```cpp
kv.close();
kv.close();
```

After a handle is closed, explicit operations return `NotOpen`:

```cpp
auto result = kv.set({"hello"}, "world");

if (result.is_err())
{
  std::cerr << result.error().message() << '\n';
}
```

The direct API throws after close:

```cpp
try
{
  kv.put("hello", "world");
}
catch (const std::runtime_error &error)
{
  std::cerr << error.what() << '\n';
}
```

## Size and empty

### size

```cpp
std::size_t count = kv.size();
```

Returns the number of live keys.

```cpp
kv.put("a", "one");
kv.put("b", "two");

std::cout << kv.size() << '\n';
```

Output:

```
2
```

### empty

```cpp
bool is_empty = kv.empty();
```

Returns `true` when no live key is visible.

```cpp
if (kv.empty())
{
  std::cout << "empty\n";
}
```

## Stats

`stats()` returns runtime information about the database.

```cpp
auto stats = kv.stats();
```

The public type is:

```
vix::kv::KvStats
```

Example:

```cpp
auto stats = kv.stats();

std::cout << "open: " << stats.open << '\n';
std::cout << "memory_only: " << stats.memory_only << '\n';
std::cout << "wal_enabled: " << stats.wal_enabled << '\n';
std::cout << "keys: " << stats.key_count << '\n';
std::cout << "tombstones: " << stats.tombstone_count << '\n';
std::cout << "last_sequence: " << stats.last_sequence << '\n';
```

Useful fields include:

- `stats.open`
- `stats.memory_only`
- `stats.wal_enabled`
- `stats.auto_flush`
- `stats.key_count`
- `stats.tombstone_count`
- `stats.memtable_entries`
- `stats.last_sequence`
- `stats.set_count`
- `stats.get_count`
- `stats.get_miss_count`
- `stats.erase_count`
- `stats.list_count`
- `stats.flush_count`
- `stats.error_count`
- `stats.wal_records_recovered`
- `stats.last_recovered_sequence`

### Stats example

```cpp
auto opened = vix::kv::open_memory();

if (opened.is_err())
{
  return 1;
}

auto kv = opened.move_value();

kv.set({"a"}, "one");
kv.set({"b"}, "two");
kv.erase(vix::kv::KeyPath{"a"});

auto stats = kv.stats();

std::cout << stats.key_count << '\n';
std::cout << stats.tombstone_count << '\n';
std::cout << stats.last_sequence << '\n';
```

Expected behavior:

```
1
1
3
```

The exact values can grow depending on previous operations and implementation details, but the visible state should show one live key and at least one tombstone.

## Error handling

The explicit API uses:

```
vix::kv::KvResult<T>
```

Example:

```cpp
auto result = kv.set({"hello"}, "world");

if (result.is_ok())
{
  std::cout << "written\n";
}

if (result.is_err())
{
  std::cerr << result.error().message() << '\n';
}
```

### KvResult

A successful result:

```cpp
vix::kv::KvResult<int> result =
    vix::kv::KvResult<int>::ok(42);
```

An error result:

```cpp
auto result = vix::kv::KvResult<int>::err(
    vix::kv::KvError::not_found("missing key"));
```

Using a result:

```cpp
if (result.is_ok())
{
  std::cout << result.value() << '\n';
}
else
{
  std::cerr << result.error().message() << '\n';
}
```

Moving a value out:

```cpp
auto value = result.move_value();
```

### Error codes

Public error codes are represented by:

```
vix::kv::KvErrorCode
```

Common error codes:

- `vix::kv::KvErrorCode::Ok`
- `vix::kv::KvErrorCode::InvalidArgument`
- `vix::kv::KvErrorCode::InvalidKey`
- `vix::kv::KvErrorCode::NotFound`
- `vix::kv::KvErrorCode::AlreadyExists`
- `vix::kv::KvErrorCode::AlreadyOpen`
- `vix::kv::KvErrorCode::NotOpen`
- `vix::kv::KvErrorCode::IoError`
- `vix::kv::KvErrorCode::Corruption`
- `vix::kv::KvErrorCode::ChecksumMismatch`
- `vix::kv::KvErrorCode::WalError`
- `vix::kv::KvErrorCode::StorageError`
- `vix::kv::KvErrorCode::SnapshotError`
- `vix::kv::KvErrorCode::CompactionError`
- `vix::kv::KvErrorCode::ConfigError`
- `vix::kv::KvErrorCode::Unsupported`
- `vix::kv::KvErrorCode::InternalError`
- `vix::kv::KvErrorCode::Unknown`

Convert an error code to a stable string:

```cpp
auto text = vix::kv::core::to_string(
    vix::kv::KvErrorCode::NotFound);
```

Output:

```
not_found
```

### Missing keys

With the direct API:

```cpp
auto value = kv.get("missing");

if (!value.has_value())
{
  std::cout << "missing\n";
}
```

With the explicit API:

```cpp
auto value = kv.get({"missing"});

if (value.is_err() &&
    value.error().code() == vix::kv::KvErrorCode::NotFound)
{
  std::cout << "missing\n";
}
```

## Direct API versus explicit API

Use the direct API when you want simple application code:

```cpp
auto kv = vix::kv::open("data/app");

kv.put("hello", "world");

auto value = kv.get("hello");
```

Use the explicit API when you need precise error handling:

```cpp
auto opened = vix::kv::open_durable("data/app");

if (opened.is_err())
{
  return 1;
}

auto kv = opened.move_value();

auto written = kv.set({"hello"}, "world");

if (written.is_err())
{
  return 1;
}
```

## Complete direct API example

```cpp
#include <vix/kv/kv.hpp>

#include <iostream>

int main()
{
  auto kv = vix::kv::open("data/app");

  kv.put("users/1/name", "Ada");
  kv.put("users/2/name", "Grace");

  auto first = kv.get("users/1/name");
  auto second = kv.get("users/2/name");

  if (first.has_value())
  {
    std::cout << "user 1: " << *first << '\n';
  }

  if (second.has_value())
  {
    std::cout << "user 2: " << *second << '\n';
  }

  kv.close();

  return 0;
}
```

Output:

```
user 1: Ada
user 2: Grace
```

## Complete explicit API example

```cpp
#include <vix/kv/kv.hpp>

#include <iostream>

int main()
{
  auto opened = vix::kv::open_memory();

  if (opened.is_err())
  {
    std::cerr << opened.error().message() << '\n';
    return 1;
  }

  auto kv = opened.move_value();

  auto written = kv.set({"users", "1", "name"}, "Ada");

  if (written.is_err())
  {
    std::cerr << written.error().message() << '\n';
    return 1;
  }

  auto value = kv.get({"users", "1", "name"});

  if (value.is_err())
  {
    std::cerr << value.error().message() << '\n';
    return 1;
  }

  std::cout << value.value().to_string() << '\n';

  auto closed = kv.close();

  if (closed.is_err())
  {
    std::cerr << closed.error().message() << '\n';
    return 1;
  }

  return 0;
}
```

Output:

```
Ada
```

## Listing example

```cpp
#include <vix/kv/kv.hpp>

#include <iostream>

int main()
{
  auto opened = vix::kv::open_memory();

  if (opened.is_err())
  {
    return 1;
  }

  auto kv = opened.move_value();

  kv.set({"users", "1", "name"}, "Ada");
  kv.set({"users", "2", "name"}, "Grace");
  kv.set({"settings", "theme"}, "dark");

  auto users = kv.list({"users"});

  if (users.is_err())
  {
    std::cerr << users.error().message() << '\n';
    return 1;
  }

  for (const auto &[key, value] : users.value())
  {
    for (std::size_t index = 0; index < key.size(); ++index)
    {
      if (index > 0)
      {
        std::cout << "/";
      }

      std::cout << key.at(index);
    }

    std::cout << " = " << value.to_string() << '\n';
  }

  kv.close();

  return 0;
}
```

Possible output:

```
users/1/name = Ada
users/2/name = Grace
```

## Persistence example

```cpp
#include <vix/kv/kv.hpp>

#include <iostream>

int main()
{
  const auto path = "data/app";

  {
    auto kv = vix::kv::open(path);

    kv.put("hello", "world");

    kv.flush();
    kv.close();
  }

  {
    auto kv = vix::kv::open(path);

    auto value = kv.get("hello");

    if (value.has_value())
    {
      std::cout << *value << '\n';
    }

    kv.close();
  }

  return 0;
}
```

Output:

```
world
```

## Fast mode example

```cpp
#include <vix/kv/kv.hpp>

#include <iostream>

int main()
{
  auto opened = vix::kv::open_fast("data/cache");

  if (opened.is_err())
  {
    std::cerr << opened.error().message() << '\n';
    return 1;
  }

  auto kv = opened.move_value();

  kv.set({"a"}, "one");
  kv.set({"b"}, "two");

  auto flushed = kv.flush();

  if (flushed.is_err())
  {
    std::cerr << flushed.error().message() << '\n';
    return 1;
  }

  kv.close();

  return 0;
}
```

## API reference

### Open helpers

```cpp
vix::kv::open();
vix::kv::open(std::filesystem::path path);
vix::kv::open(const vix::kv::KvOptions &options);
vix::kv::open_memory();
vix::kv::open_durable(std::filesystem::path path);
vix::kv::open_fast(std::filesystem::path path);
```

### Kv handle

```cpp
kv.put("key", "value");
kv.get("key");

kv.set({"key"}, "value");
kv.get({"key"});
kv.erase(vix::kv::KeyPath{"key"});
kv.contains({"key"});
kv.list({"prefix"});
kv.list();

kv.flush();
kv.close();

kv.size();
kv.empty();
kv.is_open();
kv.options();
kv.stats();
```

### KeyPath

```cpp
vix::kv::KeyPath key{"users", "1", "name"};

key.push_back("profile");
key.append("avatar");

key.size();
key.empty();
key.byte_size();

key.at(0);
key.front();
key.back();
key.segments();

key.clear();
```

### KvValue

```cpp
auto value = vix::kv::KvValue::from_string("hello");

value.to_string();
value.size();
value.empty();
value.bytes();
```

### KvStats

```cpp
auto stats = kv.stats();

stats.open;
stats.memory_only;
stats.wal_enabled;
stats.auto_flush;

stats.key_count;
stats.tombstone_count;
stats.memtable_entries;
stats.memtable_bytes;

stats.last_sequence;
stats.set_count;
stats.get_count;
stats.get_miss_count;
stats.erase_count;
stats.list_count;
stats.flush_count;
stats.error_count;

stats.wal_records_recovered;
stats.last_recovered_sequence;

stats.empty();
stats.has_tombstones();
stats.has_errors();
stats.recovered_from_wal();
stats.write_count();
stats.read_count();
stats.operation_count();
```

## Recommended usage

For application code:

```cpp
auto kv = vix::kv::open("data/app");

kv.put("hello", "world");

auto value = kv.get("hello");
```

For libraries and production systems:

```cpp
auto opened = vix::kv::open_durable("data/app");

if (opened.is_err())
{
  return 1;
}

auto kv = opened.move_value();

auto result = kv.set({"hello"}, "world");

if (result.is_err())
{
  return 1;
}
```

For tests:

```cpp
auto opened = vix::kv::open_memory();

if (opened.is_err())
{
  return 1;
}

auto kv = opened.move_value();
```

For high-throughput durable writes where you want manual flush:

```cpp
auto opened = vix::kv::open_fast("data/cache");

if (opened.is_err())
{
  return 1;
}

auto kv = opened.move_value();

kv.set({"a"}, "one");
kv.set({"b"}, "two");

kv.flush();
```

## Design notes

Vix KV keeps the public API small.

The direct API is intentionally simple:

```cpp
kv.put("hello", "world");
auto value = kv.get("hello");
```

The explicit API is intentionally precise:

```cpp
auto result = kv.set({"hello"}, "world");
```

The same engine supports both styles.

This lets Vix KV work well in small examples, backend applications, tests, and durable local-first systems.
