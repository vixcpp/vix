# async/core/error

Error handling system for the Vix async runtime.

## Overview

The `error.hpp` file defines the error model used across:

-   Scheduler
-   Thread pool
-   Runtime
-   Timers
-   Cancellation system

It is based on:

-   `std::error_code`
-   A custom `async` error category
-   A compact `enum class errc`

The design is:

-   Lightweight
-   Allocation-free
-   Stable
-   Compatible with standard C++ error handling

## Error Enumeration

``` cpp
enum class errc : std::uint8_t
```

### Generic errors

  Code               Meaning
  ------------------ ---------------------
  ok                 No error
  invalid_argument   Invalid input
  not_ready          Operation not ready
  timeout            Operation timed out
  canceled           Operation canceled
  closed             Resource closed
  overflow           Capacity overflow

### Runtime / Scheduler

  Code         Meaning
  ------------ -----------------
  stopped      Runtime stopped
  queue_full   Task queue full

### Thread Pool

  Code       Meaning
  ---------- --------------------------
  rejected   Task submission rejected

### Platform

  Code            Meaning
  --------------- -----------------------
  not_supported   Feature not supported

## Error Category

The async subsystem defines its own `std::error_category`.

``` cpp
class error_category : public std::error_category
```

Category name:

``` cpp
"async"
```

This integrates seamlessly with `std::error_code`.

## Creating Error Codes

``` cpp
using namespace vix::async::core;

std::error_code ec = make_error_code(errc::timeout);

if (ec)
{
    std::cout << ec.message(); // "timeout"
}
```

Because `is_error_code_enum` is specialized, you can write:

``` cpp
std::error_code ec = errc::canceled;
```

## Cancellation Integration

The cancellation system uses:

``` cpp
errc::canceled
```

Helper:

``` cpp
std::error_code cancelled_ec();
```

This ensures consistent error propagation across the async runtime.

## Design Goals

-   No exceptions required
-   Thread-safe propagation
-   Zero dynamic allocation
-   Deterministic behavior
-   Stable ABI-friendly enum

## Typical Usage Pattern

``` cpp
std::error_code ec;

if (queue_full)
    ec = errc::queue_full;

if (ec)
{
    return ec;
}
```

This style makes async code:

-   Explicit
-   Predictable
-   Composable

The async error model keeps the runtime deterministic and fully
compatible with modern C++ error handling.

