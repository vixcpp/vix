# Database configuration

Vix applications can configure database access in two practical ways.
For small examples, opening the database directly in C++ is enough.

```cpp
auto db = vix::db::Database::sqlite("storage/app.db");
```

For real applications, configuration should live outside the source code. Vix supports project configuration through `vix.json`, and runtime environment configuration through `.env` files loaded by `vix::config::Config`.

The two approaches serve different needs.

`vix.json` describes the project. It is useful for CLI tools, migrations, project metadata, and shared configuration that can be committed safely.

`.env` describes the runtime environment. It is useful for ports, local database paths, MySQL credentials, TLS paths, logging options, and deployment-specific values.

A serious Vix project can use both.

## The two configuration layers

A database-backed Vix project usually has:

```txt
vix.json
.env.example
.env
```

`vix.json` is committed.

`.env.example` is committed.

`.env` is local and should not be committed when it contains secrets.

A clean project layout can look like this:

```txt
my-app/
├── vix.json
├── .env.example
├── .env
├── src/
│   └── main.cpp
├── storage/
│   └── .gitkeep
└── migrations/
    ├── 2026_06_02_120000_create_users.up.sql
    └── 2026_06_02_120000_create_users.down.sql
```

This gives the project a clear split.

Project shape belongs in `vix.json`.

Runtime values belong in `.env`.

## When to use direct C++ configuration

Direct C++ configuration is useful for examples, tests, and small tools.

```cpp
auto db = vix::db::Database::sqlite("storage/app.db");
```

or:

```cpp
auto db = vix::db::Database::mysql(
    "tcp://127.0.0.1:3306",
    "root",
    "secret",
    "app_db"
);
```

This is easy to read, but it does not scale well once the application has several environments.

If the database path, credentials, port, TLS files, or logging settings change between local development and production, move those values into configuration.

## Project database configuration in vix.json

`vix.json` is the right place to describe the database shape used by Vix project tooling.

A SQLite project can use:

```json
{
  "name": "Blog",
  "database": {
    "engine": "sqlite",
    "sqlite": {
      "path": "storage/Blog.db"
    },
    "storage": "storage",
    "migrations": "migrations"
  }
}
```

A MySQL project can use:

```json
{
  "name": "Blog",
  "database": {
    "engine": "mysql",
    "mysql": {
      "host": "127.0.0.1",
      "port": 3306,
      "user": "root",
      "password": "secret",
      "database": "blog"
    },
    "migrations": "migrations"
  }
}
```

This format is useful for Vix database tooling because the CLI can inspect the configured engine, database path, storage directory, and migration directory.

Use `vix.json` for configuration that describes the project and should be visible to other developers.

Do not commit production passwords in `vix.json`.

## Runtime configuration with .env

`vix::config::Config` loads runtime configuration from environment files.

By default, it looks for:

```txt
.env
```

If a specific environment file is used, Vix can load layered files such as:

```txt
.env
.env.local
.env.production
.env.production.local
```

This is useful because local development and production usually need different runtime values.

The typical flow is:

```bash
cp .env.example .env
```

Then edit `.env` for the local machine.

## Recommended .env.example

Use this as the recommended `.env.example` for Vix database examples and backend applications.

```dotenv
# =========================================================
# Vix example environment configuration
# =========================================================
#
# Copy this file to:
#
#   .env
#
# Then adjust the values for your local machine.
#
# Vix loads environment configuration through vix::config::Config.
# By default, Config looks for ".env" in the project directory.
#
# Layered environment files are supported:
#
#   .env
#   .env.local
#   .env.production
#   .env.production.local
#
# Do not commit real secrets in .env files.
# Keep this file safe to share as an example.
#
# =========================================================


# =========================================================
# Database
# =========================================================
#
# Supported engines:
#
#   sqlite
#   mysql
#
# SQLite is the default because it works without a database server.
# MySQL should be used when the app connects to a MySQL-compatible server.
#
DATABASE_ENGINE=sqlite


# =========================================================
# SQLite
# =========================================================
#
# Used when:
#
#   DATABASE_ENGINE=sqlite
#
# Keep SQLite databases under storage/ for real projects.
#
DATABASE_SQLITE_PATH=storage/vix.db


# =========================================================
# MySQL
# =========================================================
#
# Used when:
#
#   DATABASE_ENGINE=mysql
#
# vix::config::Config reads these values through the database getters:
#
#   getDbHost()
#   getDbPort()
#   getDbUser()
#   getDbName()
#   getDbPasswordFromEnv()
#
# Password lookup order:
#
#   VIX_DB_PASSWORD
#   DATABASE_DEFAULT_PASSWORD
#   DB_PASSWORD
#   MYSQL_PASSWORD
#
# Prefer VIX_DB_PASSWORD for real deployments.
#
DATABASE_DEFAULT_HOST=127.0.0.1
DATABASE_DEFAULT_PORT=3306
DATABASE_DEFAULT_USER=root
DATABASE_DEFAULT_PASSWORD=
DATABASE_DEFAULT_NAME=vix


# =========================================================
# Server
# =========================================================
#
# SERVER_IO_THREADS=0 lets Vix choose a reasonable default.
#
# SERVER_REQUEST_TIMEOUT is expressed in milliseconds.
# SERVER_SESSION_TIMEOUT_SEC is expressed in seconds.
#
SERVER_PORT=8080
SERVER_REQUEST_TIMEOUT=2000
SERVER_IO_THREADS=0
SERVER_SESSION_TIMEOUT_SEC=20
SERVER_BENCH_MODE=false


# =========================================================
# TLS / HTTPS
# =========================================================
#
# TLS is disabled by default.
#
# Enable this only when the Vix HTTP server terminates HTTPS directly.
#
# If TLS is already handled by Nginx, Caddy, Traefik, a load balancer,
# or another reverse proxy, keep SERVER_TLS_ENABLED=false here.
#
# When SERVER_TLS_ENABLED=true, both files must be set:
#
#   SERVER_TLS_CERT_FILE
#   SERVER_TLS_KEY_FILE
#
SERVER_TLS_ENABLED=false
SERVER_TLS_CERT_FILE=
SERVER_TLS_KEY_FILE=

# Let's Encrypt example:
#
# SERVER_TLS_ENABLED=true
# SERVER_TLS_CERT_FILE=/etc/letsencrypt/live/example.com/fullchain.pem
# SERVER_TLS_KEY_FILE=/etc/letsencrypt/live/example.com/privkey.pem

# Local development certificate example:
#
# SERVER_TLS_ENABLED=true
# SERVER_TLS_CERT_FILE=certs/local.crt
# SERVER_TLS_KEY_FILE=certs/local.key


# =========================================================
# Logging
# =========================================================
#
# Async logging is enabled by default.
#
# LOGGING_QUEUE_MAX controls the maximum async log queue size.
# LOGGING_DROP_ON_OVERFLOW=true means Vix can drop logs when the queue is full
# instead of blocking the application.
#
LOGGING_ASYNC=true
LOGGING_QUEUE_MAX=20000
LOGGING_DROP_ON_OVERFLOW=true


# =========================================================
# WAF
# =========================================================
#
# Modes:
#
#   off
#   basic
#   strict
#
# basic is the default recommended mode for normal development.
#
WAF_MODE=basic
WAF_MAX_TARGET_LEN=4096
WAF_MAX_BODY_BYTES=1048576


# =========================================================
# WebSocket
# =========================================================
#
# These values are used by WebSocket-enabled examples and apps.
#
# WEBSOCKET_MAX_MESSAGE_SIZE is expressed in bytes.
# WEBSOCKET_IDLE_TIMEOUT is expressed in seconds.
# WEBSOCKET_PING_INTERVAL is expressed in seconds.
#
WEBSOCKET_MAX_MESSAGE_SIZE=65536
WEBSOCKET_IDLE_TIMEOUT=60
WEBSOCKET_ENABLE_DEFLATE=true
WEBSOCKET_PING_INTERVAL=30
WEBSOCKET_AUTO_PING_PONG=true


# =========================================================
# Production hints
# =========================================================
#
# For production, prefer environment-specific files or deployment secrets:
#
#   .env.production
#   .env.production.local
#   systemd Environment=
#   container secrets
#   CI/CD secrets
#
# Do not commit real production passwords.
#
# Recommended production password variable:
#
#   VIX_DB_PASSWORD
#
VIX_DB_PASSWORD=


# =========================================================
# MySQL example
# =========================================================
#
# Uncomment this block to use MySQL instead of SQLite.
#
# DATABASE_ENGINE=mysql
# DATABASE_DEFAULT_HOST=127.0.0.1
# DATABASE_DEFAULT_PORT=3306
# DATABASE_DEFAULT_USER=root
# DATABASE_DEFAULT_PASSWORD=
# DATABASE_DEFAULT_NAME=vixdb
#
# For production, prefer:
#
# DATABASE_DEFAULT_PASSWORD=
# VIX_DB_PASSWORD=loaded-from-secrets
#
# =========================================================
```

This file is intentionally explicit. A developer should understand what can be changed locally without reading the Vix source code.

## Loading configuration in C++

Use `vix::config::Config` when the application should read runtime values from `.env`.

```cpp
#include <vix.hpp>
#include <vix/config/Config.hpp>

int main()
{
    vix::config::Config config;

    vix::print("server port:", config.getServerPort());
    vix::print("database host:", config.getDbHost());
    vix::print("database port:", config.getDbPort());
    vix::print("database user:", config.getDbUser());
    vix::print("database name:", config.getDbName());

    return 0;
}
```

By default, this loads `.env`.

Use a specific environment file when needed:

```cpp
vix::config::Config config(".env.production");
```

When the file name contains an environment suffix, Vix can load environment-specific layered files.

## Building a Database from Config

The database module can construct a `Database` from `vix::config::Config`.

```cpp
#include <vix.hpp>
#include <vix/config/Config.hpp>
#include <vix/db.hpp>

int main()
{
    vix::config::Config config;

    auto db = vix::db::Database(config);

    db.exec(
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "email TEXT NOT NULL UNIQUE"
        ")"
    );

    vix::print("database ready");

    return 0;
}
```

This keeps the application code independent from local paths and credentials.

The configuration decides whether the application uses SQLite or MySQL.

The build must still include the selected backend.

For SQLite:

```bash
vix build --with-sqlite
```

For MySQL:

```bash
vix build --with-mysql
```

## SQLite environment values

For SQLite, the most important values are:

```dotenv
DATABASE_ENGINE=sqlite
DATABASE_SQLITE_PATH=storage/vix.db
```

Use a path under `storage/` for real projects.

```dotenv
DATABASE_SQLITE_PATH=storage/blog.db
```

Create the directory before running the application:

```bash
mkdir -p storage
```

SQLite may create sidecar files:

```txt
storage/blog.db-wal
storage/blog.db-shm
```

These are runtime files and normally should not be committed.

Use `.gitignore`:

```txt
storage/*.db
storage/*.db-wal
storage/*.db-shm
```

## MySQL environment values

For MySQL, use:

```dotenv
DATABASE_ENGINE=mysql
DATABASE_DEFAULT_HOST=127.0.0.1
DATABASE_DEFAULT_PORT=3306
DATABASE_DEFAULT_USER=root
DATABASE_DEFAULT_PASSWORD=
DATABASE_DEFAULT_NAME=vixdb
```

For local development, this is enough when the database server is running and the database exists.

For production, do not store the real password in `DATABASE_DEFAULT_PASSWORD` inside a committed file.

Prefer:

```dotenv
DATABASE_DEFAULT_PASSWORD=
VIX_DB_PASSWORD=loaded-from-secrets
```

`vix::config::Config` checks several password variable names so it can work in different deployment environments.

The recommended production variable is:

```dotenv
VIX_DB_PASSWORD
```

## Server values

The same `Config` object also exposes server values.

```dotenv
SERVER_PORT=8080
SERVER_REQUEST_TIMEOUT=2000
SERVER_IO_THREADS=0
SERVER_SESSION_TIMEOUT_SEC=20
SERVER_BENCH_MODE=false
```

These values are useful for backend examples and production services.

`SERVER_PORT` controls the HTTP server port.

`SERVER_REQUEST_TIMEOUT` is in milliseconds.

`SERVER_IO_THREADS=0` means Vix can choose the thread count.

`SERVER_SESSION_TIMEOUT_SEC` is in seconds.

`SERVER_BENCH_MODE` can be used by examples or builds that want benchmark-specific behavior.

## TLS values

TLS is disabled by default.

```dotenv
SERVER_TLS_ENABLED=false
SERVER_TLS_CERT_FILE=
SERVER_TLS_KEY_FILE=
```

Enable TLS only when the Vix HTTP server terminates HTTPS directly.

If Nginx, Caddy, Traefik, a load balancer, or another reverse proxy terminates HTTPS, keep TLS disabled in the Vix server:

```dotenv
SERVER_TLS_ENABLED=false
```

When direct TLS is enabled, both certificate files must be set.

```dotenv
SERVER_TLS_ENABLED=true
SERVER_TLS_CERT_FILE=/etc/letsencrypt/live/example.com/fullchain.pem
SERVER_TLS_KEY_FILE=/etc/letsencrypt/live/example.com/privkey.pem
```

In many production deployments, keeping TLS at the reverse proxy is simpler.

## Logging values

Vix runtime configuration can also control logging behavior.

```dotenv
LOGGING_ASYNC=true
LOGGING_QUEUE_MAX=20000
LOGGING_DROP_ON_OVERFLOW=true
```

Async logging is useful for backend applications because request handling should not block on every log write.

`LOGGING_QUEUE_MAX` controls the maximum queue size.

`LOGGING_DROP_ON_OVERFLOW=true` means the application can drop logs when the queue is full instead of blocking application work.

For debugging, you may choose stricter behavior.

For high-throughput applications, dropping logs under pressure may be safer than stalling the application.

## WAF values

The WAF section controls basic request filtering.

```dotenv
WAF_MODE=basic
WAF_MAX_TARGET_LEN=4096
WAF_MAX_BODY_BYTES=1048576
```

Supported modes:

```txt
off
basic
strict
```

Use `basic` as the normal development default.

Use `strict` only when the application has been tested with stricter filtering.

Use `off` when you deliberately want to disable this layer.

## WebSocket values

WebSocket-enabled applications can use:

```dotenv
WEBSOCKET_MAX_MESSAGE_SIZE=65536
WEBSOCKET_IDLE_TIMEOUT=60
WEBSOCKET_ENABLE_DEFLATE=true
WEBSOCKET_PING_INTERVAL=30
WEBSOCKET_AUTO_PING_PONG=true
```

These values are useful for examples and backend apps that expose realtime endpoints.

They are not database settings, but keeping them in `.env.example` helps backend templates stay consistent.

## Environment-specific files

For local development:

```txt
.env
.env.local
```

For production:

```txt
.env.production
.env.production.local
```

A common rule is:

```txt
.env.example           committed
.env                   local, not committed
.env.local             local override, not committed
.env.production        deployment template or generated file
.env.production.local  production secrets, not committed
```

The exact deployment strategy depends on the infrastructure.

The important rule is simple: do not commit real secrets.

## Using generic getters

`vix::config::Config` also supports generic accessors.

```cpp
auto port = config.getInt("server.port", 8080);
auto tls = config.getBool("server.tls.enabled", false);
auto databaseName = config.getString("database.mysql.database", "vix");
```

Generic getters can read values from the raw configuration tree or from environment variables derived from dotted keys.

A dotted key is converted to an environment variable by replacing dots with underscores and uppercasing the name.

For example:

```txt
server.port
```

maps to:

```txt
SERVER_PORT
```

This makes the generic API useful for project-specific settings.

## Configuration and CLI tools

`vix.json` remains important for Vix CLI database workflows.

Use it to describe the project database path, storage directory, and migrations directory.

```json
{
  "name": "Blog",
  "database": {
    "engine": "sqlite",
    "sqlite": {
      "path": "storage/Blog.db"
    },
    "storage": "storage",
    "migrations": "migrations"
  }
}
```

Then the CLI can inspect and manage the project database:

```bash
vix db status
vix db migrate
vix db backup
```

For migration-oriented workflows:

```bash
vix orm migrate
vix orm status
vix orm rollback --steps 1
```

The application can load `.env`.

The CLI can read `vix.json`.

Both files describe different parts of the same project.

## Recommended SQLite setup

For a new local Vix backend, start with SQLite.

`vix.json`:

```json
{
  "name": "App",
  "database": {
    "engine": "sqlite",
    "sqlite": {
      "path": "storage/App.db"
    },
    "storage": "storage",
    "migrations": "migrations"
  }
}
```

`.env`:

```dotenv
DATABASE_ENGINE=sqlite
DATABASE_SQLITE_PATH=storage/App.db
SERVER_PORT=8080
```

Create directories:

```bash
mkdir -p storage migrations
```

Build with SQLite support:

```bash
vix build --with-sqlite
```

Run:

```bash
vix run --with-sqlite
```

This gives the project a real local database workflow without requiring a database server.

## Recommended MySQL setup

Use MySQL when the application needs a database server.

`vix.json`:

```json
{
  "name": "App",
  "database": {
    "engine": "mysql",
    "mysql": {
      "host": "127.0.0.1",
      "port": 3306,
      "user": "root",
      "password": "",
      "database": "app"
    },
    "migrations": "migrations"
  }
}
```

`.env`:

```dotenv
DATABASE_ENGINE=mysql
DATABASE_DEFAULT_HOST=127.0.0.1
DATABASE_DEFAULT_PORT=3306
DATABASE_DEFAULT_USER=root
DATABASE_DEFAULT_PASSWORD=
DATABASE_DEFAULT_NAME=app
VIX_DB_PASSWORD=
SERVER_PORT=8080
```

Build with MySQL support:

```bash
vix build --with-mysql
```

Run:

```bash
vix run --with-mysql
```

For production, do not keep the real password in committed files. Inject it through deployment secrets or environment-specific files.

## Build flags

Configuration describes what the application should use.

Build flags enable the backend support in the compiled project.

For SQLite:

```bash
vix build --with-sqlite
```

For MySQL:

```bash
vix build --with-mysql
```

For release builds:

```bash
vix build --preset release --with-sqlite
vix build --preset release --with-mysql
```

A project can have correct configuration and still fail if the selected database backend was not enabled at build time.

## Common mistakes

### Committing .env

Commit `.env.example`.

Do not commit `.env` when it contains local paths, private credentials, or production secrets.

### Keeping production passwords in vix.json

`vix.json` is usually committed.

Do not put production credentials there.

Use environment variables or deployment secrets.

### Forgetting the storage directory

If SQLite uses:

```dotenv
DATABASE_SQLITE_PATH=storage/app.db
```

create the directory:

```bash
mkdir -p storage
```

### Selecting MySQL without building MySQL support

If the environment says:

```dotenv
DATABASE_ENGINE=mysql
```

build with:

```bash
vix build --with-mysql
```

### Selecting SQLite without building SQLite support

If the environment says:

```dotenv
DATABASE_ENGINE=sqlite
```

build with:

```bash
vix build --with-sqlite
```

### Mixing project config and runtime config accidentally

Use `vix.json` for project and tooling configuration.

Use `.env` for runtime environment values.

Do not duplicate secrets in both places.

## Recommended style

Keep `vix.json` stable and reviewable.

Keep `.env.example` complete and safe to commit.

Keep `.env` local.

Use SQLite as the default for examples and local development.

Use MySQL when the application needs a database server.

Use `VIX_DB_PASSWORD` for production secrets.

Build with the database backend selected by the environment.

Use `vix db status` and `vix orm status` to inspect database state.

## Next steps

Read the SQLite page for local database workflows.

Read the MySQL page for server-backed database workflows.

Read the connection pool page before tuning pool sizes.

Read the CLI page when you want to use `vix db`, `vix orm`, and environment checks in a project workflow.
