# Operation

This page documents the `Operation` type in the `sync` module.

`Operation` is the fundamental durable unit flowing through:

-   WAL
-   Outbox
-   SyncEngine
-   Transport layer

It represents **what must be done**, while its lifecycle fields track
**how it evolves over time**.

## Header

``` cpp
#include <vix/sync/Operation.hpp>
```

Namespace:

``` cpp
vix::sync
```

# Overview

An `Operation` is:

-   Immutable in intent (kind, target, payload)
-   Mutable in lifecycle state (status, attempts, timestamps)
-   Persisted durably before any network effect
-   Replay-safe and recovery-friendly

It is designed for offline-first correctness.

# OperationStatus

``` cpp
enum class OperationStatus : std::uint8_t
{
    Pending = 0,
    InFlight,
    Done,
    Failed,
    PermanentFailed
};
```

### Meaning

-   **Pending**: persisted, waiting to be processed
-   **InFlight**: claimed and currently being processed
-   **Done**: successfully completed
-   **Failed**: failed but may be retried
-   **PermanentFailed**: failed permanently, no more retries

This status is durable and drives retry logic.

# Operation Structure

``` cpp
struct Operation
{
    std::string id;
    std::string kind;
    std::string target;
    std::string payload;
    std::string idempotency_key;

    std::int64_t created_at_ms{0};
    std::int64_t updated_at_ms{0};

    std::uint32_t attempt{0};
    std::int64_t next_retry_at_ms{0};

    OperationStatus status{OperationStatus::Pending};

    std::string last_error;
};
```

# Fields Explained

### id

Unique identifier for the operation.

Generated automatically by Outbox when configured.

### kind

Logical category of operation.

Examples:

-   `"http.request"`
-   `"p2p.message"`
-   `"db.mutation"`

### target

Destination or logical target.

Examples:

-   URL
-   Peer ID
-   Resource name

### payload

Opaque serialized data required to execute the operation.

Interpretation depends on transport or higher layer.

### idempotency_key

Used to deduplicate retries on the remote side.

Ensures that repeated delivery does not cause duplicate effects.

### created_at_ms

Timestamp when operation was created.

### updated_at_ms

Timestamp of last lifecycle transition.

### attempt

Number of delivery attempts performed so far.

Incremented when retrying.

### next_retry_at_ms

Time when the operation becomes eligible for retry.

Computed using RetryPolicy.

### status

Current durable lifecycle state.

### last_error

Most recent error message, if any.

Used for diagnostics and logging.

# Lifecycle Helpers

## is_done()

``` cpp
bool is_done() const noexcept;
```

Returns true if status == Done.

## is_pending()

``` cpp
bool is_pending() const noexcept;
```

Returns true if status == Pending.

## is_failed()

``` cpp
bool is_failed() const noexcept;
```

Returns true if status == Failed.

## fail()

``` cpp
void fail(std::string err, std::int64_t now_ms);
```

Transitions operation to:

    status = Failed

Updates:

-   last_error
-   updated_at_ms

Retry scheduling is handled by Outbox using RetryPolicy.

## done()

``` cpp
void done(std::int64_t now_ms);
```

Transitions operation to:

    status = Done

Clears:

-   last_error

# Typical Lifecycle Flow

1.  Operation created
2.  Persisted in Outbox (status = Pending)
3.  Claimed (status = InFlight)
4.  Delivered successfully → Done or
5.  Failed → Failed → scheduled retry
6.  Exceeded retry attempts → PermanentFailed

All transitions are durable.

# Offline-First Guarantees

Operation enables:

-   Crash-safe retries
-   Idempotent delivery
-   Deterministic replay
-   Durable state reconstruction

It is the atomic unit of convergence in the sync system.

# Design Philosophy

Operation is:

-   Simple
-   Durable
-   Deterministic
-   Explicit
-   Policy-driven (RetryPolicy + Outbox)

One invariant:

> An operation's intent never changes, only its lifecycle state evolves.

