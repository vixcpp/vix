# expected

`expected<T, E>` is the error-handling primitive used by Vix conversion
APIs.

It provides a predictable alternative to exceptions and integrates
cleanly with strict parsing functions like `to_int`, `to_float`, and
`to_enum`.

## Header

``` cpp
#include <vix/conversion/Expected.hpp>
```

Namespace:

``` cpp
vix::conversion
```

## Why Vix Has Its Own expected

Vix conversion targets **C++20**, but `std::expected` is a **C++23**
feature.

This header provides a single abstraction:

-   Use `std::expected` when the standard library actually supports it
-   Otherwise use a small C++20 fallback that implements the minimal API
    Vix needs

This guarantees the same public API for conversions across toolchains
and language modes.

## When std::expected Is Used

Vix will alias `std::expected` only if:

-   `<expected>` header exists
-   The library feature macro `__cpp_lib_expected` is defined
-   `__cpp_lib_expected >= 202202L`

When those conditions are met:

``` cpp
template <typename T, typename E>
using expected = std::expected<T, E>;

template <typename E>
using unexpected = std::unexpected<E>;
```

Otherwise, Vix uses its fallback types.

## constexpr Behavior

In C++23 mode with `std::expected`, functions returning `expected` can
be `constexpr`.

In fallback mode (C++20), the type is not a literal type, so `constexpr`
would warn.

Vix standardizes this using:

``` cpp
#if VIX_CONVERSION_EXPECTED_IS_STD
#define VIX_EXPECTED_CONSTEXPR constexpr
#else
#define VIX_EXPECTED_CONSTEXPR inline
#endif
```

So conversion APIs can write:

``` cpp
[[nodiscard]] VIX_EXPECTED_CONSTEXPR expected<int, ConversionError>
to_int32(std::string_view input) noexcept;
```

## Public API Surface

The Vix fallback is intentionally minimal. It supports only what
conversion code needs.

### expected\<T, E\>

Main operations:

-   `has_value()`
-   `operator bool()`
-   `value()` accessors
-   `error()` accessors

### unexpected`<E>`{=html}

Used to carry an error value:

-   `error()` accessors
-   `make_unexpected(E)` helper

## Basic Usage Pattern

``` cpp
auto r = vix::conversion::to_float64("3.14");

if (!r)
{
    // failure path
    auto err = r.error();
    // inspect err.code, err.input, etc.
    return;
}

// success path
double v = r.value();
```

This style is:

-   explicit
-   branch-friendly
-   exception-free
-   easy to propagate

## Creating Errors with make_unexpected

Vix provides a helper to build `unexpected<E>` consistently:

``` cpp
return make_unexpected(ConversionError{
    ConversionErrorCode::InvalidFloat,
    input
});
```

This works in both modes:

-   std::expected mode
-   fallback mode

## Notes About the Fallback Implementation

The fallback `expected<T, E>` is implemented as:

-   A union storing either `T` or `E`
-   A boolean flag `has_` that selects which member is active
-   Placement-new constructors
-   A destructor that destroys the active member
-   `assert(...)` guards on `value()` and `error()` access

Important behavior:

-   `value()` asserts that the expected holds a value
-   `error()` asserts that the expected holds an error

This matches the "debug-checked" style used in Vix: if you misuse the
API, you get a clear assertion failure during development.

## Design Philosophy

Vix conversion APIs are strict.

So error handling must be:

-   explicit
-   typed
-   predictable
-   portable across C++20 and C++23

`expected<T, E>` is the foundation that enables this style.

