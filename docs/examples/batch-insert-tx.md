# ORM Example Guide: Batch Insert + Transaction (MySQL)

This guide explains the `batch_insert_tx` example step by step.

Goal: - Connect to MySQL using the ORM connection pool - Run multiple
INSERT operations efficiently - Wrap everything inside a transaction
(commit or rollback)

## 1. What this example demonstrates

This example shows how to:

-   Create a MySQL driver factory with `make_mysql_factory()`
-   Build a `ConnectionPool` with `PoolConfig`
-   Warm up the pool so it opens connections early
-   Use `Transaction` (RAII) for safe commits and automatic rollbacks
-   Use prepared statements with parameter binding
-   Insert multiple rows inside one transaction

## 2. Full Example Code

``` cpp
#include <vix/orm/orm.hpp>

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

using namespace vix::orm;

int main(int argc, char **argv)
{
  const std::string host = (argc > 1 ? argv[1] : "tcp://127.0.0.1:3306");
  const std::string user = (argc > 2 ? argv[2] : "root");
  const std::string pass = (argc > 3 ? argv[3] : "");
  const std::string db = (argc > 4 ? argv[4] : "vixdb");

  try
  {
    // DB factory (MySQL driver)
    auto factory = make_mysql_factory(host, user, pass, db);

    PoolConfig cfg;
    cfg.min = 1;
    cfg.max = 8;

    ConnectionPool pool{factory, cfg};
    pool.warmup();

    // Transaction (RAII rollback if not committed)
    Transaction tx(pool);
    auto &c = tx.conn();

    auto st = c.prepare("INSERT INTO users(name,email,age) VALUES(?,?,?)");

    struct Row
    {
      const char *name;
      const char *email;
      int age;
    };

    const std::vector<Row> rows = {
        {"Zoe", "zoe@example.com", 23},
        {"Mina", "mina@example.com", 31},
        {"Omar", "omar@example.com", 35},
    };

    std::uint64_t total = 0;
    for (const auto &r : rows)
    {
      st->bind(1, r.name);
      st->bind(2, r.email);
      st->bind(3, r.age);
      total += st->exec();
    }

    tx.commit();
    std::cout << "[OK] inserted rows = " << total << "\n";
    return 0;
  }
  catch (const DBError &e)
  {
    std::cerr << "[DBError] " << e.what() << "\n";
    return 1;
  }
  catch (const std::exception &e)
  {
    std::cerr << "[ERR] " << e.what() << "\n";
    return 1;
  }
}
```

## 3. Step by Step Explanation

### 3.1 CLI arguments (connection info)

``` cpp
const std::string host = (argc > 1 ? argv[1] : "tcp://127.0.0.1:3306");
const std::string user = (argc > 2 ? argv[2] : "root");
const std::string pass = (argc > 3 ? argv[3] : "");
const std::string db = (argc > 4 ? argv[4] : "vixdb");
```

This allows running the same binary with different databases without
editing code.

Example:

``` bash
./batch_insert_tx tcp://127.0.0.1:3306 root "" vixdb
```

### 3.2 Create the MySQL driver factory

``` cpp
auto factory = make_mysql_factory(host, user, pass, db);
```

The factory knows how to create new MySQL connections. The pool will use
this factory when it needs more connections.

### 3.3 Configure and create the connection pool

``` cpp
PoolConfig cfg;
cfg.min = 1;
cfg.max = 8;

ConnectionPool pool{factory, cfg};
pool.warmup();
```

-   `min` is the minimum number of connections kept ready.
-   `max` is the maximum number of connections allowed.
-   `warmup()` opens the initial connections early, so the first query
    is not slow.

Production tip: - Warmup is useful for APIs to avoid a slow first
request.

### 3.4 Start a transaction (RAII)

``` cpp
Transaction tx(pool);
auto &c = tx.conn();
```

This creates a transaction using one connection acquired from the pool.

RAII rule: - If `tx.commit()` is not called, rollback happens
automatically.

### 3.5 Prepare the insert statement once

``` cpp
auto st = c.prepare("INSERT INTO users(name,email,age) VALUES(?,?,?)");
```

Prepared statements: - Improve performance (parse/plan once) - Avoid SQL
injection - Allow safe binding

### 3.6 Define rows to insert

``` cpp
struct Row { const char *name; const char *email; int age; };
```

Then a vector of rows:

``` cpp
const std::vector<Row> rows = {
  {"Zoe", "zoe@example.com", 23},
  {"Mina", "mina@example.com", 31},
  {"Omar", "omar@example.com", 35},
};
```

### 3.7 Bind parameters and execute in a loop

``` cpp
for (const auto &r : rows)
{
  st->bind(1, r.name);
  st->bind(2, r.email);
  st->bind(3, r.age);
  total += st->exec();
}
```

Binding rules: - Indexes start at 1 - Types are converted safely - The
same prepared statement is reused for every row

Note: - This is not a multi-row SQL insert. - It is still fast because
the statement is prepared once and executed multiple times in one
transaction.

### 3.8 Commit

``` cpp
tx.commit();
```

Without commit: - rollback happens automatically - rows are not inserted

With commit: - all rows become visible and durable

## 4. Required SQL Table

This example expects a `users` table with:

-   name
-   email
-   age

Example schema:

``` sql
CREATE TABLE IF NOT EXISTS users (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  name VARCHAR(255) NOT NULL,
  email VARCHAR(255) NOT NULL,
  age INT NOT NULL
);
```

## 5. Error Handling

``` cpp
catch (const DBError &e)
{
  std::cerr << "[DBError] " << e.what() << "\n";
}
```

-   `DBError` is used for database driver / SQL errors.
-   A second catch handles other exceptions.

Production tip: - Log DBError message internally - Return generic
messages to HTTP clients

## 6. Production Notes

Use this pattern when:

-   You need to insert many rows
-   You want atomic behavior
-   You want speed and safety

Best practices:

-   Keep the transaction short
-   Avoid external API calls inside a transaction
-   Use pool warmup at startup
-   Tune `cfg.max` using load tests

## Summary

You learned:

-   How to configure a MySQL pool with Vix ORM
-   How to batch insert rows efficiently
-   How to use RAII transactions for safety
-   How to bind parameters with prepared statements
-   How to handle database exceptions

