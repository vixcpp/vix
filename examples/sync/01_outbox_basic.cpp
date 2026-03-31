/**
 *
 *  @file 01_outbox_basic.cpp
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
//   vix run examples/sync/01_outbox_basic.cpp

#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>

#include <vix/sync/Operation.hpp>
#include <vix/sync/outbox/Outbox.hpp>
#include <vix/sync/outbox/FileOutboxStore.hpp>

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

  const std::filesystem::path dir = "./build/examples/sync/basic";
  reset_dir(dir);

  auto store = std::make_shared<FileOutboxStore>(FileOutboxStore::Config{
      .file_path = dir / "outbox.json",
      .pretty_json = true});

  auto outbox = std::make_shared<Outbox>(
      Outbox::Config{
          .owner = "example-basic"},
      store);

  Operation op;
  op.kind = "http.post";
  op.target = "/api/messages";
  op.payload = R"({"text":"hello"})";

  const auto t0 = now_ms();
  const std::string id = outbox->enqueue(op, t0);

  std::cout << "enqueued: " << id << "\n";

  auto ready = outbox->peek_ready(t0, 10);
  std::cout << "ready count: " << ready.size() << "\n";

  const bool claimed = outbox->claim(id, t0 + 1);
  std::cout << "claimed: " << (claimed ? "yes" : "no") << "\n";

  const bool done = outbox->complete(id, t0 + 2);
  std::cout << "completed: " << (done ? "yes" : "no") << "\n";

  auto saved = store->get(id);
  if (saved)
  {
    std::cout << "final status: "
              << (saved->status == OperationStatus::Done ? "Done" : "Other")
              << "\n";
  }

  return 0;
}
