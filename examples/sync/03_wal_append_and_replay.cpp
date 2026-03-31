/**
 *
 *  @file 03_wal_append_and_replay.cpp
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
//   vix run examples/sync/03_wal_append_and_replay.cpp

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include <vix/sync/wal/Wal.hpp>
#include <vix/sync/wal/WalRecord.hpp>

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

static std::vector<std::uint8_t> bytes(std::string s)
{
  return std::vector<std::uint8_t>(s.begin(), s.end());
}

int main()
{
  using namespace vix::sync::wal;

  const std::filesystem::path dir = "./build/examples/sync/wal";
  reset_dir(dir);

  Wal wal(Wal::Config{
      .file_path = dir / "wal.log",
      .fsync_on_write = false});

  const auto t0 = now_ms();

  wal.append(WalRecord{
      .id = "op_1",
      .type = RecordType::PutOperation,
      .ts_ms = t0,
      .payload = bytes(R"({"kind":"http.post","target":"/api/messages"})")});

  wal.append(WalRecord{
      .id = "op_1",
      .type = RecordType::MarkFailed,
      .ts_ms = t0 + 1,
      .error = "temporary failure",
      .next_retry_at_ms = t0 + 500});

  wal.append(WalRecord{
      .id = "op_1",
      .type = RecordType::MarkDone,
      .ts_ms = t0 + 1000});

  std::cout << "replay:\n";

  wal.replay(0, [](const WalRecord &rec)
             { std::cout << "  id=" << rec.id
                         << " type=" << static_cast<int>(rec.type)
                         << " ts=" << rec.ts_ms
                         << " error=" << rec.error
                         << " next_retry=" << rec.next_retry_at_ms
                         << "\n"; });

  return 0;
}
