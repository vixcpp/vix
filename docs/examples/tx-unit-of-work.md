# ORM Example Guide: Unit of Work (Transaction Pattern)

This guide explains the `tx_unit_of_work` example step by step.

Goal: - Create multiple related records (User + Order) - Ensure atomic
consistency - Commit everything together - Automatically rollback if
something fails

This is a more structured and domain-oriented pattern compared to raw
Transaction usage.

# 1. What is Unit of Work

Unit of Work is a higher-level abstraction over transactions.

It groups multiple operations that must succeed together.

In this example: - Insert a user - Insert an order linked to that user -
Commit both at once

If anything fails → everything is rolled back.

# 2. Full Example Code

``` cpp
#include <vix/orm/orm.hpp>

#include <cstdint>
#include <iostream>
#include <string>

using namespace vix::orm;

int main(int argc, char **argv)
{
  const std::string host = (argc > 1 ? argv[1] : "tcp://127.0.0.1:3306");
  const std::string user = (argc > 2 ? argv[2] : "root");
  const std::string pass = (argc > 3 ? argv[3] : "");
  const std::string db = (argc > 4 ? argv[4] : "vixdb");

  try
  {
    auto factory = make_mysql_factory(host, user, pass, db);

    PoolConfig cfg;
    cfg.min = 1;
    cfg.max = 8;

    ConnectionPool pool{factory, cfg};
    pool.warmup();

    UnitOfWork uow{pool};
    auto &c = uow.conn();

    {
      auto st = c.prepare("INSERT INTO users(name,email,age) VALUES(?,?,?)");
      st->bind(1, "Alice");
      st->bind(2, "alice@example.com");
      st->bind(3, 27);
      st->exec();
    }

    const std::uint64_t userId = c.lastInsertId();

    {
      auto st = c.prepare("INSERT INTO orders(user_id,total) VALUES(?,?)");
      st->bind(1, userId);
      st->bind(2, 199.99);
      st->exec();
    }

    uow.commit();
    std::cout << "[OK] user+order committed. user_id=" << userId << "\n";
    return 0;
  }
  catch (const std::exception &e)
  {
    std::cerr << "[ERR] " << e.what() << "\n";
    return 1;
  }
}
```

# 3. Step by Step Explanation

## 3.1 Create the factory and pool

``` cpp
auto factory = make_mysql_factory(host, user, pass, db);
ConnectionPool pool{factory, cfg};
pool.warmup();
```

The pool manages reusable database connections.

## 3.2 Create UnitOfWork

``` cpp
UnitOfWork uow{pool};
auto &c = uow.conn();
```

What happens internally:

-   A connection is acquired from the pool
-   A transaction begins
-   All operations run on the same connection

RAII rule: If `uow.commit()` is not called, rollback happens
automatically.

## 3.3 Insert User

``` cpp
auto st = c.prepare("INSERT INTO users(name,email,age) VALUES(?,?,?)");
st->bind(1, "Alice");
st->bind(2, "alice@example.com");
st->bind(3, 27);
st->exec();
```

Prepared statement: - Safe binding - No SQL injection - Reusable pattern

## 3.4 Retrieve last insert id

``` cpp
const std::uint64_t userId = c.lastInsertId();
```

This retrieves the auto-generated primary key of the inserted user.

Important: This works because: - We are still inside the same
transaction - We are using the same connection

## 3.5 Insert Order linked to User

``` cpp
auto st = c.prepare("INSERT INTO orders(user_id,total) VALUES(?,?)");
st->bind(1, userId);
st->bind(2, 199.99);
st->exec();
```

This creates a dependent record.

If this fails: - The user insert will also be rolled back.

## 3.6 Commit

``` cpp
uow.commit();
```

Commit makes both operations permanent.

Without commit: - Everything is rolled back automatically.

# 4. Required Schema

``` sql
CREATE TABLE users (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  name VARCHAR(255) NOT NULL,
  email VARCHAR(255) NOT NULL,
  age INT NOT NULL
);

CREATE TABLE orders (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  user_id BIGINT NOT NULL,
  total DOUBLE NOT NULL,
  FOREIGN KEY (user_id) REFERENCES users(id)
);
```

# 5. Why Use Unit of Work

Use UnitOfWork when:

-   Multiple inserts must succeed together
-   You create parent + child records
-   You update multiple tables in one business operation
-   You implement domain-level logic

It is cleaner than manually handling transactions everywhere.

# 6. Production Advice

Keep unit of work small.

Do not:

-   Call external APIs inside it
-   Perform slow operations
-   Hold locks for long time

Do:

-   Keep DB operations grouped
-   Commit quickly
-   Handle errors properly

# 7. Difference: Transaction vs UnitOfWork

Transaction: - Lower level - Manual grouping

UnitOfWork: - Domain-oriented - Business-logic grouping - Cleaner
architecture for complex systems

# Summary

You learned:

-   How to use UnitOfWork
-   How to insert related records safely
-   How to retrieve last insert ID
-   How automatic rollback works
-   Why this pattern is useful in production systems


