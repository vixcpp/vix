/**
 *
 *  @file 03_lru_cache_eviction.cpp
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
//   vix run examples/cache/03_lru_cache_eviction.cpp

#include <chrono>
#include <iostream>
#include <memory>

#include <vix/cache/Cache.hpp>
#include <vix/cache/CacheContext.hpp>
#include <vix/cache/CacheEntry.hpp>
#include <vix/cache/CachePolicy.hpp>
#include <vix/cache/LruMemoryStore.hpp>

static std::int64_t now_ms()
{
  using namespace std::chrono;
  return duration_cast<milliseconds>(
             steady_clock::now().time_since_epoch())
      .count();
}

static vix::cache::CacheEntry make_entry(
    const std::string &body,
    std::int64_t t)
{
  vix::cache::CacheEntry e;
  e.status = 200;
  e.body = body;
  e.created_at_ms = t;
  return e;
}

int main()
{
  using namespace vix::cache;

  auto store = std::make_shared<LruMemoryStore>(
      LruMemoryStore::Config{.max_entries = 2});

  CachePolicy policy;
  policy.ttl_ms = 10'000;

  Cache cache(policy, store);

  const auto t0 = now_ms();

  cache.put("k1", make_entry("A", t0));
  cache.put("k2", make_entry("B", t0));

  // Touch k1 -> k2 becomes least recently used
  (void)cache.get("k1", t0 + 1, CacheContext::Online());

  cache.put("k3", make_entry("C", t0 + 2));

  auto k1 = cache.get("k1", t0 + 3, CacheContext::Online());
  auto k2 = cache.get("k2", t0 + 3, CacheContext::Online());
  auto k3 = cache.get("k3", t0 + 3, CacheContext::Online());

  std::cout << "k1: " << (k1 ? "present" : "missing") << "\n";
  std::cout << "k2: " << (k2 ? "present" : "missing") << "\n";
  std::cout << "k3: " << (k3 ? "present" : "missing") << "\n";

  return 0;
}
