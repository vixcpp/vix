# Time

The `vix::time` module provides explicit and lightweight time primitives for:

- Dates
- DateTime (UTC)
- Timestamps (epoch-based)
- Durations
- Monotonic elapsed time

Header:

```cpp
#include <vix/time/time.hpp>
```

Namespace:

```cpp
using namespace vix::time;
```

This guide focuses on minimal usage patterns.
No showcase. No benchmarking code. Just how to use it correctly.

---

# 1. Date

Represents a calendar date in UTC.

## Create current date

```cpp
Date today = Date::now();
std::cout << today.to_string() << "\n";
```

## Parse a date

Format: `YYYY-MM-DD`

```cpp
Date d = Date::parse("2026-02-07");
std::cout << d.to_string() << "\n";
```

## Convert Date to Timestamp (start of day UTC)

```cpp
Timestamp t = d.to_timestamp_utc();
std::cout << t.seconds_since_epoch() << "\n";
```

---

# 2. DateTime (UTC)

Represents date + time in UTC.

## Current UTC datetime

```cpp
DateTime now = DateTime::now_utc();
std::cout << now.to_string_utc() << "\n";
```

## Parse ISO-8601 (UTC Z)

Format: `YYYY-MM-DDTHH:MM:SSZ`

```cpp
DateTime dt = DateTime::parse("2026-02-07T10:30:15Z");
std::cout << dt.to_string_utc() << "\n";
```

---

# 3. Timestamp

Represents an instant in time (epoch seconds).

## Get current timestamp

```cpp
Timestamp t = Timestamp::now();
std::cout << t.seconds_since_epoch() << "\n";
```

## Timestamp arithmetic

```cpp
Timestamp t0 = Timestamp::now();
Timestamp t1 = t0 + Duration::seconds(5);

Duration delta = t1 - t0;
std::cout << delta.count_seconds() << "\n";
```

---

# 4. Duration

Represents a time span.

## Create durations

```cpp
Duration a = Duration::seconds(10);
Duration b = Duration::milliseconds(250);
```

## Inspect duration

```cpp
std::cout << a.count_seconds() << "\n";
std::cout << b.count_ms() << "\n";
```

---

# 5. Measuring elapsed time (SteadyClock)

Use this for performance measurement or timeouts.
It is monotonic and not affected by wall-clock changes.

```cpp
auto start = SteadyClock::now_chrono();

// do work
for (int i = 0; i < 1'000'000; ++i) {}

Duration elapsed = SteadyClock::since(start);
std::cout << elapsed.count_ms() << "\n";
```

---

# Recommended Usage

For backend systems and offline-first systems:

- Store time as `Timestamp`
- Use `Date` for calendar grouping
- Use `DateTime` for display
- Use `Duration` for retry logic and timeouts
- Use `SteadyClock` for latency measurement

The API is explicit, UTC-first, and allocation-light.


