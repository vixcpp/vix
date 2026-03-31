# Compression Middleware Guide

## What is HTTP Compression?

Compression reduces the size of HTTP responses before sending them to the client.

Benefits:

- Faster network transfer
- Lower bandwidth usage
- Better performance on mobile or slow connections

In Vix.cpp, compression is middleware-based and automatic.

---

## How Compression Works

1. Client sends:
   Accept-Encoding: gzip, br

2. Server checks:
   - Is compression enabled?
   - Is response size >= min_size?
   - Does client support compression?

3. If yes:
   - Response is compressed
   - Vary header is added (if enabled)

---

## Minimal Example

```cpp
App app;

auto mw = vix::middleware::app::adapt_ctx(
    vix::middleware::performance::compression({
        .min_size = 8,
        .add_vary = true,
        .enabled = true,
    }));

app.use(std::move(mw));

app.get("/x", [](Request&, Response& res)
{
    res.send(std::string(20, 'a'));
});

app.run(8080);
```

---

## Run

```bash
vix run compression_app_simple.cpp
```

---

## Test With curl

### 1) No Accept-Encoding

```bash
curl -i http://localhost:8080/x
```

Result:
No compression applied.

---

### 2) With Accept-Encoding

```bash
curl -i -H "Accept-Encoding: gzip, br" http://localhost:8080/x
```

If body is large enough:
Compression is applied.

---

### 3) Small body (below min_size)

```bash
curl -i -H "Accept-Encoding: gzip" http://localhost:8080/small
```

No compression because body < min_size.

---

## Important Options

.min_size
Minimum response size required to compress.

.enabled
Enable or disable compression.

.add_vary
Adds:
Vary: Accept-Encoding

This is important for proper caching behavior.

---

# Complete Working Example

Save as: compression_app_simple.cpp

```cpp
#include <iostream>
#include <string>
#include <vix.hpp>

#include <vix/middleware/app/adapter.hpp>
#include <vix/middleware/performance/compression.hpp>

using namespace vix;

int main()
{
    App app;

    // Install compression globally
    auto mw = vix::middleware::app::adapt_ctx(
        vix::middleware::performance::compression({
            .min_size = 8,
            .add_vary = true,
            .enabled = true,
        }));

    app.use(std::move(mw));

    app.get("/", [](Request&, Response& res)
    {
        res.send("Compression middleware installed.");
    });

    // Large body (should compress if client supports it)
    app.get("/x", [](Request&, Response& res)
    {
        res.status(200).send(std::string(20, 'a'));
    });

    // Small body (no compression)
    app.get("/small", [](Request&, Response& res)
    {
        res.status(200).send("aaaa");
    });

    app.run(8080);
    return 0;
}
```

---

## Summary

Compression middleware:

- Automatically checks Accept-Encoding
- Compresses only when useful
- Keeps your code clean
- Improves performance without changing route logic

Vix.cpp keeps compression simple and explicit.

