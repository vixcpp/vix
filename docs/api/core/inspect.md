# Inspect

The `inspect` API provides deep, structured inspection tools for C++ values.

It is designed for debugging, diagnostics, type exploration, container analysis, and development-time visibility.

```cpp
#include <vix/inspect.hpp>

int main()
{
  vix::inspect(42);
  vix::inspect(std::vector<int>{1, 2, 3});
  vix::inspect_type<std::vector<int>>();

  return 0;
}
```

## Header

```cpp
#include <vix/inspect.hpp>
```

Some projects may expose the API through a broader Vix include:

```cpp
#include <vix.hpp>
```

For direct usage, prefer:

```cpp
#include <vix/inspect.hpp>
```

## What inspect does

`vix::inspect` renders a value with debugging-oriented information.

Compared to `vix::print`, inspect is more diagnostic.

Use `print` when you want simple output:

```cpp
vix::print("user", 42);
```

Use `inspect` when you want to understand a value, a type, a container, a struct, or a runtime state:

```cpp
vix::inspect(std::vector<int>{1, 2, 3});
vix::inspect_type<std::vector<int>>();
vix::inspect_container(std::vector<int>{1, 2, 3});
```

## Basic usage

```cpp
#include <vix/inspect.hpp>
#include <vector>
#include <string>

int main()
{
  vix::inspect(42);
  vix::inspect(true);
  vix::inspect(std::string{"hello"});
  vix::inspect(std::vector<int>{1, 2, 3});

  return 0;
}
```

Output shape:

```txt
42
true
"hello" [len=5]
[1, 2, 3] [n=3]
```

The exact output can vary depending on options such as `show_type`, `compact`, and `max_items`.

## Inspect vs print

| API | Purpose |
| --- | --- |
| `vix::print` | Simple readable output. |
| `vix::inspect` | Debugging-oriented value inspection. |
| `vix::inspect_type` | Type metadata inspection. |
| `vix::inspect_meta` | Value plus type metadata. |
| `vix::inspect_tree` | Tree view for nested structures. |
| `vix::inspect_bytes` | Raw byte dump for trivially copyable values. |
| `vix::inspect_numeric` | Numeric range statistics. |

Use `inspect` while debugging.

Use `print` for simple developer output.

Use `log` for production logs.

## Public API overview

| API | Purpose |
| --- | --- |
| `vix::inspect(value)` | Inspect a value to stdout. |
| `vix::inspect(value, opts)` | Inspect a value with custom options. |
| `vix::inspect_to(os, value)` | Inspect a value to a specific stream. |
| `vix::inspect_to_string(value)` | Return inspection output as a string. |
| `vix::inspect_type<T>()` | Inspect type metadata for `T`. |
| `vix::inspect_type(value)` | Inspect the deduced type of a value. |
| `vix::inspect_line(...)` | Inspect multiple values on one line. |
| `vix::inspect_value(label, value)` | Inspect a named value. |
| `vix::inspect_all(...)` | Inspect multiple values, one per line. |
| `vix::inspect_meta(value)` | Inspect value and full type metadata. |
| `vix::inspect_compact(value)` | Return compact inspection string. |
| `vix::inspect_verbose(value)` | Inspect with type tags, metadata, and addresses. |
| `vix::inspect_report(obj)` | Detailed report for `field_map` structs. |
| `vix::inspect_container(c)` | Container size, capacity, value type, elements. |
| `vix::inspect_bytes(value)` | Hex dump of raw object bytes. |
| `vix::inspect_diff(a, b)` | Compare two values side by side. |
| `vix::inspect_check(label, expected, actual)` | Print PASS or FAIL check. |
| `vix::inspect_tap(value)` | Inspect and return the same value. |
| `vix::inspect_if(condition, value)` | Inspect only when condition is true. |
| `vix::tap_with(value, fn)` | Run a callback and return the same value. |
| `vix::inspect_numeric(range)` | Numeric range statistics and histogram. |
| `vix::inspect_tree(value)` | Visual tree representation. |
| `vix::inspect_path<T>()` | Return the selected inspection path. |
| `vix::inspect_paths<Ts...>()` | Print inspection paths for many types. |

## `inspect_options`

`inspect_options` controls how inspection is rendered.

```cpp
vix::inspect_options opts;
opts.show_type = true;
opts.max_depth = 4;
opts.max_items = 10;

vix::inspect(std::vector<int>{1, 2, 3}, opts);
```

### Fields

| Field | Default | Purpose |
| --- | --- | --- |
| `max_depth` | `8` | Maximum recursive nesting depth. |
| `max_items` | `64` | Maximum items per container. |
| `show_type` | `true` | Show type annotations when supported. |
| `show_meta` | `false` | Enable full metadata mode for verbose use. |
| `compact` | `false` | Use compact single-line output. |
| `show_address` | `false` | Show or follow object addresses when supported. |
| `indent_str` | `"  "` | Indentation unit. |
| `out` | `&std::cout` | Output stream. |

## Default options

`default_options()` returns the thread-local default inspection options.

```cpp
auto &opts = vix::default_options();

opts.show_type = true;
opts.max_items = 20;
opts.max_depth = 5;

vix::inspect(std::vector<int>{1, 2, 3});
```

Because the default options are thread-local, changing them affects the current thread.

## Scoped options

Use `scoped_inspect_options` to temporarily override options.

```cpp
vix::inspect(std::vector<int>{1, 2, 3});

{
  vix::scoped_inspect_options guard{};
  guard.opts.compact = true;
  guard.opts.show_type = false;

  vix::inspect(std::vector<int>{1, 2, 3});
}

vix::inspect(std::vector<int>{1, 2, 3});
```

When the guard is destroyed, the previous options are restored.

## Inspect a value

```cpp
vix::inspect(42);
vix::inspect("hello");
vix::inspect(std::vector<int>{1, 2, 3});
```

## Inspect with custom options

```cpp
vix::inspect_options opts;
opts.show_type = false;
opts.compact = true;
opts.max_items = 3;

vix::inspect(std::vector<int>{1, 2, 3, 4, 5}, opts);
```

Output shape:

```txt
[1, 2, 3, ...]
```

## Inspect to a stream

```cpp
std::ostringstream out;

vix::inspect_to(out, std::vector<int>{1, 2, 3});

std::string result = out.str();
```

## Inspect to string

```cpp
std::string s = vix::inspect_to_string(std::vector<int>{1, 2, 3});

vix::print(s);
```

## Compact inspection

`inspect_compact` returns a compact string.

```cpp
auto s = vix::inspect_compact(
  std::map<std::string, int>{{"a", 1}, {"b", 2}}
);

vix::print(s);
```

Output shape:

```txt
{a: 1, b: 2}
```

## Inspect several values on one line

```cpp
int a = 1;
int b = 2;
int c = 3;

vix::inspect_line(a, b, c);
```

Output shape:

```txt
1 | 2 | 3
```

## Inspect named values

```cpp
int batch_size = 64;

vix::inspect_value("batch_size", batch_size);
```

Output:

```txt
batch_size: 64
```

## Inspect multiple values

```cpp
vix::inspect_all(
  std::string{"hello"},
  42,
  3.14,
  std::vector<int>{1, 2, 3}
);
```

Each value is inspected on its own line.

## Supported types

`inspect` supports the same broad type categories as the Vix rendering engine, with extra diagnostic information.

| Type category | Examples |
| --- | --- |
| Booleans | `bool` |
| Characters | `char`, `wchar_t`, `char8_t`, `char16_t`, `char32_t` |
| Strings | `std::string`, `std::string_view`, `const char *` |
| Wide strings | `std::wstring`, `std::wstring_view`, `const wchar_t *` |
| Numbers | `int`, `double`, `std::size_t`, etc. |
| Null | `nullptr` |
| Enums | `enum`, `enum class` |
| Filesystem | `std::filesystem::path` |
| Optional | `std::optional<T>` |
| Variant | `std::variant<Ts...>` |
| Any | `std::any` |
| Reference wrapper | `std::reference_wrapper<T>` |
| Chrono duration | `std::chrono::seconds`, milliseconds, etc. |
| Chrono time point | `std::chrono::time_point` |
| Smart pointers | `std::unique_ptr`, `std::shared_ptr`, `std::weak_ptr` |
| Raw pointers | `T *` |
| Function pointers | function pointer values |
| Ranges | `std::vector`, `std::list`, `std::deque`, etc. |
| Sets | `std::set`, `std::unordered_set`, etc. |
| Maps | `std::map`, `std::unordered_map`, etc. |
| Container adapters | `std::stack`, `std::queue`, `std::priority_queue` |
| Tuple-like values | `std::tuple`, `std::pair`, `std::array` |
| Error types | `std::error_code`, `std::error_condition` |
| Byte | `std::byte` |
| Monostate | `std::monostate` |
| Streamable types | Any type with `operator<<` |
| Custom inspected types | `vix::inspector<T>`, `vix_inspect`, or `field_map<T>` |

If C++23 `std::expected` is available, `std::expected<T, E>` is also supported.

## Strings

`inspect` shows strings with quotes and length information when type display is enabled.

```cpp
vix::inspect(std::string{"hello"});
```

Output shape:

```txt
"hello" [len=5]
```

## Booleans

```cpp
vix::inspect(true);
vix::inspect(false);
```

Output:

```txt
true
false
```

## Characters

```cpp
vix::inspect('A');
```

Output:

```txt
'A'
```

Unsigned character values may be shown as hexadecimal.

## Null values

```cpp
vix::inspect(nullptr);
```

Output:

```txt
nullptr
```

## Vectors and sequences

```cpp
vix::inspect(std::vector<int>{1, 2, 3});
```

Output shape:

```txt
[1, 2, 3] [n=3]
```

## Maps

```cpp
std::map<std::string, int> values = {
  {"users", 120},
  {"orders", 42}
};

vix::inspect(values);
```

Output shape:

```txt
{"orders": 42, "users": 120} [n=2]
```

The exact ordering depends on the container.

## Optionals

```cpp
std::optional<int> a = 42;
std::optional<int> b;

vix::inspect(a);
vix::inspect(b);
```

Output:

```txt
Some(42)
None<int>
```

## Variants

```cpp
std::variant<int, std::string> value = std::string{"active"};

vix::inspect(value);
```

Output shape:

```txt
variant<index=1>("active" [len=6])
```

## Any

```cpp
std::any a = 42;
std::any empty;

vix::inspect(a);
vix::inspect(empty);
```

Output shape:

```txt
any(int)
any(empty)
```

`std::any` shows the stored type name, not the stored value.

## Filesystem paths

```cpp
std::filesystem::path path = "src/main.cpp";

vix::inspect(path);
```

Output shape:

```txt
path("src/main.cpp") [exists=true]
```

The `exists` value depends on the filesystem.

## Chrono durations

```cpp
using namespace std::chrono;

vix::inspect(42ns);
vix::inspect(100us);
vix::inspect(1500ms);
vix::inspect(60s);
vix::inspect(2min);
vix::inspect(1h);
```

Output shape:

```txt
42ns
100µs
1500ms
60s
2min
1h
```

## Chrono time points

```cpp
auto now = std::chrono::system_clock::now();

vix::inspect(now);
```

Output shape:

```txt
<time_point: 1710000000000ms from epoch>
```

## Smart pointers

```cpp
auto value = std::make_unique<int>(42);
std::unique_ptr<int> empty;

vix::inspect(value);
vix::inspect(empty);
```

Output:

```txt
unique_ptr(42)
unique_ptr(null)
```

Shared pointers show the use count:

```cpp
auto shared = std::make_shared<std::string>("hello");

vix::inspect(shared);
```

Output shape:

```txt
shared_ptr[use_count=1]("hello" [len=5])
```

Weak pointers show whether they are expired or lockable.

```cpp
std::weak_ptr<std::string> weak = shared;

vix::inspect(weak);
```

Output shape:

```txt
weak_ptr[use_count=2]("hello" [len=5])
```

## Raw pointers

```cpp
int value = 42;
int *ptr = &value;

vix::inspect(ptr);
```

Output shape:

```txt
<ptr:0x...>
```

By default, raw pointers show their address.

To inspect the pointed-to value, enable `show_address`:

```cpp
vix::inspect_options opts;
opts.show_address = true;

vix::inspect(ptr, opts);
```

Output shape:

```txt
<ptr:0x...> -> 42
```

## Function pointers

```cpp
void (*fn)(int) = [](int) {};

vix::inspect(fn);
```

Output shape:

```txt
<fptr:0x...>
```

## Container adapters

Container adapters are copied before inspection, so the original container is not modified.

### Stack

```cpp
std::stack<int> s;
s.push(1);
s.push(2);
s.push(3);

vix::inspect(s);
```

Output shape:

```txt
stack[1, 2, 3]
```

### Queue

```cpp
std::queue<std::string> q;
q.push("first");
q.push("second");

vix::inspect(q);
```

Output shape:

```txt
queue["first" [len=5], "second" [len=6]]
```

### Priority queue

```cpp
std::priority_queue<int> pq;
pq.push(5);
pq.push(1);
pq.push(4);

vix::inspect(pq);
```

Output shape:

```txt
priority_queue[5, 4, 1]
```

## Type metadata

`inspect_type<T>()` prints a detailed report about a type.

```cpp
vix::inspect_type<int>();
vix::inspect_type<std::vector<int>>();
```

Output shape:

```txt
type_info {
  name: vector<int>
  full_name: std::vector<int, std::allocator<int> >
  size: 24 bytes
  align: 8 bytes
  categories: [class]
  traits: [...]
}
```

## Inspect the type of a value

```cpp
std::vector<int> values = {1, 2, 3};

vix::inspect_type(values);
```

This deduces the type from the value.

## Type metadata structure

Internally, Vix builds a `type_metadata` object.

It includes:

| Field group | Examples |
| --- | --- |
| Type names | `name`, `full_name` |
| Memory layout | `size_bytes`, `align_bytes` |
| Categories | `is_integral`, `is_class`, `is_enum`, `is_pointer`, etc. |
| Qualifiers | `is_const`, `is_volatile` |
| Class traits | `is_aggregate`, `is_standard_layout`, `is_trivially_copyable`, etc. |
| Construction traits | `is_default_constructible`, `is_copy_constructible`, etc. |
| Sign traits | `is_signed`, `is_unsigned` |

You can build metadata manually:

```cpp
auto meta = vix::make_type_metadata<std::vector<int>>();
```

## Inspect value plus metadata

Use `inspect_meta` to show both the value and the type report.

```cpp
std::vector<int> values = {1, 2, 3};

vix::inspect_meta(values);
```

Output shape:

```txt
value: [1, 2, 3] [n=3]
type_info {
  name: vector<int>
  size: 24 bytes
  align: 8 bytes
  categories: [class]
  traits: [...]
}
```

## Verbose inspection

`inspect_verbose` enables type tags, metadata mode, and address display.

```cpp
int value = 42;
int *ptr = &value;

vix::inspect_verbose(ptr);
```

Use it when normal inspection is not enough.

## Inspect containers

`inspect_container` prints container-specific metadata.

```cpp
std::vector<int> values = {1, 2, 3};

vix::inspect_container(values);
```

Output shape:

```txt
vector<int> {
  size:    3
  capacity: 3
  max_size: ...
  empty:    false
  value_type: int
  value_size: 4 bytes
  elements: [1, 2, 3]
}
```

For unordered containers, it can also show bucket information when available.

## Inspect raw bytes

`inspect_bytes` prints a hexadecimal byte dump.

It only works for trivially copyable types.

```cpp
int value = 42;

vix::inspect_bytes(value, "answer");
```

Output shape:

```txt
answer - int [4 bytes @ 0x...]:
  2A 00 00 00
```

The byte order depends on the machine architecture.

## Inspect two values

Use `inspect_diff` to compare two values side by side.

```cpp
vix::inspect_diff(42, 43, "expected", "actual");
```

Output:

```txt
expected: 42
actual: 43
types_equal: true
values_equal: false
```

It prints `values_equal` only when `operator==` is available.

## Inspect checks

`inspect_check` prints a PASS or FAIL result.

```cpp
vix::inspect_check("2 + 2 == 4", 4, 2 + 2);
vix::inspect_check("1 + 1 == 3", 3, 1 + 1);
```

Output shape:

```txt
[PASS] 2 + 2 == 4
[FAIL] 1 + 1 == 3
  expected: 3
  actual:   2
```

It also returns a bool.

```cpp
bool ok = vix::inspect_check("sum", 10, 5 + 5);
```

## Inspect tap

`inspect_tap` inspects a value and returns it unchanged.

```cpp
int value = vix::inspect_tap(2 + 3, "sum");

vix::print(value);
```

Output shape:

```txt
sum: 5
5
```

Use this to inspect intermediate values without breaking expression flow.

## Conditional inspect

```cpp
int value = 42;

vix::inspect_if(value > 10, value, "value");
```

This only prints when the condition is true.

## Tap with custom function

```cpp
auto value = vix::tap_with(42, [](const auto &x) {
  vix::inspect_value("intermediate", x);
});
```

`tap_with` calls the function, then returns the original value.

## Inspect numeric ranges

`inspect_numeric` prints statistics for arithmetic ranges.

```cpp
std::vector<int> scores = {10, 20, 30, 40, 50};

vix::inspect_numeric(scores, "scores");
```

Output shape:

```txt
scores:
  count:    5
  min:      10
  max:      50
  sum:      150
  mean:     30
  stddev:   14.1421
  histogram:
    [...]
```

For small or simple ranges, the histogram may not be printed.

## Inspect a tree

`inspect_tree` prints nested structures as a visual tree.

```cpp
std::map<std::string, std::vector<int>> data = {
  {"a", {1, 2, 3}},
  {"b", {4, 5, 6}}
};

vix::inspect_tree(data, "data");
```

Output shape:

```txt
└─ data [2 items]
   ├─ a [3 items]
   │  ├─ [0]: 1
   │  ├─ [1]: 2
   │  └─ [2]: 3
   └─ b [3 items]
      ├─ [0]: 4
      ├─ [1]: 5
      └─ [2]: 6
```

Use `inspect_tree` for nested maps, vectors, optionals, variants, and structs registered with `field_map`.

## Custom type support

There are three main ways to teach `inspect` about custom types.

1. Define `operator<<`
2. Define an ADL `vix_inspect` hook
3. Specialize `vix::inspector<T>`
4. Register fields with `vix::field_map<T>`

`field_map` is the most structured option.

## Method 1: use `operator<<`

```cpp
#include <ostream>
#include <string>
#include <vix/inspect.hpp>

struct User
{
  int id{};
  std::string name;
};

std::ostream &operator<<(std::ostream &os, const User &user)
{
  return os << "User{id=" << user.id << ", name=" << user.name << "}";
}

int main()
{
  User user{1, "Ada"};

  vix::inspect(user);

  return 0;
}
```

Use this when stream output is useful outside Vix too.

## Method 2: use ADL `vix_inspect`

Define `vix_inspect` in the same namespace as the type.

```cpp
#include <vix/inspect.hpp>

namespace app
{
  struct Vec2
  {
    float x{};
    float y{};
  };

  void vix_inspect(vix::inspect_context &ctx, const Vec2 &v)
  {
    ctx.os << "Vec2{x=" << v.x << ", y=" << v.y << "}";
  }
}

int main()
{
  app::Vec2 v{1.5f, 2.5f};

  vix::inspect(v);

  return 0;
}
```

Use this when you want Vix-specific inspection without specializing templates.

## Method 3: specialize `vix::inspector<T>`

```cpp
#include <string>
#include <vix/inspect.hpp>

struct NamedResult
{
  std::string label;
  double value{};
};

template <>
struct vix::inspector<NamedResult>
{
  static void inspect(vix::inspect_context &ctx, const NamedResult &result)
  {
    ctx.os << result.label << '=' << result.value;

    if (ctx.opts.show_type)
      ctx.os << " [NamedResult]";
  }
};

int main()
{
  vix::inspect(NamedResult{"loss", 0.42});

  return 0;
}
```

Use this when you want full control over inspection.

## Method 4: register fields with `field_map`

`field_map` gives Vix a lightweight reflection-like way to inspect struct fields.

```cpp
#include <string>
#include <vector>
#include <optional>
#include <vix/inspect.hpp>

struct OptimizerConfig
{
  std::string name;
  int batch_size{};
  double learning_rate{};
  bool use_cache{};
  std::vector<int> layer_sizes;
  std::optional<double> dropout;
};

template <>
struct vix::field_map<OptimizerConfig>
{
  static constexpr auto fields()
  {
    return vix::fields(
      vix::field("name", &OptimizerConfig::name),
      vix::field("batch_size", &OptimizerConfig::batch_size),
      vix::field("learning_rate", &OptimizerConfig::learning_rate),
      vix::field("use_cache", &OptimizerConfig::use_cache),
      vix::field("layer_sizes", &OptimizerConfig::layer_sizes),
      vix::field("dropout", &OptimizerConfig::dropout)
    );
  }
};

int main()
{
  OptimizerConfig cfg{
    "adam",
    64,
    0.001,
    true,
    {128, 64, 32},
    0.1
  };

  vix::inspect(cfg);
  vix::inspect_report(cfg);
  vix::inspect_tree(cfg, "config");

  return 0;
}
```

Output shape:

```txt
OptimizerConfig {
  name: "adam" [len=4],
  batch_size: 64,
  learning_rate: 0.001,
  use_cache: true,
  layer_sizes: [128, 64, 32] [n=3],
  dropout: Some(0.1)
}
```

## Field descriptors

`field` creates a field descriptor:

```cpp
vix::field("name", &User::name)
```

`fields` groups descriptors into a tuple:

```cpp
return vix::fields(
  vix::field("id", &User::id),
  vix::field("name", &User::name)
);
```

This is used by `field_map<T>::fields()`.

## Inspect reports for structs

`inspect_report` works for types that have a `field_map`.

```cpp
vix::inspect_report(cfg);
```

Output shape:

```txt
══ Inspect Report: OptimizerConfig ══
  sizeof:  ...
  alignof: ...
  layout:  standard
  trivial: no
  fields:
    name: "adam" [len=4]
    batch_size: 64
    learning_rate: 0.001
    use_cache: true
    layer_sizes: [128, 64, 32] [n=3]
    dropout: Some(0.1)
══════════════════════════════
```

## Inspection priority

When Vix inspects a value, it chooses the first matching path.

Priority:

1. `vix::inspector<T>` specialization
2. ADL `vix_inspect`
3. `field_map<T>`
4. `nullptr_t`
5. `bool`
6. character types
7. wide strings
8. string-like types
9. `std::filesystem::path`
10. `std::any`
11. `std::optional`
12. `std::variant`
13. `std::reference_wrapper`
14. chrono duration
15. chrono time point
16. `std::unique_ptr`
17. `std::shared_ptr`
18. `std::weak_ptr`
19. container adapters
20. `std::expected`, when available
21. enum
22. map-like range
23. sequence range
24. pair
25. tuple-like
26. raw pointer
27. streamable via `operator<<`
28. fallback uninspectable type

## Inspect the selected path

Use `inspect_path<T>()` to see which path will be used.

```cpp
vix::print(vix::inspect_path<int>());
vix::print(vix::inspect_path<std::vector<int>>());
vix::print(vix::inspect_path<std::map<std::string, int>>());
```

Output shape:

```txt
bool
range
map-like range
```

Print many paths:

```cpp
vix::inspect_paths<
  int,
  std::string,
  std::vector<int>,
  std::map<std::string, int>,
  std::optional<int>
>();
```

## Demangled type names

The inspect API includes helpers under `vix::demangle`.

```cpp
std::string full = vix::demangle::type_name<std::vector<int>>();
std::string short_name = vix::demangle::short_type_name<std::vector<int>>();
```

Useful helpers:

| API | Purpose |
| --- | --- |
| `vix::demangle::type_name<T>()` | Full human-readable type name when supported. |
| `vix::demangle::type_name_of(value)` | Type name of a deduced value. |
| `vix::demangle::shorten(name)` | Remove common namespace noise. |
| `vix::demangle::short_type_name<T>()` | Shortened type name. |

On GCC and Clang, Vix uses ABI demangling when available. On other compilers, it falls back to `typeid().name()`.

## Complete example

```cpp
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>
#include <vix/inspect.hpp>
#include <vix/print.hpp>

struct User
{
  int id{};
  std::string name;
  std::vector<std::string> roles;
  std::optional<double> score;
};

template <>
struct vix::field_map<User>
{
  static constexpr auto fields()
  {
    return vix::fields(
      vix::field("id", &User::id),
      vix::field("name", &User::name),
      vix::field("roles", &User::roles),
      vix::field("score", &User::score)
    );
  }
};

int main()
{
  User user{
    1,
    "Ada",
    {"admin", "developer"},
    98.5
  };

  vix::inspect(user);
  vix::inspect_report(user);
  vix::inspect_tree(user, "user");
  vix::inspect_type<User>();
  vix::inspect_meta(user);

  std::vector<int> values = {10, 20, 30, 40, 50};
  vix::inspect_container(values);
  vix::inspect_numeric(values, "values");

  vix::inspect_check("user id", 1, user.id);

  std::string compact = vix::inspect_compact(user);
  vix::print_named("compact", compact);

  return 0;
}
```

## Use in a Vix app

```cpp
#include <vix.hpp>
#include <vix/inspect.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/debug", [](Request &req, Response &res) {
    vix::inspect_value("path", req.path());
    vix::inspect_value("query", req.query());

    res.json({
      "ok", true,
      "message", "debug inspected"
    });
  });

  app.run(8080);

  return 0;
}
```

Use this during local development.

For production logs, prefer:

```cpp
#include <vix/log.hpp>

vix::log::info("debug route called");
```

## Common mistakes

### Forgetting the header

```cpp
// Wrong
vix::inspect(value);
```

Fix:

```cpp
#include <vix/inspect.hpp>
```

### Printing instead of inspecting

Use `print` for simple output:

```cpp
vix::print("hello", 42);
```

Use `inspect` for debugging values:

```cpp
vix::inspect(std::vector<int>{1, 2, 3});
```

### Inspecting huge containers

```cpp
std::vector<int> values(1'000'000);

vix::inspect(values);
```

Better:

```cpp
vix::inspect_options opts;
opts.max_items = 20;

vix::inspect(values, opts);
```

### Recursing too deeply

For nested structures, set `max_depth`.

```cpp
vix::inspect_options opts;
opts.max_depth = 3;

vix::inspect(data, opts);
```

### Expecting `std::any` to show the stored value

```cpp
std::any value = 42;

vix::inspect(value);
```

Output shape:

```txt
any(int)
```

`std::any` shows the stored type name, not the stored value.

### Expecting raw pointers to dereference by default

```cpp
int value = 42;
int *ptr = &value;

vix::inspect(ptr);
```

Output shape:

```txt
<ptr:0x...>
```

Enable `show_address` if you want pointer dereference inspection when possible.

### Using `inspect_bytes` on non-trivially-copyable types

`inspect_bytes` requires a trivially copyable type.

Good:

```cpp
int x = 42;
vix::inspect_bytes(x);
```

Not good:

```cpp
std::string s = "hello";
// vix::inspect_bytes(s); // not intended
```

### Using inspect as production logging

This is useful during development:

```cpp
vix::inspect(user);
```

For production logs, use:

```cpp
vix::log::info("user loaded");
```

## Best practices

Use `inspect` while developing:

```cpp
vix::inspect(value);
```

Use `inspect_value` for named values:

```cpp
vix::inspect_value("config", config);
```

Use `inspect_type` when learning or debugging template types:

```cpp
vix::inspect_type<decltype(value)>();
```

Use `field_map` for important structs:

```cpp
template <>
struct vix::field_map<MyStruct>
{
  static constexpr auto fields()
  {
    return vix::fields(
      vix::field("id", &MyStruct::id),
      vix::field("name", &MyStruct::name)
    );
  }
};
```

Use `inspect_report` for structured objects.

Use `inspect_tree` for nested data.

Use `inspect_numeric` for numeric ranges.

Use `inspect_check` for lightweight debug checks.

## API reference

### `inspect`

```cpp
template <typename T>
void inspect(const T &value);
```

Inspects a value to the default output stream.

```cpp
vix::inspect(42);
```

### `inspect` with options

```cpp
template <typename T>
void inspect(const T &value, const inspect_options &opts);
```

Inspects a value with explicit options.

```cpp
vix::inspect_options opts;
opts.compact = true;

vix::inspect(value, opts);
```

### `inspect_to`

```cpp
template <typename T>
void inspect_to(std::ostream &os, const T &value);
```

Inspects a value to a specific stream.

```cpp
std::ostringstream out;
vix::inspect_to(out, value);
```

### `inspect_to_string`

```cpp
template <typename T>
std::string inspect_to_string(const T &value);

template <typename T>
std::string inspect_to_string(const T &value, const inspect_options &opts);
```

Returns inspection output as a string.

```cpp
std::string s = vix::inspect_to_string(value);
```

### `inspect_type`

```cpp
template <typename T>
void inspect_type();

template <typename T>
void inspect_type(const T &value);
```

Prints type metadata.

```cpp
vix::inspect_type<std::vector<int>>();

std::vector<int> values;
vix::inspect_type(values);
```

### `inspect_line`

```cpp
template <typename... Args>
void inspect_line(const Args &...args);
```

Inspects multiple values on a single line separated by `|`.

```cpp
vix::inspect_line(1, 2, 3);
```

### `inspect_value`

```cpp
template <typename T>
void inspect_value(std::string_view label, const T &value);
```

Inspects a named value.

```cpp
vix::inspect_value("port", 8080);
```

### `inspect_all`

```cpp
template <typename... Args>
void inspect_all(const Args &...args);
```

Inspects several values, each on its own line.

```cpp
vix::inspect_all("hello", 42, std::vector<int>{1, 2});
```

### `inspect_meta`

```cpp
template <typename T>
void inspect_meta(const T &value);
```

Prints the value and its full type metadata.

```cpp
vix::inspect_meta(value);
```

### `inspect_compact`

```cpp
template <typename T>
std::string inspect_compact(const T &value);
```

Returns a compact inspection string.

```cpp
std::string s = vix::inspect_compact(value);
```

### `inspect_verbose`

```cpp
template <typename T>
void inspect_verbose(const T &value);
```

Inspects with verbose settings.

```cpp
vix::inspect_verbose(value);
```

### `inspect_report`

```cpp
template <typename T>
void inspect_report(const T &obj);
```

Requires `field_map<T>`.

Prints a detailed report for a struct.

```cpp
vix::inspect_report(config);
```

### `inspect_container`

```cpp
template <typename T>
void inspect_container(const T &c);
```

Requires a range.

Prints container metadata and elements.

```cpp
vix::inspect_container(std::vector<int>{1, 2, 3});
```

### `inspect_bytes`

```cpp
template <typename T>
void inspect_bytes(const T &value, std::string_view label = "");
```

Requires a trivially copyable type.

Prints a byte dump.

```cpp
int value = 42;
vix::inspect_bytes(value, "value");
```

### `inspect_diff`

```cpp
template <typename A, typename B>
void inspect_diff(
  const A &a,
  const B &b,
  std::string_view label_a = "a",
  std::string_view label_b = "b"
);
```

Compares two values side by side.

```cpp
vix::inspect_diff(42, 43, "expected", "actual");
```

### `inspect_check`

```cpp
template <typename Expected, typename Actual>
bool inspect_check(
  std::string_view label,
  const Expected &expected,
  const Actual &actual
);
```

Prints PASS or FAIL and returns bool.

```cpp
bool ok = vix::inspect_check("sum", 4, 2 + 2);
```

### `inspect_tap`

```cpp
template <typename T>
const T &inspect_tap(const T &value, std::string_view label = "");
```

Inspects and returns the original value.

```cpp
auto result = vix::inspect_tap(compute(), "result");
```

### `inspect_if`

```cpp
template <typename T>
void inspect_if(bool condition, const T &value, std::string_view label = "");
```

Inspects only if the condition is true.

```cpp
vix::inspect_if(debug_enabled, value, "value");
```

### `tap_with`

```cpp
template <typename T, typename Fn>
const T &tap_with(const T &value, Fn &&fn);
```

Runs a callback and returns the original value.

```cpp
auto value = vix::tap_with(compute(), [](const auto &x) {
  vix::inspect_value("x", x);
});
```

### `inspect_numeric`

```cpp
template <typename Range>
void inspect_numeric(const Range &rng, std::string_view label = "");
```

Requires a range of arithmetic values.

Prints numeric statistics.

```cpp
vix::inspect_numeric(std::vector<int>{1, 2, 3}, "values");
```

### `inspect_tree`

```cpp
template <typename T>
void inspect_tree(const T &value, std::string_view root_label = "root");
```

Prints a tree representation.

```cpp
vix::inspect_tree(data, "data");
```

### `inspect_path`

```cpp
template <typename T>
std::string_view inspect_path() noexcept;
```

Returns the selected inspection strategy for a type.

```cpp
auto path = vix::inspect_path<std::vector<int>>();
```

### `inspect_paths`

```cpp
template <typename... Ts>
void inspect_paths();
```

Prints inspection strategies for several types.

```cpp
vix::inspect_paths<int, std::string, std::vector<int>>();
```

### `make_type_metadata`

```cpp
template <typename T>
type_metadata make_type_metadata();
```

Builds type metadata for `T`.

```cpp
auto meta = vix::make_type_metadata<MyType>();
```

### `field`

```cpp
template <typename Owner, typename FieldT>
constexpr auto field(std::string_view name, FieldT Owner::*ptr);
```

Creates a field descriptor.

```cpp
vix::field("id", &User::id)
```

### `fields`

```cpp
template <typename... Fields>
constexpr auto fields(Fields &&...fs);
```

Groups field descriptors.

```cpp
return vix::fields(
  vix::field("id", &User::id),
  vix::field("name", &User::name)
);
```

### `field_map`

```cpp
template <typename T, typename = void>
struct field_map;
```

Specialize this template to register fields for a custom struct.

```cpp
template <>
struct vix::field_map<User>
{
  static constexpr auto fields()
  {
    return vix::fields(
      vix::field("id", &User::id),
      vix::field("name", &User::name)
    );
  }
};
```

### `inspector`

```cpp
template <typename T, typename = void>
struct inspector;
```

Specialize this template to customize inspection.

```cpp
template <>
struct vix::inspector<MyType>
{
  static void inspect(vix::inspect_context &ctx, const MyType &value)
  {
    ctx.os << "...";
  }
};
```

### `streamable_inspector`

```cpp
template <typename T>
struct streamable_inspector;
```

Helper for inspector specializations that delegate to `operator<<`.

```cpp
template <>
struct vix::inspector<MyType> : vix::streamable_inspector<MyType>
{
};
```

### `scoped_inspect_options`

```cpp
class scoped_inspect_options;
```

Temporarily overrides default inspect options and restores them automatically.

```cpp
{
  vix::scoped_inspect_options guard{};
  guard.opts.compact = true;
  vix::inspect(value);
}
```

## Summary

`vix::inspect` is the debugging and diagnostics API for Vix.

Use it when you need to understand:

- values,
- types,
- containers,
- structs,
- nested data,
- memory bytes,
- numeric ranges,
- differences between values,
- selected rendering paths.

Core tools:

```cpp
vix::inspect(value);
vix::inspect_value("name", value);
vix::inspect_type<T>();
vix::inspect_meta(value);
vix::inspect_report(obj);
vix::inspect_container(container);
vix::inspect_bytes(value);
vix::inspect_numeric(values);
vix::inspect_tree(data);
vix::inspect_check("label", expected, actual);
```

For simple output, use `vix::print`.

For production logs, use `vix::log`.

For deep debugging, use `vix::inspect`.
