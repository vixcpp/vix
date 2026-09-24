# Print

The `print` API provides convenient output helpers for Vix applications.

It is designed to make C++ output simple, readable, and useful during development.

```cpp
#include <vix/print.hpp>

int main()
{
  vix::print("Hello from Vix");
  vix::print(42, true, std::vector<int>{1, 2, 3});

  return 0;
}
```

## Header

```cpp
#include <vix/print.hpp>
```

Some examples may include `vix.hpp`, which can expose the common Vix APIs:

```cpp
#include <vix.hpp>
```

For direct usage of the print API, prefer:

```cpp
#include <vix/print.hpp>
```

## What `vix::print` does

`vix::print` writes one or more values to an output stream.

By default:

- values are separated by a space,
- a newline is added at the end,
- output goes to `std::cout`,
- strings are printed as raw strings,
- containers are rendered in a readable form,
- common STL types are handled automatically.

```cpp
vix::print("Hello", "Vix", 2026);
```

Output:

```txt
Hello Vix 2026
```

## Basic usage

```cpp
#include <vix/print.hpp>

int main()
{
  vix::print("Hello from Vix");
  vix::print("version", 1);
  vix::print(true, false);
  vix::print(3.14);

  return 0;
}
```

Output shape:

```txt
Hello from Vix
version 1
true false
3.14
```

## Print a blank line

Calling `vix::print()` with no arguments prints a blank line.

```cpp
vix::print("Before");
vix::print();
vix::print("After");
```

Output:

```txt
Before

After
```

## Multiple values

```cpp
vix::print("user", 42, "active", true);
```

Output:

```txt
user 42 active true
```

By default, arguments are separated by one space.

## `vix::print` vs `std::cout`

With `std::cout`, you write:

```cpp
std::cout << "user " << id << " active " << active << "\n";
```

With `vix::print`, you write:

```cpp
vix::print("user", id, "active", active);
```

`vix::print` is useful when you want concise output and automatic rendering for STL containers and common C++ types.

## Public API overview

| API | Purpose |
| --- | --- |
| `vix::print(...)` | Print values to the default output stream. |
| `vix::print(config, ...)` | Print values using a `print_config`. |
| `vix::print(options, ...)` | Print values using lightweight options. |
| `vix::print_py(...)` | Print with Python-like raw string behavior. |
| `vix::print_to(os, ...)` | Print values to a specific stream. |
| `vix::eprint(...)` | Print values to `std::cerr`. |
| `vix::print_inline(...)` | Print without a trailing newline. |
| `vix::sprint(...)` | Format values into a `std::string`. |
| `vix::to_string(value)` | Format one value into a `std::string`. |
| `vix::write_to(os, value)` | Write one value to a stream. |
| `vix::print_named(label, value)` | Print `label: value`. |
| `vix::print_separator()` | Print a separator line. |
| `vix::print_header(title)` | Print a titled section header. |
| `vix::print_each(range)` | Print each range element on its own line. |
| `vix::print_table(map)` | Print a map-like container as a table. |
| `vix::pprint(value)` | Pretty-print nested structures on multiple lines. |
| `vix::pprint_all(...)` | Pretty-print multiple values. |
| `vix::print_stats(range)` | Print min, max, sum, and average for numeric ranges. |
| `vix::rendering_path<T>()` | Return how a type will be rendered. |
| `vix::print_rendering_paths<Ts...>()` | Print rendering paths for several types. |

## `print_config`

`print_config` controls how `vix::print` renders values.

```cpp
vix::print_config cfg;
cfg.separator = " | ";
cfg.end = "\n";
cfg.out = &std::cout;
cfg.max_items = 256;
cfg.raw_strings = true;

vix::print(cfg, "A", "B", "C");
```

Output:

```txt
A | B | C
```

### Fields

| Field | Default | Purpose |
| --- | --- | --- |
| `separator` | `" "` | Text between multiple arguments. |
| `end` | `"\n"` | Text printed after all arguments. |
| `out` | `&std::cout` | Destination stream. |
| `color` | `false` | ANSI color support, experimental. |
| `max_items` | `256` | Maximum items rendered in a container. |
| `show_type` | `false` | Type annotations, currently reserved. |
| `compact` | `false` | Compact rendering mode, mainly for pretty internals. |
| `indent_str` | `"  "` | Indentation unit. |
| `raw_strings` | `true` | Print strings without quotes by default. |

## `options`

`options` is a lightweight way to configure one print call.

```cpp
vix::print(
  vix::options{
    .sep = ", ",
    .end = ";\n"
  },
  1, 2, 3
);
```

Output:

```txt
1, 2, 3;
```

### Fields

| Field | Default | Purpose |
| --- | --- | --- |
| `sep` | `" "` | Text between arguments. |
| `end` | `"\n"` | Text after the arguments. |
| `file` | `&std::cout` | Output stream. |
| `flush` | `false` | Flush the stream after printing. |
| `raw_strings` | `true` | Print strings without quotes. |
| `max_items` | `256` | Maximum container items. |
| `compact` | `true` | Compact output. |
| `indent` | `"  "` | Indentation unit. |
| `show_type` | `false` | Type annotations, currently reserved. |
| `color` | `false` | ANSI color support, experimental. |

## Custom separator

```cpp
vix::print(vix::options{.sep = ", "}, "red", "green", "blue");
```

Output:

```txt
red, green, blue
```

## Custom line ending

```cpp
vix::print(vix::options{.end = ""}, "loading");
vix::print(" done");
```

Output shape:

```txt
loading done
```

For this specific case, you can also use `print_inline`.

## Print without newline

```cpp
vix::print_inline("Loading...");
vix::print(" done");
```

Output shape:

```txt
Loading... done
```

## Print to stderr

Use `eprint` for errors.

```cpp
vix::eprint("error:", "failed to open file");
```

This writes to `std::cerr`.

## Print to another stream

Use `print_to`.

```cpp
#include <fstream>
#include <vix/print.hpp>

int main()
{
  std::ofstream file("output.txt");

  vix::print_to(file, "Hello", "file");

  return 0;
}
```

## Format to string with `sprint`

`sprint` returns a formatted string without writing to stdout.

```cpp
std::string message = vix::sprint("user", 42, "active", true);

vix::print(message);
```

Output:

```txt
user 42 active true
```

## Format one value with `to_string`

Use `to_string` when you only need one value.

```cpp
std::string s = vix::to_string(std::vector<int>{1, 2, 3});

vix::print(s);
```

Output:

```txt
[1, 2, 3]
```

## Write one value to a stream

Use `write_to` when you want the rendering engine but not the full print behavior.

```cpp
std::ostringstream out;

vix::write_to(out, std::vector<int>{1, 2, 3});

std::string s = out.str();
```

Unlike `print`, `write_to` does not add a separator or newline.

## Named values

```cpp
vix::print_named("port", 8080);
vix::print_named("mode", "development");
```

Output:

```txt
port: 8080
mode: development
```

## Separators and headers

```cpp
vix::print_separator();
vix::print_header("Users");
```

Example:

```cpp
vix::print_header("Startup");
vix::print("server ready");
vix::print_separator();
```

Output shape:

```txt
------------------------------------------------------------
                         Startup
------------------------------------------------------------
server ready
------------------------------------------------------------
```

You can customize the width:

```cpp
vix::print_separator(30);
vix::print_header("Config", 30);
```

## Print each element of a range

```cpp
std::vector<std::string> names = {"Ada", "Bjarne", "Linus"};

vix::print_each(names);
```

Output:

```txt
Ada
Bjarne
Linus
```

With indexes:

```cpp
vix::print_each(names, true);
```

Output:

```txt
[0] Ada
[1] Bjarne
[2] Linus
```

## Print a map as a table

```cpp
std::map<std::string, int> values = {
  {"users", 120},
  {"orders", 42},
  {"errors", 3}
};

vix::print_table(values);
```

Output shape:

```txt
-------------------------------------------
errors              | 3
orders              | 42
users               | 120
-------------------------------------------
```

Custom column width:

```cpp
vix::print_table(values, 12);
```

## Pretty printing with `pprint`

`print` renders most values on one line.

```cpp
std::map<std::string, std::vector<int>> data = {
  {"evens", {2, 4, 6}},
  {"odds", {1, 3, 5}}
};

vix::print(data);
```

Output shape:

```txt
{evens => [2, 4, 6], odds => [1, 3, 5]}
```

Use `pprint` for multi-line nested output.

```cpp
vix::pprint(data);
```

Output shape:

```txt
{
  evens => [
    2
    4
    6
  ]
  odds => [
    1
    3
    5
  ]
}
```

## Pretty-print multiple values

```cpp
vix::pprint_all(
  std::vector<int>{1, 2, 3},
  std::map<std::string, int>{{"a", 1}, {"b", 2}}
);
```

Each value is printed using the pretty-printer.

## Print numeric statistics

`print_stats` works with ranges of arithmetic values.

```cpp
std::vector<int> values = {3, 7, 2, 10};

vix::print_stats(values, "scores");
```

Output shape:

```txt
scores: stats(min=2, max=10, sum=22, avg=5.5)
```

For an empty range:

```cpp
std::vector<int> values;

vix::print_stats(values, "scores");
```

Output:

```txt
scores: stats(empty)
```

## Supported types

The print engine supports many common C++ types automatically.

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
| Custom formatted types | `vix::formatter<T>` or `vix_format` |

If C++23 `std::expected` is available, `std::expected<T, E>` is also supported.

## Strings

By default, strings are printed raw.

```cpp
vix::print("hello");
vix::print(std::string{"world"});
```

Output:

```txt
hello
world
```

If you want quoted strings, use `raw_strings = false`.

```cpp
vix::print(vix::options{.raw_strings = false}, "hello");
```

Output:

```txt
"hello"
```

## Booleans

```cpp
vix::print(true, false);
```

Output:

```txt
true false
```

## Characters

```cpp
vix::print('A');
```

Output:

```txt
'A'
```

Wide and Unicode character types are rendered using escaped forms when needed.

## Null values

```cpp
vix::print(nullptr);
```

Output:

```txt
nullptr
```

## Vectors and sequences

```cpp
vix::print(std::vector<int>{1, 2, 3});
vix::print(std::list<std::string>{"a", "b", "c"});
```

Output shape:

```txt
[1, 2, 3]
[a, b, c]
```

## Sets

```cpp
vix::print(std::set<int>{1, 2, 3});
```

Output shape:

```txt
[1, 2, 3]
```

Sets are rendered like ranges.

## Maps

```cpp
std::map<std::string, int> ages = {
  {"Ada", 36},
  {"Bjarne", 75}
};

vix::print(ages);
```

Output shape:

```txt
{Ada => 36, Bjarne => 75}
```

## Pairs

```cpp
vix::print(std::make_pair("id", 42));
```

Output shape:

```txt
(id: 42)
```

## Tuples

```cpp
auto value = std::make_tuple("user", 42, true);

vix::print(value);
```

Output shape:

```txt
(user, 42, true)
```

## Optionals

```cpp
std::optional<int> a = 42;
std::optional<int> b;

vix::print(a);
vix::print(b);
```

Output:

```txt
Some(42)
None
```

## Variants

```cpp
std::variant<int, std::string> value = std::string{"hello"};

vix::print(value);
```

Output shape:

```txt
variant<hello>
```

## Any

```cpp
std::any a = 42;
std::any empty;

vix::print(a);
vix::print(empty);
```

Output shape:

```txt
any(i)
any(empty)
```

`std::any` prints the stored type name, not the stored value.

## Chrono durations

```cpp
using namespace std::chrono;

vix::print(42ns);
vix::print(100us);
vix::print(1500ms);
vix::print(60s);
vix::print(2min);
vix::print(1h);
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

On Windows, microseconds may be rendered as `us`.

## Chrono time points

```cpp
auto now = std::chrono::system_clock::now();

vix::print(now);
```

Output shape:

```txt
<time_point: 1710000000000ms since epoch>
```

The exact value depends on the clock and current time.

## Filesystem paths

```cpp
std::filesystem::path path = "/usr/local/include";

vix::print(path);
```

Output:

```txt
path("/usr/local/include")
```

## Smart pointers

```cpp
auto value = std::make_unique<int>(42);
std::unique_ptr<int> empty;

vix::print(value);
vix::print(empty);
```

Output:

```txt
unique_ptr(42)
unique_ptr(null)
```

Shared pointers include the use count:

```cpp
auto shared = std::make_shared<std::string>("hello");

vix::print(shared);
```

Output shape:

```txt
shared_ptr[use=1](hello)
```

Weak pointers show whether they are expired or lockable:

```cpp
std::weak_ptr<std::string> weak = shared;

vix::print(weak);
```

Output shape:

```txt
weak_ptr[use=2](hello)
```

## Raw pointers

```cpp
int value = 42;
int *ptr = &value;
int *empty = nullptr;

vix::print(ptr);
vix::print(empty);
```

Output shape:

```txt
<ptr:0x...>
nullptr
```

Pointers print their address, not the pointed value.

## Function pointers

```cpp
void (*fn)(int) = [](int) {};

vix::print(fn);
```

Output shape:

```txt
<fptr:0x...>
```

## Container adapters

### Stack

```cpp
std::stack<int> values;
values.push(1);
values.push(2);
values.push(3);

vix::print(values);
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

vix::print(q);
```

Output shape:

```txt
queue[first, second]
```

### Priority queue

```cpp
std::priority_queue<int> pq;
pq.push(5);
pq.push(1);
pq.push(4);

vix::print(pq);
```

Output shape:

```txt
priority_queue[5, 4, 1]
```

Container adapters are copied internally before printing, so printing does not mutate the original container.

## Limit container output

Use `max_items` to avoid printing very large containers.

```cpp
std::vector<int> values = {1, 2, 3, 4, 5};

vix::print(vix::options{.max_items = 3}, values);
```

Output shape:

```txt
[1, 2, 3, ...]
```

## Default configuration

`default_config()` returns a thread-local default configuration.

```cpp
auto &cfg = vix::default_config();

cfg.separator = " | ";
cfg.end = "\n";
cfg.max_items = 10;

vix::print(1, 2, 3);
```

Output:

```txt
1 | 2 | 3
```

Because the default config is thread-local, changing it affects the current thread.

## Scoped configuration

Use `scoped_config` to temporarily override the default print configuration.

```cpp
vix::print(1, 2, 3);

{
  vix::scoped_config guard{};
  guard.cfg.separator = ", ";
  guard.cfg.end = ";\n";

  vix::print(1, 2, 3);
}

vix::print(1, 2, 3);
```

Output:

```txt
1 2 3
1, 2, 3;
1 2 3
```

When the guard is destroyed, the previous config is restored.

## Custom type support

There are three common ways to make a custom type printable.

1. Define `operator<<`
2. Define an ADL `vix_format` hook
3. Specialize `vix::formatter<T>`

## Method 1: use `operator<<`

```cpp
#include <ostream>
#include <string>
#include <vix/print.hpp>

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

  vix::print(user);

  return 0;
}
```

Output:

```txt
User{id=1, name=Ada}
```

Use this when the stream representation is useful everywhere, not only in Vix.

## Method 2: use ADL `vix_format`

Define `vix_format` in the same namespace as your type.

```cpp
#include <ostream>
#include <vix/print.hpp>

namespace app
{
  struct Point
  {
    double x{};
    double y{};
  };

  void vix_format(std::ostream &os, const Point &point)
  {
    os << "Point{x=" << point.x << ", y=" << point.y << "}";
  }
}

int main()
{
  app::Point p{1.5, 2.5};

  vix::print(p);

  return 0;
}
```

Output:

```txt
Point{x=1.5, y=2.5}
```

Use this when you want Vix-specific formatting without defining a global stream operator.

## Method 3: specialize `vix::formatter<T>`

```cpp
#include <ostream>
#include <string>
#include <vix/print.hpp>

struct Color
{
  unsigned char r{};
  unsigned char g{};
  unsigned char b{};
  unsigned char a{255};
};

template <>
struct vix::formatter<Color>
{
  static void format(std::ostream &os, const Color &color)
  {
    os << "Color("
       << "r=" << static_cast<int>(color.r)
       << ", g=" << static_cast<int>(color.g)
       << ", b=" << static_cast<int>(color.b)
       << ", a=" << static_cast<int>(color.a)
       << ")";
  }
};

int main()
{
  Color red{255, 0, 0, 255};

  vix::print(red);

  return 0;
}
```

Output:

```txt
Color(r=255, g=0, b=0, a=255)
```

Use this when you want full control over how Vix prints a type.

## `streamable_formatter`

If your type already has `operator<<`, you can delegate to it:

```cpp
template <>
struct vix::formatter<MyType> : vix::streamable_formatter<MyType>
{
};
```

This is useful when you want an explicit `vix::formatter` specialization but still want to reuse stream output.

## Rendering priority

When Vix prints a value, it chooses the first matching renderer.

The priority is:

1. `vix::formatter<T>` specialization
2. ADL `vix_format`
3. `nullptr_t`
4. `bool`
5. character types
6. wide strings
7. string-like types
8. `std::filesystem::path`
9. `std::any`
10. `std::optional<T>`
11. `std::variant<Ts...>`
12. `std::reference_wrapper<T>`
13. chrono duration
14. chrono time point
15. smart pointers
16. container adapters
17. `std::expected<T, E>`, when available
18. enum types
19. map-like ranges
20. sequence and set ranges
21. pair
22. tuple-like types
23. raw pointers
24. function pointers
25. streamable types using `operator<<`
26. fallback unprintable type name

This matters when a type can match more than one category.

## Inspect the rendering path

Use `rendering_path<T>()` to understand which renderer Vix will use.

```cpp
vix::print(vix::rendering_path<int>());
vix::print(vix::rendering_path<std::vector<int>>());
vix::print(vix::rendering_path<std::map<std::string, int>>());
```

Output shape:

```txt
bool
range
map-like range
```

You can also print several paths at once:

```cpp
vix::print_rendering_paths<
  int,
  std::string,
  std::vector<int>,
  std::map<std::string, int>,
  std::optional<int>
>();
```

## Complete example

```cpp
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>
#include <vix/print.hpp>

struct Point
{
  int x{};
  int y{};
};

void vix_format(std::ostream &os, const Point &point)
{
  os << "Point{x=" << point.x << ", y=" << point.y << "}";
}

int main()
{
  vix::print_header("Vix print");

  vix::print("hello", 42, true);

  vix::print(std::vector<int>{1, 2, 3});

  vix::print(std::map<std::string, int>{
    {"users", 120},
    {"orders", 42}
  });

  vix::print(std::optional<int>{42});
  vix::print(std::optional<int>{});

  std::variant<int, std::string> value = std::string{"active"};
  vix::print(value);

  vix::print(Point{10, 20});

  std::string message = vix::sprint("status", "ok");
  vix::print_named("message", message);

  vix::print_each(std::vector<std::string>{"alpha", "beta"}, true);

  vix::print_table(std::map<std::string, int>{
    {"a", 1},
    {"b", 2}
  });

  return 0;
}
```

## Use in Vix apps

`vix::print` is useful in simple scripts, examples, demos, and local development.

```cpp
#include <vix.hpp>
#include <vix/print.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/health", [](Request &, Response &res) {
    vix::print("health route called");

    res.json({
      "ok", true,
      "service", "api"
    });
  });

  app.run(8080);

  return 0;
}
```

For production application logs, prefer the Vix logging API:

```cpp
#include <vix/log.hpp>

vix::log::info("server started");
vix::log::error("database connection failed");
```

Use `print` for direct output.

Use `log` for structured production logging.

## Common mistakes

### Forgetting the header

```cpp
// Wrong
vix::print("Hello");
```

Fix:

```cpp
#include <vix/print.hpp>
```

### Expecting strings to be quoted by default

By default, strings are raw:

```cpp
vix::print("hello");
```

Output:

```txt
hello
```

Use `raw_strings = false` if you want quotes:

```cpp
vix::print(vix::options{.raw_strings = false}, "hello");
```

Output:

```txt
"hello"
```

### Printing huge containers

```cpp
std::vector<int> values(1'000'000);

vix::print(values);
```

Better:

```cpp
vix::print(vix::options{.max_items = 20}, values);
```

### Using print for production logs

This works:

```cpp
vix::print("user created");
```

But for production logs, prefer:

```cpp
vix::log::info("user created");
```

### Expecting `std::any` to print the stored value

```cpp
std::any value = 42;

vix::print(value);
```

Output shape:

```txt
any(i)
```

`std::any` prints the stored type name, not the stored value.

### Expecting raw pointers to dereference

```cpp
int value = 42;
int *ptr = &value;

vix::print(ptr);
```

Output shape:

```txt
<ptr:0x...>
```

Raw pointers print addresses.

Smart pointers print the contained value when non-null.

## Best practices

Use `vix::print` for quick readable output:

```cpp
vix::print("server ready", 8080);
```

Use `vix::sprint` when you need a string:

```cpp
std::string label = vix::sprint("user", id);
```

Use `vix::eprint` for errors in command-line tools:

```cpp
vix::eprint("error:", "missing input file");
```

Use `print_each` for lists:

```cpp
vix::print_each(items, true);
```

Use `print_table` for maps:

```cpp
vix::print_table(config);
```

Use `pprint` for nested structures:

```cpp
vix::pprint(data);
```

Use `formatter<T>` or `vix_format` for custom domain types:

```cpp
vix::print(User{1, "Ada"});
```

## API reference

### `print`

```cpp
template <typename... Args>
void print(const Args &...args);
```

Prints values using the thread-local default config.

```cpp
vix::print("hello", 42);
```

### `print` with config

```cpp
template <typename... Args>
void print(const print_config &cfg, const Args &...args);
```

Prints values using an explicit `print_config`.

```cpp
vix::print_config cfg;
cfg.separator = ", ";

vix::print(cfg, 1, 2, 3);
```

### `print` with options

```cpp
template <typename... Args>
void print(const options &opts, const Args &...args);
```

Prints values using lightweight options.

```cpp
vix::print(vix::options{.sep = ", "}, 1, 2, 3);
```

### `print_py`

```cpp
template <typename... Args>
void print_py(const Args &...args);
```

Prints with raw string behavior enabled.

```cpp
vix::print_py("hello", "world");
```

### `print_to`

```cpp
template <typename... Args>
void print_to(std::ostream &os, const Args &...args);
```

Prints to a specific stream.

```cpp
std::ostringstream out;
vix::print_to(out, "hello");
```

### `eprint`

```cpp
template <typename... Args>
void eprint(const Args &...args);
```

Prints to `std::cerr`.

```cpp
vix::eprint("error:", "not found");
```

### `print_inline`

```cpp
template <typename... Args>
void print_inline(const Args &...args);
```

Prints without a trailing newline.

```cpp
vix::print_inline("Loading...");
```

### `sprint`

```cpp
template <typename... Args>
std::string sprint(const Args &...args);
```

Formats values into a string.

```cpp
std::string s = vix::sprint("id", 42);
```

### `to_string`

```cpp
template <typename T>
std::string to_string(const T &value);
```

Formats one value into a string.

```cpp
std::string s = vix::to_string(std::vector<int>{1, 2, 3});
```

### `write_to`

```cpp
template <typename T>
void write_to(std::ostream &os, const T &value);
```

Writes one rendered value to a stream.

```cpp
vix::write_to(std::cout, std::vector<int>{1, 2, 3});
```

### `print_named`

```cpp
template <typename T>
void print_named(std::string_view label, const T &value);
```

Prints a label and a value.

```cpp
vix::print_named("port", 8080);
```

### `print_separator`

```cpp
void print_separator(std::size_t width = 60, char ch = '-');
```

Prints a separator line.

```cpp
vix::print_separator();
vix::print_separator(30, '=');
```

### `print_header`

```cpp
void print_header(std::string_view title, std::size_t width = 60);
```

Prints a section header.

```cpp
vix::print_header("Startup");
```

### `print_each`

```cpp
template <typename Range>
void print_each(const Range &rng, bool show_index = false);
```

Prints each element of a range on its own line.

```cpp
vix::print_each(std::vector<std::string>{"a", "b"}, true);
```

### `print_table`

```cpp
template <typename Map>
void print_table(const Map &m, std::size_t col_width = 20);
```

Prints a map-like container as a two-column table.

```cpp
vix::print_table(std::map<std::string, int>{{"a", 1}, {"b", 2}});
```

### `pprint`

```cpp
template <typename T>
void pprint(const T &value, const print_config &cfg = default_config());
```

Pretty-prints a value using multi-line indentation.

```cpp
vix::pprint(std::map<std::string, std::vector<int>>{
  {"a", {1, 2}},
  {"b", {3, 4}}
});
```

### `pprint_all`

```cpp
template <typename... Args>
void pprint_all(const Args &...args);
```

Pretty-prints several values.

```cpp
vix::pprint_all(std::vector<int>{1, 2}, std::map<std::string, int>{{"x", 1}});
```

### `print_stats`

```cpp
template <typename Range>
void print_stats(const Range &rng, std::string_view label = "");
```

Prints statistics for a numeric range.

```cpp
vix::print_stats(std::vector<int>{1, 2, 3}, "values");
```

### `default_config`

```cpp
print_config &default_config() noexcept;
```

Returns the thread-local print configuration.

```cpp
vix::default_config().separator = " | ";
```

### `to_print_config`

```cpp
print_config to_print_config(const options &opts);
```

Converts `options` into `print_config`.

```cpp
vix::print_config cfg = vix::to_print_config(vix::options{.sep = ", "});
```

### `scoped_config`

```cpp
class scoped_config;
```

Temporarily overrides the default config and restores it automatically.

```cpp
{
  vix::scoped_config guard{};
  guard.cfg.separator = ", ";
  vix::print(1, 2, 3);
}
```

### `formatter`

```cpp
template <typename T, typename = void>
struct formatter;
```

Specialize this template to define custom Vix rendering.

```cpp
template <>
struct vix::formatter<MyType>
{
  static void format(std::ostream &os, const MyType &value)
  {
    os << "...";
  }
};
```

### `streamable_formatter`

```cpp
template <typename T>
struct streamable_formatter;
```

Helper formatter that delegates to `operator<<`.

```cpp
template <>
struct vix::formatter<MyType> : vix::streamable_formatter<MyType>
{
};
```

### `rendering_path`

```cpp
template <typename T>
std::string_view rendering_path() noexcept;
```

Returns the selected rendering strategy for a type.

```cpp
vix::print(vix::rendering_path<std::vector<int>>());
```

### `print_rendering_paths`

```cpp
template <typename... Ts>
void print_rendering_paths();
```

Prints rendering strategies for several types.

```cpp
vix::print_rendering_paths<int, std::string, std::vector<int>>();
```

## Summary

`vix::print` is the simple output API for Vix.

Use it for:

- examples,
- demos,
- local debugging,
- CLI tools,
- quick scripts,
- readable STL output,
- custom type formatting.

Remember the core tools:

```cpp
vix::print(...);          // print values
vix::eprint(...);         // print to stderr
vix::print_inline(...);   // no newline
vix::sprint(...);         // return string
vix::print_each(...);     // one item per line
vix::print_table(...);    // map-like table
vix::pprint(...);         // multi-line nested output
```

For production logs, use `vix::log`.

For direct developer output, use `vix::print`.
