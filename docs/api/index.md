# API Reference

This section documents the public Vix APIs used to build applications.

```cpp
#include <vix.hpp>
#include <vix/json.hpp>
#include <vix/middleware.hpp>
#include <vix/db.hpp>
#include <vix/websocket.hpp>
#include <vix/async.hpp>
#include <vix/cache.hpp>
#include <vix/sync.hpp>
#include <vix/p2p.hpp>
#include <vix/p2p_http.hpp>
#include <vix/log.hpp>
#include <vix/validation.hpp>
#include <vix/fs.hpp>
```

## What this section is for

The book teaches Vix step by step. The guides show practical recipes. The API reference explains the public API surface.

Use this section when you want to answer: which header to include, which class to use, what an API does, how App/Request/Response work, how JSON/middleware/WebSocket/async/P2P fit together.

## Main public API areas

| Area | Header | Purpose |
|------|--------|---------|
| HTTP | `<vix.hpp>` | App, Request, Response, routes |
| JSON | `<vix/json.hpp>` | JSON values, objects, arrays, helpers |
| Middleware | `<vix/middleware.hpp>` | CORS, rate limiting, static files, adapters |
| Config | `<vix/env.hpp>` or `<vix.hpp>` | .env and runtime configuration |
| WebSocket | `<vix/websocket.hpp>` | Real-time bidirectional communication |
| Async | `<vix/async.hpp>` | Event loop, tasks, timers, signals, networking |
| Database | `<vix/db.hpp>` | SQLite, MySQL, queries, prepared statements |
| Cache | `<vix/cache.hpp>` | Memory cache, file cache, TTL, stale data |
| Sync | `<vix/sync.hpp>` | WAL, outbox, retry, offline-first sync |
| P2P | `<vix/p2p.hpp>` | Peer-to-peer runtime, discovery, messages |
| P2P HTTP | `<vix/p2p_http.hpp>` | HTTP control routes for P2P runtimes |
| Logging | `<vix/log.hpp>` | Structured logs and log levels |
| Validation | `<vix/validation.hpp>` | Field validation, schemas, models |
| Filesystem | `<vix/fs.hpp>` | Filesystem helpers |

## The most common header

For most HTTP applications:

```cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;
  app.get("/", [](Request &, Response &res)
          { res.json({"message", "Hello from Vix"}); });
  app.run(8080);
  return 0;
}
```

## How the APIs fit together

```txt
HTTP routes → JSON responses → middleware → validation → database → logs
                                                       → cache or sync when needed

HTTP API → WebSocket → async runtime          (real-time systems)

local write → cache → sync → P2P              (offline-first systems)

Vix app → release binary → systemd → Nginx    (production systems)
```

## Public headers summary

```txt
vix.hpp            → core HTTP application API
vix/json.hpp       → JSON API
vix/middleware.hpp → middleware API
vix/db.hpp         → database API
vix/websocket.hpp  → WebSocket API
vix/async.hpp      → async runtime API
vix/cache.hpp      → cache API
vix/sync.hpp       → offline-first sync API
vix/p2p.hpp        → P2P API
vix/p2p_http.hpp   → P2P HTTP control API
vix/log.hpp        → logging API
vix/validation.hpp → validation API
vix/fs.hpp         → filesystem helpers
```

## Which page to read next

| Need | Read |
|------|------|
| Build routes | [HTTP API](/api/http) |
| Return JSON | [JSON API](/api/json) |
| Add CORS or rate limiting | [Middleware API](/api/middleware) |
| Read .env settings | [Config API](/api/config) |
| Add realtime features | [WebSocket API](/api/websocket) |
| Learn runtime primitives | [Async API](/api/async) |
| Build distributed nodes | [P2P API](/api/p2p) |

## What you should remember

The API reference is not the best place to start learning Vix from zero — for that, read the book. Use this section when you already know what you want and need the exact public API surface.

The most common starting point is `#include <vix.hpp>`.

The core idea: Vix gives C++ a runtime-oriented application API.
