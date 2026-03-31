# Outbox

This page documents the durable outbox layer in the `sync` module.

The outbox implements the core **offline-first outbox pattern**:

-   All operations are persisted before any network attempt
-   Operations are claimed before being processed
-   Completion, retry and permanent failure are recorded durably
-   Network failures never lose accepted local writes

## Headers

``` cpp
#include <vix/sync/outbox/Outbox.hpp>
#include <vix/sync/outbox/OutboxStore.hpp>
#include <vix/sync/outbox/FileOutboxStore.hpp>
```

Namespace:

``` cpp
vix::sync::outbox
```

# Architecture Overview

The outbox layer is composed of:

-   **Outbox**: high-level coordination and retry logic
-   **OutboxStore**: abstract persistence contract
-   **FileOutboxStore**: file-backed JSON implementation
-   **RetryPolicy**: retry scheduling logic
-   **Operation**: immutable sync payload unit

The Outbox itself contains no network logic. It delegates transport to
the engine layer.

# Outbox

`Outbox` is the high-level facade that enforces the outbox pattern.

Responsibilities:

-   Persist operations before sending
-   Generate ids and idempotency keys when required
-   Apply retry policy
-   Coordinate claim / complete / fail transitions
-   Delegate persistence to `OutboxStore`

## Config

``` cpp
struct Config
{
    std::string owner{"vix-sync"};
    RetryPolicy retry{};
    bool auto_generate_ids{true};
    bool auto_generate_idempotency_key{true};
};
```

Meaning:

-   **owner**: logical worker/engine name used when claiming operations
-   **retry**: retry scheduling policy
-   **auto_generate_ids**: auto-create operation ids if missing
-   **auto_generate_idempotency_key**: auto-create idempotency keys if
    missing

## Core API

``` cpp
Outbox(Config cfg, std::shared_ptr<OutboxStore> store);

std::string enqueue(Operation op, std::int64_t now_ms);

std::vector<Operation> peek_ready(std::int64_t now_ms, std::size_t limit = 50);

bool claim(const std::string& id, std::int64_t now_ms);

bool complete(const std::string& id, std::int64_t now_ms);

bool fail(
    const std::string& id,
    const std::string& error,
    std::int64_t now_ms,
    bool retryable = true);
```

### Lifecycle

1.  `enqueue()` persists operation immediately
2.  `peek_ready()` returns eligible candidates
3.  `claim()` marks operation as in-flight
4.  `complete()` marks success
5.  `fail()` schedules retry or permanent failure

# OutboxStore

`OutboxStore` defines the minimal persistence contract required by
Outbox.

It guarantees:

-   Durable persistence
-   Claim ownership tracking
-   Retry scheduling
-   Cleanup helpers
-   Crash recovery support

It is transport-agnostic.

## ListOptions

``` cpp
struct ListOptions
{
    std::size_t limit{50};
    std::int64_t now_ms{0};
    bool only_ready{true};
    bool include_inflight{false};
};
```

Used to filter operations based on readiness and state.

## Required Methods

``` cpp
virtual void put(const Operation& op) = 0;
virtual std::optional<Operation> get(const std::string& id) = 0;
virtual std::vector<Operation> list(const ListOptions& opt) = 0;

virtual bool claim(const std::string& id,
                   const std::string& owner,
                   std::int64_t now_ms) = 0;

virtual bool mark_done(const std::string& id,
                       std::int64_t now_ms) = 0;

virtual bool mark_failed(const std::string& id,
                         const std::string& error,
                         std::int64_t now_ms,
                         std::int64_t next_retry_at_ms) = 0;

virtual bool mark_permanent_failed(const std::string& id,
                                   const std::string& error,
                                   std::int64_t now_ms) = 0;

virtual std::size_t prune_done(std::int64_t older_than_ms) = 0;

virtual std::size_t requeue_inflight_older_than(std::int64_t now_ms,
                                                std::int64_t timeout_ms) = 0;
```

Implementations must preserve correctness guarantees.

Thread-safety is implementation-defined.

# FileOutboxStore

`FileOutboxStore` is a simple, durable JSON-based implementation.

Designed for:

-   Offline-first environments
-   Crash recovery
-   Local-first apps
-   Zero external dependencies

It favors correctness and simplicity over throughput.

## Config

``` cpp
struct Config
{
    std::filesystem::path file_path{"./.vix/outbox.json"};
    bool pretty_json{false};
    bool fsync_on_write{false};
};
```

Meaning:

-   **file_path**: persistence location
-   **pretty_json**: human-readable output
-   **fsync_on_write**: stronger durability at cost of performance

## Behavior

-   Lazy load on first access
-   In-memory representation protected by mutex
-   Writes flushed to disk on mutation
-   Optional fsync for crash durability

# Crash Recovery

Outbox supports:

-   Reclaiming timed-out in-flight operations
-   Retry scheduling
-   Permanent failure marking
-   Pruning old completed operations

Recovery helpers:

``` cpp
requeue_inflight_older_than(now_ms, timeout_ms);
prune_done(older_than_ms);
```

# Offline-First Guarantees

The outbox pattern ensures:

-   Local write is durable before network attempt
-   Network failure never discards accepted state
-   Retries are deterministic
-   Idempotency prevents duplicate effects
-   Recovery after crash is possible

This is foundational for Softadastra-style convergence.

# Design Philosophy

Outbox is:

-   Durable first
-   Transport-agnostic
-   Deterministic
-   Explicit
-   Policy-driven (RetryPolicy)

It enforces one invariant:

> No network attempt without prior durable persistence.

