# Outbox Pattern

This page shows how to use the Outbox pattern in Vix.cpp to make local writes durable before any network attempt.

If you are building offline-first systems (mobile, edge, unstable networks), this is one of the safest building blocks you can start with.

You will learn:

- What the Outbox pattern is (in simple terms)
- How to enqueue operations (durable local writes)
- How to process operations with a transport
- How retries and permanent failures work
- How to test everything quickly on your machine

---

## What is the Outbox pattern?

When your app wants to do a remote action (send an HTTP request, publish an event, replicate to a peer), you have two choices:

1. Do the network call immediately
2. Save the intent locally first, then do the network call

The Outbox pattern is the second choice:

- You first persist an Operation on disk (durable)
- A worker later sends it over the network
- If the network fails, the operation stays on disk and is retried

This prevents a common failure:

- "User clicked Save, we said OK, then the network dropped and the write was lost."

With Outbox:

- If the app accepted the write, it is stored locally and cannot disappear.

---

## Concepts used in Vix sync

- Operation: the unit of work you want to deliver
- Outbox: durable queue that stores operations and manages lifecycle
- OutboxStore: persistence backend (file store, database store, etc.)
- SyncEngine: drives workers that send operations using a transport
- Transport: pluggable sender (HTTP, WS, P2P, edge gateway, etc.)

---

## 1) Minimal Outbox (enqueue + send + Done)

This is the smallest complete workflow:

- create a file-backed outbox store
- create an outbox
- enqueue one operation
- run one engine tick
- verify it becomes Done

### Example

```cpp
#include <cassert>
#include <chrono>
#include <iostream>
#include <memory>

#include <vix/net/NetworkProbe.hpp>
#include <vix/sync/Operation.hpp>
#include <vix/sync/engine/SyncEngine.hpp>
#include <vix/sync/outbox/FileOutboxStore.hpp>
#include <vix/sync/outbox/Outbox.hpp>

// A tiny rule-based transport used in examples/tests
#include "fake_http_transport.hpp"

static std::int64_t now_ms()
{
  using namespace std::chrono;
  return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

int main()
{
  using namespace vix::sync;
  using namespace vix::sync::outbox;
  using namespace vix::sync::engine;

  auto store = std::make_shared<FileOutboxStore>(FileOutboxStore::Config{
      .file_path = "./.vix_demo/outbox.json",
      .pretty_json = true,
      .fsync_on_write = false});

  auto outbox = std::make_shared<Outbox>(Outbox::Config{
      .owner = "demo-engine",
      .auto_generate_ids = true,
      .auto_generate_idempotency_key = true}, store);

  auto probe = std::make_shared<vix::net::NetworkProbe>(
      vix::net::NetworkProbe::Config{},
      [] { return true; }); // always online for demo

  auto transport = std::make_shared<FakeHttpTransport>();
  transport->setDefault({.ok = true});

  SyncEngine engine(SyncEngine::Config{.worker_count = 1, .batch_limit = 10},
                    outbox, probe, transport);

  Operation op;
  op.kind = "http.post";
  op.target = "/api/messages";
  op.payload = R"({"text":"hello from outbox"})";

  const auto id = outbox->enqueue(op, now_ms());

  const auto processed = engine.tick(now_ms());
  (void)processed;

  auto saved = store->get(id);
  assert(saved.has_value());
  assert(saved->status == OperationStatus::Done);

  std::cout << "OK: operation was sent and marked Done\n";
  return 0;
}
```

### How to run this example

Typical workflow (adapt to your project layout):

```bash
# 1) Build
cmake -S . -B build && cmake --build build -j

# 2) Run
./build/examples/outbox_minimal
```

### What to look for

After running, you should see a local file created:

- ./.vix_demo/outbox.json

It represents durable state. Even if the process crashed, the operation record exists.

---

## 2) Understanding Operation fields (beginner-friendly)

An Operation has two parts:

### A) Intent (what you want to do)

- kind: category, ex: "http.post", "p2p.replicate"
- target: destination, ex: "/api/messages", "peer:abc123"
- payload: serialized data, ex: JSON string
- idempotency_key: used to deduplicate retries on remote side

### B) Lifecycle (what happened so far)

- status: Pending, InFlight, Done, Failed, PermanentFailed
- attempt: how many attempts were made
- next_retry_at_ms: when it can be retried
- last_error: last failure reason

The key idea:

- intent stays the same
- lifecycle evolves over time

---

## 3) Retryable failure vs permanent failure

Your transport returns:

```cpp
struct SendResult
{
  bool ok{false};
  bool retryable{true};
  std::string error;
};
```

Meaning:

- ok = true -> operation becomes Done
- ok = false and retryable = true -> operation becomes Failed and will retry later
- ok = false and retryable = false -> operation becomes PermanentFailed and will not retry

### Example: permanent failure

```cpp
transport->setRuleForTarget("/api/messages", FakeHttpTransport::Rule{
  .ok = false,
  .retryable = false,
  .error = "bad request (permanent)"
});
```

This is what you want for:

- schema validation errors (400)
- forbidden (403)
- "this will never succeed if retried"

---

## 4) Offline mode simulation (no network)

A beginner way to understand offline-first is to simulate offline:

```cpp
auto probe = std::make_shared<vix::net::NetworkProbe>(
  vix::net::NetworkProbe::Config{},
  [] { return false; } // always offline
);
```

When offline:

- the engine ticks, but does not deliver
- operations stay Pending (durable on disk)

Later, when the probe returns true again, delivery resumes.

---

## 5) Crash safety: stuck InFlight operations

A classic crash scenario:

1. Operation is claimed (status becomes InFlight)
2. Process crashes before marking Done/Failed
3. On restart, the operation is stuck InFlight forever if you do nothing

Vix outbox stores support recovery:

- requeue_inflight_older_than(now_ms, timeout_ms)

This makes stuck InFlight operations eligible again.

### Minimal recovery flow

```cpp
// On startup (or periodically):
store->requeue_inflight_older_than(now_ms(), 10'000); // 10 seconds timeout
```

This is one of the simplest and most important reliability steps.

---

## 6) Testing tips (beginner + expert)

### Beginner checklist

- Use a test directory like ./.vix_demo/
- Set pretty_json = true so you can open the outbox file and understand it
- Use assert(...) in examples to fail fast and see what broke

### Expert checklist

- Enable fsync_on_write = true to reduce power-loss risk (slower but stronger)
- Add metrics: counts of Pending/Failed/PermanentFailed
- Log transitions: enqueue, claim, done, fail
- Ensure your remote endpoint uses idempotency_key to dedupe retries

---

## Appendix: Minimal Fake Transport used by examples

If you need a tiny transport for demos/tests, use this:

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

    SendResult send(const vix::sync::Operation &op) override
    {
      ++calls_;

      if (auto it = by_target_.find(op.target); it != by_target_.end())
        return toResult(it->second);

      if (auto it = by_kind_.find(op.kind); it != by_kind_.end())
        return toResult(it->second);

      return toResult(def_);
    }

  private:
    static SendResult toResult(const Rule &r)
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

