# Config API

The Config API loads application settings from environment files and exposes typed accessors.

```txt
.env → config::Config → App / Database / WebSocket / runtime
```

## Public header

```cpp
#include <vix.hpp>        // for normal applications
#include <vix/Config.hpp>    // for config-only code
```

Main type: `vix::config::Config`.

## Basic example

```cpp
vix::config::Config cfg{".env"};  // or Config cfg; (defaults to .env)

app.run(cfg);  // start HTTP server using config
```

## Environment files

```cpp
vix::config::Config cfg{".env.production"};
```

Typical files: `.env`, `.env.local`, `.env.production`.

## Complete .env reference

```dotenv
# Server
SERVER_PORT=8080
SERVER_REQUEST_TIMEOUT=2000
SERVER_IO_THREADS=0
SERVER_SESSION_TIMEOUT_SEC=20
SERVER_BENCH_MODE=false

# TLS
SERVER_TLS_ENABLED=false
SERVER_TLS_CERT_FILE=
SERVER_TLS_KEY_FILE=

# Database
DATABASE_ENGINE=sqlite
DATABASE_SQLITE_PATH=vix.db
DATABASE_DEFAULT_HOST=127.0.0.1
DATABASE_DEFAULT_PORT=3306
DATABASE_DEFAULT_USER=root
DATABASE_DEFAULT_PASSWORD=
DATABASE_DEFAULT_NAME=vix

# Logging
LOGGING_ASYNC=true
LOGGING_QUEUE_MAX=20000
LOGGING_DROP_ON_OVERFLOW=true

# WAF
WAF_MODE=basic
WAF_MAX_TARGET_LEN=4096
WAF_MAX_BODY_BYTES=1048576

# WebSocket
WEBSOCKET_MAX_MESSAGE_SIZE=65536
WEBSOCKET_IDLE_TIMEOUT=60
WEBSOCKET_ENABLE_DEFLATE=true
WEBSOCKET_PING_INTERVAL=30
WEBSOCKET_AUTO_PING_PONG=true
```

## Typed accessors

### Server

```cpp
cfg.getServerPort()           // SERVER_PORT
cfg.getRequestTimeout()       // SERVER_REQUEST_TIMEOUT
cfg.getIOThreads()            // SERVER_IO_THREADS
cfg.getSessionTimeoutSec()    // SERVER_SESSION_TIMEOUT_SEC
cfg.isBenchMode()             // SERVER_BENCH_MODE
cfg.setServerPort(9000);      // override in code
```

### TLS

```cpp
cfg.isTlsEnabled()    // SERVER_TLS_ENABLED
cfg.getTlsCertFile()  // SERVER_TLS_CERT_FILE
cfg.getTlsKeyFile()   // SERVER_TLS_KEY_FILE
cfg.getTlsConfig()    // full TLS config object
```

> When Nginx handles HTTPS, keep `SERVER_TLS_ENABLED=false`. Nginx terminates TLS, Vix stays plain HTTP locally.

### Database

```cpp
cfg.getDbHost()  // DATABASE_DEFAULT_HOST
cfg.getDbUser()  // DATABASE_DEFAULT_USER
cfg.getDbName()  // DATABASE_DEFAULT_NAME
cfg.getDbPort()  // DATABASE_DEFAULT_PORT
```

Password priority: `VIX_DB_PASSWORD` → `DATABASE_DEFAULT_PASSWORD` → `DB_PASSWORD` → `MYSQL_PASSWORD`.

### Logging

```cpp
cfg.getLogAsync()           // LOGGING_ASYNC
cfg.getLogQueueMax()        // LOGGING_QUEUE_MAX
cfg.getLogDropOnOverflow()  // LOGGING_DROP_ON_OVERFLOW
```

### WAF

```cpp
cfg.getWafMode()          // WAF_MODE: off | basic | strict
cfg.getWafMaxTargetLen()  // WAF_MAX_TARGET_LEN
cfg.getWafMaxBodyBytes()  // WAF_MAX_BODY_BYTES
```

## Generic accessors

Dotted keys map to uppercase env names:

```cpp
cfg.getString("database.engine", "sqlite");       // DATABASE_ENGINE
cfg.getString("database.default.host", "127.0.0.1"); // DATABASE_DEFAULT_HOST
cfg.getInt("database.default.port", 3306);        // DATABASE_DEFAULT_PORT
cfg.getBool("server.bench_mode", false);          // SERVER_BENCH_MODE
cfg.has("database.default.host");                 // check existence
cfg.set("server.port", 9090);                     // in-memory override
```

## Config with database

```cpp
vix::config::Config cfg{".env"};
vix::db::Database db{cfg};  // creates database from config
```

Build flags still required: `vix run main.cpp --with-sqlite` or `--with-mysql`.

## Config with WebSocket

```cpp
vix::config::Config core_cfg{".env"};
vix::websocket::Config ws_cfg = vix::websocket::Config::from_core(core_cfg);

std::cout << ws_cfg.maxMessageSize << "\n";         // WEBSOCKET_MAX_MESSAGE_SIZE
std::cout << ws_cfg.idleTimeout.count() << "\n";    // WEBSOCKET_IDLE_TIMEOUT
std::cout << ws_cfg.pingInterval.count() << "\n";   // WEBSOCKET_PING_INTERVAL
```

## Config with HTTP + WebSocket

```cpp
vix::config::Config cfg{".env"};
auto executor = std::make_shared<vix::executor::RuntimeExecutor>(1u);
vix::App app{executor};
vix::websocket::Server ws{cfg, executor};
vix::run_http_and_ws(app, ws, executor, cfg);
```

## Production recommendations

```dotenv
SERVER_PORT=8080
SERVER_TLS_ENABLED=false    # let Nginx handle TLS
WAF_MODE=basic
LOGGING_ASYNC=true
LOGGING_DROP_ON_OVERFLOW=true
```

For direct TLS (without Nginx):

```dotenv
SERVER_TLS_ENABLED=true
SERVER_TLS_CERT_FILE=/etc/letsencrypt/live/example.com/fullchain.pem
SERVER_TLS_KEY_FILE=/etc/letsencrypt/live/example.com/privkey.pem
```

## Recommended .env.example

```dotenv
DATABASE_ENGINE=sqlite
DATABASE_SQLITE_PATH=vix.db
DATABASE_DEFAULT_HOST=127.0.0.1
DATABASE_DEFAULT_PORT=3306
DATABASE_DEFAULT_USER=root
DATABASE_DEFAULT_PASSWORD=
DATABASE_DEFAULT_NAME=vix
SERVER_PORT=8080
SERVER_REQUEST_TIMEOUT=2000
SERVER_IO_THREADS=0
SERVER_TLS_ENABLED=false
SERVER_TLS_CERT_FILE=
SERVER_TLS_KEY_FILE=
LOGGING_ASYNC=true
LOGGING_QUEUE_MAX=20000
LOGGING_DROP_ON_OVERFLOW=true
WAF_MODE=basic
WAF_MAX_TARGET_LEN=4096
WAF_MAX_BODY_BYTES=1048576
WEBSOCKET_MAX_MESSAGE_SIZE=65536
WEBSOCKET_IDLE_TIMEOUT=60
WEBSOCKET_ENABLE_DEFLATE=true
WEBSOCKET_PING_INTERVAL=30
WEBSOCKET_AUTO_PING_PONG=true
```

Commit `.env.example` (safe defaults, no secrets). Never commit `.env` with real credentials.

## Common mistakes

### Forgetting database build flags

```bash
vix run main.cpp --with-sqlite   # SQLite
vix run main.cpp --with-mysql    # MySQL
```

### Using the wrong key style

```cpp
cfg.getInt("server.port", 8080);  // dotted — for generic getters
// SERVER_PORT=8080               // uppercase — in .env file
```

### Expecting Nginx TLS and Vix TLS simultaneously

If Nginx handles HTTPS, always keep `SERVER_TLS_ENABLED=false`.

## What you should remember

```cpp
vix::config::Config cfg{".env"};
app.run(cfg);

cfg.getServerPort();
cfg.isTlsEnabled();
cfg.getDbHost();
cfg.getLogAsync();
cfg.getWafMode();

cfg.getString("database.engine", "sqlite");
cfg.getInt("database.default.port", 3306);
```

The core idea: configuration should live outside your C++ source code.

Next: [WebSocket API](/api/websocket)
