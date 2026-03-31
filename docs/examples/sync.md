# Offline-first Sync

This page is a practical, beginner-friendly guide for the Vix.cpp offline-first sync engine.

Goal: You can enqueue operations locally, stay offline, crash/restart, then safely converge when the network comes back.

You will learn:

- What the SyncEngine does
- How Outbox makes writes durable before any network attempt
- How RetryPolicy schedules retries
- How to test your setup quickly (copy-paste + run)
- How to simulate failures (offline, retryable errors, permanent errors, crash)

---

## 0) Mental model (what happens at runtime)

When you call `outbox.enqueue(op)`:

1. The operation is persisted to disk (OutboxStore).
2. The engine can try to deliver it later (now or when online).

When the engine ticks:

1. It asks Outbox for ready operations.
2. It claims them (InFlight).
3. It sends them through a transport (HTTP, WS, P2P, edge).
4. It marks them Done or Failed (and schedules retry).

Key offline-first rule:

- No network attempt happens unless the operation was persisted first.

---

## Headers

Engine:

```cpp
#include <vix/sync/engine/SyncEngine.hpp>
#include <vix/sync/engine/SyncWorker.hpp>
```

Outbox:

```cpp
#include <vix/sync/outbox/Outbox.hpp>
#include <vix/sync/outbox/OutboxStore.hpp>
#include <vix/sync/outbox/FileOutboxStore.hpp>
```

Network:

```cpp
#include <vix/net/NetworkProbe.hpp>
```

---

## How to run these examples

You have two common ways:

### Option A) Use Vix CLI (fastest)

If you already use `vix run`:

```bash
vix run main.cpp
```

This compiles and runs the file.

### Option B) Build with CMake

If your project is a normal CMake project, create a small target and link Vix. Then:

```bash
cmake -S . -B build
cmake --build build -j
./build/sync_example
```

Notes for beginners:

- These examples write files under `./.vix_test_*`. You can delete that folder anytime.
- They use `assert(...)` to keep checks minimal.

---

## Helper: now_ms() (used by all examples)

The sync engine uses millisecond timestamps.

```cpp
#include <chrono>
#include <cstdint>

static std::int64_t now_ms()
{
  using namespace std::chrono;
  return duration_cast<milliseconds>(
           steady_clock::now().time_since_epoch()
         ).count();
}
```

---

## Example 1: Minimal smoke test (enqueue + tick + Done)

This is the smallest complete setup: file-backed outbox + always-online probe + transport + engine.

What you should see:

- The engine processes at least 1 operation.
- The operation ends as `Done`.

```cpp
#include <cassert>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

#include <vix/net/NetworkProbe.hpp>
#include <vix/sync/Operation.hpp>
#include <vix/sync/engine/SyncEngine.hpp>
#include <vix/sync/outbox/FileOutboxStore.hpp>
#include <vix/sync/outbox/Outbox.hpp>

// Copy from Appendix at the end of this file
#include "fake_http_transport.hpp"

static std::int64_t now_ms()
{
  using namespace std::chrono;
  return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

static void reset_dir(const std::filesystem::path& dir)
{
  std::error_code ec;
  std::filesystem::remove_all(dir, ec);
  std::filesystem::create_directories(dir, ec);
}

static void run_example_1()
{
  using namespace vix::sync;
  using namespace vix::sync::engine;
  using namespace vix::sync::outbox;

  const std::filesystem::path dir = "./.vix_test_smoke";
  reset_dir(dir);

  auto store = std::make_shared<FileOutboxStore>(FileOutboxStore::Config{
    .file_path = dir / "outbox.json",
    .pretty_json = true,
    .fsync_on_write = false
  });

  auto outbox = std::make_shared<Outbox>(
    Outbox::Config{ .owner = "example-1" },
    store
  );

  auto probe = std::make_shared<vix::net::NetworkProbe>(
    vix::net::NetworkProbe::Config{},
    [] { return true; } // always online
  );

  auto transport = std::make_shared<FakeHttpTransport>();
  transport->setDefault({ .ok = true });

  SyncEngine engine(
    SyncEngine::Config{ .worker_count = 1, .batch_limit = 10 },
    outbox, probe, transport
  );

  Operation op;
  op.kind = "http.post";
  op.target = "/api/messages";
  op.payload = R"({"text":"hello offline"})";

  const auto id = outbox->enqueue(op, now_ms());

  const auto processed = engine.tick(now_ms());
  assert(processed >= 1);

  auto saved = store->get(id);
  assert(saved.has_value());
  assert(saved->status == OperationStatus::Done);

  std::cout << "[example-1] OK: operation is Done\n";
}

int main()
{
  run_example_1();
  return 0;
}
```

Beginner tip:

- Open `./.vix_test_smoke/outbox.json` to see the durable state.

---

## Example 2: Retryable failure (fails first, succeeds later)

This simulates a flaky network or a temporary server error.

What you should see:

- First tick: operation becomes Failed and a retry is scheduled.
- Later tick: operation becomes Done.

```cpp
#include <cassert>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

#include <vix/net/NetworkProbe.hpp>
#include <vix/sync/Operation.hpp>
#include <vix/sync/engine/SyncEngine.hpp>
#include <vix/sync/outbox/FileOutboxStore.hpp>
#include <vix/sync/outbox/Outbox.hpp>

#include "fake_http_transport.hpp"

static std::int64_t now_ms()
{
  using namespace std::chrono;
  return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

static void reset_dir(const std::filesystem::path& dir)
{
  std::error_code ec;
  std::filesystem::remove_all(dir, ec);
  std::filesystem::create_directories(dir, ec);
}

static void run_example_2()
{
  using namespace vix::sync;
  using namespace vix::sync::engine;
  using namespace vix::sync::outbox;

  const std::filesystem::path dir = "./.vix_test_retry";
  reset_dir(dir);

  auto store = std::make_shared<FileOutboxStore>(FileOutboxStore::Config{
    .file_path = dir / "outbox.json",
    .pretty_json = true,
    .fsync_on_write = false
  });

  auto outbox = std::make_shared<Outbox>(
    Outbox::Config{ .owner = "example-2" },
    store
  );

  auto probe = std::make_shared<vix::net::NetworkProbe>(
    vix::net::NetworkProbe::Config{},
    [] { return true; }
  );

  auto transport = std::make_shared<FakeHttpTransport>();

  // First attempt fails (retryable), then succeed.
  transport->setRuleForTarget("/api/messages", FakeHttpTransport::Rule{
    .ok = false,
    .retryable = true,
    .error = "temporary error (retryable)"
  });

  SyncEngine engine(
    SyncEngine::Config{ .worker_count = 1, .batch_limit = 10 },
    outbox, probe, transport
  );

  Operation op;
  op.kind = "http.post";
  op.target = "/api/messages";
  op.payload = R"({"text":"retry me"})";

  const auto t0 = now_ms();
  const auto id = outbox->enqueue(op, t0);

  // Tick 1: should fail (retryable)
  engine.tick(t0);

  {
    auto saved = store->get(id);
    assert(saved.has_value());
    assert(saved->status == OperationStatus::Failed || saved->status == OperationStatus::Pending);
  }

  // Switch transport to success for next attempt
  transport->setRuleForTarget("/api/messages", FakeHttpTransport::Rule{ .ok = true });

  // Tick 2: run in the future so retry window can pass.
  const auto t1 = t0 + 10'000;
  engine.tick(t1);

  auto final = store->get(id);
  assert(final.has_value());
  assert(final->status == OperationStatus::Done);

  std::cout << "[example-2] OK: retryable failure eventually becomes Done\n";
}

int main()
{
  run_example_2();
  return 0;
}
```

Beginner tip:

- Retry scheduling is computed using RetryPolicy.
- You can tune delays and max attempts in `Outbox::Config{ .retry = ... }`.

---

## Example 3: Permanent failure (do not retry)

This simulates a permanent error such as invalid request data.

What you should see:

- The operation becomes PermanentFailed.
- The transport is not called again for that operation.

```cpp
#include <cassert>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

#include <vix/net/NetworkProbe.hpp>
#include <vix/sync/Operation.hpp>
#include <vix/sync/engine/SyncEngine.hpp>
#include <vix/sync/outbox/FileOutboxStore.hpp>
#include <vix/sync/outbox/Outbox.hpp>

#include "fake_http_transport.hpp"

static std::int64_t now_ms()
{
  using namespace std::chrono;
  return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

static void reset_dir(const std::filesystem::path& dir)
{
  std::error_code ec;
  std::filesystem::remove_all(dir, ec);
  std::filesystem::create_directories(dir, ec);
}

static void run_example_3()
{
  using namespace vix::sync;
  using namespace vix::sync::engine;
  using namespace vix::sync::outbox;

  const std::filesystem::path dir = "./.vix_test_perm";
  reset_dir(dir);

  auto store = std::make_shared<FileOutboxStore>(FileOutboxStore::Config{
    .file_path = dir / "outbox.json",
    .pretty_json = true,
    .fsync_on_write = false
  });

  auto outbox = std::make_shared<Outbox>(
    Outbox::Config{ .owner = "example-3" },
    store
  );

  auto probe = std::make_shared<vix::net::NetworkProbe>(
    vix::net::NetworkProbe::Config{},
    [] { return true; }
  );

  auto transport = std::make_shared<FakeHttpTransport>();
  transport->setRuleForTarget("/api/messages", FakeHttpTransport::Rule{
    .ok = false,
    .retryable = false,
    .error = "bad request (permanent)"
  });

  SyncEngine engine(
    SyncEngine::Config{ .worker_count = 1, .batch_limit = 10 },
    outbox, probe, transport
  );

  Operation op;
  op.kind = "http.post";
  op.target = "/api/messages";
  op.payload = R"({"text":"invalid data"})";

  const auto t0 = now_ms();
  const auto id = outbox->enqueue(op, t0);

  engine.tick(t0);

  {
    auto saved = store->get(id);
    assert(saved.has_value());
    assert(saved->status == OperationStatus::PermanentFailed);
  }

  const auto calls_after_first = transport->callCount();
  engine.tick(t0 + 10'000);
  assert(transport->callCount() == calls_after_first);

  std::cout << "[example-3] OK: permanent failure is not retried\n";
}

int main()
{
  run_example_3();
  return 0;
}
```

---

## Example 4: Crash simulation (InFlight timeout requeue)

This simulates a crash after claiming an operation but before completing it.

What you should see:

- The operation is InFlight.
- After inflight_timeout_ms, the engine requeues it.
- Next tick delivers it and marks Done.

```cpp
#include <cassert>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

#include <vix/net/NetworkProbe.hpp>
#include <vix/sync/Operation.hpp>
#include <vix/sync/engine/SyncEngine.hpp>
#include <vix/sync/outbox/FileOutboxStore.hpp>
#include <vix/sync/outbox/Outbox.hpp>

#include "fake_http_transport.hpp"

static std::int64_t now_ms()
{
  using namespace std::chrono;
  return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

static void reset_dir(const std::filesystem::path& dir)
{
  std::error_code ec;
  std::filesystem::remove_all(dir, ec);
  std::filesystem::create_directories(dir, ec);
}

static void run_example_4()
{
  using namespace vix::sync;
  using namespace vix::sync::engine;
  using namespace vix::sync::outbox;

  const std::filesystem::path dir = "./.vix_test_inflight";
  reset_dir(dir);

  auto store = std::make_shared<FileOutboxStore>(FileOutboxStore::Config{
    .file_path = dir / "outbox.json",
    .pretty_json = true,
    .fsync_on_write = false
  });

  auto outbox = std::make_shared<Outbox>(
    Outbox::Config{ .owner = "example-4" },
    store
  );

  auto probe = std::make_shared<vix::net::NetworkProbe>(
    vix::net::NetworkProbe::Config{},
    [] { return true; }
  );

  auto transport = std::make_shared<FakeHttpTransport>();
  transport->setDefault({ .ok = true });

  SyncEngine::Config cfg;
  cfg.worker_count = 1;
  cfg.batch_limit = 10;
  cfg.idle_sleep_ms = 0;
  cfg.offline_sleep_ms = 0;
  cfg.inflight_timeout_ms = 50;

  SyncEngine engine(cfg, outbox, probe, transport);

  Operation op;
  op.kind = "http.post";
  op.target = "/api/messages";
  op.payload = R"({"text":"recover me"})";

  const auto t0 = now_ms();
  const auto id = outbox->enqueue(op, t0);

  // Simulate crash: claim without completing.
  const bool claimed = outbox->claim(id, t0);
  assert(claimed);

  {
    auto saved = store->get(id);
    assert(saved.has_value());
    assert(saved->status == OperationStatus::InFlight);
  }

  // Engine sees inflight timeout and requeues
  const auto t1 = t0 + 60;
  engine.tick(t1);

  // Next tick should deliver
  engine.tick(t1 + 1);

  auto final = store->get(id);
  assert(final.has_value());
  assert(final->status == OperationStatus::Done);

  std::cout << "[example-4] OK: inflight timeout requeues and completes\n";
}

int main()
{
  run_example_4();
  return 0;
}
```

---

## Beginner debugging checklist

If an operation does not become Done:

1. Open your outbox file (example: `./.vix_test_smoke/outbox.json`).
2. Check status, attempt, last_error, and next_retry_at_ms.
3. Confirm your NetworkProbe returns true (online).
4. Confirm your transport returns { ok=true }.
5. Confirm your engine tick uses a now_ms that is increasing.

---

## Appendix: Minimal Fake Transport (copy-paste)

Save this as `fake_http_transport.hpp` next to your `main.cpp`.

```cpp
#ifndef VIX_FAKE_HTTP_TRANSPORT_HPP
#define VIX_FAKE_HTTP_TRANSPORT_HPP

#include <string>
#include <unordered_map>

#include <vix/sync/engine/SyncWorker.hpp>

namespace vix::sync::engine
{
  class FakeHttpTransport final : public ISyncTransport
  {
  public:
    struct Rule
    {
      bool ok{true};
      bool retryable{true};
      std::string error{"simulated failure"};
    };

    void setDefault(Rule r) { def_ = std::move(r); }

    void setRuleForKind(std::string kind, Rule r)
    {
      by_kind_[std::move(kind)] = std::move(r);
    }

    void setRuleForTarget(std::string target, Rule r)
    {
      by_target_[std::move(target)] = std::move(r);
    }

    std::size_t callCount() const noexcept { return calls_; }

    SendResult send(const vix::sync::Operation& op) override
    {
      ++calls_;

      if (auto it = by_target_.find(op.target); it != by_target_.end())
        return toResult(it->second);

      if (auto it = by_kind_.find(op.kind); it != by_kind_.end())
        return toResult(it->second);

      return toResult(def_);
    }

  private:
    static SendResult toResult(const Rule& r)
    {
      SendResult res;
      res.ok = r.ok;
      res.retryable = r.retryable;
      res.error = r.ok ? "" : r.error;
      return res;
    }

  private:
    Rule def_{};
    std::unordered_map<std::string, Rule> by_kind_;
    std::unordered_map<std::string, Rule> by_target_;
    std::size_t calls_{0};
  };
} // namespace vix::sync::engine

#endif
```

