# Configuration

This page shows how configuration works in Vix Core.

Use it when you want to configure the server port, request timeout, I/O threads, session timeout, WAF, TLS, logging, benchmark mode, or database settings.

## Public header

```cpp
#include <vix.hpp>
```

You can also include the configuration header directly:

```cpp
#include <vix/config/Config.hpp>
```

## What configuration provides

`vix::config::Config` is the configuration object used by Vix Core.

It provides typed access to:

- server port
- request timeout
- I/O thread count
- session timeout
- benchmark mode
- logging options
- WAF options
- TLS options
- database options
- environment-based configuration
- dotted key access
- runtime overrides

Most applications can start with the default configuration.

```cpp
vix::App app;

app.run(8080);
```

For advanced setup, create a config object and pass it to the app.

```cpp
vix::config::Config cfg;

cfg.setServerPort(8080);

vix::App app;
app.run(cfg);
```

## Basic configuration

```cpp
#include <vix.hpp>

int main()
{
  vix::config::Config cfg;

  cfg.setServerPort(8080);

  vix::App app;

  app.get("/", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.text("configured app");
  });

  app.run(cfg);

  return 0;
}
```

Run:

```bash
vix run main.cpp
```

Then open:

```text
http://localhost:8080
```

## Default configuration

A default app owns a default `Config`.

```cpp
vix::App app;
```

You can access it with:

```cpp
app.config();
```

Example:

```cpp
app.config().setServerPort(8080);
```

Then run:

```cpp
app.run(app.config());
```

Or simply pass the port to `run`.

```cpp
app.run(8080);
```

## Construct a Config

Create a config object with the default path.

```cpp
vix::config::Config cfg;
```

You can also provide a config path.

```cpp
vix::config::Config cfg{".env.production"};
```

When a path is provided, Vix can use it to determine layered environment loading.

## Environment loading

`Config` loads environment values using the Vix env module.

The default config path is:

```text
.env
```

When an environment-specific file is used, Vix can load layered files.

Example:

```cpp
vix::config::Config cfg{".env.production"};
```

This can load environment-specific values for production.

## Server port

Use `setServerPort(...)` to set the server port.

```cpp
vix::config::Config cfg;

cfg.setServerPort(8080);
```

Read the port with:

```cpp
const int port = cfg.getServerPort();
```

Run with the config:

```cpp
vix::App app;
app.run(cfg);
```

## Port from App

You can also pass the port directly.

```cpp
vix::App app;

app.run(8080);
```

This is the simplest option for local development.

## Port 0

Port `0` means the operating system chooses an available port.

```cpp
vix::App app;

app.listen_port(0, [](int port)
{
  vix::print("listening on", port);
});

app.wait();
```

This is useful for tests.

## Request timeout

Use `getRequestTimeout()` to read the configured request timeout in milliseconds.

```cpp
const int timeout_ms = cfg.getRequestTimeout();
```

This value can be used by Core and higher-level systems to control request behavior.

## I/O threads

Use `getIOThreads()` to read the configured I/O thread count.

```cpp
const int io_threads = cfg.getIOThreads();
```

A value of `0` means automatic selection.

```text
0 -> auto
```

The HTTP server can use hardware concurrency when auto mode is selected.

## Session timeout

Use `getSessionTimeoutSec()` to read the session timeout in seconds.

```cpp
const int timeout_sec = cfg.getSessionTimeoutSec();
```

Sessions use this value to protect connections from staying active forever.

Conceptually:

```text
session work starts
  -> timer starts
  -> timeout fires
  -> connection closes
```

## Benchmark mode

Use `isBenchMode()` to check whether benchmark mode is enabled.

```cpp
if (cfg.isBenchMode())
{
  vix::print("bench mode enabled");
}
```

Benchmark mode can reduce some runtime overhead for measuring HTTP performance.

Normal applications usually keep benchmark mode disabled.

## Logging configuration

The config exposes logging-related values.

```cpp
cfg.getLogAsync();
cfg.getLogQueueMax();
cfg.getLogDropOnOverflow();
```

These control:

| API                      | Purpose                                             |
| ------------------------ | --------------------------------------------------- |
| `getLogAsync()`          | Whether async logging is enabled.                   |
| `getLogQueueMax()`       | Maximum async log queue size.                       |
| `getLogDropOnOverflow()` | Whether logs may be dropped when the queue is full. |

The app also reads logging behavior from environment variables such as `VIX_LOG_LEVEL` and `VIX_LOG_FORMAT`.

## WAF configuration

Vix Core includes basic request filtering options.

```cpp
cfg.getWafMode();
cfg.getWafMaxTargetLen();
cfg.getWafMaxBodyBytes();
```

These control request checks in the session layer.

| API                    | Purpose                                  |
| ---------------------- | ---------------------------------------- |
| `getWafMode()`         | WAF mode: `off`, `basic`, or `strict`.   |
| `getWafMaxTargetLen()` | Maximum allowed target length.           |
| `getWafMaxBodyBytes()` | Maximum body bytes checked by WAF logic. |

## WAF modes

The WAF mode can be:

```text
off
basic
strict
```

Use `off` when you want to disable WAF checks.

Use `basic` for lightweight checks.

Use `strict` when you want stronger checks.

The exact behavior depends on the session request validation path.

## TLS configuration

TLS settings are exposed through `Config`.

```cpp
cfg.isTlsEnabled();
cfg.getTlsCertFile();
cfg.getTlsKeyFile();
cfg.getTlsConfig();
```

These values are used by `HTTPServer` and `TlsSession`.

## Enable TLS

Use config keys to enable TLS.

```cpp
vix::config::Config cfg;

cfg.setServerPort(8443);
cfg.set("server.tls.enabled", true);
cfg.set("server.tls.cert_file", "/etc/letsencrypt/live/example.com/fullchain.pem");
cfg.set("server.tls.key_file", "/etc/letsencrypt/live/example.com/privkey.pem");
```

Then run:

```cpp
vix::App app;
app.run(cfg);
```

## Get TLS config

Use `getTlsConfig()` to get a complete TLS configuration object.

```cpp
const auto tls = cfg.getTlsConfig();

if (tls.is_valid())
{
  vix::print("TLS is configured");
}
```

The returned type is:

```cpp
vix::server::TlsConfig
```

It contains:

```cpp
bool enabled;
std::string cert_file;
std::string key_file;
```

## Plain HTTP configuration

For plain HTTP, keep TLS disabled.

```cpp
vix::config::Config cfg;

cfg.setServerPort(8080);
cfg.set("server.tls.enabled", false);
```

Then run:

```cpp
app.run(cfg);
```

The server accepts plain HTTP connections.

```text
http://localhost:8080
```

## HTTPS configuration

For HTTPS, enable TLS and provide certificate paths.

```cpp
vix::config::Config cfg;

cfg.setServerPort(8443);
cfg.set("server.tls.enabled", true);
cfg.set("server.tls.cert_file", "/etc/letsencrypt/live/example.com/fullchain.pem");
cfg.set("server.tls.key_file", "/etc/letsencrypt/live/example.com/privkey.pem");
```

Then run:

```cpp
app.run(cfg);
```

The server accepts HTTPS connections.

```text
https://localhost:8443
```

## Dotted keys

Use `set(...)` with dotted keys to override configuration values.

```cpp
cfg.set("server.port", 8080);
cfg.set("server.tls.enabled", true);
cfg.set("server.tls.cert_file", "/path/fullchain.pem");
cfg.set("server.tls.key_file", "/path/privkey.pem");
```

Dotted keys are useful for structured configuration.

## Check a key

Use `has(...)`.

```cpp
if (cfg.has("server.port"))
{
  vix::print("server port is set");
}
```

## Get an integer value

Use `getInt(...)`.

```cpp
const int port = cfg.getInt("server.port", 8080);
```

The second argument is the fallback value.

## Get a boolean value

Use `getBool(...)`.

```cpp
const bool tls_enabled = cfg.getBool("server.tls.enabled", false);
```

## Get a string value

Use `getString(...)`.

```cpp
const std::string cert =
    cfg.getString("server.tls.cert_file", "");
```

## Database configuration

`Config` exposes database-related accessors.

```cpp
cfg.getDbHost();
cfg.getDbUser();
cfg.getDbName();
cfg.getDbPort();
cfg.getDbPasswordFromEnv();
```

These values are useful for modules or applications that need database configuration.

## Database password

Database passwords are read from environment variables.

The config checks common names such as:

```text
VIX_DB_PASSWORD
DATABASE_DEFAULT_PASSWORD
DB_PASSWORD
MYSQL_PASSWORD
```

If none is set, it falls back to the default value.

## Environment variables

Common environment variables include:

```text
SERVER_PORT=8080
SERVER_REQUEST_TIMEOUT=2000
SERVER_IO_THREADS=0
SERVER_SESSION_TIMEOUT_SEC=30
SERVER_BENCH_MODE=false

SERVER_TLS_ENABLED=false
SERVER_TLS_CERT_FILE=
SERVER_TLS_KEY_FILE=

LOGGING_ASYNC=true
LOGGING_QUEUE_MAX=20000
LOGGING_DROP_ON_OVERFLOW=true

WAF_MODE=basic
WAF_MAX_TARGET_LEN=4096
WAF_MAX_BODY_BYTES=1048576
```

Database-related variables can include:

```text
DATABASE_DEFAULT_HOST=localhost
DATABASE_DEFAULT_USER=root
DATABASE_DEFAULT_NAME=
DATABASE_DEFAULT_PORT=3306
DATABASE_DEFAULT_PASSWORD=
```

## Example .env

```dotenv
SERVER_PORT=8080
SERVER_REQUEST_TIMEOUT=2000
SERVER_IO_THREADS=0
SERVER_SESSION_TIMEOUT_SEC=30
SERVER_BENCH_MODE=false

WAF_MODE=basic
WAF_MAX_TARGET_LEN=4096
WAF_MAX_BODY_BYTES=1048576

SERVER_TLS_ENABLED=false
SERVER_TLS_CERT_FILE=
SERVER_TLS_KEY_FILE=

LOGGING_ASYNC=true
LOGGING_QUEUE_MAX=20000
LOGGING_DROP_ON_OVERFLOW=true
```

## Example .env.production

```dotenv
SERVER_PORT=8443
SERVER_REQUEST_TIMEOUT=5000
SERVER_IO_THREADS=0
SERVER_SESSION_TIMEOUT_SEC=60
SERVER_BENCH_MODE=false

WAF_MODE=strict
WAF_MAX_TARGET_LEN=4096
WAF_MAX_BODY_BYTES=1048576

SERVER_TLS_ENABLED=true
SERVER_TLS_CERT_FILE=/etc/letsencrypt/live/example.com/fullchain.pem
SERVER_TLS_KEY_FILE=/etc/letsencrypt/live/example.com/privkey.pem

LOGGING_ASYNC=true
LOGGING_QUEUE_MAX=50000
LOGGING_DROP_ON_OVERFLOW=true
```

Load it with:

```cpp
vix::config::Config cfg{".env.production"};
```

## App config access

The app owns a mutable config.

```cpp
vix::App app;

auto &cfg = app.config();
```

You can update it before starting the server.

```cpp
app.config().setServerPort(8080);

app.run(app.config());
```

## Run with explicit config

```cpp
#include <vix.hpp>

int main()
{
  vix::config::Config cfg;

  cfg.setServerPort(8080);
  cfg.set("server.tls.enabled", false);

  vix::App app;

  app.get("/", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.text("hello");
  });

  app.run(cfg);

  return 0;
}
```

## Listen with explicit config

```cpp
#include <vix.hpp>

int main()
{
  vix::config::Config cfg;

  cfg.setServerPort(8080);

  vix::App app;

  app.get("/", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.text("hello");
  });

  app.listen(cfg);

  app.wait();

  return 0;
}
```

## Configuration and HTTPServer

The HTTP server receives the config from the app.

```text
App
  -> Config
  -> HTTPServer
```

The server uses config for:

- port
- TLS
- benchmark mode
- session behavior
- WAF behavior

## Configuration and Session

Sessions also use config.

```text
Session
  -> Config
```

Session behavior can depend on:

- session timeout
- benchmark mode
- WAF mode
- WAF limits
- TLS behavior through transport selection

## Configuration and TLS

The HTTP server checks TLS configuration when handling a client.

```text
HTTPServer
  -> Config::getTlsConfig
  -> TlsConfig::is_valid
  -> Session or TlsSession
```

If TLS is valid, the server creates a TLS session.

If TLS is disabled, the server creates a normal session.

## Configuration and logging

The app initializes logging during startup.

Environment variables can control logging behavior.

Common examples:

```text
VIX_LOG_LEVEL=debug
VIX_LOG_FORMAT=json
VIX_LOG_ASYNC=true
VIX_INTERNAL_LOGS=false
VIX_ACCESS_LOGS=true
```

These are useful for development and production diagnostics.

## Configuration and benchmark mode

Benchmark mode can be enabled through environment configuration.

```dotenv
SERVER_BENCH_MODE=true
```

or through a config key:

```cpp
cfg.set("server.bench_mode", true);
```

Benchmark mode is useful for performance tests.

It should normally be disabled for regular applications.

## Configuration and WAF

WAF behavior can be configured through environment values.

```dotenv
WAF_MODE=basic
WAF_MAX_TARGET_LEN=4096
WAF_MAX_BODY_BYTES=1048576
```

In the session layer, these values can block malformed or suspicious requests before they reach user handlers.

## Complete plain HTTP example

```cpp
#include <vix.hpp>

int main()
{
  vix::config::Config cfg;

  cfg.setServerPort(8080);
  cfg.set("server.tls.enabled", false);
  cfg.set("server.session_timeout_sec", 30);
  cfg.set("waf.mode", "basic");

  vix::App app;

  app.get("/", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.text("plain HTTP app");
  });

  app.get("/api/status", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.json({
      "status", "ok",
      "tls", false
    });
  });

  app.run(cfg);

  return 0;
}
```

## Complete HTTPS example

```cpp
#include <vix.hpp>

int main()
{
  vix::config::Config cfg{".env.production"};

  cfg.setServerPort(8443);
  cfg.set("server.tls.enabled", true);
  cfg.set("server.tls.cert_file", "/etc/letsencrypt/live/example.com/fullchain.pem");
  cfg.set("server.tls.key_file", "/etc/letsencrypt/live/example.com/privkey.pem");

  vix::App app;

  app.get("/", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.text("HTTPS app");
  });

  app.get("/api/status", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.json({
      "status", "ok",
      "tls", true
    });
  });

  app.run(cfg);

  return 0;
}
```

## API summary

| API                        | Purpose                                        |
| -------------------------- | ---------------------------------------------- |
| `Config(path)`             | Construct config with an optional config path. |
| `loadConfig()`             | Reload configuration from environment files.   |
| `set(key, value)`          | Set a dotted configuration value.              |
| `has(key)`                 | Check whether a dotted key exists.             |
| `getInt(key, fallback)`    | Read an integer config value.                  |
| `getBool(key, fallback)`   | Read a boolean config value.                   |
| `getString(key, fallback)` | Read a string config value.                    |
| `setServerPort(port)`      | Set the HTTP server port.                      |
| `getServerPort()`          | Return the HTTP server port.                   |
| `getRequestTimeout()`      | Return request timeout in milliseconds.        |
| `getIOThreads()`           | Return configured I/O threads.                 |
| `isBenchMode()`            | Return whether benchmark mode is enabled.      |
| `getSessionTimeoutSec()`   | Return session timeout in seconds.             |
| `getWafMode()`             | Return WAF mode.                               |
| `getWafMaxTargetLen()`     | Return maximum WAF target length.              |
| `getWafMaxBodyBytes()`     | Return maximum WAF body bytes.                 |
| `isTlsEnabled()`           | Return whether TLS is enabled.                 |
| `getTlsCertFile()`         | Return TLS certificate path.                   |
| `getTlsKeyFile()`          | Return TLS private key path.                   |
| `getTlsConfig()`           | Return complete TLS configuration.             |
| `getLogAsync()`            | Return async logging setting.                  |
| `getLogQueueMax()`         | Return async log queue max size.               |
| `getLogDropOnOverflow()`   | Return log overflow behavior.                  |
| `getDbHost()`              | Return database host.                          |
| `getDbUser()`              | Return database user.                          |
| `getDbName()`              | Return database name.                          |
| `getDbPort()`              | Return database port.                          |
| `getDbPasswordFromEnv()`   | Return database password from environment.     |

## Best practices

Use direct `app.run(port)` for simple local development.

```cpp
app.run(8080);
```

Use `Config` when you need TLS, WAF, timeouts, or production settings.

```cpp
vix::config::Config cfg{".env.production"};
app.run(cfg);
```

Keep secrets in environment variables, not source code.

```text
DATABASE_DEFAULT_PASSWORD=...
```

Use TLS through a reverse proxy when certificate management should stay outside the app.

```text
Nginx or Caddy -> Vix
```

Use direct TLS when the Vix process should terminate HTTPS itself.

```cpp
cfg.set("server.tls.enabled", true);
```

Use benchmark mode only for performance testing.

```dotenv
SERVER_BENCH_MODE=true
```

## Next steps

Read the next pages:

- [TLS](./tls.md)
- [HTTP server](./http-server.md)
- [Sessions](./sessions.md)
- [Async and runtime](./async-and-runtime.md)
- [API Reference](./api-reference.md)
