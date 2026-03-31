# ConversionError

`ConversionError` is the structured, allocation-free error type used by
Vix conversion APIs.

It carries:

-   a stable error code (`ConversionErrorCode`)
-   the original input (`std::string_view`)
-   an optional position hint (`std::size_t`)

This is designed for logs and diagnostics, not end-user UI messages.

------------------------------------------------------------------------

## Header

``` cpp
#include <vix/conversion/ConversionError.hpp>
```

Namespace:

``` cpp
vix::conversion
```

------------------------------------------------------------------------

## ConversionErrorCode

``` cpp
enum class ConversionErrorCode : std::uint8_t
{
    None = 0,

    // Generic
    EmptyInput,
    InvalidCharacter,
    TrailingCharacters,

    // Numeric
    Overflow,
    Underflow,

    // Boolean
    InvalidBoolean,

    // Enum
    UnknownEnumValue,

    // Float
    InvalidFloat
};
```

### Meaning

-   **None**: no error
-   **EmptyInput**: input is empty after trimming
-   **InvalidCharacter**: unexpected character for the target type
-   **TrailingCharacters**: a valid prefix was parsed but extra
    characters remained
-   **Overflow**: value is above representable range
-   **Underflow**: value is below representable range
-   **InvalidBoolean**: boolean string did not match accepted tokens
-   **UnknownEnumValue**: enum mapping table contains no match
-   **InvalidFloat**: float parsing failed (format is not a valid float)

------------------------------------------------------------------------

## ConversionError

``` cpp
struct ConversionError
{
    ConversionErrorCode code{ConversionErrorCode::None};
    std::string_view input{};
    std::size_t position{0};

    constexpr ConversionError() = default;

    constexpr ConversionError(
        ConversionErrorCode c,
        std::string_view in,
        std::size_t pos = 0) noexcept
        : code(c), input(in), position(pos)
    {
    }

    [[nodiscard]] constexpr bool ok() const noexcept
    {
        return code == ConversionErrorCode::None;
    }
};
```

### Fields

-   `code`: the classification of failure
-   `input`: the original input passed to the conversion API
-   `position`: optional index hint into `input` (0 when not applicable)

`position` is useful when a parser detects the exact character offset of
a failure.

------------------------------------------------------------------------

## ok()

``` cpp
if (err.ok()) { /* no error */ }
```

`ok()` is a convenience method. It returns true only when:

``` cpp
err.code == ConversionErrorCode::None
```

------------------------------------------------------------------------

## to_string(ConversionErrorCode)

Vix provides a stable, short, non-localized string for logs:

``` cpp
std::string_view s = to_string(err.code);
```

Implementation notes:

-   Not localized
-   Not user-facing UI text
-   Intended for developer diagnostics and logs
-   Returns stable strings suitable for metrics keys and debugging

Example strings:

-   `"empty input"`
-   `"trailing characters"`
-   `"numeric overflow"`
-   `"invalid floating-point value"`

------------------------------------------------------------------------

## Typical Usage with expected

Most conversion functions return:

``` cpp
expected<T, ConversionError>
```

Example:

``` cpp
#include <vix/conversion/ToFloat.hpp>
#include <vix/conversion/ConversionError.hpp>
#include <iostream>

using namespace vix::conversion;

void parse_price(std::string_view input)
{
    auto r = to_float64(input);

    if (!r)
    {
        const auto &e = r.error();
        std::cout << "code: " << to_string(e.code) << "\n";
        std::cout << "input: " << e.input << "\n";
        std::cout << "pos: " << e.position << "\n";
        return;
    }

    std::cout << "value: " << r.value() << "\n";
}

int main()
{
    parse_price("12.3abc");
}
```

------------------------------------------------------------------------

## Design Philosophy

Conversion errors must be:

-   purely technical (no business logic)
-   allocation-free
-   stable and inspectable
-   easy to map into domain-level errors

`ConversionError` is intentionally small and predictable, so higher
layers can build richer validation and user-facing messages on top.

