# Sync

Build an offline-first flow: persist operations locally, sync when possible.

```txt
local operation → persist first → sync later
```

## What you will build

```txt
WAL      → records what happened
Outbox   → stores pending operations
Worker   → sends ready operations
Retry    → retries temporary failures
Done     → marks successful operations
```

## Create a workspace

```bash
mkdir -p ~/tmp/vix-examples/sync
cd ~/tmp/vix-examples/sync
touch main.cpp
```

## Full code

```cpp
#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <vix/sync.hpp>

static std::int64_t now_ms()
{
  using namespace std::chrono;
  return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

static void reset_dir(const std::filesystem::path &dir)
{
  std::error_code ec;
  std::filesystem::remove_all(dir, ec);
  std::filesystem::create_directories(dir, ec);
}

namespace
{
  class ExampleTransport final : public vix::sync::engine::ISyncTransport
  {
  public:
    explicit ExampleTransport(bool online) : online_(online) {}
    void set_online(bool online) { online_ = online; }

    vix::sync::engine::SendResult send(const vix::sync::Operation &op) override
    {
      std::cout << "[transport] sending: " << op.id << " -> " << op.target << "\n";
      if (!online_)
        return {.ok = false, .retryable = true, .error = "network offline"};
      return {.ok = true};
    }
  private:
    bool online_{false};
  };
}

int main()
{
  using namespace vix::sync;
  using namespace vix::sync::outbox;
  using namespace vix::sync::engine;

  const std::filesystem::path dir = "./build/sync-example";
  reset_dir(dir);

  auto store = std::make_shared<FileOutboxStore>(FileOutboxStore::Config{
      .file_path = dir / "outbox.json", .pretty_json = true});

  RetryPolicy retry;
  retry.max_attempts = 3;
  retry.base_delay_ms = 500;
  retry.factor = 2.0;

  auto outbox = std::make_shared<Outbox>(
      Outbox::Config{.owner = "sync-example", .retry = retry}, store);

  auto probe = std::make_shared<vix::net::NetworkProbe>(
      vix::net::NetworkProbe::Config{}, [] { return true; });

  auto transport = std::make_shared<ExampleTransport>(false);  // start offline

  SyncWorker worker(SyncWorker::Config{.batch_limit = 10}, outbox, probe, transport);

  const auto t0 = now_ms();

  // 1. Persist the operation BEFORE any network call
  Operation op;
  op.kind = "message.send";
  op.target = "/api/messages";
  op.payload = R"({"text":"hello offline-first"})";
  const std::string id = outbox->enqueue(op, t0);
  std::cout << "[app] operation saved: " << id << "\n";

  // 2. First tick — offline, fails with retryable error
  std::cout << "\n[app] first sync attempt (offline)\n";
  const auto processed_offline = worker.tick(t0 + 1);
  std::cout << "[worker] processed: " << processed_offline << "\n";

  auto after_offline = store->get(id);
  if (after_offline)
  {
    std::cout << "[outbox] attempt: " << after_offline->attempt << "\n";
    std::cout << "[outbox] last_error: " << after_offline->last_error << "\n";
    std::cout << "[outbox] next_retry_at_ms: " << after_offline->next_retry_at_ms << "\n";
  }

  // 3. Network recovers — second tick succeeds
  std::cout << "\n[app] network recovers\n";
  transport->set_online(true);
  const auto retry_time = after_offline ? after_offline->next_retry_at_ms + 1 : t0 + 1000;
  const auto processed_online = worker.tick(retry_time);
  std::cout << "[worker] processed: " << processed_online << "\n";

  auto final = store->get(id);
  if (final)
  {
    std::cout << "[outbox] final status: " << static_cast<int>(final->status) << "\n";
    if (final->status == OperationStatus::Done)
      std::cout << "[outbox] operation completed\n";
  }

  std::cout << "\n[files] outbox saved at: " << (dir / "outbox.json") << "\n";
  return 0;
}
```

## Run

```bash
vix run main.cpp
```

Check the durable outbox file:

```bash
cat ./build/sync-example/outbox.json
```

## Operation lifecycle

```txt
Pending → Inflight → Done
               ↓ (retryable failure)
             Retry → Pending again
               ↓ (permanent failure)
             Failed
```

## Retryable vs permanent failure

```cpp
// Retryable — try again later
return SendResult{.ok = false, .retryable = true, .error = "network offline"};

// Permanent — do not retry
return SendResult{.ok = false, .retryable = false, .error = "bad request"};
```

| Failure | Retry? | Example |
|---------|--------|---------|
| Network timeout | Yes | temporary connection issue |
| Server unavailable | Yes | 503 |
| Bad request | No | invalid payload |
| Unauthorized | Usually no | invalid token |

## WAL mini example

```bash
touch wal_demo.cpp
```

```cpp
#include <chrono>
#include <filesystem>
#include <iostream>
#include <vix/sync.hpp>

static std::int64_t now_ms()
{
  using namespace std::chrono;
  return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

static std::vector<std::uint8_t> bytes(std::string s)
{
  return std::vector<std::uint8_t>(s.begin(), s.end());
}

int main()
{
  using namespace vix::sync::wal;
  std::filesystem::create_directories("./build/wal-demo");

  Wal wal(Wal::Config{.file_path = "./build/wal-demo/wal.log", .fsync_on_write = false});
  const auto t0 = now_ms();

  wal.append(WalRecord{.id = "op_1", .type = RecordType::PutOperation, .ts_ms = t0,
                        .payload = bytes(R"({"kind":"message.send","target":"/api/messages"})")});

  wal.append(WalRecord{.id = "op_1", .type = RecordType::MarkDone, .ts_ms = t0 + 1});

  std::cout << "replay:\n";
  wal.replay(0, [](const WalRecord &r){
    std::cout << "  id=" << r.id << " type=" << static_cast<int>(r.type) << " ts=" << r.ts_ms << "\n";
  });
  return 0;
}
```

```bash
vix run wal_demo.cpp
```

## Complete offline-first flow

```txt
local write → WAL append → outbox enqueue → sync worker → transport → ack
```

## Common mistakes

```txt
Wrong: send request → store if success
Correct: store operation → send request

Wrong: retry bad request forever
Correct: retryable=false for validation/auth errors

Wrong: payload={"action":"save"}
Correct: payload={"id":"msg_1","text":"hello","created_at":1710000000000}
```

## What you should remember

```cpp
#include <vix/sync.hpp>

// local operation → outbox → worker → transport → done
Operation op;
op.kind = "message.send";
op.target = "/api/messages";
op.payload = R"({"text":"hello"})";
outbox->enqueue(op, t0);
worker.tick(t0 + 1);
```

The core idea: **if the network fails, the operation must still exist locally.**

Next: [P2P](/examples/p2p)
