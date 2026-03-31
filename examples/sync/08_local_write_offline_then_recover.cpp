/**
 *
 *  @file 08_local_write_offline_then_recover.cpp
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
//   vix run examples/sync/08_local_write_offline_then_recover.cpp

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <atomic>

#include <vix/net/NetworkProbe.hpp>
#include <vix/sync/Operation.hpp>
#include <vix/sync/engine/SyncEngine.hpp>
#include <vix/sync/outbox/FileOutboxStore.hpp>
#include <vix/sync/outbox/Outbox.hpp>
#include <vix/sync/wal/Wal.hpp>
#include <vix/sync/wal/WalRecord.hpp>

namespace
{
  std::int64_t now_ms()
  {
    using namespace std::chrono;
    return duration_cast<milliseconds>(
               steady_clock::now().time_since_epoch())
        .count();
  }

  void reset_dir(const std::filesystem::path &dir)
  {
    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
    std::filesystem::create_directories(dir, ec);
  }

  std::vector<std::uint8_t> to_bytes(const std::string &s)
  {
    return std::vector<std::uint8_t>(s.begin(), s.end());
  }

  void write_local_file(
      const std::filesystem::path &path,
      const std::string &content)
  {
    std::filesystem::create_directories(path.parent_path());

    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out)
    {
      throw std::runtime_error("cannot write local file");
    }

    out << content;
  }
} // namespace

namespace vix::sync::engine
{
  class ExampleRecoverTransport final : public ISyncTransport
  {
  public:
    SendResult send(const vix::sync::Operation &op) override
    {
      ++calls_;

      std::cout << "[transport] sending after recovery\n";
      std::cout << "  id      : " << op.id << "\n";
      std::cout << "  kind    : " << op.kind << "\n";
      std::cout << "  target  : " << op.target << "\n";
      std::cout << "  payload : " << op.payload << "\n";

      return SendResult{
          .ok = true,
          .retryable = true,
          .error = ""};
    }

    std::size_t call_count() const noexcept
    {
      return calls_;
    }

  private:
    std::size_t calls_{0};
  };
} // namespace vix::sync::engine

int main()
{
  using namespace vix::sync;
  using namespace vix::sync::outbox;
  using namespace vix::sync::wal;
  using namespace vix::sync::engine;

  const std::filesystem::path dir = "./build/examples/sync/local_write_offline_then_recover";
  reset_dir(dir);

  const std::filesystem::path data_file = dir / "data" / "draft.txt";
  const std::filesystem::path wal_file = dir / "wal.log";
  const std::filesystem::path outbox_file = dir / "outbox.json";

  // 1) Local write always succeeds first
  const std::string local_content = "hello from offline mode";
  write_local_file(data_file, local_content);

  std::cout << "[local] wrote file: " << data_file << "\n";
  std::cout << "[local] content   : " << local_content << "\n";

  // 2) Append durable intent to WAL
  Wal wal(Wal::Config{
      .file_path = wal_file,
      .fsync_on_write = false});

  const auto t0 = now_ms();

  const std::string payload =
      std::string("{\"path\":\"") + data_file.string() +
      "\",\"content\":\"" + local_content + "\"}";

  const std::string op_id = "offline_write_1";

  const auto offset = wal.append(WalRecord{
      .id = op_id,
      .type = RecordType::PutOperation,
      .ts_ms = t0,
      .payload = to_bytes(payload)});

  std::cout << "[wal] appended PutOperation at offset: " << offset << "\n";

  // 3) Enqueue in durable outbox
  auto store = std::make_shared<FileOutboxStore>(FileOutboxStore::Config{
      .file_path = outbox_file,
      .pretty_json = true,
      .fsync_on_write = false});

  auto outbox = std::make_shared<Outbox>(
      Outbox::Config{
          .owner = "example-offline-recover"},
      store);

  Operation op;
  op.id = op_id;
  op.kind = "fs.write.sync";
  op.target = "/sync/file";
  op.payload = payload;

  outbox->enqueue(op, t0);
  std::cout << "[outbox] enqueued id: " << op_id << "\n";

  // 4) Probe starts offline, then recovers
  auto network_online = std::make_shared<std::atomic<bool>>(false);

  auto probe = std::make_shared<vix::net::NetworkProbe>(
      vix::net::NetworkProbe::Config{},
      [network_online]
      { return network_online->load(); });

  auto transport = std::make_shared<ExampleRecoverTransport>();

  SyncEngine engine(
      SyncEngine::Config{
          .worker_count = 1,
          .idle_sleep_ms = 0,
          .offline_sleep_ms = 0,
          .batch_limit = 10,
          .inflight_timeout_ms = 10'000},
      outbox,
      probe,
      transport);

  // 5) First tick while offline -> nothing should be sent
  const auto processed_offline = engine.tick(t0 + 1);

  std::cout << "[engine] processed while offline: " << processed_offline << "\n";

  auto after_offline = store->get(op_id);
  if (!after_offline)
  {
    std::cerr << "error: operation missing after offline tick\n";
    return 1;
  }

  std::cout << "[outbox] status after offline tick: "
            << static_cast<int>(after_offline->status) << "\n";

  // 6) Network comes back
  network_online->store(true);
  std::cout << "[network] connectivity restored\n";

  // 7) Tick again -> should send and complete
  const auto processed_recover = engine.tick(t0 + 2);

  std::cout << "[engine] processed after recovery: " << processed_recover << "\n";

  auto final = store->get(op_id);
  if (!final)
  {
    std::cerr << "error: operation missing after recovery tick\n";
    return 1;
  }

  std::cout << "[outbox] final status : " << static_cast<int>(final->status) << "\n";
  std::cout << "[outbox] attempts     : " << final->attempt << "\n";
  std::cout << "[outbox] last_error   : " << final->last_error << "\n";

  // 8) Replay WAL to show durable local history remains available
  std::cout << "[wal] replay:\n";
  wal.replay(0, [](const WalRecord &rec)
             { std::cout << "  id=" << rec.id
                         << " type=" << static_cast<int>(rec.type)
                         << " ts=" << rec.ts_ms
                         << " payload_size=" << rec.payload.size()
                         << "\n"; });

  // 9) Final checks
  if (processed_offline != 0)
  {
    std::cerr << "error: offline tick should not send anything\n";
    return 1;
  }

  if (final->status != OperationStatus::Done)
  {
    std::cerr << "error: operation was not completed after recovery\n";
    return 1;
  }

  if (transport->call_count() != 1)
  {
    std::cerr << "error: expected exactly one send after recovery\n";
    return 1;
  }

  std::cout << "[OK] local write -> wal append -> outbox enqueue -> offline hold -> recover -> done\n";
  return 0;
}
