# to_enum

Strict enum parsing utilities for Vix.cpp.

## Header

``` cpp
#include <vix/conversion/ToEnum.hpp>
```

Namespace:

``` cpp
vix::conversion
```

## Overview

`to_enum` converts string input into a strongly-typed enum value using
an explicit mapping table.

Design goals:

-   No reflection
-   No magic
-   Explicit mapping
-   Predictable behavior
-   Typed error handling

You define exactly which strings map to which enum values.

## EnumEntry

``` cpp
template <typename Enum>
struct EnumEntry
{
    std::string_view name;
    Enum value;
};
```

Each entry maps a string name to an enum value.

## Core API

``` cpp
template <typename Enum>
expected<Enum, ConversionError>
to_enum(
    std::string_view input,
    const EnumEntry<Enum>* entries,
    std::size_t count,
    bool case_insensitive = true) noexcept;
```

### Constraints

``` cpp
Enum must satisfy std::is_enum_v<Enum>
```

## Behavior

-   Input is ASCII-trimmed
-   Matching is exact length
-   Optional ASCII case-insensitive comparison
-   First matching entry wins
-   Entire input must match an entry name

## Error Handling

Return type:

``` cpp
expected<Enum, ConversionError>
```

Possible error codes:

-   `EmptyInput`
-   `UnknownEnumValue`

## Example

``` cpp
#include <vix/conversion/ToEnum.hpp>
#include <iostream>

using namespace vix::conversion;

enum class Role
{
    Admin,
    User,
    Guest
};

static constexpr EnumEntry<Role> roles[] = {
    {"admin", Role::Admin},
    {"user",  Role::User},
    {"guest", Role::Guest}
};

void parse_role(std::string_view input)
{
    auto r = to_enum(input, roles);

    if (!r)
    {
        std::cout << "Invalid role\n";
        return;
    }

    std::cout << "Parsed successfully\n";
}

int main()
{
    parse_role("Admin");
}
```

## Static Array Overload

You can use the overload that deduces the array size automatically:

``` cpp
template <typename Enum, std::size_t N>
expected<Enum, ConversionError>
to_enum(
    std::string_view input,
    const EnumEntry<Enum>(&entries)[N],
    bool case_insensitive = true) noexcept;
```

No need to manually pass the count.

## Case Sensitivity

By default:

``` cpp
case_insensitive = true
```

To require strict case matching:

``` cpp
auto r = to_enum(input, roles, false);
```

## Why Explicit Mapping?

C++ has no built-in reflection for enums.

Instead of relying on macros or unsafe tricks, Vix uses:

-   Compile-time tables
-   Explicit string definitions
-   Deterministic matching
-   Clear error reporting

You control the mapping fully.

## Design Philosophy

Enum parsing should be:

-   Explicit
-   Safe
-   Deterministic
-   Transparent

No hidden global registry.\
No implicit stringification.\
No undefined behavior.

Just predictable enum conversion.

