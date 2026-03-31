# Write-Ahead Log (WAL)

This page documents the WAL (Write-Ahead Log) layer in the `sync`
module.

The WAL is a foundational durability primitive used to persist state
transitions before they are applied or synchronized.

It guarantees:

-   Append-only writes
-   Monotonic offsets
-   Deterministic replay
-   Crash recovery safety

## Headers

``` cpp
#include <vix/sync/wal/Wal.hpp>
#include <vix/sync/wal/WalWriter.hpp>
#include <vix/sync/wal/WalReader.hpp>
#include <vix/sync/wal/WalRecord.hpp>
```

Namespace:

``` cpp
vix::sync::wal
```

# Architecture Overview

The WAL layer is composed of:

-   **Wal**: high-level append + replay interface
-   **WalWriter**: append-only record writer
-   **WalReader**: sequential forward reader
-   **WalRecord**: immutable record structure
-   **RecordType**: durable state transition classification

The WAL is append-only and does not modify past entries.

# Wal

`Wal` provides a minimal high-level interface.

``` cpp
class Wal
{
public:
    struct Config
    {
        std::filesystem::path file_path{"./.vix/wal.log"};
        bool fsync_on_write{false};
    };

    explicit Wal(Config cfg);

    std::int64_t append(const WalRecord& rec);

    std::int64_t replay(
        std::int64_t from_offset,
        const std::function<void(const WalRecord&)>& on_record);
};
```

### Guarantees

-   Each `append()` returns a monotonically increasing offset
-   `replay()` processes records strictly in append order
-   Replay is deterministic
-   Durability depends on `fsync_on_write`

# WalRecord

`WalRecord` describes a durable state transition.

``` cpp
enum class RecordType : std::uint8_t
{
    PutOperation = 1,
    MarkDone = 2,
    MarkFailed = 3
};
```

``` cpp
struct WalRecord
{
    std::string id;
    RecordType type{RecordType::PutOperation};
    std::int64_t ts_ms{0};
    std::vector<std::uint8_t> payload;
    std::string error;
    std::int64_t next_retry_at_ms{0};
};
```

### Meaning

-   **id**: operation identifier
-   **type**: transition category
-   **ts_ms**: creation timestamp
-   **payload**: serialized Operation (for PutOperation)
-   **error**: failure diagnostics (for MarkFailed)
-   **next_retry_at_ms**: retry scheduling timestamp

WalRecord is immutable after append.

# WalWriter

Append-only writer.

``` cpp
class WalWriter
{
public:
    struct Config
    {
        std::filesystem::path file_path;
        bool fsync_on_write{false};
    };

    explicit WalWriter(Config cfg);
    ~WalWriter();

    std::int64_t append(const WalRecord& rec);
    void flush();
};
```

Behavior:

-   Opens file lazily
-   Appends serialized records sequentially
-   Returns byte offset of appended record
-   Optional fsync for stronger crash safety

# WalReader

Sequential forward reader.

``` cpp
class WalReader
{
public:
    explicit WalReader(std::filesystem::path file_path);

    void seek(std::int64_t offset);

    std::optional<WalRecord> next();

    std::int64_t current_offset() const noexcept;
};
```

Behavior:

-   Forward-only iteration
-   Lazy file open
-   Offset-based replay
-   Used during recovery

# Typical Usage

## Append before side effects

``` cpp
Wal wal({ "./.vix/wal.log", true });

WalRecord rec;
rec.id = "op-123";
rec.type = RecordType::PutOperation;
rec.ts_ms = now_ms;

auto offset = wal.append(rec);
```

After append succeeds, external effects may proceed.

## Replay during recovery

``` cpp
wal.replay(0, [](const WalRecord& rec)
{
    // Reconstruct Outbox state
});
```

Replay restores state in exact original order.

# Crash Recovery Model

WAL ensures:

-   No committed state transition is lost
-   On restart, replay rebuilds consistent state
-   In-flight operations can be reconstructed
-   Ordering is preserved

WAL is foundational for:

-   Outbox durability
-   Sync engine convergence
-   Deterministic recovery

# Durability Model

Durability depends on configuration:

-   `fsync_on_write = false`
    -   Fast writes
    -   OS-level buffering
    -   Risk window on sudden power loss
-   `fsync_on_write = true`
    -   Stronger crash guarantees
    -   Slower writes

Applications choose tradeoff.

# Offline-First Guarantees

WAL enforces one critical invariant:

> Every state transition is persisted before it can affect the outside
> world.

This ensures:

-   Local writes are durable
-   Replay is deterministic
-   Network failures never corrupt state
-   Crash recovery is correct

# Design Philosophy

The WAL layer is:

-   Append-only
-   Minimal
-   Deterministic
-   Explicit
-   Transport-independent

It is the lowest-level durability primitive in the sync stack.

