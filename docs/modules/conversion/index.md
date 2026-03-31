# Conversion

The `vix::conversion` module provides explicit, allocation-friendly conversions between strings and common C++ types.
It avoids exceptions: each conversion returns an `Expected<T>` that contains either a value or a `ConversionError`.

## Include

```cpp
#include <vix/conversion/Parse.hpp>
#include <vix/conversion/ToBool.hpp>
#include <vix/conversion/ToInt.hpp>
#include <vix/conversion/ToFloat.hpp>
#include <vix/conversion/ToEnum.hpp>
#include <vix/conversion/ToString.hpp>
#include <vix/conversion/ConversionError.hpp>
#include <vix/conversion/Expected.hpp>
```

Namespace used in examples:

```cpp
using namespace vix::conversion;
```

---

# Result model

All conversion functions return `Expected&lt;T&gt;`.

- Success: `r.value()`
- Failure: `r.error()` (a `ConversionError`)

A minimal error printer:

```cpp
static void print_err(const ConversionError& e)
{
  std::cout << "error: " << to_string(e.code)
            << " position=" << e.position
            << " input='" << e.input << "'\n";
}
```

Recommended usage pattern:

```cpp
auto r = to_int<int>(input);
if (!r)
{
  print_err(r.error());
  return;
}

int value = r.value();
```

---

# to_bool

Parse common boolean strings (case-insensitive).

```cpp
auto r = to_bool("yes");
if (!r) { print_err(r.error()); return; }

bool v = r.value();
```

---

# to_int

Parse an integer type `T` from a string view (whitespace is accepted).
Overflow and invalid characters return a `ConversionError`.

```cpp
auto r = to_int<int>("  -7  ");
if (!r) { print_err(r.error()); return; }

int v = r.value();
```

---

# to_float

Parse a floating type (for example `double`). Whitespace is accepted.

```cpp
auto r = to_float<double>("  1e-3 ");
if (!r) { print_err(r.error()); return; }

double v = r.value();
```

---

# to_enum

Convert a string to an enum value using a compile-time table of `EnumEntry&lt;T&gt;`.
Default behavior is case-insensitive.

```cpp
enum class Role
{
  Admin,
  User,
};

static constexpr EnumEntry<Role> roles[] = {
  {"admin", Role::Admin},
  {"user", Role::User},
};

auto r = to_enum<Role>("USER", roles);
if (!r) { print_err(r.error()); return; }

Role v = r.value();
```

---

# to_string

Convert values to strings while keeping the same `Expected&lt;&gt;` error model.

```cpp
auto a = to_string(42);
if (!a) { print_err(a.error()); return; }
std::string s1 = a.value();

auto b = to_string(true);
if (!b) { print_err(b.error()); return; }
std::string s2 = b.value();
```

Enum to string (via table):

```cpp
auto r = to_string(Role::Admin, roles);
if (!r) { print_err(r.error()); return; }

std::string name = r.value();
```

---

# parse&lt;T&gt;

`parse&lt;T&gt;(input)` is the generic entry point.
It chooses the correct conversion based on `T`.

```cpp
auto a = parse<int>(" 123 ");
auto b = parse<double>(" 3.14 ");
auto c = parse<bool>("off");

if (!a) print_err(a.error());
if (!b) print_err(b.error());
if (!c) print_err(c.error());
```

---

# Notes

- Designed for explicit error handling: no exceptions required.
- Errors include an error code, best-effort position, and the original input.
- Prefer `parse&lt;T&gt;` when you want a single generic entry point.
- Prefer `to_int / to_float / to_bool / to_enum` when you want explicit intent.


