# WAL Recovery

This page shows how to recover state using the Vix.cpp sync WAL (Write-Ahead Log).

Goal:
- Persist state transitions before any external effect
- Rebuild state deterministically after crash or restart
- Resume safely from an offset (checkpoint)

This guide is written for both:
- Beginners who want a copy-paste recovery flow
- Experts who want deterministic replay and offset discipline

---

## What is WAL recovery

A WAL is an append-only log of state transitions. Recovery means:
1) Open the WAL file
2) Replay records in strict append order
3) Rebuild in-memory state (Outbox, indexes, counters, etc.)
4) Continue processing from a known offset

A key invariant:
- Every transition that matters is appended before side effects.

If the process crashes, you can replay the WAL and reconstruct the last correct state.

---

## Headers

```cpp
#include <vix/sync/wal/Wal.hpp>
#include <vix/sync/wal/WalRecord.hpp>
```

Namespace:
```cpp
vix::sync::wal
```

---

## WAL record types

WAL records describe durable transitions. Typical record types:

- PutOperation: operation was created and persisted
- MarkDone: operation completed successfully
- MarkFailed: operation failed (retryable or permanent, depending on stored fields and policy)

A common recovery strategy:
- Start with empty state
- Apply records in order
- Last record for a given operation id wins

---

## 1) Minimal recovery example (rebuild a map of operations)

This is the simplest possible recovery: we rebuild a map keyed by operation id.

What you learn:
- How to replay from offset 0
- How to apply records deterministically

```cpp
#include <cstdint>
#include <iostream>
#include <string>
#include <unordered_map>

#include <vix/sync/wal/Wal.hpp>
#include <vix/sync/wal/WalRecord.hpp>

using vix::sync::wal::RecordType;
using vix::sync::wal::Wal;
using vix::sync::wal::WalRecord;

struct RecoveredOp
{
  std::string id;
  RecordType last_type{RecordType::PutOperation};
  std::string last_error;
  std::int64_t next_retry_at_ms{0};
};

int main()
{
  std::unordered_map<std::string, RecoveredOp> ops;

  Wal wal(Wal::Config{ "./.vix/wal.log", false });

  wal.replay(0, [&](const WalRecord& rec)
  {
    auto& o = ops[rec.id];
    o.id = rec.id;
    o.last_type = rec.type;

    if (rec.type == RecordType::MarkFailed)
    {
      o.last_error = rec.error;
      o.next_retry_at_ms = rec.next_retry_at_ms;
    }
    else
    {
      o.last_error.clear();
      o.next_retry_at_ms = 0;
    }
  });

  std::cout << "Recovered ops: " << ops.size() << "\n";
  return 0;
}
```

When to use this pattern:
- When you need a minimal reconstruction step before loading higher-level components
- When you want to validate WAL correctness quickly

---

## 2) WAL offsets and checkpoints

WAL replay can be large. You should store a checkpoint offset.

Concept:
- last_applied_offset is the WAL byte offset you have fully processed
- On restart you replay from last_applied_offset, not from 0

Typical places to store the checkpoint:
- A small file: .vix/wal.offset
- A local database table (if you already use SQLite)
- A config store

Minimal checkpoint file example:

```cpp
#include <cstdint>
#include <fstream>
#include <string>

static std::int64_t load_checkpoint(const std::string& path)
{
  std::ifstream in(path);
  std::int64_t off = 0;
  if (in.good())
    in >> off;
  return off;
}

static void save_checkpoint(const std::string& path, std::int64_t off)
{
  std::ofstream out(path, std::ios::trunc);
  out << off;
}
```

Important rule:
- Only save the checkpoint after you have fully applied all records up to that offset.

---

## 3) Practical recovery: rebuild Outbox from WAL

In many offline-first designs:
- The WAL is the source of truth
- The Outbox is reconstructed from WAL at startup

High-level approach:
1) Replay WAL records
2) For PutOperation: insert or update operation data
3) For MarkDone: mark operation done
4) For MarkFailed: mark operation failed and set next retry time

Pseudo-flow:

```cpp
wal.replay(from, [&](const WalRecord& rec)
{
  switch (rec.type)
  {
    case RecordType::PutOperation:
      // decode payload into Operation, then store it
      break;

    case RecordType::MarkDone:
      // mark operation done
      break;

    case RecordType::MarkFailed:
      // mark failed, store error and next retry time
      break;
  }
});
```

Note:
- WAL payload is typically a serialized Operation for PutOperation.
- During recovery, decode and rebuild the Outbox state you need.

---

## 4) How to test WAL recovery (beginner-friendly)

### A) Create a test directory

```bash
mkdir -p .vix_test
```

### B) Run a small program that appends records

Write a tiny program that:
- Creates Wal at .vix_test/wal.log
- Appends a PutOperation
- Appends MarkFailed
- Exits

Then run the recovery program and verify that it reports 1 recovered op, last_type MarkFailed, and the error message.

### C) Re-run recovery multiple times

Recovery must be deterministic. Running the recovery program twice should give the same result.

---

## 5) Crash simulation test (append then crash)

A classic test:
1) Append PutOperation
2) Start sending (simulate by printing)
3) Crash before MarkDone
4) Restart and recover
5) You must see the operation as not done, and eligible for retry if policy says so

Minimal example:

```cpp
// Process:
// - PutOperation appended
// - program exits before MarkDone
// On restart, recovery sees the operation as pending and can retry.
```

This test proves:
- Local accepted work is not lost
- Recovery can safely resume

---

## 6) Common mistakes

### Saving checkpoint too early
If you store last_applied_offset before applying the record, you can skip it on restart.

Rule:
- Apply record first, then persist checkpoint.

### Non-deterministic application
Avoid reading current time during replay decisions. Replay should be pure.
If you need time, store it in the record.

### Mixing WAL and non-WAL writes
If you mutate external state without WAL append first, recovery becomes ambiguous.

---

## 7) Recommended structure in real systems

A robust boot sequence:

1) Load checkpoint offset
2) Replay WAL from checkpoint
3) Rebuild Outbox and indexes
4) Requeue inflight operations older than timeout
5) Start SyncEngine or schedule ticks

That sequence aligns with offline-first invariants:
- durable local state
- deterministic recovery
- safe convergence after failures

