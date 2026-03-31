# Sync Examples

This page contains minimal, copy-paste examples for the `sync` module.

These examples are intentionally small and based on the module tests,
since official examples are not published yet.

They demonstrate:

-   Building an Outbox with a file store
-   Using a NetworkProbe
-   Wiring a SyncEngine with a transport
-   Handling permanent failure vs retryable failures
-   Re-queuing in-flight operations after a timeout

------------------------------------------------------------------------

## Example 1: Minimal smoke test (enqueue + tick + done)

This is the smallest complete setup: Outbox + File store + always-online
probe + transport + engine.

``` cpp
#include <cassert>
#include <chrono>
#include <iostream>
#include <memory>

#include <vix/net/NetworkProbe.hpp>
#include <vix/sync/engine/SyncEngine.hpp>
#include <vix/sync/outbox/FileOutboxStore.hpp>
#include <vix/sync/outbox/Outbox.hpp>

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
      .file_path = "./.vix_test/outbox.json",
      .pretty_json = true,
      .fsync_on_write = false});

  auto outbox = std::make_shared<Outbox>(Outbox::Config{.owner = "test-engine"}, store);

  auto probe = std::make_shared<vix::net::NetworkProbe>(
      vix::net::NetworkProbe::Config{},
      [] { return true; }); // always online

  auto transport = std::make_shared<FakeHttpTransport>();
  transport->setDefault({.ok = true});

  SyncEngine engine(SyncEngine::Config{.worker_count = 1, .batch_limit = 10},
                   outbox, probe, transport);

  Operation op;
  op.kind = "http.post";
  op.target = "/api/messages";
  op.payload = R"({"text":"hello offline"})";

  const auto t0 = now_ms();
  const auto id = outbox->enqueue(op, t0);

  const auto processed = engine.tick(now_ms());
  assert(processed >= 1);

  auto saved = store->get(id);
  assert(saved.has_value());
  assert(saved->status == OperationStatus::Done);

  std::cout << "OK: operation sent and marked Done\n";
  return 0;
}
```

------------------------------------------------------------------------

## Example 2: Permanent failure (retryable = false)

This shows how the transport can tell the worker that the failure is not
retryable. The operation must become `PermanentFailed` and must not be
retried.

``` cpp
#include <cassert>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>

#include <vix/net/NetworkProbe.hpp>
#include <vix/sync/engine/SyncEngine.hpp>
#include <vix/sync/outbox/FileOutboxStore.hpp>
#include <vix/sync/outbox/Outbox.hpp>

#include "fake_http_transport.hpp"

static std::int64_t now_ms()
{
  using namespace std::chrono;
  return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

static void reset_test_dir(const std::filesystem::path &dir)
{
  std::error_code ec;
  std::filesystem::remove_all(dir, ec);
  std::filesystem::create_directories(dir, ec);
}

int main()
{
  using namespace vix::sync;
  using namespace vix::sync::outbox;
  using namespace vix::sync::engine;

  const std::filesystem::path test_dir = "./.vix_test_perm";
  reset_test_dir(test_dir);

  auto store = std::make_shared<FileOutboxStore>(FileOutboxStore::Config{
      .file_path = test_dir / "outbox.json",
      .pretty_json = true,
      .fsync_on_write = false});

  auto outbox = std::make_shared<Outbox>(Outbox::Config{.owner = "test-engine"}, store);

  auto probe = std::make_shared<vix::net::NetworkProbe>(
      vix::net::NetworkProbe::Config{},
      [] { return true; });

  auto transport = std::make_shared<FakeHttpTransport>();
  transport->setRuleForTarget("/api/messages", FakeHttpTransport::Rule{
      .ok = false,
      .retryable = false,
      .error = "bad request (permanent)"});

  SyncEngine engine(SyncEngine::Config{.worker_count = 1, .batch_limit = 10},
                   outbox, probe, transport);

  Operation op;
  op.kind = "http.post";
  op.target = "/api/messages";
  op.payload = R"({"text":"hello offline"})";

  const auto id = outbox->enqueue(op, now_ms());

  const auto processed1 = engine.tick(now_ms());
  assert(processed1 >= 1);
  assert(transport->callCount() == 1);

  auto saved1 = store->get(id);
  assert(saved1.has_value());
  assert(saved1->status == OperationStatus::PermanentFailed);

  const auto processed2 = engine.tick(now_ms());
  (void)processed2;
  assert(transport->callCount() == 1); // no retry

  std::cout << "OK: permanent failure is not retried\n";
  return 0;
}
```

------------------------------------------------------------------------

## Example 3: In-flight timeout requeue (simulate crash)

This simulates a crash after the operation is claimed but before it is
completed. After `inflight_timeout_ms`, the engine tick should requeue
the stuck operation, then the next tick should send it and mark it Done.

``` cpp
#include <cassert>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>

#include <vix/net/NetworkProbe.hpp>
#include <vix/sync/engine/SyncEngine.hpp>
#include <vix/sync/outbox/FileOutboxStore.hpp>
#include <vix/sync/outbox/Outbox.hpp>

#include "fake_http_transport.hpp"

static std::int64_t now_ms()
{
  using namespace std::chrono;
  return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

static void reset_test_dir(const std::filesystem::path &dir)
{
  std::error_code ec;
  std::filesystem::remove_all(dir, ec);
  std::filesystem::create_directories(dir, ec);
}

int main()
{
  using namespace vix::sync;
  using namespace vix::sync::outbox;
  using namespace vix::sync::engine;

  const std::filesystem::path test_dir = "./.vix_test_inflight";
  reset_test_dir(test_dir);

  auto store = std::make_shared<FileOutboxStore>(FileOutboxStore::Config{
      .file_path = test_dir / "outbox.json",
      .pretty_json = true,
      .fsync_on_write = false});

  auto outbox = std::make_shared<Outbox>(Outbox::Config{.owner = "test-engine"}, store);

  auto probe = std::make_shared<vix::net::NetworkProbe>(
      vix::net::NetworkProbe::Config{},
      [] { return true; });

  auto transport = std::make_shared<FakeHttpTransport>();
  transport->setDefault({.ok = true});

  SyncEngine::Config ecfg;
  ecfg.worker_count = 1;
  ecfg.batch_limit = 10;
  ecfg.idle_sleep_ms = 0;
  ecfg.offline_sleep_ms = 0;
  ecfg.inflight_timeout_ms = 50; // 50ms for test

  SyncEngine engine(ecfg, outbox, probe, transport);

  Operation op;
  op.kind = "http.post";
  op.target = "/api/messages";
  op.payload = R"({"text":"hello offline"})";

  const auto t0 = now_ms();
  const auto id = outbox->enqueue(op, t0);

  const bool claimed = outbox->claim(id, t0);
  assert(claimed);

  {
    auto saved = store->get(id);
    assert(saved.has_value());
    assert(saved->status == OperationStatus::InFlight);
  }

  const auto t1 = t0 + 60; // > 50ms timeout
  engine.tick(t1);

  {
    auto saved = store->get(id);
    assert(saved.has_value());
    assert(saved->status != OperationStatus::InFlight);
  }

  engine.tick(t1 + 1);

  auto final = store->get(id);
  assert(final.has_value());
  assert(final->status == OperationStatus::Done);

  std::cout << "OK: inflight timeout requeues stuck ops and they complete\n";
  return 0;
}
```

------------------------------------------------------------------------

## Appendix: Minimal Fake Transport

This is a tiny rule-based transport for tests and examples.

``` cpp
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

------------------------------------------------------------------------

## Notes

-   These examples use `assert` for clarity and minimalism.
-   `NetworkProbe` is configured here with a lambda that always returns
    true.
-   For real systems, `NetworkProbe` should reflect actual connectivity
    state.
-   The Outbox store paths are local and suitable for quick testing.

