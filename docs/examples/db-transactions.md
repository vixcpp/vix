# Vix DB Transactions Guide

This guide explains how transactions work in Vix DB and how to use them
correctly in real applications.

Transactions ensure that multiple database operations behave as a single
atomic unit.

If something fails, everything is rolled back.

# 1. What is a Transaction

A transaction guarantees:

-   Atomicity
-   Consistency
-   Isolation
-   Durability

In simple terms:

Either everything succeeds, or nothing is applied.

# 2. Basic RAII Transaction

``` cpp
#include <vix/db/db.hpp>
using namespace vix::db;

void create_user(Database& db)
{
    Transaction tx(db.pool());

    auto st = tx.conn().prepare(
        "INSERT INTO users (name, age) VALUES (?, ?)"
    );

    st->bind(1, std::string("Alice"));
    st->bind(2, static_cast<std::int64_t>(25));
    st->exec();

    tx.commit();
}
```

Important:

If commit() is not called, rollback happens automatically when tx goes
out of scope.

This is RAII safety.

# 3. Multiple Operations in One Transaction

``` cpp
Transaction tx(db.pool());

// Insert user
auto insert_user = tx.conn().prepare(
    "INSERT INTO users (name, age) VALUES (?, ?)"
);
insert_user->bind(1, "Bob");
insert_user->bind(2, static_cast<std::int64_t>(30));
insert_user->exec();

// Insert profile
auto insert_profile = tx.conn().prepare(
    "INSERT INTO profiles (user_id, bio) VALUES (?, ?)"
);
insert_profile->bind(1, static_cast<std::int64_t>(1));
insert_profile->bind(2, "Engineer");
insert_profile->exec();

tx.commit();
```

If the second insert fails, both inserts are rolled back.

# 4. Automatic Rollback on Exception

``` cpp
try
{
    Transaction tx(db.pool());

    auto st = tx.conn().prepare("DELETE FROM users WHERE id = ?");
    st->bind(1, static_cast<std::int64_t>(5));
    st->exec();

    throw std::runtime_error("Something failed");

    tx.commit();
}
catch (...)
{
    // No manual rollback needed
}
```

Because commit() was not called, rollback happens automatically.

# 5. Best Practices

Keep transactions short.

Do not:

-   Perform HTTP calls inside a transaction
-   Wait for external APIs
-   Sleep or block unnecessarily

Do:

-   Execute DB operations quickly
-   Commit immediately
-   Handle errors properly

# 6. Nested Transactions

Avoid nested transactions unless your database supports savepoints.

If needed, use explicit savepoint logic at the SQL level.

# 7. Production Advice

Transactions lock resources.

Long transactions reduce concurrency and performance.

Monitor:

-   Slow queries
-   Deadlocks
-   Lock wait timeouts

Design carefully.

# Summary

Transactions in Vix DB are:

-   RAII safe
-   Automatic rollback
-   Explicit commit required

Use them for:

-   Multi-step writes
-   Data integrity guarantees
-   Critical operations

Correct transaction usage is essential for production-grade systems.

