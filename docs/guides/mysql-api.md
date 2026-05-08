# MySQL API

This guide shows how to build a Vix REST API backed by MySQL.

## Public headers

```cpp
#include <vix.hpp>
#include <vix/db.hpp>
```

## Requirements

```bash
vix --version
mysql --version
```

```sql
CREATE DATABASE vixdb;
```

## Setup

```bash
vix new mysql-api
cd mysql-api
vix build --with-mysql
```

## Minimal MySQL connection

```cpp
auto db = vix::db::Database::mysql(
    "tcp://127.0.0.1:3306", "root", "", "vixdb");
```

## Initialize the database

```cpp
static void initialize_database(vix::db::Database &db)
{
  db.exec(
      "CREATE TABLE IF NOT EXISTS users ("
      "id BIGINT PRIMARY KEY AUTO_INCREMENT, "
      "name VARCHAR(255) NOT NULL, "
      "email VARCHAR(255) NOT NULL UNIQUE, "
      "role VARCHAR(50) NOT NULL DEFAULT 'user', "
      "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
      ")");
}
```

## Query, find, create (same pattern as SQLite)

```cpp
// Query all users — same as SQLite
auto rows = db.query("SELECT id, name, email, role FROM users ORDER BY id DESC");

// Find by id — same prepared statement pattern
auto stmt = conn->prepare("SELECT id, name, email, role FROM users WHERE id = ?");
stmt->bind(1, id);

// Create — uses MySQL LAST_INSERT_ID()
auto rows = conn->prepare("SELECT LAST_INSERT_ID()")->query();
```

## Use .env configuration

```cpp
vix::config::Config cfg{".env"};
vix::db::Database db{cfg};
```

```dotenv
DATABASE_ENGINE=mysql
DATABASE_DEFAULT_HOST=127.0.0.1
DATABASE_DEFAULT_PORT=3306
DATABASE_DEFAULT_USER=root
DATABASE_DEFAULT_PASS=
DATABASE_DEFAULT_NAME=vixdb
```

## Test the API

```bash
curl -i http://127.0.0.1:8080/health
curl -i http://127.0.0.1:8080/users
curl -i http://127.0.0.1:8080/users/1
curl -i -X POST http://127.0.0.1:8080/users \
  -H "Content-Type: application/json" \
  -d '{"name":"Ada","email":"ada@example.com","role":"admin"}'
```

## Common MySQL types

| MySQL type | Use for |
|-----------|---------|
| `BIGINT` | ids, counters |
| `VARCHAR(255)` | names, emails |
| `TEXT` | long text |
| `BOOLEAN` | flags |
| `DECIMAL(10,2)` | money |
| `TIMESTAMP` | dates |

## SQLite vs MySQL

| Feature     | SQLite                             | MySQL                             |
|-------------|------------------------------------|-----------------------------------|
| Deployment  | Stores data in a local file.       | Stores data in a server database. |
| Setup       | Very simple, no server required.   | Requires a database server.       |
| Best for    | Local apps, small APIs, and MVPs.  | Multi-user production APIs.       |

## Common mistakes

### Forgetting MySQL build support

```bash
vix build --with-mysql
```

### Using root in production

Create a dedicated database user with only the permissions needed by the app.

### Exposing database errors in production

```cpp
// Log the real error server-side, return a generic message to clients
res.status(500).json(json::kv({
  {"ok", json::Json(false)},
  {"error", json::Json("database_error")}
}));
```

## What to use next

- [Validation guide](/guides/validation)
- [Production Nginx + systemd guide](/guides/production-nginx-systemd)
