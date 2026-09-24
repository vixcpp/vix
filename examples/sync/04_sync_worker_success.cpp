/**
 *
 *  @file 04_sync_worker_success.cpp
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
//   vix run examples/sync/04_sync_worker_success.cpp

#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>

#include <vix/net/NetworkProbe.hpp>
#include <vix/sync/Operation.hpp>
#include <vix/sync/outbox/Outbox.hpp>
#include <vix/sync/outbox/FileOutboxStore.hpp>
#include <vix/sync/engine/SyncWorker.hpp>

namespace vix::sync::engine
{
  class ExampleSuccessTransport final : public ISyncTransport
  {
  public:
    SendResult send(const vix::sync::Operation &op) override
    {
      std::cout << "sending op: " << op.id << " -> " << op.target << "\n";
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

  const std::filesystem::path dir = "./build/examples/sync/worker_success";
  reset_dir(dir);

  auto store = std::make_shared<FileOutboxStore>(FileOutboxStore::Config{
      .file_path = dir / "outbox.json",
      .pretty_json = true});

  auto outbox = std::make_shared<Outbox>(
      Outbox::Config{.owner = "worker-success"},
      store);

  auto probe = std::make_shared<vix::net::NetworkProbe>(
      vix::net::NetworkProbe::Config{},
      []
      { return true; });

  auto transport = std::make_shared<ExampleSuccessTransport>();

  SyncWorker worker(
      SyncWorker::Config{
          .batch_limit = 10},
      outbox,
      probe,
      transport);

  Operation op;
  op.kind = "http.post";
  op.target = "/api/messages";
  op.payload = R"({"text":"hello sync"})";

  const auto t0 = now_ms();
  const auto id = outbox->enqueue(op, t0);

  const auto processed = worker.tick(t0 + 1);

  std::cout << "processed: " << processed << "\n";

  auto saved = store->get(id);
  if (saved)
  {
    std::cout << "final status: " << static_cast<int>(saved->status) << "\n";
  }

  return 0;
}
