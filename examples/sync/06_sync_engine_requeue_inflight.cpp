/**
 *
 *  @file 06_sync_engine_requeue_inflight.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.
 *  All rights reserved.
 *  https://github.com/vixcpp/vix
 *
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 *
 */
// Run:
//   vix run examples/sync/06_sync_engine_requeue_inflight.cpp

#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>

#include <vix/net/NetworkProbe.hpp>
#include <vix/sync/Operation.hpp>
#include <vix/sync/outbox/Outbox.hpp>
#include <vix/sync/outbox/FileOutboxStore.hpp>
#include <vix/sync/engine/SyncEngine.hpp>

namespace vix::sync::engine
{
  class ExampleSuccessTransport final : public ISyncTransport
  {
  public:
    SendResult send(const vix::sync::Operation &op) override
    {
      std::cout << "sending requeued op: " << op.id << "\n";
      return SendResult{.ok = true};
    }
  };
}

static std::int64_t now_ms()
{
  using namespace std::chrono;
  return duration_cast<milliseconds>(
             steady_clock::now().time_since_epoch())
      .count();
}

static void reset_dir(const std::filesystem::path &dir)
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

  const std::filesystem::path dir = "./build/examples/sync/engine_requeue";
  reset_dir(dir);

  auto store = std::make_shared<FileOutboxStore>(FileOutboxStore::Config{
      .file_path = dir / "outbox.json",
      .pretty_json = true});

  auto outbox = std::make_shared<Outbox>(
      Outbox::Config{.owner = "engine-requeue"},
      store);

  auto probe = std::make_shared<vix::net::NetworkProbe>(
      vix::net::NetworkProbe::Config{},
      []
      { return true; });

  auto transport = std::make_shared<ExampleSuccessTransport>();

  SyncEngine engine(
      SyncEngine::Config{
          .worker_count = 1,
          .batch_limit = 10,
          .idle_sleep_ms = 0,
          .offline_sleep_ms = 0,
          .inflight_timeout_ms = 50},
      outbox,
      probe,
      transport);

  Operation op;
  op.kind = "http.post";
  op.target = "/api/messages";
  op.payload = R"({"text":"hello inflight"})";

  const auto t0 = now_ms();
  const auto id = outbox->enqueue(op, t0);

  // Simulate crash after claim
  const bool claimed = outbox->claim(id, t0);
  std::cout << "claimed manually: " << (claimed ? "yes" : "no") << "\n";

  engine.tick(t0 + 60);
  auto after_requeue = store->get(id);
  if (after_requeue)
  {
    std::cout << "status after timeout sweep: "
              << static_cast<int>(after_requeue->status) << "\n";
  }

  engine.tick(t0 + 61);
  auto final = store->get(id);
  if (final)
  {
    std::cout << "final status: " << static_cast<int>(final->status) << "\n";
  }

  return 0;
}
