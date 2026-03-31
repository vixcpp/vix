# to_bool

## Overview

`to_bool` converts a textual input into a `bool` value in a safe and explicit way.

It returns:

- `expected<bool, ConversionError>` on success
- A `ConversionError` on failure

The input is trimmed before parsing, and keyword matching is ASCII case-insensitive.

---

## Supported Inputs

The following values are accepted:

### True values

- `"true"`
- `"1"`
- `"yes"`
- `"on"`

### False values

- `"false"`
- `"0"`
- `"no"`
- `"off"`

Matching is **case-insensitive** for keyword forms.

---

## Function Signature

```cpp
[[nodiscard]] VIX_EXPECTED_CONSTEXPR expected<bool, ConversionError>
to_bool(std::string_view input) noexcept;
```

---

## Error Handling

`to_bool` returns structured errors instead of throwing.

### Possible Error Codes

| Error Code        | Description                                  |
|------------------|----------------------------------------------|
| `EmptyInput`      | Input is empty after trimming                |
| `InvalidBoolean`  | Input does not match any accepted value      |

---

## Example Usage

```cpp
#include <vix/conversion/ToBool.hpp>
#include <iostream>

using namespace vix::conversion;

void example()
{
    auto v = to_bool("YES");

    if (v)
        std::cout << "Parsed value: " << (*v ? "true" : "false") << "\n";
    else
        std::cout << "Error parsing boolean\n";
}
```

---

## Implementation Notes

- Input is trimmed before evaluation.
- Comparison is ASCII case-insensitive using a lightweight internal helper.
- No heap allocations.
- Fully constexpr-compatible (depending on build configuration).
- No exceptions are thrown.

---

## Design Philosophy

`to_bool` follows Vix principles:

- Explicit results via `expected`
- No hidden conversions
- No implicit exceptions
- Clear, predictable behavior

