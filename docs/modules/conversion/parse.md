# vix::conversion::parse

The `parse` API provides a strongly-typed, safe, and explicit way to
convert string input into scalar C++ types.

It returns an `expected<T, ConversionError>` so parsing is never
implicit and never throws. You always handle success and failure
explicitly.

## Supported Types

`parse<T>(std::string_view)` supports:

-   bool
-   Integral types (int, long, uint32_t, etc.)
-   Floating point types (float, double, etc.)

For enums, use:

-   parse_enum`<T>`{=html}(...)

## Basic Usage

``` cpp
#include <vix/conversion/Parse.hpp>

using namespace vix::conversion;

void example()
{
    auto i = parse<int>("42");
    auto f = parse<double>("3.14");
    auto b = parse<bool>("true");

    if (i)
    {
        int value = i.value();
    }
}
```

## Error Handling

`parse` returns:

expected\<T, ConversionError\>

Always check before accessing the value:

``` cpp
auto result = parse<int>("abc");

if (!result)
{
    auto err = result.error();
    // inspect ConversionError
}
```

No exceptions. No undefined behavior. Explicit failure.

## Enum Parsing

Enums require an explicit mapping table.

``` cpp
enum class Mode { Dev, Prod };

constexpr EnumEntry<Mode> entries[] = {
    {"dev", Mode::Dev},
    {"prod", Mode::Prod}
};

auto m = parse_enum<Mode>("dev", entries);
```

Case-insensitive matching is enabled by default.

## Design Philosophy

-   No implicit conversions
-   No silent truncation
-   No throwing for control flow
-   Fully constexpr-compatible where possible

This makes `parse` suitable for:

-   CLI arguments
-   Environment variables
-   HTTP query parameters
-   Configuration files

## Summary

vix::conversion::parse is:

-   Safe
-   Explicit
-   Predictable
-   Production-oriented

It turns text into strongly typed values without surprises.

