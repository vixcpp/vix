# Vix Config

The `vix::config::Config` class loads configuration from `.env` files and provides safe, typed accessors.

It is designed for:

- `.env` files (primary source of configuration)
- Layered environments (`.env`, `.env.local`, `.env.production`, etc.)
- Dot-path access (`server.port`, `database.default.host`, etc.)
- Defaults on missing values
- One-time load at startup

---

## Include

```cpp
#include <vix/config/Config.hpp>
```

---

## Namespace

```cpp
using vix::config::Config;
```

---

## 1. Environment file format

Vix uses `.env` files as the main configuration source.

Example `.env`:

```env
# Server
SERVER_PORT=8080
SERVER_REQUEST_TIMEOUT=2000
SERVER_IO_THREADS=0

# Database
DATABASE_ENGINE=mysql
DATABASE_DEFAULT_HOST=127.0.0.1
DATABASE_DEFAULT_PORT=3306
DATABASE_DEFAULT_USER=root
DATABASE_DEFAULT_PASSWORD=
DATABASE_DEFAULT_NAME=appdb

# Logging
LOGGING_ASYNC=true
LOGGING_QUEUE_MAX=20000

# WAF
WAF_MODE=basic
```

---

## 2. Loading configuration

Configuration is loaded explicitly at startup.

```cpp
Config cfg{".env"};
```

This loads all layered environment files.

---

## 3. Layered environment files

Vix automatically loads layered files:

- `.env`
- `.env.local`
- `.env.production`
- `.env.production.local`

This allows:

- local development overrides
- production configuration
- environment-specific behavior

---

## 4. Typed accessors

Configuration values are accessed using typed getters.

```cpp
int port = cfg.getInt("server.port", 8080);
bool async = cfg.getBool("logging.async", true);
std::string host = cfg.getString("database.default.host", "127.0.0.1");
```

Available methods:

- `getInt(key, default)`
- `getBool(key, default)`
- `getString(key, default)`
- `has(key)`

---

## 5. Environment variable mapping

Dot-path keys are mapped to environment variables.

| Key | Env variable |
|-----|-------------|
| server.port | SERVER_PORT |
| database.default.host | DATABASE_DEFAULT_HOST |
| database.default.name | DATABASE_DEFAULT_NAME |
| logging.async | LOGGING_ASYNC |

---

## 6. Convenience getters

Config exposes pre-parsed values.

### Server

```cpp
getServerPort();
getRequestTimeout();
getIOThreads();
```

### Logging

```cpp
getLogAsync();
getLogQueueMax();
```

### WAF

```cpp
getWafMode();
```

Example:

```cpp
int port = cfg.getServerPort();

if (cfg.getLogAsync())
{
  // configure logger
}
```

---

## 7. Passing configuration

Configuration must be passed explicitly.

```cpp
void register_routes(App& app, const Config& cfg)
{
  app.get("/", [&cfg](Request&, Response& res) {
    res.send("Port: " + std::to_string(cfg.getServerPort()));
  });
}
```

---

## 8. Secrets handling

Sensitive values should be stored in environment variables.

Example:

```env
DATABASE_DEFAULT_PASSWORD=secret
```

You can also use dedicated helpers:

```cpp
std::string pass = cfg.getDbPasswordFromEnv();
```

---

## 9. Behavior and defaults

- Missing `.env` file → no crash
- Missing variable → default used
- Invalid value → default used
- Configuration is loaded once at startup

---

## Best practices

- Load config once at startup
- Pass it explicitly to your application
- Do not use global state
- Keep secrets in `.env`
- Do not read config inside request handlers repeatedly

