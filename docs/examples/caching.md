# HTTP Caching Guide (Vix.cpp) — Beginner Friendly

Caching means: **save a response**, then reuse it for the next request.

This makes your API:

- Faster
- Cheaper (less CPU)
- More scalable

In this guide, we cache **GET** responses under `/api/*`.

---

## What gets cached?

Usually only:

- GET requests
- Safe endpoints (no user-specific secrets unless you vary by headers)

You typically do NOT cache:

- POST
- PUT
- PATCH
- DELETE

---

## Minimal Caching Example

This caches GET `/api/*` for 30 seconds:

```cpp
App app;

app.use("/api/", middleware::app::http_cache({
  .ttl_ms = 30'000,
  .allow_bypass = true,
  .bypass_header = "x-vix-cache",
  .bypass_value = "bypass",
}));

app.get("/api/users", [](Request&, Response& res){
  res.text("users from origin");
});

app.run(8080);
```

---

## Run

```bash
vix run examples/http_cache_app_simple.cpp
```

---

## Test

### 1) First request = MISS (origin)

```bash
curl -i http://localhost:8080/api/users
```

### 2) Second request = HIT (cached)

```bash
curl -i http://localhost:8080/api/users
```

### 3) Force origin (bypass)

```bash
curl -i -H "x-vix-cache: bypass" http://localhost:8080/api/users
```

---

## Debug Header

If you enable debug headers:

```cpp
.add_debug_header = true,
.debug_header = "x-vix-cache-status",
```

You will see:

- MISS
- HIT
- BYPASS

Example:

```bash
curl -i http://localhost:8080/api/users
```

Look for:

x-vix-cache-status: HIT

---

## Vary Headers (different cache per language)

If you return different content depending on headers, you must vary the cache key:

Example: `Accept-Language`

```cpp
app.use("/api/", middleware::app::http_cache({
  .ttl_ms = 30'000,
  .vary_headers = {"accept-language"},
  .add_debug_header = true,
  .debug_header = "x-vix-cache-status",
}));
```

Test:

```bash
curl -i -H "Accept-Language: fr" http://localhost:8080/api/users
curl -i -H "Accept-Language: en" http://localhost:8080/api/users
```

These should be **two separate cache entries**.

---

## Custom Cache Injection (advanced but useful)

Sometimes you want to inject your own cache instance.

Example:

- MemoryStore
- Custom eviction policy
- Shared cache between middlewares

```cpp
auto cache = middleware::app::make_default_cache({
  .ttl_ms = 30'000,
});

app.use("/api/", middleware::app::http_cache_mw({
  .prefix = "/api/",
  .only_get = true,
  .ttl_ms = 30'000,
  .cache = cache,
  .add_debug_header = true,
  .debug_header = "x-vix-cache-status",
}));
```

---

# Complete Example (copy-paste)

This is a full working file you can run.

Save as: `http_cache_app_simple.cpp`

```cpp
#include <vix.hpp>
#include <vix/middleware/app/app_middleware.hpp>

using namespace vix;

static void register_routes(App& app)
{
  app.get("/", [](Request&, Response& res)
  {
    res.text("home (not cached)");
  });

  app.get("/api/users", [](Request&, Response& res)
  {
    res.text("users from origin");
  });
}

int main()
{
  App app;

  // Cache GET requests under /api/*
  app.use("/api/", middleware::app::http_cache({
    .ttl_ms = 30'000,
    .allow_bypass = true,
    .bypass_header = "x-vix-cache",
    .bypass_value = "bypass",
  }));

  register_routes(app);

  app.run(8080);
  return 0;
}
```

## Run

```bash
vix run http_cache_app_simple.cpp
```


