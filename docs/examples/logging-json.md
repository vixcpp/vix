# Logging (JSON) in Vix.cpp

You are mixing two different things:

- **`dumps()`** is like `JSON.stringify()` in JavaScript. It converts JSON to a string.
- **`console.*`** is like `console.log()` in JavaScript. It prints logs (with levels, filtering, stdout/stderr routing).

So the answer is:

- If you want a **string**, use `dumps()` or `dumps_compact()`.
- If you want to **log**, use `vix::console` (dev) or `vix::utils::Logger` (production).
- If you want **JSON logs**, log a single line JSON string (usually compact) so log collectors can parse it.

---

## Include

```cpp
#include <vix.hpp>
#include <vix/json/dumps.hpp>
```

```cpp
using namespace vix;
using vix::json::Json;
using vix::json::dumps;
using vix::json::dumps_compact;
```

---

## dumps vs console: mapping to JS

| JavaScript | Vix.cpp equivalent | What it does |
|---|---|---|
| `JSON.stringify(obj)` | `dumps_compact(j)` | serialize JSON to one line |
| `JSON.stringify(obj, null, 2)` | `dumps(j)` | serialize JSON pretty |
| `console.log(...)` | `console.info(...)` | log to stdout |
| `console.error(...)` | `console.error(...)` | log to stderr |
| structured log line (JSON) | `console.info(dumps_compact(j))` | easy ingestion by ELK/Loki |

---

## 1) Dev logging: console + compact JSON

This is the simplest and most useful pattern for "logging-json".

```cpp
#include <vix.hpp>
#include <vix/json/dumps.hpp>

using namespace vix;
using vix::json::Json;
using vix::json::dumps_compact;

int main()
{
  App app;

  app.get("/api/ping", [](Request& req, Response& res)
  {
    Json log = {
      {"level", "info"},
      {"event", "ping"},
      {"method", req.method()},
      {"path", req.path()},
    };

    console.info(dumps_compact(log));

    res.json({ "ok", true });
  });

  app.run(8080);
  return 0;
}
```

Why compact?
- One request produces one log line.
- Easy parsing by tools.
- Fast and low overhead.

---

## 2) Dev pretty logs: only when debugging

Pretty JSON is human-friendly but it creates multi-line logs.

```cpp
Json j = {
  {"event", "debug_dump"},
  {"path", "/api/ping"}
};

console.debug(dumps(j)); // multi-line
```

Use it only during debugging.

---

## 3) Log format: json-pretty vs one-line JSON

In Vix, you have two layers:

1) **Your log payload**: the JSON you decide to log
2) **The runtime log format** (CLI flags): how internal runtime logs are printed

Recommended:
- Your payload: `dumps_compact(payload)` for stable one-line ingestion
- Runtime logs: `--log-format=json-pretty` during development

Example run:

```bash
vix run logging_json_server.cpp --log-level=debug --log-format=json-pretty --log-color=always
```

---

## 4) Production logging: use Logger, not console

Console is great for development.
In production you usually want:
- sinks (file, stdout, syslog, collector)
- structured fields
- consistent schema
- rotation policies

If you have `vix::utils::Logger`, use it as your main production logger.

Pattern:

```cpp
#include <vix.hpp>
#include <vix/json/dumps.hpp>
// include your logger header (depends on your module path)
#include <vix/utils/Logger.hpp>

using namespace vix;
using vix::json::Json;
using vix::json::dumps_compact;

int main()
{
  App app;

  app.get("/api/ping", [](Request& req, Response& res)
  {
    Json evt = {
      {"event", "ping"},
      {"method", req.method()},
      {"path", req.path()},
    };

    // Option A: Logger can accept a string
    vix::log().info(dumps_compact(evt));

    res.json({ "ok", true });
  });

  app.run(8080);
  return 0;
}
```

---

## 5) A minimal JSON log schema (recommended)

Keep a stable schema so dashboards and alerts are easy.

Fields that work well:
- `ts` (timestamp) if your logger does not add it automatically
- `level`
- `event`
- `method`
- `path`
- `status`
- `request_id` (if you set one)
- `latency_ms`

Example payload:

```cpp
Json evt = {
  {"level", "info"},
  {"event", "request_done"},
  {"method", req.method()},
  {"path", req.path()},
  {"status", 200},
  {"latency_ms", 3}
};
```

---

## 6) Do not log secrets

Never log these:
- Authorization header
- cookies
- tokens
- passwords
- personal data

If you must log headers, redact:

```cpp
auto ua = req.header("User-Agent");
console.debug("ua=", ua);
```

Do not print Authorization.

---

## 7) Quick test

Run:

```bash
vix run logging_json_server.cpp
```

Test:

```bash
curl -i http://localhost:8080/api/ping
```

You should see:
- a JSON log line in your terminal
- a JSON response from the endpoint

---

## Summary

- `dumps()` and `dumps_compact()` are JSON serialization helpers, not logging.
- `console.*` is like JavaScript console logging.
- For JSON logs: `console.info(dumps_compact(payload))`.
- For production: prefer `vix::utils::Logger` with structured fields.

