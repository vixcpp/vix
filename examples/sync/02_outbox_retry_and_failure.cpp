/**
 *
 *  @file 02_outbox_retry_and_failure.cpp
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
//   vix run examples/sync/02_outbox_retry_and_failure.cpp

#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>

#include <vix/sync/Operation.hpp>
#include <vix/sync/RetryPolicy.hpp>
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

  const std::filesystem::path dir = "./build/examples/sync/retry";
  reset_dir(dir);

  auto store = std::make_shared<FileOutboxStore>(FileOutboxStore::Config{
      .file_path = dir / "outbox.json",
      .pretty_json = true});

  RetryPolicy retry;
  retry.max_attempts = 3;
  retry.base_delay_ms = 500;
  retry.factor = 2.0;

  auto outbox = std::make_shared<Outbox>(
      Outbox::Config{
          .owner = "example-retry",
          .retry = retry},
      store);

  const auto t0 = now_ms();

  Operation retryable;
  retryable.kind = "http.post";
  retryable.target = "/api/retryable";
  retryable.payload = "{}";

  const std::string retryable_id = outbox->enqueue(retryable, t0);
  outbox->claim(retryable_id, t0 + 1);
  outbox->fail(retryable_id, "temporary network error", t0 + 2, true);

  auto saved1 = store->get(retryable_id);
  if (saved1)
  {
    std::cout << "retryable status      : " << static_cast<int>(saved1->status) << "\n";
    std::cout << "retryable attempt     : " << saved1->attempt << "\n";
    std::cout << "retryable next_retry  : " << saved1->next_retry_at_ms << "\n";
    std::cout << "retryable last_error  : " << saved1->last_error << "\n";
  }

  Operation permanent;
  permanent.kind = "http.post";
  permanent.target = "/api/permanent";
  permanent.payload = "{}";

  const std::string permanent_id = outbox->enqueue(permanent, t0 + 10);
  outbox->claim(permanent_id, t0 + 11);
  outbox->fail(permanent_id, "bad request", t0 + 12, false);

  auto saved2 = store->get(permanent_id);
  if (saved2)
  {
    std::cout << "permanent status      : " << static_cast<int>(saved2->status) << "\n";
    std::cout << "permanent last_error  : " << saved2->last_error << "\n";
  }

  return 0;
}
