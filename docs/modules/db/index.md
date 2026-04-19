# Database

The `vix::db` module is a small, explicit database layer built around:

- A tiny driver abstraction (`Connection`, `Statement`)
- A thread safe connection pool (`ConnectionPool`)
- RAII transactions (`Transaction`)
- A minimal, type erased value model (`DbValue`)
- Migrations (code and file based)
- Optional schema snapshot utilities (`vix::db::schema`)

This is a guide, not a showcase: it explains the mental model and the core APIs you will use in real apps.

All examples assume you include the umbrella header:

```cpp
#include <vix/db/db.hpp>
```

Namespace used in examples:

```cpp
using namespace vix::db;
```

---

## 1) Build and feature flags

Vix DB can be built with different backends.

Common macros:

- `VIX_DB_HAS_MYSQL`
- `VIX_DB_HAS_SQLITE`

---

## 2) Core model

### Connection

- `prepare(sql)`
- `begin()`, `commit()`, `rollback()`
- `lastInsertId()`
- `ping()`

### Statement

- `bind(idx, value)`
- `bindNull(idx)`
- `exec()`
- `query()`

### ResultSet / ResultRow

- `next()`
- `row()`
- `getString(i)`, `getInt64(i)`, `getDouble(i)`

---

## 3) Connection pool

- `acquire()`
- `release()`
- `warmup()`

RAII:

- `PooledConn`

---

## 4) Database config (ENV BASED)

Vix uses `.env` for configuration.

### Example `.env`

```env
DATABASE_ENGINE=mysql
DATABASE_DEFAULT_HOST=127.0.0.1
DATABASE_DEFAULT_PORT=3306
DATABASE_DEFAULT_USER=root
DATABASE_DEFAULT_PASSWORD=
DATABASE_DEFAULT_NAME=vixdb
DATABASE_POOL_MIN=1
DATABASE_POOL_MAX=8
DATABASE_SQLITE_PATH=vix.db
```

### Usage

```cpp
#include <vix.hpp>
#include <vix/db/Database.hpp>

vix::config::Config cfg{".env"};
vix::db::Database db{cfg};
```

## 5) Minimal example (recommended)

```cpp
#include <vix.hpp>
#include <vix/db/Database.hpp>
#include <iostream>

int main()
{
  try
  {
    vix::config::Config cfg{".env"};
    vix::db::Database db{cfg};

    vix::db::PooledConn conn(db.pool());

    conn->prepare("CREATE TABLE IF NOT EXISTS users (id BIGINT AUTO_INCREMENT PRIMARY KEY, name TEXT)")
        ->exec();

    conn->prepare("INSERT INTO users (name) VALUES (?)")
        ->bind(1, "Gaspard")
        ->exec();

    std::cout << "[OK] inserted user\n";
  }
  catch (const std::exception& e)
  {
    std::cerr << "[ERR] " << e.what() << "\n";
  }
}
```

## 6) Transactions

- RAII safe
- auto rollback
- explicit commit

---

## 7) Migrations

Supports:

- Code migrations
- File migrations

---

## 8) Drivers

### SQLite
- simple
- embedded

### MySQL
- production
- network

---

## 9) Schema utilities

- Schema snapshot
- JSON export/import

---

## 10) Common errors

- Wrong `.env` values
- Pool exhaustion
- Bind mismatch
- NULL handling

---

## Final architecture

```
.env (source of truth)
  ↓
vix::config::Config (typed access)
  ↓
vix::db::Database (runtime initialization)
```

