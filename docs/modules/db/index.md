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

Common macros you will see in the codebase:

- `VIX_DB_HAS_MYSQL` enables MySQL driver integration
- `VIX_DB_HAS_SQLITE` enables SQLite driver integration

If a driver is disabled, its header is typically guarded with the macro (example: `#if VIX_DB_HAS_SQLITE`).

---

## 2) Core model

The public API is built around a few concepts.

### Connection

A live database connection.

Key operations:

- `prepare(sql)` returns a prepared `Statement`
- `begin()`, `commit()`, `rollback()` manage transactions
- `lastInsertId()` returns the last generated id (semantics depend on engine)
- `ping()` checks if the connection is still usable (driver dependent)

### Statement

A prepared SQL statement with positional parameters.

Key operations:

- `bind(idx, value)` binds a parameter
- `bindNull(idx)` binds SQL NULL
- `exec()` runs a non returning statement (INSERT, UPDATE, DELETE)
- `query()` runs a SELECT and returns a `ResultSet`

Vix binds values through a small type erased wrapper: `DbValue`.

### ResultSet and ResultRow

Query results are forward only.

- `ResultSet::next()` advances to the next row
- `ResultSet::row()` returns a reference to the current row
- `ResultRow::getString(i)`, `getInt64(i)`, `getDouble(i)` read columns by index
- Helper methods `getStringOr`, `getInt64Or`, `getDoubleOr` provide defaults for NULL

Column indexes are zero based and match the SELECT order.

---

## 3) DbValue and binding

The binding model is intentionally simple.

`DbValue` supports:

- `nullptr` for SQL NULL
- `bool`
- `std::int64_t`
- `double`
- `std::string`
- `Blob` (bytes)

Helpers:

- `null()`, `b(x)`, `i64(x)`, `f64(x)`, `str(x)`, `blob(bytes)`

`Statement::bind` also provides convenience overloads for common C++ types:

- `bind(idx, int)`, `bind(idx, std::int64_t)`, `bind(idx, double)`, `bind(idx, bool)`, `bind(idx, std::string)`, `bind(idx, const char*)`

When in doubt for integers, bind `std::int64_t`.

---

## 4) Connection pool

`ConnectionPool` owns a factory function and reuses connections.

- `acquire()` blocks if the pool is at max and no idle connection exists
- `release(c)` returns a connection to the idle queue
- `warmup()` pre creates at least `PoolConfig::min` connections

A convenience RAII type exists:

- `PooledConn` acquires on construction and releases on destruction

---

## 5) Database facade and config

`Database` is a small facade that owns a `ConnectionPool`.

The configuration is represented by:

- `Engine` enum: `MySQL` or `SQLite`
- `DbConfig` holding engine specific config
- `MySQLConfig` with host, user, password, database, and pool sizing
- `SQLiteConfig` with path and pool sizing

There is also a helper that maps your main Vix config:

```cpp
DbConfig make_db_config_from_vix_config(const vix::config::Config& cfg);
```

Use it when you want DB config to come from `config.json` and your `vix::config::Config` loader.

---

## 6) Minimal examples

### 6.1 Connect and ping

```cpp
#include <vix/db/db.hpp>
#include <iostream>

int main()
{
  using namespace vix::db;

  DbConfig cfg;
  cfg.engine = Engine::MySQL;
  cfg.mysql.host = "tcp://127.0.0.1:3306";
  cfg.mysql.user = "root";
  cfg.mysql.password = "";
  cfg.mysql.database = "vixdb";

  Database db(cfg);

  auto conn = db.pool().acquire();
  if (!conn->ping())
  {
    std::cerr << "DB ping failed\n";
    return 1;
  }

  std::cout << "DB connected successfully\n";
  return 0;
}
```

### 6.2 Transaction, create table, insert, query

This shows:

- `Transaction` RAII
- Prepared statements and binding
- Iterating results

```cpp
#include <vix/db/db.hpp>
#include <cstdint>
#include <iostream>
#include <string>

int main()
{
  using namespace vix::db;

  DbConfig cfg;
  cfg.engine = Engine::MySQL;
  cfg.mysql.host = "tcp://127.0.0.1:3306";
  cfg.mysql.user = "root";
  cfg.mysql.password = "";
  cfg.mysql.database = "vixdb";
  cfg.mysql.pool.min = 1;
  cfg.mysql.pool.max = 8;

  Database db(cfg);

  try
  {
    Transaction tx(db.pool());

    tx.conn().prepare(
      "CREATE TABLE IF NOT EXISTS users ("
      "  id BIGINT PRIMARY KEY AUTO_INCREMENT,"
      "  name VARCHAR(255) NOT NULL,"
      "  age INT NOT NULL"
      ");"
    )->exec();

    {
      auto st = tx.conn().prepare("INSERT INTO users (name, age) VALUES (?, ?)");
      st->bind(1, std::string("Alice"));
      st->bind(2, static_cast<std::int64_t>(20));
      st->exec();
    }

    {
      auto st = tx.conn().prepare("SELECT id, name, age FROM users WHERE age >= ?");
      st->bind(1, static_cast<std::int64_t>(18));

      auto rs = st->query();
      while (rs->next())
      {
        const auto& row = rs->row();
        std::cout
          << row.getInt64(0) << " "
          << row.getString(1) << " "
          << row.getInt64(2) << "\n";
      }
    }

    tx.commit();
    std::cout << "Committed\n";
    return 0;
  }
  catch (const std::exception& e)
  {
    std::cerr << "DB error: " << e.what() << "\n";
    return 1;
  }
}
```

### 6.3 Prepared SELECT with parameter

```cpp
#include <vix/db/db.hpp>
#include <cstdint>
#include <iostream>

int main()
{
  using namespace vix::db;

  DbConfig cfg;
  cfg.engine = Engine::MySQL;
  cfg.mysql.host = "tcp://127.0.0.1:3306";
  cfg.mysql.user = "root";
  cfg.mysql.password = "";
  cfg.mysql.database = "vixdb";

  Database db(cfg);

  auto conn = db.pool().acquire();
  auto st = conn->prepare("SELECT id, name FROM users WHERE age > ?");
  st->bind(1, static_cast<std::int64_t>(18));

  auto rs = st->query();
  while (rs->next())
  {
    const auto& row = rs->row();
    std::cout << row.getInt64(0) << " " << row.getString(1) << "\n";
  }

  return 0;
}
```

---

## 7) Transactions

`Transaction` is designed to be exception safe:

- Begins a transaction in the constructor
- Rolls back in the destructor if still active
- `commit()` marks it inactive so the destructor does nothing
- `rollback()` explicitly rolls back and marks inactive

Guideline:

- Use one transaction per unit of work
- Keep it short
- Prefer committing at the end

---

## 8) Migrations

Vix supports two styles.

### Code migrations

You write a `Migration` class with `id()`, `up()`, `down()`.
Then run it through `MigrationsRunner`.

### File migrations

You store SQL files:

- `./migrations/<id>.up.sql`
- `./migrations/<id>.down.sql`

Then run them via `FileMigrationsRunner`.

Example that shows both:

```cpp
#include <vix/db/db.hpp>
#include <filesystem>
#include <iostream>
#include <string>

int main()
{
  using namespace vix::db;

  DbConfig cfg;
  cfg.engine = Engine::MySQL;
  cfg.mysql.host = "tcp://127.0.0.1:3306";
  cfg.mysql.user = "root";
  cfg.mysql.password = "";
  cfg.mysql.database = "vixdb";
  cfg.mysql.pool.min = 1;
  cfg.mysql.pool.max = 4;

  class CreateUsersTable final : public Migration
  {
  public:
    std::string id() const override { return "2026-01-22-create-users"; }

    void up(Connection& c) override
    {
      c.prepare(
        "CREATE TABLE IF NOT EXISTS users ("
        "  id BIGINT PRIMARY KEY AUTO_INCREMENT,"
        "  name VARCHAR(255) NOT NULL,"
        "  age INT NOT NULL"
        ");"
      )->exec();
    }

    void down(Connection& c) override
    {
      c.prepare("DROP TABLE IF EXISTS users;")->exec();
    }
  };

  try
  {
    Database db(cfg);

    {
      Transaction tx(db.pool());
      CreateUsersTable m1;

      MigrationsRunner runner(tx.conn());
      runner.add(&m1);
      runner.runAll();

      tx.commit();
      std::cout << "[migrations] code ok\n";
    }

    {
      Transaction tx(db.pool());

      FileMigrationsRunner runner(tx.conn(), std::filesystem::path{"migrations"});
      runner.setTable("schema_migrations"); // optional
      runner.applyAll();

      tx.commit();
      std::cout << "[migrations] files ok\n";
    }

    return 0;
  }
  catch (const std::exception& e)
  {
    std::cerr << "ERROR: " << e.what() << "\n";
    return 1;
  }
}
```

---

## 9) Drivers overview

### SQLite driver

- Implements `Connection` on top of `sqlite3*`
- `ping()` is a lightweight validity check
- Lifetime is managed by the `SQLiteConnection` destructor

### MySQL driver

- Implements `Connection` on top of MySQL Connector/C++
- `ping()` uses `isValid()` with exception protection
- Transactions are implemented via autocommit toggling

Driver factories:

- `make_sqlite_factory(path)` returns a `ConnectionFactory`
- `make_mysql_factory(host, user, pass, db)` returns a `ConnectionFactory`

Those factories are what you feed to `ConnectionPool`.

---

## 10) Schema utilities

The `vix::db::schema` namespace provides a minimal schema model:

- `Schema` contains multiple `Table`
- `Table` contains `Column` and `Index`
- `Type` describes column type with optional size (example: `VARCHAR(n)`)
- `Dialect` indicates SQL emission target (MySQL or SQLite)

JSON snapshot helpers:

- `to_json_string(schema, pretty)`
- `from_json_string_or_throw(json)`

This layer is useful for:

- Persisting schema snapshots
- Diffing schemas
- Building migration tooling

---

## 11) Common errors

- Cannot connect: verify host, port, user, password, database, and driver enable flags
- Pool blocks forever: max connections reached and callers never release connections
- Bind mismatch: prefer `std::int64_t` for integer parameters
- NULL handling: use `row.isNull(i)` or the `getXOr` helpers



