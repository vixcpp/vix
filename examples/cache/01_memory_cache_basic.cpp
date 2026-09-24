/**
 *
 *  @file 01_memory_cache_basic.cpp
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
//   vix run examples/cache/01_memory_cache_basic.cpp

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
  policy.ttl_ms = 5'000;

  Cache cache(policy, store);

  const std::string key = "GET /api/users?page=1";
  const auto t0 = now_ms();

  CacheEntry entry;
  entry.status = 200;
  entry.body = R"({"users":[1,2,3]})";
  entry.headers["Content-Type"] = "application/json";
  entry.created_at_ms = t0;

  cache.put(key, entry);

  auto cached = cache.get(key, t0 + 100, CacheContext::Online());

  if (!cached)
  {
    std::cout << "cache miss\n";
    return 1;
  }

  std::cout << "cache hit\n";
  std::cout << "status: " << cached->status << "\n";
  std::cout << "body  : " << cached->body << "\n";
  return 0;
}
