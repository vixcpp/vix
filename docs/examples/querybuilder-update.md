# ORM Example Guide: QueryBuilder Update

This guide explains the `querybuilder_update` example step by step.

Goal: - Build an UPDATE query dynamically - Bind parameters safely -
Execute using a pooled connection - Keep SQL explicit and predictable

This example demonstrates how `QueryBuilder` helps you construct queries
while still keeping full SQL control.

# 1. What is QueryBuilder?

`QueryBuilder` is a lightweight helper that:

-   Stores a SQL string
-   Stores bound parameters
-   Keeps SQL and parameters organized
-   Avoids string concatenation mistakes

It does NOT: - Abstract SQL away - Generate complex schema logic -
Replace repositories

It is a structured way to manage dynamic SQL.

# 2. Full Example Code

``` cpp
#include <vix/orm/orm.hpp>

#include <cstddef>
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

    QueryBuilder qb;
    qb.raw("UPDATE users SET age=? WHERE email=?")
        .param(29)
        .param(std::string("gaspardkirira@example.com"));

    PooledConn pc(pool);
    auto st = pc.get().prepare(qb.sql());

    const auto &ps = qb.params();
    for (std::size_t i = 0; i < ps.size(); ++i)
      st->bind(i + 1, any_to_dbvalue_or_throw(ps[i]));

    const auto affected = st->exec();
    std::cout << "[OK] affected rows = " << affected << "\n";
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

# 3. Step-by-Step Explanation

## 3.1 Build the SQL

``` cpp
qb.raw("UPDATE users SET age=? WHERE email=?")
```

-   `raw()` sets the SQL string.
-   You still control the full SQL syntax.
-   This keeps the design explicit.

## 3.2 Add parameters

``` cpp
.param(29)
.param(std::string("gaspardkirira@example.com"));
```

Each `.param()`:

-   Appends a value to an internal parameter list
-   Maintains correct binding order
-   Avoids manual index tracking

Internally, QueryBuilder stores parameters in a vector.

## 3.3 Prepare the statement

``` cpp
auto st = pc.get().prepare(qb.sql());
```

`qb.sql()` returns the final SQL string.

## 3.4 Bind parameters

``` cpp
const auto &ps = qb.params();
for (std::size_t i = 0; i < ps.size(); ++i)
  st->bind(i + 1, any_to_dbvalue_or_throw(ps[i]));
```

Important rules:

-   Binding starts at index 1
-   Parameters must match `?` placeholders
-   `any_to_dbvalue_or_throw` converts std::any safely

## 3.5 Execute

``` cpp
const auto affected = st->exec();
```

-   Returns number of affected rows
-   For UPDATE, usually 0 or 1 (or more)

# 4. When to Use QueryBuilder

Use QueryBuilder when:

-   SQL is dynamic
-   Filters are optional
-   You build WHERE clauses conditionally
-   You want clean parameter handling

Example pattern:

``` cpp
QueryBuilder qb;
qb.raw("UPDATE users SET age=? WHERE 1=1")
  .param(newAge);

if (hasEmail)
{
  qb.raw(" AND email=?").param(email);
}
```

# 5. Production Advice

-   Keep SQL readable
-   Avoid over-dynamic query generation
-   Log qb.sql() for debugging
-   Validate user input before binding
-   Use transactions if multiple updates must be atomic

# 6. Difference vs Repository

Repository: - Fixed CRUD structure - Ideal for standard entity
operations

QueryBuilder: - Flexible - Manual control over SQL - Useful for complex
or custom queries

They complement each other.

# Summary

You learned:

-   How to use QueryBuilder for updates
-   How parameters are managed safely
-   How pooled connections execute queries
-   When to use QueryBuilder vs Repository


