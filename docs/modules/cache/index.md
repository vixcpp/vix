# Cache

The cache module is a small, explicit, policy driven cache layer designed for offline first systems.

It is built around three ideas:

- A `CacheStore` persists entries (memory, file, etc).
- A `CachePolicy` decides freshness and stale reuse rules.
- A `CacheContext` carries runtime semantics (offline, network error) that influence decisions.

This module is intentionally deterministic: the cache does not perform network IO, does not hide state transitions, and makes reuse decisions based on explicit inputs.

---

## Concepts

### CacheEntry

A cached response entry.

Fields (typical HTTP use):

- `status` (int)
- `body` (string)
- `headers` (map)
- `created_at_ms` (int64)

This is the minimal metadata required for policy decisions.

### CacheContext

A small runtime context used during `get()`:

- `offline`: true when the system is considered offline
- `network_error`: true when the last request failed due to network issues

Helpers:

- `CacheContext::Online()`
- `CacheContext::Offline()`
- `CacheContext::NetworkError()`

There is also `CacheContextMapper` to derive context from `vix::net::NetworkProbe` and a request outcome.

### CachePolicy

Defines freshness and stale reuse rules, using only entry age:

- `ttl_ms`: entry is fresh when `age_ms <= ttl_ms`
- `stale_if_error_ms`: maximum age allowed when `network_error` is true
- `stale_if_offline_ms`: maximum age allowed when `offline` is true
- flags to enable or disable stale reuse on error or offline

Helpers:

- `is_fresh(age_ms)`
- `allow_stale_error(age_ms)`
- `allow_stale_offline(age_ms)`

### CacheStore

Storage backend interface:

- `put(key, entry)`
- `get(key) -> optional`
- `erase(key)`
- `clear()`

Stores are synchronous by design to keep behavior explicit and testable.

### Cache facade

`Cache` composes a policy and a store:

- `get(key, now_ms, ctx) -> optional<CacheEntry>`
- `put(key, entry)`
- `prune(now_ms) -> size_t`

Policy is consulted on `get()` to decide whether the stored entry is usable for the given context.

---

## Cache keys

### CacheKey

A deterministic key builder for request based caching.

It normalizes:

- method uppercased
- query parameters sorted
- selected header names lowercased
- header values trimmed

Key format:

`METHOD path?sorted_query |h:header=value;header=value;`

Use it for HTTP GET caching where equivalent requests must map to the same cache entry.

Example:

```cpp
#include <vix/cache/CacheKey.hpp>

std::unordered_map<std::string, std::string> headers = {
  {"Accept", "application/json"},
  {"Authorization", "Bearer X"} // typically excluded
};

std::string key = vix::cache::CacheKey::fromRequest(
  "GET",
  "/api/products",
  "page=2&limit=20",
  headers,
  {"accept"} // include only what should vary the cache
);
```

Tip: include as few headers as possible. Prefer stable headers like `accept` or a locale header. Avoid volatile ones like authorization unless you really need per user caching.

---

## Stores

### MemoryStore

Simple hash map store.

- Thread safe via a single mutex
- No eviction
- No persistence

Good for tests, small caches, and short lived processes.

### LruMemoryStore

An in memory LRU store.

- O(1) average put get erase
- Fixed max number of entries
- Thread safe via a single mutex

Good for hot path caching where you want bounded memory.

### FileStore

A file backed store that persists all entries into a single JSON file.

- Loads lazily on first access
- Keeps an in memory map protected by a mutex
- Any mutation triggers a flush to disk

Config:

- `file_path` (default `./.vix/cache_http.json`)
- `pretty_json` (debug friendly, larger file)

This is best for durable local caching (offline first HTTP cache) when you want the cache to survive restarts.

---

## Typical usage

This example shows the intended workflow for HTTP GET caching:

1) Build a deterministic key.
2) Try `cache.get()` with the current context.
3) If cache hit, serve it.
4) If miss, fetch from network, then `cache.put()`.

```cpp
#include <vix/cache/Cache.hpp>
#include <vix/cache/CacheEntry.hpp>
#include <vix/cache/CacheKey.hpp>
#include <vix/cache/CachePolicy.hpp>
#include <vix/cache/LruMemoryStore.hpp>

using vix::cache::Cache;
using vix::cache::CacheContext;
using vix::cache::CacheEntry;
using vix::cache::CachePolicy;
using vix::cache::LruMemoryStore;

static std::int64_t now_ms(); // your clock helper

int main()
{
  CachePolicy policy;
  policy.ttl_ms = 30'000;
  policy.stale_if_error_ms = 5 * 60'000;
  policy.stale_if_offline_ms = 30 * 60'000;

  auto store = std::make_shared<LruMemoryStore>(LruMemoryStore::Config{.max_entries = 2048});
  Cache cache(policy, store);

  std::unordered_map<std::string, std::string> headers = {{"Accept", "application/json"}};

  std::string key = vix::cache::CacheKey::fromRequest(
    "GET",
    "/api/products",
    "page=1&limit=20",
    headers,
    {"accept"}
  );

  CacheContext ctx = CacheContext::Online();
  if (auto hit = cache.get(key, now_ms(), ctx))
  {
    // Serve from cache
    // status = hit->status
    // body = hit->body
    // headers = hit->headers
    return 0;
  }

  // Network fetch happens outside of cache.
  // After success:
  CacheEntry e;
  e.status = 200;
  e.body = "{\"ok\":true}";
  e.created_at_ms = now_ms();

  cache.put(key, e);
  return 0;
}
```

---

## Offline and network error behavior

The cache does not detect offline state by itself. You pass it in via `CacheContext`.

- When `ctx.offline == true`, policy may allow stale reuse up to `stale_if_offline_ms`.
- When `ctx.network_error == true`, policy may allow stale reuse up to `stale_if_error_ms`.

If neither applies and the entry is older than `ttl_ms`, the cache returns `nullopt`.

To derive context from `vix::net::NetworkProbe`, use `CacheContextMapper`:

```cpp
#include <vix/cache/CacheContextMapper.hpp>

auto ctx = vix::cache::contextFromProbeAndOutcome(
  probe,
  now_ms,
  vix::cache::RequestOutcome::NetworkError
);
```

---

## Pruning

`Cache::prune(now_ms)` removes expired entries according to the policy.

Call it:

- at startup
- periodically (timer)
- before persisting big batches
- after long offline sessions

Exact pruning behavior depends on the store implementation and the Cache facade implementation.

---

## Design notes

- This cache layer is optimized for explicitness, determinism, and offline first correctness.
- It is ideal for HTTP GET caching and store and forward style systems.
- If you need advanced validation (ETag, If-Modified-Since), keep it explicit at the HTTP layer and store any validation metadata in `CacheEntry::headers`.

