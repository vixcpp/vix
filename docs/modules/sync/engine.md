# Sync Engine

This page documents the sync engine layer in the `sync` module.

The engine layer coordinates the offline-first synchronization loop by
driving workers that pull operations from the Outbox and deliver them
through a transport.

## Headers

``` cpp
#include <vix/sync/engine/SyncEngine.hpp>
#include <vix/sync/engine/SyncWorker.hpp>
```

Namespace:

``` cpp
vix::sync::engine
```

## Architecture Overview

The engine layer is composed of:

-   SyncEngine: orchestrator and background loop
-   SyncWorker: executes batch processing of operations
-   ISyncTransport: pluggable delivery mechanism (HTTP, WebSocket, P2P,
    edge, etc.)
-   NetworkProbe: connectivity signal used to adapt behavior
-   Outbox: durable queue of operations to send (local-first)

High-level control flow:

1.  SyncEngine starts (manual ticks or background thread)
2.  Each tick drives one or more SyncWorker instances
3.  Each worker pulls ready operations from the Outbox
4.  Worker sends operations via ISyncTransport
5.  Worker updates Outbox state based on success or retryability
6.  Engine sleeps differently when idle or offline

## SyncEngine

SyncEngine is the coordinator for the offline-first sync loop.

Responsibilities:

-   Spawns and owns a set of SyncWorker instances
-   Periodically ticks workers to pull operations from the Outbox
-   Uses NetworkProbe to adapt behavior when offline or online
-   Delegates actual I/O to an ISyncTransport implementation
-   Can be run manually (tick) or as a background loop (start/stop)

### Config

All time values are expressed in milliseconds.

``` cpp
struct Config
{
    std::size_t worker_count{1};
    std::int64_t idle_sleep_ms{250};
    std::int64_t offline_sleep_ms{500};
    std::size_t batch_limit{25};
    std::int64_t inflight_timeout_ms{10'000};
};
```

Meaning:

-   worker_count: number of workers created and owned by the engine
-   idle_sleep_ms: sleep duration when there is nothing to process
-   offline_sleep_ms: sleep duration when the network is considered
    offline
-   batch_limit: maximum operations pulled per batch (per worker tick)
-   inflight_timeout_ms: how long an operation may remain in-flight
    before it can be treated as timed out

### Lifecycle

``` cpp
SyncEngine(
    Config cfg,
    std::shared_ptr<vix::sync::outbox::Outbox> outbox,
    std::shared_ptr<vix::net::NetworkProbe> probe,
    std::shared_ptr<ISyncTransport> transport);

~SyncEngine();

std::size_t tick(std::int64_t now_ms);

void start();
void stop();

bool running() const noexcept;
```

Important notes:

-   start() spawns an internal background thread and runs the control
    loop
-   stop() requests shutdown and joins the thread
-   tick(now_ms) runs one engine iteration and returns a best-effort
    processed count
-   Avoid calling tick() concurrently with start()/stop()

## SyncWorker

SyncWorker is the unit that processes ready operations from the Outbox.

Responsibilities:

-   Consults NetworkProbe to decide whether sending should proceed
-   Pulls a batch of ready operations from the Outbox
-   Sends operations through ISyncTransport
-   Applies retry/backoff decisions by updating Outbox state (policy
    lives in Outbox)

### Config

All time values are expressed in milliseconds.

``` cpp
struct Config
{
    std::size_t batch_limit{25};
    std::int64_t idle_sleep_ms{250};
    std::int64_t offline_sleep_ms{500};
    std::int64_t inflight_timeout_ms{10'000};
};
```

Meaning:

-   batch_limit: max operations per tick
-   idle_sleep_ms: recommended sleep when idle (used by orchestrators)
-   offline_sleep_ms: recommended sleep when offline (used by
    orchestrators)
-   inflight_timeout_ms: max time an op may remain in-flight before it
    becomes eligible for retry

### API

``` cpp
SyncWorker(
    Config cfg,
    std::shared_ptr<vix::sync::outbox::Outbox> outbox,
    std::shared_ptr<vix::net::NetworkProbe> probe,
    std::shared_ptr<ISyncTransport> transport);

std::size_t tick(std::int64_t now_ms);
```

tick(now_ms) processes up to batch_limit operations, best-effort.

## ISyncTransport and SendResult

The engine is transport-agnostic.

### SendResult

``` cpp
struct SendResult
{
    bool ok{false};
    bool retryable{true};
    std::string error;
};
```

Meaning:

-   ok: remote accepted the operation
-   retryable: if false, the worker may drop or dead-letter the
    operation (depending on Outbox policy)
-   error: optional diagnostics text

### Transport Interface

``` cpp
class ISyncTransport
{
public:
    virtual ~ISyncTransport() = default;
    virtual SendResult send(const vix::sync::Operation &op) = 0;
};
```

Implementation targets can include:

-   HTTP batch endpoints
-   WebSocket push/ack
-   P2P peer replication
-   Edge store-and-forward gateways

## Usage Pattern

There are two ways to drive the engine.

### Manual driving (single-threaded integration)

Use this when you already have a loop (game loop, server loop, embedded
runtime).

``` cpp
#include <vix/sync/engine/SyncEngine.hpp>

void sync_step(vix::sync::engine::SyncEngine& engine, std::int64_t now_ms)
{
    engine.tick(now_ms);
}
```

### Background loop

Use this when you want the sync engine to self-manage in its own thread.

``` cpp
engine.start();

// ... application runs ...

engine.stop();
```

## Thread-Safety Contract

-   start()/stop() manage an internal thread
-   tick() is intended to be called by the engine thread or by a single
    external driver
-   Avoid calling tick() concurrently with start()/stop()

## Design Philosophy

The engine layer is built to satisfy offline-first requirements:

-   Outbox is the source of truth for pending operations
-   Network is an optimization, not a requirement
-   Transport is pluggable and interchangeable
-   All behavior is explicit, observable, and deterministic where
    possible

