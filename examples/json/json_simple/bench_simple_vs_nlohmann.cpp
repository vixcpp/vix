/**
 *
 *  @file bench_simple_vs_nlohmann.cpp
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
 */

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <vix/json/Simple.hpp>
#include <vix/json/convert.hpp>
#include <vix/json/json.hpp>

namespace
{
  using Clock = std::chrono::steady_clock;

  static volatile std::uint64_t g_sink_u64 = 0;
  static volatile std::size_t g_sink_sz = 0;

  inline std::uint64_t hash64(std::uint64_t x)
  {
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;
    return x;
  }

  inline void sink_u64(std::uint64_t x)
  {
    g_sink_u64 += hash64(x + 0x9e3779b97f4a7c15ULL);
  }

  inline void sink_sz(std::size_t x)
  {
    g_sink_sz += static_cast<std::size_t>(
        hash64(static_cast<std::uint64_t>(x) + 0x9e3779b97f4a7c15ULL));
  }

  struct Timer
  {
    std::string name;
    Clock::time_point t0;

    explicit Timer(std::string n) : name(std::move(n)), t0(Clock::now()) {}

    ~Timer()
    {
      const auto t1 = Clock::now();
      const auto us =
          std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
      std::cout << name << ": " << us << " us\n";
    }
  };

  // Build a fixed payload size to stress arrays and objects.
  // This keeps the comparison fair: same logical structure.
  inline vix::json::kvs build_simple_payload(int idx, int items)
  {
    using namespace vix::json;

    array_t tags;
    tags.reserve(static_cast<std::size_t>(items));
    for (int i = 0; i < items; ++i)
      tags.push_string("tag_" + std::to_string(i));

    array_t nums;
    nums.reserve(static_cast<std::size_t>(items));
    for (int i = 0; i < items; ++i)
      nums.push_i64(static_cast<std::int64_t>(idx * 1000 + i));

    kvs meta = obj({
        "version",
        "1.0.0",
        "country",
        "UG",
        "active",
        true,
        "score",
        12.5,
    });

    kvs user = obj({
        "id",
        idx,
        "name",
        std::string("user_") + std::to_string(idx),
        "tags",
        tags,
        "nums",
        nums,
        "meta",
        meta,
    });

    kvs root = obj({
        "type",
        "bench",
        "user",
        user,
        "ok",
        true,
    });

    return root;
  }

  inline vix::json::Json build_nlohmann_payload(int idx, int items)
  {
    using namespace vix::json;

    Json tags = Json::array();
    for (int i = 0; i < items; ++i)
      tags.push_back(std::string("tag_") + std::to_string(i));

    Json nums = Json::array();
    for (int i = 0; i < items; ++i)
      nums.push_back(static_cast<std::int64_t>(idx * 1000 + i));

    Json meta = Json::object();
    meta["version"] = "1.0.0";
    meta["country"] = "UG";
    meta["active"] = true;
    meta["score"] = 12.5;

    Json user = Json::object();
    user["id"] = idx;
    user["name"] = std::string("user_") + std::to_string(idx);
    user["tags"] = std::move(tags);
    user["nums"] = std::move(nums);
    user["meta"] = std::move(meta);

    Json root = Json::object();
    root["type"] = "bench";
    root["user"] = std::move(user);
    root["ok"] = true;

    return root;
  }

  inline void bench_build_simple(int iters, int items)
  {
    using namespace vix::json;

    Timer t("build Simple");
    std::uint64_t acc = 0;

    for (int i = 0; i < iters; ++i)
    {
      kvs s = build_simple_payload(i, items);

      const token *u = s.get_ptr("user");
      if (u && u->is_object())
      {
        auto up = u->as_object_ptr();
        if (up)
        {
          const auto id = up->get_i64_or("id", -1);
          acc ^= hash64(static_cast<std::uint64_t>(id));
        }
      }
    }

    sink_u64(acc);
  }

  inline void bench_build_nlohmann(int iters, int items)
  {
    using namespace vix::json;

    Timer t("build nlohmann::json");
    std::uint64_t acc = 0;

    for (int i = 0; i < iters; ++i)
    {
      Json j = build_nlohmann_payload(i, items);

      if (j.contains("user") && j["user"].contains("id"))
      {
        const auto id = j["user"]["id"].get<std::int64_t>();
        acc ^= hash64(static_cast<std::uint64_t>(id));
      }
    }

    sink_u64(acc);
  }

  inline void bench_convert_simple_to_json(int iters, int items)
  {
    using namespace vix::json;

    Timer t("convert Simple -> Json");
    std::uint64_t acc = 0;

    for (int i = 0; i < iters; ++i)
    {
      kvs s = build_simple_payload(i, items);
      Json j = to_json(s);

      if (j.contains("user") && j["user"].contains("name"))
      {
        const auto &name = j["user"]["name"].get_ref<const std::string &>();
        acc ^= hash64(static_cast<std::uint64_t>(name.size()));
      }
    }

    sink_u64(acc);
  }

  inline void bench_dump_from_simple(int iters, int items)
  {
    using namespace vix::json;

    Timer t("dump from Simple (Simple -> Json -> dump)");
    std::size_t acc = 0;

    for (int i = 0; i < iters; ++i)
    {
      kvs s = build_simple_payload(i, items);
      Json j = to_json(s);
      std::string out = j.dump();
      acc ^= out.size();
    }

    sink_sz(acc);
  }

  inline void bench_dump_from_nlohmann(int iters, int items)
  {
    using namespace vix::json;

    Timer t("dump from nlohmann::json (build -> dump)");
    std::size_t acc = 0;

    for (int i = 0; i < iters; ++i)
    {
      Json j = build_nlohmann_payload(i, items);
      std::string out = j.dump();
      acc ^= out.size();
    }

    sink_sz(acc);
  }

  inline void bench_dump_only_json(int iters, int items)
  {
    using namespace vix::json;

    const int pool_n = std::max(1, iters / 10);

    std::vector<Json> pool;
    pool.reserve(static_cast<std::size_t>(pool_n));
    for (int i = 0; i < pool_n; ++i)
      pool.push_back(build_nlohmann_payload(i, items));

    Timer t("dump only (prebuilt nlohmann::json)");
    std::size_t acc = 0;

    for (int i = 0; i < iters; ++i)
    {
      const Json &j = pool[static_cast<std::size_t>(i) % pool.size()];
      std::string out = j.dump();
      acc ^= out.size();
    }

    sink_sz(acc);
  }

  inline void run_all(int iters, int items)
  {
    bench_build_simple(100, 16);
    bench_build_nlohmann(100, 16);
    bench_convert_simple_to_json(100, 16);
    bench_dump_from_simple(50, 16);
    bench_dump_from_nlohmann(50, 16);
    bench_dump_only_json(50, 16);

    std::cout << "\n--- bench (iters=" << iters << ", items=" << items << ") ---\n";
    bench_build_simple(iters, items);
    bench_build_nlohmann(iters, items);
    bench_convert_simple_to_json(iters, items);
    bench_dump_from_simple(iters, items);
    bench_dump_from_nlohmann(iters, items);
    bench_dump_only_json(iters, items);

    std::cout << "\n(sink_u64=0x" << std::hex << g_sink_u64
              << ", sink_sz=0x" << g_sink_sz << std::dec << ")\n";
  }

} // namespace

int main(int argc, char **argv)
{
  int iters = 2000;
  int items = 64;

  if (argc >= 2)
    iters = std::max(1, std::atoi(argv[1]));
  if (argc >= 3)
    items = std::max(1, std::atoi(argv[2]));

  run_all(iters, items);
  return 0;
}
