# Configuration

The `vix::config::Config` class loads a JSON file and provides safe, typed accessors.
It is designed for:

- JSON config files
- Dot-path access (`server.port`, `database.default.host`, etc.)
- Defaults on missing or invalid values
- One-time load at startup
- Optional environment override (example: `DB_PASSWORD`)

Include:

```cpp
#include <vix/config/Config.hpp>
```

Namespace:

```cpp
using vix::config::Config;
```

---

# 1. Config file format

Vix config is JSON.
Nested objects are accessed via dot paths.

Example `config/config.json`:

```json
{
  "server": {
    "port": 8080,
    "request_timeout": 2000,
    "io_threads": 0,
    "session_timeout_sec": 300
  },
  "logging": {
    "async": true,
    "queue_max": 20000,
    "drop_on_overflow": true
  },
  "waf": {
    "mode": "basic",
    "max_target_len": 4096,
    "max_body_bytes": 1048576
  },
  "database": {
    "default": {
      "host": "localhost",
      "port": 3306,
      "name": "mydb",
      "user": "myuser",
      "password": ""
    }
  }
}
```

---

# 2. Loading config

You can build a `Config` directly.
The constructor searches common locations and loads if found.

```cpp
Config cfg{"config/config.json"};
```

If no file is found, defaults are used.

## Singleton access

The class also exposes a lazy singleton:

```cpp
auto& cfg = Config::getInstance("config/config.json");
```

This is useful when you want a global config with a single load point.

---

# 3. Typed getters with defaults

The safe accessors always take a default value.
If the key is missing or cannot be parsed, the default is returned.

```cpp
Config cfg{"config/config.json"};

int port = cfg.getInt("server.port", 8080);
int timeout_ms = cfg.getInt("server.request_timeout", 2000);

bool log_async = cfg.getBool("logging.async", true);
std::string waf_mode = cfg.getString("waf.mode", "basic");
```

Supported getters:

- `getInt(dottedKey, defaultValue)`
- `getBool(dottedKey, defaultValue)`
- `getString(dottedKey, defaultValue)`
- `has(dottedKey)`

---

# 4. Dot-path lookup

Dot paths traverse nested JSON objects.

Examples:

- `server.port`
- `logging.queue_max`
- `database.default.host`
- `waf.max_body_bytes`

```cpp
if (cfg.has("database.default.host"))
{
  auto host = cfg.getString("database.default.host", "localhost");
}
```

---

# 5. Convenience getters (server, logging, waf)

`Config` also stores parsed values and exposes dedicated getters.
This avoids repeating dot paths in your runtime code.

Server:

- `getServerPort()`
- `getRequestTimeout()`
- `getIOThreads()`
- `getSessionTimeoutSec()`
- `setServerPort(port)`

Logging:

- `getLogAsync()`
- `getLogQueueMax()`
- `getLogDropOnOverflow()`

WAF:

- `getWafMode()`
- `getWafMaxTargetLen()`
- `getWafMaxBodyBytes()`

Minimal usage:

```cpp
auto& cfg = Config::getInstance("config/config.json");

int port = cfg.getServerPort();
int io_threads = cfg.getIOThreads();

if (cfg.getLogAsync())
{
  // configure async logger
}
```

---

# 6. Environment override (DB password)

Database password can be provided via environment variable:

- `DB_PASSWORD`

`getDbPasswordFromEnv()` returns:

- `DB_PASSWORD` if set
- otherwise the config value

Example pattern:

```cpp
auto& cfg = Config::getInstance("config/config.json");

std::string db_pass = cfg.getDbPasswordFromEnv();
```

This keeps secrets out of files.

---

# 7. Defaults and validation behavior

Important behavior from the implementation:

- Missing file: uses defaults (no crash)
- Missing key: returns your default
- Wrong type: best-effort parse (example: string to int via `stoi`), otherwise default
- `setServerPort(port)` validates range (0 or 1024..65535)

---

# Best practices

- Load config once at startup.
- Always pass defaults to `getInt/getBool/getString`.
- Keep secrets in environment variables (example: `DB_PASSWORD`).
- Validate critical settings at boot (ports, timeouts, limits).
- Avoid reading config per request.

Configuration should be stable and deterministic after startup.


