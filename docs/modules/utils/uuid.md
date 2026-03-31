# UUID Guide (vix::utils::uuid4)

This guide explains how UUID generation works in Vix and how to use it
correctly.

The implementation follows **RFC 4122 version 4** (random UUID).\
It is header-only and thread-safe.

------------------------------------------------------------------------

## What is a UUID v4?

A UUID v4 is a 128-bit random identifier formatted as:

xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx

Example:

550e8400-e29b-41d4-a716-446655440000

Properties:

-   128-bit value
-   Random-based (version 4)
-   Lowercase hexadecimal
-   RFC 4122 variant compliant (10xxxxxx)

------------------------------------------------------------------------

## Basic Usage

Include the header:

``` cpp
#include <vix/utils/UUID.hpp>
```

Minimal example:

``` cpp
#include <vix/utils/UUID.hpp>
#include <iostream>

int main()
{
    std::string id = vix::utils::uuid4();
    std::cout << id << std::endl;
}
```

Run:

``` bash
vix run main.cpp
```

------------------------------------------------------------------------

## Thread Safety

The generator uses a **thread-local Mersenne Twister
(std::mt19937_64)**.

Each thread:

-   Has its own RNG
-   Is seeded once
-   Produces independent UUID sequences

This prevents cross-thread contention.

------------------------------------------------------------------------

## How It Works (Internally)

1.  Generate 16 random bytes
2.  Set version bits:
    -   Byte 6 → version 4 (0100)
3.  Set variant bits:
    -   Byte 8 → 10xxxxxx
4.  Format into canonical string (8-4-4-4-12)

------------------------------------------------------------------------

## When To Use UUID v4

Use uuid4() when you need:

-   Request IDs
-   Correlation IDs
-   Database primary keys
-   Distributed system identifiers
-   Temporary object identifiers

Do NOT use UUID v4 when you need:

-   Time ordering
-   Sortable IDs
-   Cryptographic guarantees

------------------------------------------------------------------------

## Performance Characteristics

-   No dynamic allocations except final string
-   O(1) generation
-   No global locks
-   Header-only implementation

------------------------------------------------------------------------

## Summary

uuid4() provides:

-   RFC 4122 compliant UUIDs
-   Thread-safe generation
-   Simple header-only API
-   Zero configuration

Minimal. Predictable. Safe.

