# Architecture

Vix is not only an HTTP framework.
Internally, it is composed of several layers:
- developer workflow,
- runtime core,
- application APIs,
- module system,
- diagnostics,
- build pipeline,
- and production runtime.

```txt
CLI → project detection → build/run pipeline → runtime modules → application
```

## High-level architecture

```txt
Vix CLI
  ↓
Project / Script / Manifest detection
  ↓
Build pipeline
  ↓
Runtime modules
  ↓
Application code
```

## Main layers

```txt
┌─────────────────────────────────────┐
│ User application                    │
│ App, routes, services, business code │
└─────────────────────────────────────┘
                ↓
┌─────────────────────────────────────┐
│ Public APIs                         │
│ HTTP, JSON, DB, WebSocket, Sync, P2P │
└─────────────────────────────────────┘
                ↓
┌─────────────────────────────────────┐
│ Runtime modules                     │
│ async, cache, middleware, log, env   │
└─────────────────────────────────────┘
                ↓
┌─────────────────────────────────────┐
│ Core foundation                     │
│ types, errors, utilities, config     │
└─────────────────────────────────────┘
                ↓
┌─────────────────────────────────────┐
│ System layer                        │
│ files, network, OS, process, time    │
└─────────────────────────────────────┘
```

## Execution modes

| Mode          | Used when                              | Build strategy                    |
|---------------|---------------------------------------- |-----------------------------------|
| Script mode   | Running one `.cpp` file with `vix run`. | Uses direct compilation.          |
| Project mode  | `CMakeLists.txt`, `vix.json`, or `src/` is present. | Uses the project build system. |
| Manifest mode | An `app.vix` manifest is present.      | Uses the manifest-defined build.  |

## Public API layer

```cpp
#include <vix.hpp>           // most HTTP applications
#include <vix/json.hpp>
#include <vix/db.hpp>
#include <vix/middleware.hpp>
#include <vix/websocket.hpp>
#include <vix/cache.hpp>
#include <vix/sync.hpp>
#include <vix/p2p.hpp>
```

## HTTP architecture

```txt
client request → App → middleware chain → route matching → handler → Response → client
```

Route order matters:

```cpp
// Correct
app.get("/users/search", search_handler);
app.get("/users/{id}", user_handler);
app.get("/*", fallback_handler);

// Wrong — wildcard catches everything first
app.get("/*", fallback_handler);
app.get("/users/{id}", user_handler);
```

## Middleware architecture

```txt
request → middleware 1 → middleware 2 → route handler → response
```

A middleware can continue (`next()`) or stop the request (send response and `return`).

## Database architecture

```txt
Database → Connection pool → Connection → Prepared statement → Rows → Row
```

## WebSocket architecture

```txt
client connects → on_open → typed messages → broadcast/targeted response → on_close
```

## Async architecture

```txt
create task → post coroutine handle → run io_context → task resumes through scheduler
```

## Cache architecture

```txt
request → check cache → hit → return cached value
                      → miss → compute → store → return
```

## Sync architecture

```txt
local operation → WAL → outbox → sync worker → transport → done or retry
```

Key invariant: **an operation must be persisted before the network is used.**

## P2P architecture

```txt
node → listen → connect → handshake → message exchange
```

P2P can transport sync messages: `WalPush`, `WalAck`, `OutboxPull`.

## Production architecture

```txt
browser → HTTPS → Nginx → Vix app on 127.0.0.1:8080 → systemd
```

## Repository organization

```txt
src/
├── main.cpp
├── app/        → AppContext
├── routes/     → HTTP route registration
├── services/   → business logic
├── repositories/ → database access
├── models/     → data structures
└── support/    → shared helpers
```

## Module dependency direction

```txt
high-level APIs → runtime modules → core primitives → system utilities
```

HTTP may use JSON. JSON should not depend on HTTP. Avoid circular dependencies.

## Design principles

1. **Application-first:** start from what you want to build, not from build configuration.
2. **Explicit C++:** ownership, lifetimes, types, and errors remain understandable.
3. **Fast path for simple cases:** a single file should run quickly.
4. **Modules are focused:** each module solves a clear, well-defined problem.
5. **Reliability matters:** handle errors safely with durable operations, retries, and health checks.
6. **No unnecessary magic:** the system should remain transparent and debuggable.

## What you should remember

The main layers are:
CLI workflow → public APIs → runtime modules → core foundation → system layer.

The core idea:
**make C++ applications easier to build without hiding C++.**

Next: [Runtime Model](/internals/runtime-model)
