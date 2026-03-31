# Logging (Core)

Logging in Vix is **explicit and layered**.

Vix does not automatically log every request. You decide:

- What to log
- When to log
- Whether it is **dev output** (`vix::console`)
- Or **production logging** (`vix::utils::Logger`)

Vix intentionally separates:

- **Console** → developer-facing runtime output
- **Logger** → structured, production-grade logging

---

## 1. Development logging (vix::console)

For quick debugging and local feedback, use:

```cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request&, Response& res) {
    console.info("Incoming request on /");
    res.json({"message", "Hello"});
  });

  app.run(8080);
}
```

### Why use console instead of std::cout?

- Thread-safe atomic lines
- Level filtering (`debug` off by default)
- Proper stdout/stderr routing
- Near-zero cost when filtered
- Environment-controlled verbosity

Console is designed for **developer ergonomics**, not ingestion pipelines.

---

## 2. Logging request data

```cpp
app.get("/inspect", [](Request& req, Response& res) {

  console.debug("Path:", "/inspect");
  console.debug("User-Agent:", req.header("User-Agent"));

  res.json({"ok", true});
});
```

Be careful not to log:

- Authorization headers
- Tokens
- Passwords
- Personal data

---

## 3. Logging errors (dev mode)

```cpp
app.get("/error", [](Request&, Response& res) {

  console.error("Simulated error occurred");

  res.status(500).json({
    "error", "Internal server error"
  });
});
```

Console automatically routes:

- `warn` / `error` → stderr
- `log` / `info` / `debug` → stdout

---

## 4. Production logging (vix::utils::Logger)

For real systems, do **not** rely on console.

Use `vix::utils::Logger` for:

- Structured logs (JSON / key-value)
- File outputs
- Ingestion pipelines
- Production observability
- Rotating logs
- Persistent storage

Example (illustrative):

```cpp
#include <vix/utils/logger.hpp>

using namespace vix::utils;

int main()
{
  Logger logger;

  logger.info({
    {"event", "request_received"},
    {"path", "/inspect"}
  });
}
```

Console and Logger serve different purposes by design.

---

## 5. Structured logging pattern

Instead of:

```cpp
console.info("User login failed");
```

Prefer structured logging in production:

```cpp
logger.warn({
  {"event", "login_failed"},
  {"user", "unknown"}
});
```

Structured logs are easier to:

- Parse
- Filter
- Aggregate
- Send to ELK / Loki / OpenTelemetry

---

## 6. Performance considerations

Logging is I/O.

Best practices:

- Do not log inside hot loops
- Avoid per-request heavy logs under high load
- Keep debug logs disabled in production
- Use sampling if traffic is extreme
- Avoid blocking file I/O in request threads

Remember:

- Console is cheap when filtered
- Logger is more powerful but must be configured responsibly

---

## 7. Philosophy

Logging in Vix is:

- Explicit
- Layered
- Predictable
- Performance-aware

Nothing is logged unless you log it.

Console is for developers.
Logger is for systems.

