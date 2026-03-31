# Retry Policy Example

This page demonstrates how to use the RetryPolicy in Vix.cpp sync
module.

It explains:

-   What exponential backoff means
-   How retry delay is computed
-   How to test retry logic
-   How retry interacts with Outbox and SyncEngine

## 1) Minimal RetryPolicy Usage

``` cpp
#include <vix/sync/RetryPolicy.hpp>
#include <iostream>

int main()
{
    vix::sync::RetryPolicy policy;

    for (std::uint32_t attempt = 0; attempt < 6; ++attempt)
    {
        if (policy.can_retry(attempt))
        {
            auto delay = policy.compute_delay_ms(attempt);
            std::cout << "Attempt " << attempt
                      << " -> delay = " << delay << " ms\n";
        }
        else
        {
            std::cout << "Attempt " << attempt
                      << " -> no more retries\n";
        }
    }

    return 0;
}
```

What this shows:

-   Delay grows exponentially
-   Delay is clamped by max_delay_ms
-   Retries stop after max_attempts

## 2) Custom Retry Configuration

``` cpp
#include <vix/sync/RetryPolicy.hpp>

int main()
{
    vix::sync::RetryPolicy policy;
    policy.max_attempts = 5;
    policy.base_delay_ms = 1000;   // 1 second
    policy.max_delay_ms = 10000;   // 10 seconds
    policy.factor = 2.0;

    for (std::uint32_t attempt = 0; attempt < 7; ++attempt)
    {
        if (policy.can_retry(attempt))
        {
            auto delay = policy.compute_delay_ms(attempt);
        }
    }

    return 0;
}
```

This configuration means:

Attempt 0 -\> 1s\
Attempt 1 -\> 2s\
Attempt 2 -\> 4s\
Attempt 3 -\> 8s\
Attempt 4 -\> 10s (clamped)\
Attempt 5 -\> stop

## 3) How Retry Works with Outbox

When an operation fails:

1)  SyncWorker calls Outbox::fail()
2)  RetryPolicy computes delay
3)  next_retry_at_ms is set
4)  Operation becomes eligible again later

Minimal flow example:

``` cpp
if (!send_result.ok)
{
    if (send_result.retryable)
    {
        outbox->fail(id, send_result.error, now_ms, true);
    }
    else
    {
        outbox->fail(id, send_result.error, now_ms, false);
    }
}
```

## 4) How to Test Retry Logic (Beginner-Friendly)

You can simulate failure using a fake transport:

-   Return ok = false
-   Return retryable = true
-   Call engine.tick(now_ms)
-   Advance time manually
-   Call engine.tick(later_time)

If attempt \< max_attempts: Operation status -\> Failed (retry
scheduled)

If attempt \>= max_attempts: Operation status -\> PermanentFailed

## 5) Important Rules

-   RetryPolicy is deterministic
-   Delay is recomputable during recovery
-   Retry never mutates payload intent
-   Only lifecycle state changes

RetryPolicy ensures:

Network failures do not cause retry storms. Recovery is predictable.
Offline-first systems remain stable.

