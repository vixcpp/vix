# to_float

Strict floating-point parsing utilities for Vix.cpp.

## Header

``` cpp
#include <vix/conversion/ToFloat.hpp>
```

Namespace:

``` cpp
vix::conversion
```

## Overview

`to_float` provides **strict, predictable, no-magic** floating-point
parsing.

It is designed for:

-   Deterministic behavior
-   Explicit error handling
-   No silent truncation
-   No partial parsing surprises

Unlike `std::stof` / `std::stod`, it:

-   Trims ASCII whitespace
-   Requires the entire input to be consumed
-   Detects overflow and underflow
-   Returns `expected<T, ConversionError>`

## Template API

``` cpp
template <typename Float>
expected<Float, ConversionError>
to_float(std::string_view input) noexcept;
```

### Constraints

``` cpp
Float must satisfy std::is_floating_point_v<Float>
```

## Supported Formats

-   Decimal notation: `123.45`
-   Integer form: `42`
-   Scientific notation: `1.23e10`, `4.5E-3`
-   Leading / trailing ASCII whitespace allowed

Example:

``` cpp
auto r = to_float<double>("  3.1415  ");
```

## Strict Rules

1.  Input is ASCII-trimmed.
2.  The entire trimmed input must represent a valid number.
3.  No trailing characters allowed.
4.  Overflow and underflow are detected.
5.  Errors are explicit and typed.

## Error Handling

Return type:

``` cpp
expected<Float, ConversionError>
```

Possible error codes:

-   `EmptyInput`
-   `InvalidFloat`
-   `TrailingCharacters`
-   `Overflow`
-   `Underflow`

Example:

``` cpp
auto r = to_float<double>("12.3abc");

if (!r) {
    std::cout << r.error().message();
}
```

## Convenience Wrappers

### to_float32

``` cpp
expected<float, ConversionError>
to_float32(std::string_view input);
```

### to_float64

``` cpp
expected<double, ConversionError>
to_float64(std::string_view input);
```

### to_float80

``` cpp
expected<long double, ConversionError>
to_float80(std::string_view input);
```

Note: actual precision of `long double` depends on platform ABI.

## Example: Safe Parsing

``` cpp
#include <vix/conversion/ToFloat.hpp>
#include <iostream>

using namespace vix::conversion;

void parse_example()
{
    auto r = to_float64("2.5e3");

    if (!r)
    {
        std::cout << "Error: " << r.error().message() << "\n";
        return;
    }

    std::cout << "Value: " << r.value() << "\n";
}

int main()
{
    parse_example();
}
```

## Comparison with std::stod

  Feature                   std::stod   vix::conversion::to_float
  ------------------------- ----------- ---------------------------
  Throws exceptions         Yes         No
  Partial parsing allowed   Yes         No
  Typed error codes         No          Yes
  Strict full consumption   No          Yes
  Overflow detection        Partial     Explicit
  Predictable behavior      No          Yes

## Design Philosophy

Floating point parsing should be:

-   Explicit
-   Safe
-   Strict
-   Observable

No hidden behavior.\
No silent acceptance of invalid input.\
No exception-driven control flow.

Just predictable conversion.

