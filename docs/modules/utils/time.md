# Time

Guide for `vix::utils::Time` helpers (UTC timestamps, HTTP dates, monotonic timing, UNIX epoch milliseconds).

This utility is header only and designed for logging, metrics, request tracing, and any place where you need:
- A stable duration timer (monotonic clock)
- A real wall clock timestamp (UTC)
- Standard string formats (ISO-8601, RFC-1123)

File: `vix/utils/Time.hpp`

---

## What you get

### 1) UTC calendar time (strings)
- `iso8601_now()` returns an ISO-8601 UTC timestamp:
  - `YYYY-MM-DDTHH:MM:SSZ`
- `rfc1123_now()` returns an RFC-1123 HTTP date:
  - `Wed, 08 Oct 2025 14:07:12 GMT`

These are for logs, HTTP headers, debugging, and interoperability.

### 2) Monotonic milliseconds for durations
- `now_ms()` uses `std::chrono::steady_clock`
- Monotonic means it never goes backwards even if the OS clock changes
- Use it for measuring elapsed time (latency, benchmarks, timeouts)

### 3) UNIX epoch milliseconds for persisted timestamps
- `unix_ms()` uses `std::chrono::system_clock`
- It can jump if the system time is adjusted
- Use it when you need a real timestamp to store or transmit

---

## Include and namespace

```cpp
#include <vix/utils/Time.hpp>

using namespace vix::utils;
```

---

## Pick the right function

### Measure latency or elapsed time
Use `now_ms()`.

Why: `steady_clock` is monotonic, so time adjustments do not break your measurements.

```cpp
#include <vix/utils/Time.hpp>
#include <iostream>

static void do_work()
{
  volatile int x = 0;
  for (int i = 0; i < 2'000'000; ++i) x += i;
}

int main()
{
  const auto t0 = vix::utils::now_ms();
  do_work();
  const auto dt = vix::utils::now_ms() - t0;

  std::cout << "elapsed_ms=" << dt << "\n";
  return 0;
}
```

### Create a timestamp string for logs
Use `iso8601_now()`.

```cpp
#include <vix/utils/Time.hpp>
#include <iostream>

int main()
{
  std::cout << vix::utils::iso8601_now() << "\n";
  return 0;
}
```

### Build HTTP `Date` header
Use `rfc1123_now()`.

```cpp
#include <vix/utils/Time.hpp>
#include <iostream>
#include <string>

int main()
{
  const std::string dateHeader = "Date: " + vix::utils::rfc1123_now();
  std::cout << dateHeader << "\n";
  return 0;
}
```

### Store an event timestamp in the database
Use `unix_ms()`.

```cpp
#include <vix/utils/Time.hpp>
#include <cstdint>
#include <iostream>

int main()
{
  const std::uint64_t ts = vix::utils::unix_ms();
  std::cout << "ts_ms=" << ts << "\n";
  return 0;
}
```

---

## Internals and portability notes

### Thread safe UTC extraction
`utc_tm(time_t)` uses:
- Windows: `gmtime_s`
- POSIX: `gmtime_r`
- Fallback: `std::gmtime` (not thread safe, used only on exotic targets)

This keeps formatting safe across platforms.

### Strings and allocations
`iso8601_now()` and `rfc1123_now()` use `std::ostringstream` for formatting.
That is fine for logging and low frequency timestamps.
If you need extremely hot path formatting, keep the time in integer form (`unix_ms()`) and format only at the edge.

---

## Common mistakes

### Using `system_clock` for durations
Do not measure latency with `unix_ms()`.

Reason: system time can jump due to NTP, VM resume, manual clock changes.

Correct:
- durations: `now_ms()`
- persisted time: `unix_ms()`

### Expecting `now_ms()` to be an epoch time
`now_ms()` is time since an arbitrary start point.
It is only meaningful when you subtract two values from the same process lifetime.

---

## Quick recipe

- Latency and timeouts: `now_ms()`
- Log timestamps: `iso8601_now()`
- HTTP date header: `rfc1123_now()`
- Persisted event time: `unix_ms()`

