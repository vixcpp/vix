# Cache

In the previous chapter, you learned the async runtime.
Now you will learn cache.

```txt
request → cache lookup → cache hit → return cached result
                       → cache miss → compute or fetch → store → return response
```

In Vix, cache is not only about speed — it is also part of offline-first behavior.
When the network fails, a cache can safely serve previously stored data.

## Public headers

```cpp
#include <vix/cache/Cache.hpp>
#include <vix/cache/CacheContext.hpp>
#include <vix/cache/CacheEntry.hpp>
#include <vix/cache/CachePolicy.hpp>
#include <vix/cache/MemoryStore.hpp>
#include <vix/cache/FileStore.hpp>
#include <vix/cache/LruMemoryStore.hpp>
#include <vix/cache/CacheKey.hpp>
```

## Core concepts

| Concept          | Purpose                                     |
| ---------------- | ------------------------------------------- |
| `Cache`          | Decides whether a cached entry is usable.   |
| `CacheEntry`     | Stores one cached response or value.        |
| `CachePolicy`    | Defines TTL, freshness, and stale behavior. |
| `CacheContext`   | Describes the current network condition.    |
| `MemoryStore`    | Stores cached entries in memory.            |
| `FileStore`      | Stores cached entries on disk.              |
| `LruMemoryStore` | Stores bounded entries with LRU eviction.   |
| `CacheKey`       | Builds stable keys for cached entries.      |

## Time helper

```cpp
static std::int64_t now_ms()
{
  using namespace std::chrono;
  return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}
```

## CacheEntry

```cpp
vix::cache::CacheEntry entry;
entry.status = 200;
entry.body = R"({"users":[1,2,3]})";
entry.headers["Content-Type"] = "application/json";
entry.created_at_ms = t0;
```

## CachePolicy

```cpp
vix::cache::CachePolicy policy;
policy.ttl_ms = 5'000;                    // fresh for 5 seconds
policy.allow_stale_if_offline = true;
policy.stale_if_offline_ms = 10'000;      // allow stale up to 10s when offline
policy.allow_stale_if_error = true;
policy.stale_if_error_ms = 5'000;         // allow stale up to 5s on network error
```

## CacheContext

```cpp
CacheContext::Online()        // normal operation
CacheContext::Offline()       // no network
CacheContext::NetworkError()  // network request failed
```

## Minimal memory cache

```cpp
auto store = std::make_shared<vix::cache::MemoryStore>();
vix::cache::CachePolicy policy;
policy.ttl_ms = 5'000;
vix::cache::Cache cache(policy, store);

const std::string key = "GET /api/users?page=1";
const auto t0 = now_ms();

vix::cache::CacheEntry entry;
entry.status = 200;
entry.body = R"({"users":[1,2,3]})";
entry.created_at_ms = t0;
cache.put(key, entry);

auto cached = cache.get(key, t0 + 100, vix::cache::CacheContext::Online());
if (cached) std::cout << "cache hit: " << cached->body << "\n";
```

## FileStore (persistence)

```cpp
auto store = std::make_shared<vix::cache::FileStore>(vix::cache::FileStore::Config{
    .file_path = "./cache.json",
    .pretty_json = true});
```

Entries persist to disk and survive process restart.

## LruMemoryStore (bounded memory)

```cpp
auto store = std::make_shared<vix::cache::LruMemoryStore>(
    vix::cache::LruMemoryStore::Config{.max_entries = 2048});
```

When capacity is reached, the least recently used entry is evicted.

## Stale data

```cpp
// Online — fresh data only (within TTL)
cache.get(key, t0 + 50, CacheContext::Online());     // hit if within 5000ms

// Offline — allow stale up to stale_if_offline_ms
cache.get(key, t0 + 3000, CacheContext::Offline());  // hit if within 10000ms

// Network error — allow stale up to stale_if_error_ms
cache.get(key, t0 + 4000, CacheContext::NetworkError());  // hit if within 5000ms

// Too old — miss regardless of context
cache.get(key, t0 + 20'000, CacheContext::Offline());  // miss
```

## CacheKey builder

```cpp
#include <vix/cache/CacheKey.hpp>

std::unordered_map<std::string, std::string> req_headers;
req_headers["Accept"] = "application/json";

const std::string key = vix::cache::CacheKey::fromRequest(
    "get",
    "/api/users",
    "b=2&a=1",      // query is normalized
    req_headers,
    {"Accept"});    // vary on Accept header
```

Good cache keys include:
method,
path,
normalized query params,
and selected vary headers.

## Cache in an HTTP route

```cpp
app.get("/api/products", [&cache](Request &, Response &res){
  const auto now = now_ms();
  const std::string key = "GET /api/products";

  auto cached = cache.get(key, now, vix::cache::CacheContext::Online());
  if (cached)
  {
    res.header("X-Vix-Cache", "HIT");
    res.status(cached->status).send(cached->body);
    return;
  }

  const std::string body = R"({"ok":true,"data":[]})";

  vix::cache::CacheEntry entry;
  entry.status = 200;
  entry.body = body;
  entry.headers["Content-Type"] = "application/json";
  entry.created_at_ms = now;
  cache.put(key, entry);

  res.header("X-Vix-Cache", "MISS");
  res.header("Content-Type", "application/json");
  res.send(body);
});
```

## Policy examples

```cpp
// Short API cache
policy.ttl_ms = 5'000;

// Offline-friendly
policy.ttl_ms = 30'000;
policy.allow_stale_if_offline = true;
policy.stale_if_offline_ms = 10 * 60 * 1000;

// Network-error fallback
policy.ttl_ms = 10'000;
policy.allow_stale_if_error = true;
policy.stale_if_error_ms = 60'000;
```

## Good vs bad cache candidates

#### Good use cases

- Public `GET` responses
- Product lists
- Configuration
- Feature flags
- Read-heavy dashboards

#### Be careful with

- Private user data
- Payment state
- Security decisions
- Rapidly changing values

## Common mistakes

### Ignoring query parameters

```cpp
// Wrong — same key for page=1 and page=2
const std::string key = "/api/products";

// Correct — use CacheKey builder
const std::string key = vix::cache::CacheKey::fromRequest("get", "/api/users", "page=1", {}, {});
```

### No memory limit

```cpp
// Protect memory for long-running servers
LruMemoryStore::Config{.max_entries = 2048}
```

### Not invalidating after writes

If a POST, PUT, PATCH, or DELETE changes data, old cached GET responses may become stale.
Use short TTLs or explicit invalidation.

### Caching error responses

A temporary 500 response should usually not be cached.

## What you should remember

Vix cache is built from explicit primitives:
`Cache`,
`CacheEntry`,
`CachePolicy`,
`CacheContext`,
`store`,
`CacheKey`.

For offline-first:
`Online` prefers fresh,
`Offline` optionally allows stale,
`NetworkError` optionally allows stale.

The core idea:
cache is not only an optimization — in Vix,
cache is part of predictable behavior when the network is slow,
unstable,
or unavailable.

## Next chapter

[Next: Offline-first sync](/book/14-offline-first-sync)
