# Offline-first sync

In the previous chapter, you learned cache.
Now you will learn offline-first sync.

```txt
local write → WAL → outbox → sync engine → transport → done
```

The core idea:
**write locally first, persist the operation, sync when the network is available.**

## Why offline-first sync exists

Real applications do not run in perfect conditions.
Networks fail,
servers restart,
requests timeout,
devices go offline.

A normal online-first flow fails when the network is unavailable:

```txt
user action → network request → server response → local state updated
```

Offline-first sync uses a safer flow:

```txt
user action → local write → durable log → outbox → sync later
```

## The mental model

```txt
Never depend on the network to preserve user intent.
```

| Property        | Meaning                                               |
|-----------------|-------------------------------------------------------|
| `Durable`       | The operation survives a process restart.             |
| `Retryable`     | Failed operations can be attempted again safely.      |
| `Offline-first` | The app can continue working without network access.  |

## Vix Sync architecture

```txt
Local Write → WAL → Outbox → SyncWorker → Transport → Done / Retry / Failed
```

| Component        | Purpose                                                    |
|------------------|------------------------------------------------------------|
| `Operation`      | Describes one durable unit of work.                        |
| `Wal`            | Stores an append-only operation history.                   |
| `Outbox`         | Stores operations waiting to be synchronized.              |
| `RetryPolicy`    | Decides when failed operations should retry.               |
| `NetworkProbe`   | Checks whether network access is currently available.      |
| `SyncWorker`     | Processes operations that are ready to run.                |
| `SyncEngine`     | Orchestrates workers, retries, and recovery.               |
| `ISyncTransport` | Sends operations to HTTP, P2P, or custom transport targets.|

## Public headers

```cpp
#include <vix/sync.hpp>
#include <vix/net.hpp>
```

## Operation

```cpp
vix::sync::Operation op;
op.kind = "http.post";
op.target = "/api/messages";
op.payload = R"({"text":"hello"})";
```

## Basic outbox lifecycle

```cpp
// enqueue → claim → complete → Done
const std::string id = outbox->enqueue(op, t0);
auto ready = outbox->peek_ready(t0, 10);
const bool claimed = outbox->claim(id, t0 + 1);
const bool done = outbox->complete(id, t0 + 2);
```

## FileOutboxStore

```cpp
auto store = std::make_shared<vix::sync::outbox::FileOutboxStore>(
    vix::sync::outbox::FileOutboxStore::Config{
        .file_path = dir / "outbox.json",
        .pretty_json = true});

auto outbox = std::make_shared<vix::sync::outbox::Outbox>(
    vix::sync::outbox::Outbox::Config{.owner = "demo"},
    store);
```

## RetryPolicy

```cpp
vix::sync::RetryPolicy retry;
retry.max_attempts = 3;
retry.base_delay_ms = 500;
retry.factor = 2.0;

auto outbox = std::make_shared<vix::sync::outbox::Outbox>(
    vix::sync::outbox::Outbox::Config{
        .owner = "demo-retry",
        .retry = retry},
    store);
```

### Retryable vs permanent failure

```cpp
// Temporary failure — retry later
outbox->fail(id, "temporary network error", now + 2, true);

// Permanent failure — stop retrying
outbox->fail(id, "bad request", now + 2, false);
```

| Failure              | Retry | Example                         |
|----------------------|-------|---------------------------------|
| Network timeout      | Yes   | Temporary connection issue.     |
| Server unavailable   | Yes   | HTTP `503` response.            |
| Offline device       | Yes   | No active network connection.   |
| Bad request          | No    | Invalid request payload.        |
| Validation error     | No    | Missing required input field.   |

## WAL

A WAL (Write-Ahead Log) is an append-only log of durable intent.

```cpp
vix::sync::wal::Wal wal(vix::sync::wal::Wal::Config{
    .file_path = dir / "wal.log",
    .fsync_on_write = false});

const auto offset = wal.append(vix::sync::wal::WalRecord{
    .id = "op_1",
    .type = vix::sync::wal::RecordType::PutOperation,
    .ts_ms = t0,
    .payload = to_bytes(payload)});

wal.replay(0, [](const vix::sync::wal::WalRecord &rec)
           { std::cout << "id=" << rec.id << " type=" << static_cast<int>(rec.type) << "\n"; });
```

## Sync transport

```cpp
namespace vix::sync::engine
{
  class ExampleTransport final : public ISyncTransport
  {
  public:
    SendResult send(const vix::sync::Operation &op) override
    {
      std::cout << "sending: " << op.id << " -> " << op.target << "\n";
      return SendResult{.ok = true};
    }
  };
}
```

## NetworkProbe

```cpp
auto probe = std::make_shared<vix::net::NetworkProbe>(
    vix::net::NetworkProbe::Config{},
    [] { return true; });  // online

// Offline-first control
auto network_online = std::make_shared<std::atomic<bool>>(false);
auto probe = std::make_shared<vix::net::NetworkProbe>(
    vix::net::NetworkProbe::Config{},
    [network_online] { return network_online->load(); });

// Later when network returns
network_online->store(true);
```

## SyncEngine

```cpp
vix::sync::engine::SyncEngine engine(
    vix::sync::engine::SyncEngine::Config{
        .worker_count = 1,
        .idle_sleep_ms = 0,
        .offline_sleep_ms = 0,
        .batch_limit = 10,
        .inflight_timeout_ms = 10'000},
    outbox,
    probe,
    transport);

const auto processed = engine.tick(now_ms());
```

## Complete local-first flow

```txt
local file write → WAL append → outbox enqueue → sync engine tick → transport sends → done
```

The critical rule: **local write happens before sync.**
**Network is not responsible for preserving the user action.**

## Offline then recover pattern

```txt
1. local write succeeds
2. WAL stores intent
3. outbox stores operation
4. network is offline → engine sends nothing (processed = 0)
5. network comes back → network_online->store(true)
6. engine sends operation → operation becomes done
```

## Inflight recovery

If a worker claims an operation and then crashes:

```txt
claim → crash → restart → engine detects stale InFlight → requeue → worker sends → Done
```

This is why durable outbox state matters.

## Recommended operation kinds

```txt
http.post, http.put, http.patch, http.delete
fs.write.sync, fs.delete.sync
db.insert.sync, db.update.sync
message.send, event.publish
```

## Design rules

### Persist before sending
Store the operation first, then send the request. Never send before persisting.

### Treat the network as optional
When the network is available, sync now. When it is unavailable, sync later.

### Make operations idempotent
Use stable operation IDs so the server can detect and ignore duplicates.

### Separate temporary and permanent failures
Retry temporary failures. Do not retry invalid data.

### Keep payloads replayable
Include enough data to replay the operation later.

## Common mistakes

### Sending before persisting
If the process crashes after sending but before storing, recovery becomes impossible. **Persist first.**

### Treating offline as an error
Offline is a normal state in offline-first systems. The operation should remain pending.

### Forgetting inflight recovery
If a worker crashes after claiming, use inflight timeout recovery to requeue.

### Using non-idempotent remote writes
Use stable operation ids and deduplication on the receiver.

## When to use Vix Sync

Use Vix Sync when your application needs durable operations, offline execution, and safe synchronization.

Good use cases:

- Offline-first applications
- Local-first file synchronization
- Reliable message delivery
- Retry-safe background jobs
- Edge applications
- Unstable network environments
- P2P synchronization

## Production notes

- Enable fsync for stronger durability
- Use stable operation ids
- Make remote endpoints idempotent
- Separate retryable and permanent failures
- Monitor pending and failed outbox size
- Expose sync health in `/health`

## What you should remember

The core flow: `local write → WAL → outbox → sync engine → transport → done`

The WAL keeps durable history.
The outbox keeps pending work.
The sync worker processes ready operations.
The transport sends to HTTP, P2P, or custom targets.

The core idea: **persist first, sync later, never lose user intent.**

## Next chapter

[Next: P2P](/book/19-p2p)
