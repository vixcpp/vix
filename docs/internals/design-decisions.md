# Design Decisions

Vix is built around one idea:

```txt
C++ should feel direct for application development without losing its power.
```

## Summary of key decisions

| Decision                  | Reason                                           |
|---------------------------|--------------------------------------------------|
| Runtime, not new language | Keeps the power of C++ while improving workflow. |
| Direct compile            | Makes single-file C++ fast and simple.           |
| CMake support             | Keeps real project builds correct and portable.  |
| Explicit APIs             | Avoids hidden behavior and unexpected magic.     |
| Public module headers     | Keeps documentation stable and learnable.        |
| Middleware                | Keeps shared logic out of route handlers.        |
| Config via `.env`         | Changes runtime behavior without recompiling.    |
| Prepared statements       | Provides safer database access.                  |
| Cache is temporary        | Avoids treating cache as durable state.          |
| Sync persists first       | Prevents local operations from being lost.       |
| P2P as transport          | Separates connection logic from delivery logic.  |
| Diagnostics               | Makes C++ errors easier to understand and fix.   |
| Benchmark mode            | Measures performance with less runtime noise.    |
| Native deployment         | Runs applications as normal Linux services.      |

## 1. Vix is a runtime, not a new language

Vix keeps C++, improves the workflow around it:

```cpp
#include <vix.hpp>
using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request &, Response &res) {
    res.text("Hello Vix");
  });

  app.run(8080);

  return 0;
}
```

## 2. Application-first workflow

Start from "what do I want to build?", not from build setup:

```bash
vix run main.cpp    # immediate start
vix new api && cd api && vix dev  # real project
```

## 3. Direct compile for the simple case

One file → fast compile → run. Docs examples should be self-contained:

```bash
# Good — reader creates their own file
touch main.cpp && vix run main.cpp

# Bad — depends on repo being cloned
vix run examples/internal/file.cpp
```

## 4. CMake is supported, not replaced

```txt
direct compile → one file, fast experiment
project build  → multiple files, real app, CMake presets
```

Vix is an application workflow above the build system.

## 5. --run is separate from --

```bash
--    = compiler or linker flags (sent to compiler)
--run = program arguments (sent to argv)

vix run main.cpp -- -O2          # compiler optimization
vix run main.cpp --run server    # runtime mode
```

## 6. Public headers should be simple

```cpp
#include <vix.hpp>           // core HTTP
#include <vix/db.hpp>        // database
#include <vix/sync.hpp>      // offline-first sync
#include <vix/p2p.hpp>       // P2P runtime
```

Docs prefer public headers. Internal paths exist but users shouldn't need them first.

## 7. APIs should be explicit

```cpp
// Route — explicit
app.get("/users/{id}", [](Request &req, Response &res){
  const std::string id = req.param("id");
  res.json({"id", id});
});

// Database — explicit
auto stmt = conn->prepare("SELECT id, name FROM users WHERE id = ?");
stmt->bind(1, id);

// Sync — explicit
Operation op;
op.kind = "message.send"; op.target = "/api/messages"; op.payload = R"({"text":"hello"})";
outbox->enqueue(op, t0);
```

## 8. No garbage collector

Vix keeps the C++ ownership model: RAII, `std::unique_ptr`, `std::shared_ptr`, move semantics, destructors. Deterministic lifetimes matter for servers, networking, and database connections.

## 9. Modules are focused

```txt
json       → JSON values and responses
db         → database access
middleware → logic around routes
websocket  → realtime communication
cache      → caching layer
sync       → offline-first delivery
p2p        → peer-to-peer runtime
```

One module solves one clear problem. Module dependencies go downward: high-level API → runtime modules → core primitives → system utilities. No circular dependencies.

## 10. HTTP uses App, Request, Response

```txt
Request  → what the client sent
Response → what the app returns
App      → routes and server lifecycle
```

Small, memorable mental model.

## 11. Middleware keeps routes clean

```txt
middleware → shared request logic (CORS, rate limiting, auth, logging)
route      → business endpoint
```

## 12. Route order is explicit

```cpp
// Correct — specific before wildcard
app.get("/users/search", search_handler);
app.get("/users/{id}", user_handler);
app.get("/*", fallback_handler);
```

## 13. Config lives outside source code

```cpp
config::Config cfg{".env"};
app.run(cfg);  // port, TLS, database from .env
```

Change runtime behavior without recompiling.

## 14. Database uses prepared statements

```cpp
// Safe — prepared statement
auto stmt = conn->prepare("SELECT * FROM users WHERE id = ?");
stmt->bind(1, id);

// Dangerous — never do this
auto rows = db.query("SELECT * FROM users WHERE id = " + id);
```

## 15. Cache is not the source of truth

```txt
cache    → faster reads, temporary resilience, stale fallback
database → durable state, source of truth
WAL      → durable intent
outbox   → durable delivery queue
```

## 16. Sync persists first

```txt
persist first → then network call (not the reverse)
```

Key invariant: **an operation must exist locally before the network is used.**

## 17. Retry distinguishes temporary vs permanent failure

```cpp
// Retryable — try again later
return SendResult{.ok = false, .retryable = true, .error = "network offline"};

// Permanent — do not retry
return SendResult{.ok = false, .retryable = false, .error = "bad request"};
```

Retryable: network offline, timeout, 503. Permanent: bad request, unauthorized, validation failed.

## 18. P2P is transport, not a replacement for Sync

```txt
Sync → durability, outbox, retry, convergence
P2P  → node discovery, connection, transport
```

Sync works over HTTP, P2P, or another transport. Keep delivery logic separate from connection logic.

## 19. P2P control routes must be protected

```txt
POST /p2p/connect    → sensitive
GET  /p2p/peers      → sensitive
GET  /p2p/logs       → sensitive
POST /p2p/admin/hook → sensitive
```

Protect with authentication or keep internal. Do not expose publicly by default.

## 20. Diagnostics are part of the experience

```txt
raw error → parse → classify → code frame → hint
```

The compiler remains the source of truth. Vix adds classification, deduplication, code frames, and next-action hints.

## 21. First error matters most

Show 3 distinct errors, deduplicate the rest. Fix the first error — many later ones may disappear.

## 22. Performance must be measured

```txt
v2.4 → ~66k to 68k req/s
v2.5 → ~98k req/s        (on /bench route)
```

Concrete numbers are stronger than vague claims. Optimize one thing at a time, measure before and after.

## 23. Benchmark mode reduces noise

`VIX_BENCH_MODE=ON` removes development overhead to measure the runtime path more clearly. Never remove required behavior (validation, security, durability) for benchmark numbers.

## 24. Native deployment is intentional

```txt
systemd → Vix binary → Nginx → HTTPS users
```

No special application server. Deploy like a normal Linux service.

## 25. Health checks are normal

```cpp
app.get("/health", [](Request &, Response &res) {
  res.json({"ok", true});
});
```

Production readiness is part of normal app design.

## 26. Logs must be safe

Good logs include:

- Startup events
- Request method and path
- Error codes
- Request duration
- Sync failures
- P2P statistics

Never log:

- Passwords
- Tokens
- Cookies
- Private keys
- Authorization headers

## 27. One-file docs are intentional

Start simple, add structure later:

```bash
touch main.cpp → vix run main.cpp → works
```

Then grow to routes/, services/, repositories/ when the concept is understood.

## 28. app.del is used for DELETE

```cpp
app.del("/users/{id}", delete_handler);  // correct
```

## 29. Avoid magical claims

Better than "no configuration ever" or "everything is automatic":

> Vix makes the common workflow smoother while keeping the full C++ path available.

## What you should remember

Vix is designed around a balance:

```txt
simple workflow + explicit C++ + production reliability
```

The core design idea:
**make C++ application development feel direct without hiding how the system works.**

Vix helps developers move from:

```txt
one file → real project → production service → reliable distributed system
```

without changing the identity of C++.
