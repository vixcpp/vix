# Utils

Small, dependency-light building blocks used across Vix.cpp.

This module focuses on correctness, portability, and predictable
behavior. Most helpers are header-only and designed to be safe in
multithreaded runtimes.

## What is inside

-   **Env**: environment variable helpers with type conversions (bool,
    int, uint, double).
-   **Time**: UTC timestamps (ISO-8601, RFC-1123), monotonic timers,
    unix epoch in ms.
-   **String**: trim, case helpers, split/join, URL decode, query string
    parsing.
-   **UUID**: UUID v4 generator (RFC 4122 style, thread-local RNG).
-   **Result**: explicit error handling without exceptions
    (`Result<T, E>`).
-   **Validation**: schema-based validation for string maps (required,
    length, numeric range, regex).
-   **Logger**: minimal logging primitives used by the runtime and
    tools.
-   **PrettyLogs**: server-ready banner and terminal formatting
    utilities.

## Quick start

``` cpp
#include <vix/utils/Env.hpp>
#include <vix/utils/Time.hpp>
#include <vix/utils/String.hpp>
#include <vix/utils/Result.hpp>

using namespace vix::utils;

Result<int> parse_port()
{
  int p = env_int("PORT", 8080);
  if (p <= 0 || p > 65535)
    return Result<int>::Err("PORT must be in range 1..65535");
  return Result<int>::Ok(p);
}

int main()
{
  auto r = parse_port();
  if (r.is_err())
    return 1;

  const int port = r.value();

  const std::string ts = iso8601_now();
  const auto parts = split("a,b,,c", ',');

  (void)ts;
  (void)parts;
  (void)port;
}
```

## Guides

-   Overview (./guide.md)
-   Env (./env.md)
-   Time (./time.md)
-   String (./string.md)
-   UUID (./uuid.md)
-   Result (./result.md)
-   Validation (./validation.md)
-   Logger (./logger.md)
-   PrettyLogs (./prettylogs.md)

## Design goals

-   No hidden allocations when possible
-   No exceptions required
-   Works on Linux, macOS, and Windows
-   Safe defaults and explicit control flow

