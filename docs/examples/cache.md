# Cache

Add cached responses with TTL and cache invalidation.

```txt
request → check cache → hit → return fast response
                      → miss → compute → store → return response
```

## What you will build

```txt
GET /health         → health check
GET /time           → uncached current time
GET /cached-time    → cached time with 5s TTL
POST /cache/clear   → clear the cache
```

## Create a workspace

```bash
mkdir -p ~/tmp/vix-examples/cache
cd ~/tmp/vix-examples/cache
touch main.cpp
```

## Full code

```cpp
#include <chrono>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>
#include <vix.hpp>
using namespace vix;

struct CacheEntry {
  std::string value; std::int64_t expires_at_ms{};
};

class SimpleMemoryCache
{
public:
  void set(const std::string &key, const std::string &value,
           std::int64_t now_ms, std::int64_t ttl_ms)
  {
    entries_[key] = CacheEntry{value, now_ms + ttl_ms};
  }

  std::optional<std::string> get(const std::string &key, std::int64_t now_ms)
  {
    auto it = entries_.find(key);
    if (it == entries_.end()) return std::nullopt;
    if (it->second.expires_at_ms <= now_ms) {
      entries_.erase(it);
      return std::nullopt;
    }

    return it->second.value;
  }

  void clear() {
    entries_.clear();
  }

  std::size_t size() const {
    return entries_.size();
  }

private:
  std::unordered_map<std::string, CacheEntry> entries_;
};

static std::int64_t now_ms()
{
  using namespace std::chrono;
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

static std::string current_time_string() {
  return std::to_string(now_ms());
}

static void register_routes(App &app, SimpleMemoryCache &cache)
{
  app.get("/health", [](Request &, Response &res){
    res.json({
      "ok", true,
      "service", "cache-example"
    });
  });

  app.get("/time", [](Request &, Response &res){
    res.json({
      "ok", true,
      "cached", false,
      "time_ms", current_time_string()
    });
  });

  app.get("/cached-time", [&cache](Request &, Response &res){
    const std::string key = "current_time";
    const std::int64_t now = now_ms();
    const std::int64_t ttl_ms = 5000;

    const auto cached = cache.get(key, now);
    if (cached)
    {
      res.header("X-Cache", "HIT");

      res.json({
        "ok", true,
        "cached", true,
        "time_ms", *cached,
        "cache_size", static_cast<int>(cache.size())
      });

      return;
    }

    const std::string fresh = current_time_string();
    cache.set(key, fresh, now, ttl_ms);

    res.header("X-Cache", "MISS");
    res.json({
      "ok", true,
      "cached", false,
      "time_ms", fresh,
      "ttl_ms", ttl_ms,
      "cache_size", static_cast<int>(cache.size())
    });

  });

  app.post("/cache/clear", [&cache](Request &, Response &res){
    cache.clear();
    res.json({
      "ok", true,
      "message", "cache cleared",
      "cache_size", static_cast<int>(cache.size())
    });
  });
}

int main()
{
  App app;

  SimpleMemoryCache cache;
  register_routes(app, cache);

  app.run(8080);

  return 0;
}
```

## Run and test

```bash
vix run main.cpp
```

```bash
curl -i http://127.0.0.1:8080/health
curl -i http://127.0.0.1:8080/time           # always fresh
curl -i http://127.0.0.1:8080/cached-time    # X-Cache: MISS
curl -i http://127.0.0.1:8080/cached-time    # X-Cache: HIT, same time_ms
curl -i -X POST http://127.0.0.1:8080/cache/clear
curl -i http://127.0.0.1:8080/cached-time    # X-Cache: MISS again
```

## Cache key design

```txt
Good keys:           users:list:page:1, user:id:42, settings:public
Weak keys:           data, value, cache, result
User-specific keys:  profile:user:42 (not just "profile")
```

## Cache invalidation after writes

```cpp
app.post("/users", [&cache](Request &req, Response &res){
  // create user...
  cache.clear();  // or: cache.remove("users:list")
  res.status(201).json({
    "ok", true,
    "message", "user created"
  });
});
```

## HTTP cache headers

```cpp
res.header("Cache-Control", "public, max-age=5");   // browser/proxy cache
res.header("Cache-Control", "no-store");             // never cache
res.header("Cache-Control", "private, max-age=60"); // browser only
```

## Cache vs database vs sync

| | Cache | Database | Sync |
|-|-------|----------|------|
| Purpose | Fast reads | Durable state | Durable writes |
| Survives restart? | No (memory) | Yes | Yes |
| Source of truth? | No | Yes | — |

## Common mistakes

```cpp
// Wrong — same key for all users
cache.set("profile", value, now, ttl);

// Correct — user-specific key
cache.set("profile:user:" + std::to_string(user_id), value, now, ttl);
```

Do not cache 5xx errors for long TTLs. Do not use cache as your source of truth.

## What you should remember

```txt
check cache → hit → return fast
           → miss → compute → cache.set → return
```

The core idea: **cache is a performance layer, not a durability layer.**

Next: [Sync](/examples/sync)
