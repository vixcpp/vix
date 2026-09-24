---
title: Format
---

# Format

`vix::format` provides lightweight placeholder-based string formatting for Vix.

Use it when you want to build strings with simple `{}` or `{0}` placeholders without pulling in a large formatting API.

## Public header

```cpp
#include <vix/format.hpp>
```

You can also include the implementation header directly:

```cpp
#include <vix/format/Format.hpp>
```

## What Format provides

The format API provides:

- automatic placeholders with `{}`
- explicit positional placeholders with `{0}`, `{1}`, `{2}`
- escaped braces with <code v-pre>{{</code> and <code v-pre>}}</code>
- formatted output into a new `std::string`
- appending formatted output to an existing string
- replacing an existing string with formatted output
- `vix::format_error` for invalid format strings
- integration with the Vix rendering pipeline used by `vix::print`

It is intentionally small.

Unsupported on purpose:

```text
{:>10}
{:.2f}
{:x}
```

Format specifiers are not supported.

## Basic usage

```cpp
#include <vix/format.hpp>
#include <vix/print.hpp>

int main()
{
  std::string message = vix::format("Hello, {}", "world");

  vix::print(message);

  return 0;
}
```

Output:

```text
Hello, world
```

## Automatic placeholders

Use `{}` to insert arguments in order.

```cpp
std::string s = vix::format("{} + {} = {}", 2, 3, 5);
```

Result:

```text
2 + 3 = 5
```

Automatic placeholders consume arguments from left to right.

```cpp
vix::format("Name: {}, age: {}", "Ada", 37);
```

Result:

```text
Name: Ada, age: 37
```

## Explicit positional placeholders

Use `{0}`, `{1}`, `{2}` to select arguments by index.

```cpp
std::string s = vix::format("{0} + {0} = {1}", 2, 4);
```

Result:

```text
2 + 2 = 4
```

Argument indexes are zero-based.

```cpp
vix::format("first={0}, second={1}", "A", "B");
```

Result:

```text
first=A, second=B
```

## Reuse arguments

Explicit placeholders can reuse the same argument multiple times.

```cpp
std::string s = vix::format("{0}/{0}/{1}", "api", "status");
```

Result:

```text
api/api/status
```

## Escaped braces

Use <code v-pre>{{</code> to write a literal <code>{</code>.

Use <code v-pre>}}</code> to write a literal <code>}</code>.

<pre v-pre><code class="language-cpp">std::string s = vix::format("{{ config }} = {}", "ready");</code></pre>

Result:

```text
{ config } = ready
```

## Automatic and explicit indexing cannot be mixed

This is invalid:

```cpp
vix::format("{} {0}", "A");
```

It throws:

```cpp
vix::format_error
```

Use only one style per format string.

Good:

```cpp
vix::format("{} {}", "A", "B");
```

Good:

```cpp
vix::format("{0} {1}", "A", "B");
```

Bad:

```cpp
vix::format("{} {1}", "A", "B");
```

## Format specifiers are not supported

This is invalid:

```cpp
vix::format("{:.2f}", 3.14159);
```

It throws:

```cpp
vix::format_error
```

Use simple placeholders only:

```cpp
vix::format("value = {}", 3.14159);
```

## format

`vix::format(...)` returns a new string.

```cpp
std::string s = vix::format("User {} has id {}", "Ada", 42);
```

Result:

```text
User Ada has id 42
```

Signature:

```cpp
template <typename... Args>
std::string format(std::string_view fmt, const Args &...args);
```

## format_append

`vix::format_append(...)` appends formatted output to an existing string.

```cpp
std::string out = "prefix: ";

vix::format_append(out, "{} = {}", "status", "ok");
```

Result:

```text
prefix: status = ok
```

Signature:

```cpp
template <typename... Args>
void format_append(std::string &out, std::string_view fmt, const Args &...args);
```

Use it when you want to build a string incrementally.

```cpp
std::string out;

vix::format_append(out, "method={}", "GET");
vix::format_append(out, ", path={}", "/api/status");
```

Result:

```text
method=GET, path=/api/status
```

## format_to

`vix::format_to(...)` replaces the destination string with formatted output.

```cpp
std::string out = "old value";

vix::format_to(out, "new value: {}", 42);
```

Result:

```text
new value: 42
```

Signature:

```cpp
template <typename... Args>
void format_to(std::string &out, std::string_view fmt, const Args &...args);
```

Use it when you want to reuse an existing string object but replace its content.

## format_error

`vix::format_error` is thrown when the format string is invalid or when an argument index is invalid.

```cpp
try
{
  auto s = vix::format("{1}", "only one argument");
}
catch (const vix::format_error &e)
{
  vix::print("format error:", e.what());
}
```

Common causes:

- missing argument
- invalid explicit index
- unmatched `{`
- single `}`
- mixed automatic and explicit placeholders
- unsupported format specifier

## Missing argument

This is invalid:

```cpp
vix::format("{} {}", "one");
```

There are two placeholders but only one argument.

It throws:

```cpp
vix::format_error
```

## Invalid argument index

This is invalid:

```cpp
vix::format("{2}", "a", "b");
```

Indexes start at `0`.

Available indexes are:

```text
0
1
```

Index `2` is out of range.

## Unmatched opening brace

This is invalid:

```cpp
vix::format("Hello {", "world");
```

It throws:

```cpp
vix::format_error
```

Use escaped braces if you want a literal brace.

```cpp
vix::format("Hello { { ");
```

## Single closing brace

This is invalid:

```cpp
vix::format("Hello }");
```

It throws:

```cpp
vix::format_error
```

Use `}}` for a literal closing brace.

<pre v-pre><code class="language-cpp">vix::format("Hello }}");</code></pre>

## Supported placeholder syntax

| Syntax | Meaning |
|---|---|
| `{}` | Insert next automatic argument. |
| `{0}` | Insert argument at index `0`. |
| `{1}` | Insert argument at index `1`. |
| <code v-pre>{{</code> | Insert literal `{`. |
| <code v-pre>}}</code> | Insert literal `}`. |

## Unsupported syntax

| Syntax | Reason |
|---|---|
| `{:>10}` | Format specifiers are not supported. |
| `{:.2f}` | Format specifiers are not supported. |
| `{:x}` | Format specifiers are not supported. |
| `{name}` | Named placeholders are not supported. |
| `{}` mixed with `{0}` | Mixed indexing is rejected. |

## Formatting strings

`vix::format` uses the Vix rendering pipeline.

Strings are inserted without extra quotes.

```cpp
std::string s = vix::format("Hello, {}", std::string{"Ada"});
```

Result:

```text
Hello, Ada
```

This is different from debug-style printing where strings may be shown with quotes depending on print configuration.

## Formatting numbers

```cpp
std::string s = vix::format("x={}, y={}", 10, 20);
```

Result:

```text
x=10, y=20
```

## Formatting booleans

```cpp
std::string s = vix::format("enabled={}", true);
```

Result:

```text
enabled=true
```

## Formatting containers

Because `vix::format` uses the Vix rendering pipeline, containers can be formatted too.

```cpp
#include <vix/format.hpp>
#include <vector>

int main()
{
  std::vector<int> values{1, 2, 3};

  std::string s = vix::format("values = {}", values);

  return 0;
}
```

Result shape:

```text
values = [1, 2, 3]
```

## Formatting maps

```cpp
#include <vix/format.hpp>
#include <map>
#include <string>

int main()
{
  std::map<std::string, int> scores{
      {"Ada", 10},
      {"Bob", 20}
  };

  std::string s = vix::format("scores = {}", scores);

  return 0;
}
```

Result shape:

```text
scores = {Ada => 10, Bob => 20}
```

## Formatting optional values

```cpp
#include <vix/format.hpp>
#include <optional>

int main()
{
  std::optional<int> value = 42;

  std::string s = vix::format("value = {}", value);

  return 0;
}
```

Result:

```text
value = Some(42)
```

Empty optional:

```cpp
std::optional<int> value;

std::string s = vix::format("value = {}", value);
```

Result:

```text
value = None
```

## Formatting filesystem paths

```cpp
#include <vix/format.hpp>
#include <filesystem>

int main()
{
  std::filesystem::path path{"public/index.html"};

  std::string s = vix::format("file = {}", path);

  return 0;
}
```

Result shape:

```text
file = path("public/index.html")
```

## Formatting custom types

`vix::format` uses the same rendering engine as `vix::print`.

That means custom types can be supported with:

- `operator<<`
- `vix_format(...)` ADL hook
- `vix::formatter<T>` specialization

## Custom type with operator<<

```cpp
#include <vix/format.hpp>
#include <ostream>
#include <string>

struct User
{
  std::string name;
  int age;
};

std::ostream &operator<<(std::ostream &os, const User &user)
{
  return os << "User{name=" << user.name << ", age=" << user.age << "}";
}

int main()
{
  User user{"Ada", 37};

  std::string s = vix::format("{}", user);

  return 0;
}
```

Result:

```text
User{name=Ada, age=37}
```

## Custom type with vix::formatter

```cpp
#include <vix/format.hpp>
#include <ostream>
#include <string>

struct Point
{
  int x;
  int y;
};

template <>
struct vix::formatter<Point>
{
  static void format(std::ostream &os, const Point &p)
  {
    os << "Point(" << p.x << ", " << p.y << ")";
  }
};

int main()
{
  Point p{10, 20};

  std::string s = vix::format("position = {}", p);

  return 0;
}
```

Result:

```text
position = Point(10, 20)
```

## Custom type with ADL vix_format

```cpp
#include <vix/format.hpp>
#include <ostream>

namespace app
{
  struct Color
  {
    int r;
    int g;
    int b;
  };

  void vix_format(std::ostream &os, const Color &color)
  {
    os << "rgb(" << color.r << ", " << color.g << ", " << color.b << ")";
  }
}

int main()
{
  app::Color color{255, 128, 0};

  std::string s = vix::format("color = {}", color);

  return 0;
}
```

Result:

```text
color = rgb(255, 128, 0)
```

## Use with Vix HTTP handlers

`vix::format` is useful inside handlers when building small messages.

```cpp
#include <vix.hpp>
#include <vix/format.hpp>

int main()
{
  vix::App app;

  app.get("/users/{id}", [](vix::Request &req, vix::Response &res)
  {
    const std::string id = req.param("id");

    res.text(vix::format("User id: {}", id));
  });

  app.run(8080);

  return 0;
}
```

Request:

```text
GET /users/42
```

Response:

```text
User id: 42
```

## Use with logs

```cpp
#include <vix/format.hpp>
#include <vix/print.hpp>

int main()
{
  const std::string method = "GET";
  const std::string path = "/api/status";

  vix::print(vix::format("{} {}", method, path));

  return 0;
}
```

Output:

```text
GET /api/status
```

## Build strings incrementally

Use `format_append(...)`.

```cpp
#include <vix/format.hpp>

int main()
{
  std::string out;

  vix::format_append(out, "method={}", "GET");
  vix::format_append(out, " path={}", "/api/status");
  vix::format_append(out, " status={}", 200);

  return 0;
}
```

Result:

```text
method=GET path=/api/status status=200
```

## Replace an existing string

Use `format_to(...)`.

```cpp
#include <vix/format.hpp>

int main()
{
  std::string message = "old";

  vix::format_to(message, "status={}", "ok");

  return 0;
}
```

Result:

```text
status=ok
```

## Error handling example

```cpp
#include <vix/format.hpp>
#include <vix/print.hpp>

int main()
{
  try
  {
    std::string s = vix::format("{} {}", "only one argument");

    vix::print(s);
  }
  catch (const vix::format_error &e)
  {
    vix::print("format error:", e.what());
  }

  return 0;
}
```

Output shape:

```text
format error: format argument index out of range
```

## API summary

| API | Purpose |
|---|---|
| `vix::format(fmt, args...)` | Return a new formatted string. |
| `vix::format_append(out, fmt, args...)` | Append formatted output to an existing string. |
| `vix::format_to(out, fmt, args...)` | Replace an existing string with formatted output. |
| `vix::format_error` | Exception thrown for invalid format strings or invalid argument access. |

## Placeholder summary

| Placeholder | Meaning |
|---|---|
| `{}` | Insert the next automatic argument. |
| `{0}` | Insert argument at index `0`. |
| `{1}` | Insert argument at index `1`. |
| <code v-pre>{{</code> | Insert literal `{`. |
| <code v-pre>}}</code> | Insert literal `}`. |

## Error summary

| Invalid input | Error reason |
|---|---|
| `{` | Unmatched opening brace. |
| `}` | Single closing brace. |
| `{abc}` | Invalid explicit index. |
| `{:>10}` | Format specifiers are not supported. |
| `{} {0}` | Cannot mix automatic and explicit indexing. |
| `{2}` with two arguments | Argument index out of range. |

## Best practices

Use `{}` for simple left-to-right formatting.

```cpp
vix::format("{} {}", "Hello", "world");
```

Use explicit indexes when reusing arguments.

```cpp
vix::format("{0} + {0} = {1}", 2, 4);
```

Do not mix `{}` and `{0}` in the same format string.

```cpp
vix::format("{} {}", "A", "B");
```

or:

```cpp
vix::format("{0} {1}", "A", "B");
```

Use `format_append(...)` for incremental string building.

```cpp
vix::format_append(out, "id={}", id);
```

Use `format_to(...)` when reusing a string object.

```cpp
vix::format_to(out, "status={}", status);
```

Use `vix::formatter<T>` or `operator<<` for custom types.

```cpp
template <>
struct vix::formatter<MyType>
{
  static void format(std::ostream &os, const MyType &value)
  {
    os << value.name;
  }
};
```

Keep formatting simple.

```text
Vix format is intentionally lightweight.
Use it for simple readable strings, not advanced alignment or numeric formatting.
```

## Related APIs

| API | Purpose |
|---|---|
| `vix::print(...)` | Print values to stdout. |
| `vix::sprint(...)` | Convert multiple printable values to a string. |
| `vix::to_string(value)` | Convert one printable value to a string. |
| `vix::write_to(os, value)` | Write one value to a stream. |
| `vix::formatter<T>` | Customize rendering for a type. |

## Next steps

Read the related pages:

- [Print](./print.md)
- [Console](./console.md)
- [Input](./input.md)
- [Inspect](./inspect.md)
