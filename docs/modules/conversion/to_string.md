# to_string

Header: `<vix/conversion/ToString.hpp>`

The `to_string` utilities convert strongly typed values into textual representations
using fast, allocation-aware formatting.

The API returns:

```
expected<std::string, ConversionError>
```

This keeps conversion explicit and failure-aware.

---

## Integral Types

```cpp
auto r = to_string(42);
if (r) {
    std::string s = r.value(); // "42"
}
```

Characteristics:

- Uses `std::to_chars`
- Base 10 formatting
- No heap allocation during formatting (only final string construction)
- `bool` is excluded from this overload

---

## Floating Point Types

```cpp
auto r = to_string(3.14159);
if (r) {
    std::string s = r.value();
}
```

Notes:

- Uses `std::to_chars`
- Returns `InvalidFloat` on formatting failure

---

## Boolean

```cpp
auto r1 = to_string(true);   // "true"
auto r2 = to_string(false);  // "false"
```

Always succeeds.

---

## Enum Conversion (Mapping Table)

```cpp
enum class Color { Red, Green, Blue };

constexpr EnumEntry<Color> entries[] = {
    {"red", Color::Red},
    {"green", Color::Green},
    {"blue", Color::Blue}
};

auto r = to_string(Color::Green, entries);
```

If the enum value is not found, returns:

```
ConversionErrorCode::UnknownEnumValue
```

---

## Design Notes

- Explicit failure reporting
- No exceptions
- Predictable formatting behavior
- Strongly typed enum mapping

