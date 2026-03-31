---
title: to_int
---

# to_int

`to_int` parses a base-10 integer from a string, with strict validation and overflow checks.

It is part of `vix::conversion` and returns an `expected<T, ConversionError>`.

## Header

```cpp
#include <vix/conversion/ToInt.hpp>
```

## What it does

Rules implemented by `to_int<T>()`:

- Input is ASCII-trimmed (leading and trailing whitespace removed)
- Optional leading `+` or `-`
- At least one digit is required
- The entire trimmed input must be numeric (no trailing characters)
- Overflow and underflow are detected for the target type

## API

### Generic parser

```cpp
template <typename Int>
expected<Int, ConversionError> to_int(std::string_view input) noexcept;
```

Constraints:

- `Int` must be an integral type (`std::is_integral_v<Int>`)

### Convenience helpers

```cpp
expected<int,        ConversionError> to_int32 (std::string_view input) noexcept;
expected<std::int64_t, ConversionError> to_int64 (std::string_view input) noexcept;

expected<unsigned,      ConversionError> to_uint32(std::string_view input) noexcept;
expected<std::uint64_t, ConversionError> to_uint64(std::string_view input) noexcept;
```

## Error behavior

On failure, `to_int` returns `make_unexpected(ConversionError{...})`.

Typical error codes:

- `EmptyInput` if the trimmed input is empty
- `InvalidCharacter` if a non-digit appears where a digit is expected
- `TrailingCharacters` if extra characters remain after the number
- `Overflow` or `Underflow` if the value does not fit the target type

Note: the parser uses the trimmed view to parse, but preserves the original `input` inside the returned `ConversionError` to improve diagnostics upstream.

## Examples

### Parse signed integer

```cpp
#include <vix/conversion/ToInt.hpp>
#include <iostream>

using vix::conversion::to_int;

void example_basic()
{
  auto r = to_int<int>("  -42  ");
  if (!r)
  {
    std::cout << "error: " << r.error().message() << "\n";
    return;
  }

  std::cout << "value: " << r.value() << "\n";
}
```

### Reject trailing characters

```cpp
#include <vix/conversion/ToInt.hpp>
#include <iostream>

using vix::conversion::to_int;

void example_trailing()
{
  auto r = to_int<int>("12px");
  if (!r)
  {
    // TrailingCharacters
    std::cout << r.error().message() << "\n";
    return;
  }
}
```

### Parse unsigned integer

```cpp
#include <vix/conversion/ToInt.hpp>
#include <iostream>

using vix::conversion::to_uint64;

void example_unsigned()
{
  auto r = to_uint64("18446744073709551615");
  if (!r)
  {
    std::cout << "error: " << r.error().message() << "\n";
    return;
  }

  std::cout << "value: " << r.value() << "\n";
}
```

### Overflow detection

```cpp
#include <vix/conversion/ToInt.hpp>
#include <iostream>

using vix::conversion::to_int32;

void example_overflow()
{
  auto r = to_int32("999999999999");
  if (!r)
  {
    // Overflow
    std::cout << r.error().message() << "\n";
    return;
  }
}
```

## Tips

- Prefer the typed helpers (`to_int32`, `to_uint64`, etc.) when your intent is fixed.
- If you need different bases (hex, octal) or relaxed parsing, implement a separate parser to keep this one strict and predictable.

