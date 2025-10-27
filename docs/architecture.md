# Architecture — Vix.cpp

Vix.cpp is organized as a **small, sharp core** with optional modules. You can consume modules independently or as an umbrella package.

---

## High‑Level View

```
+----------------------+      +---------------------+
|      Your App        |      |     CLI (vix)       |
|  (routes, services)  |      |  new/build/run      |
+----------+-----------+      +----------+----------+
           |                               |
           v                               v
+----------------------+      +---------------------+
|        core          |<---->|       devtools      |
|  App, Router, HTTP   |      |   scripts, presets  |
+----+----+----+-------+      +---------------------+
     |    |    |
     |    |    +-------------------+
     |    |                        |
     v    v                        v
+----+----+----+--------+  +------+-------+  +-------------------+
|   utils   |  json     |  |   websocket  |  |        orm        |
| Logger    | builders  |  | (WIP)        |  | Repo/UoW, drivers |
| UUID/Env  | nlohmann  |  |              |  | MySQL/SQLite      |
+-----------+-----------+  +--------------+  +--------------------+
```

- **core** — HTTP server (Asio/Beast), router, request/response, status codes.
- **utils** — Logger (sync/async), UUID, Time, Env, Validation helpers.
- **json** — light wrappers/builders around _nlohmann/json_ for ergonomic responses.
- **orm** — optional: Repository + Unit of Work, QueryBuilder, connection pool.
- **websocket** — WIP: channels, rooms, backpressure.
- **cli** — `vix new/build/run` to scaffold and operate projects.
- **devtools** — local tooling, scripts, presets.

---

## Core Components

### `App`

- Owns the HTTP server and route registry.
- Methods: `get`, `post`, `put`, `del`, `use(middleware)`.
- `run(port)` starts the event loop; graceful stop via signals.

### Router

- Templates routes with path parameters: `/users/{id}`.
- Parameter extraction is type‑agnostic (string‑first, convert as needed).
- Middleware hooks (logging/validation) composed per route or globally.

### HTTP

- Thin wrappers around Beast types.
- `ResponseWrapper` exposes convenient `json(...)`, `status(...)`, `send(...)` helpers.
- Intentional minimalism: no hidden thread‑locals or globals.

---

## Concurrency Model

- Event‑driven I/O based on Asio.
- Compute offloading via a **thread pool** (configurable).
- Lock boundaries minimized; prefer immutable data in hot paths.
- Graceful shutdown coordinates listener, workers, and in‑flight requests.

---

## Error Handling & Logging

- Runtime errors bubble to a predictable handler in core.
- Structured logs via `utils::Logger` (sync/async) with context (request_id/module).
- Sanitizer‑friendly builds and optional static analysis (`clang-tidy`, `cppcheck`).

---

## JSON Path

- Input: `nlohmann::json` parsing recommended for request bodies.
- Output: `Vix::json` builders (tokens/obj/array) for zero‑friction responses.
- Avoids repetitive boilerplate while keeping conversions explicit.

---

## ORM Integration (Optional)

- Connection pooling, prepared statements, RAII transactions.
- Repository/Unit‑of‑Work pattern reduces boilerplate and enforces consistency.
- Can be used standalone (separate CMake target) or with the umbrella build.

---

## Configuration

- Simple JSON at `config/config.json` copied to build dir.
- ENV helpers: `utils::env_str`, `env_int`, `env_bool` for overrides.
- Prefer explicit constructor injection for services and repositories.

---

## Build System

- CMake ≥ 3.20, generator‑agnostic (Make/Ninja/VS).
- Presets recommended for debug/asan/release.
- `compile_commands.json` exported automatically for IDEs.

---

## Roadmap Notes

- WebSocket engine (channels/rooms/backpressure).
- Middlewares: CORS presets, rate limiting, auth helpers.
- Devtools: profiler hooks, trace exporters.
- ORM: query planner and driver adapters.
