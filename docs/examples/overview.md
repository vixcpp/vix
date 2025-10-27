# Examples — Overview (Vix.cpp)

This directory showcases runnable examples that cover routing, JSON helpers, validation, logging, time/env utilities, and CRUD patterns.

---

## Quick Start

Build examples (assuming umbrella build):

```bash
cmake -S . -B build-rel -DCMAKE_BUILD_TYPE=Release
cmake --build build-rel -j
```

Run any example from the build directory:

```bash
./build-rel/<example_name>
```

---

## Catalog

| Example                     | What it shows                                          | Link                             |
| --------------------------- | ------------------------------------------------------ | -------------------------------- |
| `hello_routes`              | Basic GET routes and path params                       | `./hello_routes.md`              |
| `user_crud_with_validation` | Full CRUD + validation + JSON responses                | `./user_crud_with_validation.md` |
| `post_put_delete`           | Individual POST/PUT/DELETE examples                    | `./post_put_delete.md`           |
| `json_builders_routes`      | JSON builders with `Vix::json::obj/array/tokens`       | `./json_builders_routes.md`      |
| `logger_context_and_uuid`   | Contextual logging (request_id/module) + async logging | `./logger_context_and_uuid.md`   |
| `env_time_port`             | Env helpers + time utilities + ISO8601 responses       | `./env_time_port.md`             |

> Each page provides the source code, how to run, and sample outputs (curl / wrk).

---

## Tips

- Use `wrk` to stress‑test:
  ```bash
  wrk -t8 -c200 -d30s --latency http://localhost:8080/
  ```
- Prefer `-G Ninja` for iterative edits.
- Keep ports unique if you run multiple examples at once.
