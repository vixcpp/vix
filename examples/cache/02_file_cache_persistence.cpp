/**
 *
 *  @file 02_file_cache_persistence.cpp
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
//   vix run examples/cache/02_file_cache_persistence.cpp

#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>

#include <vix/cache/Cache.hpp>
#include <vix/cache/CacheContext.hpp>
#include <vix/cache/CacheEntry.hpp>
#include <vix/cache/CachePolicy.hpp>
#include <vix/cache/FileStore.hpp>

static std::int64_t now_ms()
{
  using namespace std::chrono;
  return duration_cast<std::chrono::milliseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

int main()
{
  using namespace vix::cache;

  const std::filesystem::path file = "./build/examples/cache/cache_http.json";

  {
    auto store = std::make_shared<FileStore>(FileStore::Config{
        .file_path = file,
        .pretty_json = true});

    CachePolicy policy;
    policy.ttl_ms = 10'000;

    Cache cache(policy, store);

    CacheEntry entry;
    entry.status = 200;
    entry.body = R"({"source":"disk"})";
    entry.headers["Content-Type"] = "application/json";
    entry.created_at_ms = now_ms();

    cache.put("GET /api/products?limit=3", entry);

    std::cout << "entry written to disk\n";
  }

  {
    auto store = std::make_shared<FileStore>(FileStore::Config{
        .file_path = file,
        .pretty_json = false});

    CachePolicy policy;
    policy.ttl_ms = 10'000;

    Cache cache(policy, store);

    auto cached = cache.get(
        "GET /api/products?limit=3",
        now_ms() + 100,
        CacheContext::Online());

    if (!cached)
    {
      std::cout << "cache miss after reload\n";
      return 1;
    }

    std::cout << "cache hit after reload\n";
    std::cout << "body: " << cached->body << "\n";
  }

  return 0;
}
