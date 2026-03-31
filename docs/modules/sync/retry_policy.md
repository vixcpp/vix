# RetryPolicy

This page documents the retry policy used in the `sync` module.

`RetryPolicy` defines how failed operations are retried over time using
deterministic exponential backoff.

It is:

-   Deterministic
-   Side-effect free
-   Recovery-safe
-   Suitable for durable offline-first systems

## Header

``` cpp
#include <vix/sync/RetryPolicy.hpp>
```

Namespace:

``` cpp
vix::sync
```

# Overview

`RetryPolicy` controls:

-   Maximum number of retry attempts
-   Exponential backoff growth
-   Maximum delay clamp
-   Optional jitter ratio (applied by caller)

The policy itself does not perform I/O or scheduling. It only computes
timing decisions.

# Structure

``` cpp
struct RetryPolicy
{
    std::uint32_t max_attempts{8};
    std::int64_t base_delay_ms{500};
    std::int64_t max_delay_ms{30'000};
    double factor{2.0};
    double jitter_ratio{0.2};

    bool can_retry(std::uint32_t attempt) const noexcept;
    std::int64_t compute_delay_ms(std::uint32_t attempt) const noexcept;
};
```

# Fields Explained

### max_attempts

Maximum number of retry attempts allowed.

If:

``` cpp
attempt >= max_attempts
```

No further retry should be scheduled.

### base_delay_ms

Initial delay before the first retry.

Default:

    500 ms

### max_delay_ms

Upper clamp for exponential growth.

Default:

    30 seconds

Prevents unbounded retry intervals.

### factor

Exponential growth factor.

Delay evolves as:

    base_delay_ms * (factor ^ attempt)

Default:

    2.0

Example progression:

  Attempt   Delay (ms)
  --------- -------------------------
  0         500
  1         1000
  2         2000
  3         4000
  4         8000
  ...       clamped at max_delay_ms

### jitter_ratio

Expressed as a fraction:

-   0.0 → no jitter
-   0.2 → ±20% randomization window

Important:

The policy does NOT apply jitter directly.

It exposes the parameter so higher layers (Outbox or Engine) can apply
randomness in a controlled, deterministic-aware manner.

# Methods

## can_retry

``` cpp
bool can_retry(std::uint32_t attempt) const noexcept;
```

Returns true if:

    attempt < max_attempts

## compute_delay_ms

``` cpp
std::int64_t compute_delay_ms(std::uint32_t attempt) const noexcept;
```

Computes exponential backoff and clamps the result between:

-   base_delay_ms
-   max_delay_ms

Behavior:

-   attempt 0 → base_delay
-   delay grows exponentially
-   final value is clamped

# Example Usage

``` cpp
RetryPolicy policy;

std::uint32_t attempt = 3;

if (policy.can_retry(attempt))
{
    auto delay = policy.compute_delay_ms(attempt);
    auto next_retry_at = now_ms + delay;
}
else
{
    // mark permanent failure
}
```

# Deterministic Recovery

A key design goal:

Retry timing must be recomputable during recovery.

Since `RetryPolicy` is:

-   Pure
-   Stateless
-   Deterministic

You can replay WAL records and recompute retry windows safely.

This is essential for:

-   Crash recovery
-   Outbox reconstruction
-   Distributed convergence

# Offline-First Considerations

Exponential backoff prevents:

-   Retry storms
-   Network flooding
-   Thundering herd effects

The policy ensures:

-   Controlled retry growth
-   Upper bounded delay
-   Predictable behavior

# Design Philosophy

RetryPolicy is:

-   Minimal
-   Deterministic
-   Side-effect free
-   Configurable
-   Durable-system friendly

It provides one invariant:

> Retry timing must always be recomputable from durable state.

