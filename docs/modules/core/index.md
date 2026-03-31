# Core Overview

This page explains how the Vix.cpp core works at a high level. It is written for beginners coming from Python (FastAPI) or Node.js (Express).

## What the core provides

Vix core is the runtime layer that makes these things work together:

- HTTP server (Boost.Asio + Boost.Beast)
- Per-connection session handling (timeouts, basic safety checks)
- Router (method + path matching, path params, optional heavy scheduling)
- Request and response wrappers (simple route authoring)
- Optional OpenAPI + Swagger UI (served locally with offline assets)
- Executor and thread pool utilities for heavy workloads and periodic jobs

In practice, you write routes. The core handles networking, parsing, dispatching, and concurrency.

## Request lifecycle (end to end)

A single HTTP request goes through this flow:

1. **HTTPServer** accepts a TCP connection.
2. **Session** is created for that socket.
3. Session reads and parses an HTTP request (Beast parser).
4. Session applies basic checks:
   - request body limit
   - timeout timer
   - optional WAF rules (URL and body patterns, configurable)
5. Session asks the **Router** to match `method + path`.
6. If the route is marked **heavy**, Session posts it to the **Executor**.
   - If not heavy, Session runs the route immediately on the I/O thread.
7. The route handler writes a response.
8. Session writes the response back to the client.
9. If `keep-alive` is enabled, the same Session reads the next request on that socket.

### Why this architecture exists

If you come from Node.js:

- Node has an event loop. Vix uses Asio I/O threads for networking.
- You still want to avoid blocking I/O threads with CPU or DB-heavy work.
- Heavy routes get scheduled on a dedicated executor so the server stays responsive.

If you come from Python:

- FastAPI uses an async event loop and thread pools for blocking work.
- Vix splits the same idea into I/O threads (network) and an executor (heavy tasks).

## Core components

## HTTPServer

**Role:** Owns the `io_context`, TCP acceptor, and I/O threads.

Key behaviors:

- Initializes the acceptor and binds to the configured port.
- Starts an async accept loop (`start_accept()`).
- Launches multiple I/O worker threads (`io_context->run()`).
- Spawns a `Session` per connection.
- Runs a metrics monitor that periodically logs executor metrics.

Useful details:

- If port is `0`, the OS picks an ephemeral port. You can read it via `bound_port()`.
- `calculate_io_thread_count()` uses `config.io_threads` if set, otherwise uses hardware concurrency.
- `stop_async()` closes the acceptor and stops the io_context.
- `stop_blocking()` waits for executor idle then joins threads.

## Session

**Role:** Owns a single client connection, reads requests, routes them, writes responses.

Key behaviors:

- Uses a request parser and enforces `MAX_REQUEST_BODY_SIZE`.
- Starts a per-request timeout timer and closes slow/stalled connections.
- Optional WAF checks:
  - WAF mode: `off`, `basic`, `strict`
  - URL checks: length, null bytes, CR/LF, suspicious tokens
  - Body checks for mutating methods (POST/PUT/PATCH/DELETE)
- Uses a strand for serialized socket writes, so responses remain ordered and thread-safe.
- Heavy routes are executed on the executor, then written back to the client on the strand.

Important mental model:

- A Session is like a single `req/res` loop bound to one socket.
- It may handle multiple requests if keep-alive is enabled.

## Router

**Role:** Matches `(HTTP method, path)` and dispatches to the registered handler.

Core ideas:

- Internally it uses a route tree, keyed by `METHOD + /path/segments`.
- Path parameters are represented as `{name}`. They are stored in the tree using a wildcard key.
- It supports HEAD fallback: if a HEAD route is not found, it can reuse GET.
- It has a customizable not-found handler (default is JSON 404).

### Heavy routes

A route can be marked as heavy using `RouteOptions{ .heavy = true }`.

What it means:

- Session will schedule the route on the executor.
- This is the recommended way to isolate CPU-heavy or DB-heavy handlers.
- The Router also exposes this in docs via `x-vix-heavy: true`.

## OpenAPI and Docs

Vix can expose:

- `GET /openapi.json` generated from the Router metadata and any extra docs registered by modules.
- Swagger UI at:
  - `GET /docs`
  - `GET /docs/`
  - `GET /docs/index.html`
- Offline assets:
  - `/docs/swagger-ui.css`
  - `/docs/swagger-ui-bundle.js`

The UI is served locally, with Vix theme tokens, so the docs can work even without external CDNs.

### Disable auto docs

If you want to disable automatic docs routes for a run, use:

```bash
vix run api.cpp --no-docs
```

## Executor, ThreadPool, and timers

Vix core supports separating lightweight request handling from heavy work.

## ThreadPool

**Role:** Priority-based pool that executes tasks concurrently.

Key behaviors:

- Tasks have:
  - a function
  - a priority
  - a sequence number for stable ordering
- The pool can grow threads up to `maxThreads` when saturated and backlog increases.
- `TaskGuard` tracks active tasks using RAII.
- The pool can schedule periodic jobs via `periodicTask()`.
- Metrics snapshot:
  - pendingTasks
  - activeTasks
  - timedOutTasks

When to use it:

- CPU heavy operations (hashing, image work, large JSON processing)
- DB work (if your DB client is blocking)
- Any expensive logic you do not want on I/O threads

## interval() timer

**Role:** Simple repeating timer that posts work onto an executor.

`interval()` creates a tiny worker thread that wakes up every period and posts a task. It returns an RAII handle that stops and joins on destruction.

When to use it:

- periodic metrics logging
- cache refresh jobs
- background cleanup tasks

Important rule:

- Keep the scheduled function fast, or schedule into a heavy executor if needed.

## Mental mapping for Node.js and Python developers

## Node.js mapping (Express)

- `app.get("/path", handler)` feels like Express.
- `Request` is like `req`.
- `Response` is like `res`.
- Heavy route scheduling is like pushing work into a worker pool.

## Python mapping (FastAPI)

- Routes are simple functions.
- Returning a JSON-like object can auto-send.
- Heavy routes are conceptually similar to using thread pools for blocking work.

## Minimal example

```cpp
#include <vix.hpp>
using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request&, Response& res){
    res.json({"message", "Hello, Vix!"});
  });

  app.run(8080);

  return 0;
}
```

## Practical recommendations

- Keep I/O threads clean: do not block them with CPU-heavy work.
- Mark heavy endpoints as heavy and use an executor.
- Use `keep-alive` where appropriate for performance.
- Keep responses deterministic and always set proper status codes.
- Start with the docs routes enabled. They help beginners discover the API quickly.

