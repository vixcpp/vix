/**
 *
 *  @file 05_cache_key_and_headers.cpp
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
//   vix run examples/cache/05_cache_key_and_headers.cpp

#include <chrono>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

#include <vix/cache/Cache.hpp>
#include <vix/cache/CacheContext.hpp>
#include <vix/cache/CacheEntry.hpp>
#include <vix/cache/CacheKey.hpp>
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

  std::unordered_map<std::string, std::string> req_headers;
  req_headers["Accept"] = "application/json";
  req_headers["X-Device"] = "mobile";

  const std::string key = CacheKey::fromRequest(
      "get",
      "/api/users",
      "b=2&a=1",
      req_headers,
      {"Accept"});

  std::cout << "cache key: " << key << "\n";

  auto store = std::make_shared<MemoryStore>();

  CachePolicy policy;
  policy.ttl_ms = 10'000;

  Cache cache(policy, store);

  CacheEntry entry;
  entry.status = 200;
  entry.body = R"({"ok":true})";
  entry.headers["Content-Type"] = "application/json";
  entry.headers["X-Powered-By"] = "Vix";
  entry.created_at_ms = now_ms();

  cache.put(key, entry);

  auto cached = cache.get(key, now_ms() + 1, CacheContext::Online());
  if (!cached)
  {
    std::cout << "cache miss\n";
    return 1;
  }

  std::cout << "normalized headers:\n";
  for (const auto &[name, value] : cached->headers)
  {
    std::cout << "  " << name << " = " << value << "\n";
  }

  return 0;
}
