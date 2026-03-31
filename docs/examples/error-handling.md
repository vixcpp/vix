# ORM Example Guide: Error Handling

This guide explains the `error_handling` example.

Goal: - Show what happens when the database connection is invalid -
Demonstrate how to catch `DBError` cleanly - Encourage safe production
error handling patterns

This example intentionally uses a wrong database name to trigger errors.

# 1. What this example demonstrates

You will learn:

-   Where database errors happen (factory, pool warmup, queries)
-   Why `pool.warmup()` is important
-   How to catch `DBError` separately from other exceptions
-   How to write safe CLI-style error handling

# 2. Full Example Code

``` cpp
#include <vix/orm/orm.hpp>

#include <iostream>
#include <string>

using namespace vix::orm;

int main(int argc, char **argv)
{
  (void)argc;
  (void)argv;

  try
  {
    // Intentionally wrong DB name to show error handling
    const std::string host = "tcp://127.0.0.1:3306";
    const std::string user = "root";
    const std::string pass = "";
    const std::string db = "db_does_not_exist";

    auto factory = make_mysql_factory(host, user, pass, db);

    PoolConfig cfg;
    cfg.min = 1;
    cfg.max = 8;

    ConnectionPool pool{factory, cfg};

    // will throw if factory returns invalid connection (recommended after our warmup fix),
    // or later when first query fails.
    pool.warmup();

    UnitOfWork uow{pool};
    auto &con = uow.conn();

    auto st = con.prepare("SELECT 1");
    (void)st->exec();

    std::cout << "[INFO] This message may not be reached if connection fails.\n";
    return 0;
  }
  catch (const DBError &e)
  {
    std::cerr << "[DBError] " << e.what() << "\n";
    return 1;
  }
  catch (const std::exception &e)
  {
    std::cerr << "[std::exception] " << e.what() << "\n";
    return 1;
  }
}
```

# 3. Step by Step Explanation

## 3.1 Intentional failure

``` cpp
const std::string db = "db_does_not_exist";
```

This database does not exist, so MySQL will fail during connection or
query.

This is a controlled demo to show how errors look and how to handle
them.

## 3.2 Create factory and pool

``` cpp
auto factory = make_mysql_factory(host, user, pass, db);
ConnectionPool pool{factory, cfg};
```

The factory creates MySQL connections. The pool stores and reuses them.

## 3.3 Why warmup matters

``` cpp
pool.warmup();
```

Warmup tries to pre-create connections early.

Benefits:

-   Fail fast at startup (not during first real request)
-   Catch invalid credentials immediately
-   Avoid "first request slow" latency

In this example, warmup is expected to throw because the database is
invalid.

## 3.4 UnitOfWork and query

``` cpp
UnitOfWork uow{pool};
auto st = con.prepare("SELECT 1");
st->exec();
```

If warmup did not throw, the next likely failure point is the first
query.

Either way, errors become exceptions.

# 4. Catching DBError vs std::exception

``` cpp
catch (const DBError &e)
{
  std::cerr << "[DBError] " << e.what() << "\n";
}
```

`DBError` is the dedicated Vix ORM exception type for database errors,
such as:

-   Connection failures
-   Authentication failures
-   Database not found
-   Query syntax errors
-   Constraint violations

Then a generic catch for everything else:

``` cpp
catch (const std::exception &e)
{
  std::cerr << "[std::exception] " << e.what() << "\n";
}
```

This separation makes logs easier to understand.

# 5. Production Advice

## 5.1 Fail fast at startup

Call `pool.warmup()` in your application startup:

-   you detect DB misconfiguration immediately
-   you avoid serving traffic with broken DB access

## 5.2 Do not leak DB errors to clients

If you build an HTTP API:

-   log full DBError internally
-   return a generic error to clients

Example mapping:

-   DBError: "database unavailable"
-   client message: "temporary server issue"

## 5.3 Add context in logs

In production, log:

-   host
-   database name
-   error message
-   operation context (connect, warmup, query)

Do not log secrets like passwords.

# 6. Typical causes of DBError

-   Wrong host or port
-   Wrong username/password
-   Database missing
-   Permissions missing
-   Network blocked (firewall)
-   Connection limit reached
-   Invalid SQL syntax
-   Foreign key constraint failure

# Summary

This example demonstrates:

-   How to force a predictable DB failure
-   Why warmup is important
-   How to catch DBError cleanly
-   How to structure safe production error handling

