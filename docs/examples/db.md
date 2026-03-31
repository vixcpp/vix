# Vix DB Module – Beginner Guide

This guide introduces the Vix C++ database module in a simple and practical way.

The goal is to help beginners understand:

- How to connect to a database
- How to run queries
- How to use transactions
- How to run migrations

---

## 1. Basic Connection (MySQL)

```cpp
#include <vix/db/db.hpp>
#include <iostream>

using namespace vix::db;

int main()
{
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

What happens here:

1. We configure the database connection.
2. We create a Database object.
3. We acquire a connection from the pool.
4. We ping the database to verify connectivity.

---

## 2. Simple Query with Prepared Statements

```cpp
auto conn = db.pool().acquire();
auto st = conn->prepare("SELECT id, name FROM users WHERE age > ?");

st->bind(1, 18);

auto rs = st->query();
while (rs->next())
{
  const auto &row = rs->row();
  std::cout << row.getInt64(0) << " "
            << row.getString(1) << "\n";
}
```

Why prepared statements?

- Prevent SQL injection
- Handle type-safe parameter binding
- Improve performance

---

## 3. Transactions (RAII Style)

Transactions are automatically rolled back if not committed.

```cpp
Transaction tx(db.pool());

auto st = tx.conn().prepare(
  "INSERT INTO users (name, age) VALUES (?, ?)"
);

st->bind(1, std::string("Alice"));
st->bind(2, static_cast<std::int64_t>(20));
st->exec();

tx.commit();
```

If commit() is not called, the transaction rolls back automatically.

---

## 4. Code-Based Migration

```cpp
class CreateUsersTable final : public Migration
{
public:
  std::string id() const override { return "2026-01-22-create-users"; }

  void up(Connection &c) override
  {
    c.prepare(
      "CREATE TABLE IF NOT EXISTS users ("
      "id BIGINT PRIMARY KEY AUTO_INCREMENT,"
      "name VARCHAR(255) NOT NULL,"
      "age INT NOT NULL);"
    )->exec();
  }

  void down(Connection &c) override
  {
    c.prepare("DROP TABLE IF EXISTS users;")->exec();
  }
};
```

Migrations allow you to version your schema safely.

---

## 5. File-Based Migrations

Place files inside:

```
migrations/
  001_create_users.up.sql
  001_create_users.down.sql
```

Then run:

```cpp
FileMigrationsRunner runner(tx.conn(), "migrations");
runner.applyAll();
```

---

## Key Concepts for Beginners

Connection Pool:
- Reuses database connections
- Improves performance

Prepared Statements:
- Use ? placeholders
- Bind values safely

Transactions:
- Ensure atomic operations
- Commit or rollback

Migrations:
- Keep database schema versioned
- Safe evolution of your database

---

You are now ready to use Vix DB in real applications.

