/**
 *
 *  @file 04_offline_and_network_error.cpp
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
//   vix run examples/cache/04_offline_and_network_error.cpp

#include <chrono>
#include <iostream>
#include <memory>

#include <vix/cache/Cache.hpp>
#include <vix/cache/CacheContext.hpp>
#include <vix/cache/CacheEntry.hpp>
#include <vix/cache/CachePolicy.hpp>
#include <vix/cache/MemoryStore.hpp>

static std::int64_t now_ms()
{
  using namespace std::chrono;
  return duration_cast<milliseconds>(
             steady_clock::now().time_since_epoch())
      .count();
}

int main()
{
  using namespace vix::cache;

  auto store = std::make_shared<MemoryStore>();

  CachePolicy policy;
  policy.ttl_ms = 100;
  policy.allow_stale_if_offline = true;
  policy.stale_if_offline_ms = 10'000;
  policy.allow_stale_if_error = true;
  policy.stale_if_error_ms = 5'000;

  Cache cache(policy, store);

  const auto t0 = now_ms();

  CacheEntry entry;
  entry.status = 200;
  entry.body = R"({"cached":true})";
  entry.created_at_ms = t0;

  cache.put("GET /api/profile", entry);

  {
    auto fresh = cache.get("GET /api/profile", t0 + 50, CacheContext::Online());
    std::cout << "fresh online: " << (fresh ? "hit" : "miss") << "\n";
  }

  {
    auto offline_stale = cache.get("GET /api/profile", t0 + 3000, CacheContext::Offline());
    std::cout << "offline stale: " << (offline_stale ? "hit" : "miss") << "\n";
  }

  {
    auto error_stale = cache.get("GET /api/profile", t0 + 4000, CacheContext::NetworkError());
    std::cout << "network error stale: " << (error_stale ? "hit" : "miss") << "\n";
  }

  {
    auto too_old = cache.get("GET /api/profile", t0 + 20'000, CacheContext::Offline());
    std::cout << "too old offline: " << (too_old ? "hit" : "miss") << "\n";
  }

  return 0;
}
