# Unit of Work

`vix::orm::UnitOfWork` groups several database operations into one transaction.

It is the ORM-level transaction helper. Internally, it uses `vix::db::Transaction`, so it follows the same rule as the database transaction layer:

```txt
commit explicitly
rollback automatically if not committed
```

A unit of work is useful when multiple changes must succeed or fail together.

The public ORM header is:

```cpp
#include <vix/orm.hpp>
```

## Why UnitOfWork exists

A repository operation such as `create()` or `updateById()` is useful for simple CRUD.

But some application operations are bigger than one repository call.

Examples:

```txt
create a user and a profile
create an order and order items
move balance between accounts
write a message and an audit log
update inventory and create a checkout record
```

These operations should not partially succeed.

If one statement fails, the whole operation should roll back.

That is what `UnitOfWork` is for.

## Basic usage

```cpp
#include <vix.hpp>
#include <vix/db.hpp>
#include <vix/orm.hpp>

int main()
{
    auto db = vix::db::Database::sqlite("storage/app.db");
    auto work = vix::orm::unit_of_work(db);
    auto& conn = work.conn();

    auto insertUser = conn.prepare(
        "INSERT INTO users (email, name) VALUES (?, ?)"
    );

    insertUser->bind(1, "ada@example.com");
    insertUser->bind(2, "Ada");
    insertUser->exec();

    auto insertAudit = conn.prepare(
        "INSERT INTO audit_log (message) VALUES (?)"
    );

    insertAudit->bind(1, "created user ada@example.com");
    insertAudit->exec();

    work.commit();

    vix::print("unit of work committed");

    return 0;
}
```

The transaction starts when the unit of work is created.

All statements use the same connection through:

```cpp
auto& conn = work.conn();
```

The transaction commits when `work.commit()` is called.

If `commit()` is not called, the underlying transaction rolls back.

## Creating a UnitOfWork

The high-level helper accepts a database facade.

```cpp
auto work = vix::orm::unit_of_work(db);
```

This is the usual form when the application owns a `vix::db::Database`.

You can also create it from a connection pool.

```cpp
auto work = vix::orm::unit_of_work(db.pool());
```

Both forms create a unit of work backed by the same database pool.

## The connection

A unit of work exposes the active transaction connection.

```cpp
auto& conn = work.conn();
```

Use that connection for every SQL statement that must be part of the same transaction.

```cpp
auto stmt = conn.prepare(
    "UPDATE accounts SET balance = balance - ? WHERE id = ?"
);

stmt->bind(1, 100);
stmt->bind(2, 1);
stmt->exec();
```

This is important.

A transaction belongs to one connection. If a statement uses another connection, it is not part of the same transaction.

## Commit

Call `commit()` only after all operations have succeeded.

```cpp
auto work = vix::orm::unit_of_work(db);

auto& conn = work.conn();

// database operations

work.commit();
```

After `commit()`, the unit of work becomes inactive.

Calling `commit()` again does nothing.

The destructor will not roll back a committed unit of work.

## Rollback

You can roll back explicitly.

```cpp
auto work = vix::orm::unit_of_work(db);

auto& conn = work.conn();

auto stmt = conn.prepare(
    "UPDATE accounts SET balance = balance - ? WHERE id = ?"
);

stmt->bind(1, 100);
stmt->bind(2, 1);
stmt->exec();

work.rollback();

vix::print("unit of work rolled back");
```

After `rollback()`, the unit of work becomes inactive.

Use explicit rollback when the application detects a business condition that should cancel the operation.

## Automatic rollback

If `commit()` is not called, rollback happens automatically through the underlying database transaction.

```cpp
int createUser(vix::db::Database& db)
{
    auto work = vix::orm::unit_of_work(db);
    auto& conn = work.conn();
    auto stmt = conn.prepare(
        "INSERT INTO users (email, name) VALUES (?, ?)"
    );

    stmt->bind(1, "ada@example.com");
    stmt->bind(2, "Ada");
    stmt->exec();

    return 1;

    work.commit();
}
```

Here, `work.commit()` is never reached.

The unit of work leaves scope while still active.

The underlying transaction rolls back.

This is a safe default. A transaction should never commit by accident.

## Checking active state

A unit of work exposes whether it is still active.

```cpp
auto work = vix::orm::unit_of_work(db);

if (work.active())
{
    vix::print("work is active");
}
```

After `commit()` or `rollback()`, it becomes inactive.

```cpp
work.commit();

if (!work.active())
{
    vix::print("work is closed");
}
```

This can be useful in infrastructure code, but most application code should simply commit at the end of the successful path.

## A complete transfer example

This example moves balance from one account to another.

Both updates must succeed together.

```cpp
#include <vix.hpp>
#include <vix/db.hpp>
#include <vix/orm.hpp>

int main()
{
    auto db = vix::db::Database::sqlite("storage/app.db");

    db.exec(
        "CREATE TABLE IF NOT EXISTS accounts ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT NOT NULL UNIQUE,"
        "balance INTEGER NOT NULL"
        ")"
    );

    db.exec(
        "INSERT OR IGNORE INTO accounts (name, balance) VALUES (?, ?)",
        "source",
        1000
    );

    db.exec(
        "INSERT OR IGNORE INTO accounts (name, balance) VALUES (?, ?)",
        "target",
        0
    );

    auto work = vix::orm::unit_of_work(db);
    auto& conn = work.conn();

    auto debit = conn.prepare(
        "UPDATE accounts SET balance = balance - ? WHERE name = ?"
    );

    debit->bind(1, 100);
    debit->bind(2, "source");
    debit->exec();

    auto credit = conn.prepare(
        "UPDATE accounts SET balance = balance + ? WHERE name = ?"
    );

    credit->bind(1, 100);
    credit->bind(2, "target");
    credit->exec();

    work.commit();

    auto rows = db.query(
        "SELECT name, balance FROM accounts ORDER BY name"
    );

    while (rows->next())
    {
        const auto& row = rows->row();

        vix::print(
            vix::options{.sep = " | "},
            row.getString(0),
            row.getInt64(1)
        );
    }

    return 0;
}
```

If the debit succeeds and the credit fails, the transaction rolls back.

The database is not left with half of the transfer.

## Validate before commit

A unit of work should commit only when the application has verified that the operation is valid.

```cpp
auto work = vix::orm::unit_of_work(db);
auto& conn = work.conn();

auto balanceStmt = conn.prepare(
    "SELECT balance FROM accounts WHERE name = ?"
);

balanceStmt->bind(1, "source");

auto balanceRows = balanceStmt->query();

if (!balanceRows->next())
{
    vix::print("source account not found");
    return 1;
}

auto balance = balanceRows->row().getInt64(0);

if (balance < 100)
{
    vix::print("not enough balance");
    return 1;
}

auto debit = conn.prepare(
    "UPDATE accounts SET balance = balance - ? WHERE name = ?"
);

debit->bind(1, 100);
debit->bind(2, "source");
debit->exec();

auto credit = conn.prepare(
    "UPDATE accounts SET balance = balance + ? WHERE name = ?"
);

credit->bind(1, 100);
credit->bind(2, "target");
credit->exec();

work.commit();
```

The early returns are safe.

Since `commit()` is not called, the transaction rolls back.

## UnitOfWork and repositories

Generic repository methods use the database pool internally.

That is fine for normal CRUD operations.

But a transaction requires all operations to use the same connection.

This is important:

```cpp
auto work = vix::orm::unit_of_work(db);

auto& conn = work.conn();
```

Inside the unit of work, use `conn` for statements that must be in the transaction.

Do not assume this is transaction-bound:

```cpp
auto work = vix::orm::unit_of_work(db);
auto users = vix::orm::repository<User>(db, "users");

users.create(User{
    0,
    "ada@example.com",
    "Ada"
});

work.commit();
```

The repository call may acquire its own connection from the pool. That operation is not guaranteed to use the unit-of-work transaction connection.

For transaction-bound work, use `work.conn()` directly or write repository methods that accept a connection.

## Transaction-aware repository methods

A project-specific repository can expose methods that accept a `vix::db::Connection&`.

```cpp
#include <string>
#include <vix/db.hpp>

struct User
{
    std::int64_t id{};
    std::string email;
    std::string name;
};

class UserRepository
{
public:
    void createWithConnection(
        vix::db::Connection& conn,
        const User& user
    )
    {
        auto stmt = conn.prepare(
            "INSERT INTO users (email, name) VALUES (?, ?)"
        );

        stmt->bind(1, user.email);
        stmt->bind(2, user.name);
        stmt->exec();
    }
};
```

Use it with a unit of work:

```cpp
UserRepository users;

auto work = vix::orm::unit_of_work(db);

users.createWithConnection(
    work.conn(),
    User{
        0,
        "grace@example.com",
        "Grace Hopper"
    }
);

work.commit();
```

This keeps the repository method inside the transaction.

## UnitOfWork with QueryBuilder

`QueryBuilder` can also be used inside a unit of work.

```cpp
auto work = vix::orm::unit_of_work(db);
vix::orm::QueryBuilder qb;

qb.raw("UPDATE accounts SET balance = balance - ? WHERE id = ?");
qb.param(100);
qb.param(1);

auto stmt = work.conn().prepare(qb.sql());

qb.bind(*stmt);

stmt->exec();

work.commit();
```

The important part is that the statement is prepared from `work.conn()`.

Do not use a separate `PooledConn` if the query must be part of the transaction.

## UnitOfWork with direct SQL

It is normal to use direct SQL inside a unit of work.

The ORM does not require every operation to go through `Repository<T>`.

```cpp
auto work = vix::orm::unit_of_work(db);
auto& conn = work.conn();

auto insert = conn.prepare(
    "INSERT INTO audit_log (message) VALUES (?)"
);

insert->bind(1, "important operation completed");
insert->exec();

work.commit();
```

This is often the clearest approach for audit rows, counters, logs, or custom transactional statements.

Use the ORM where it helps.

Use direct SQL where it is clearer.

## Keep units of work short

A unit of work holds one database connection until it commits or rolls back.

That connection cannot be reused by other operations during that time.

Keep the unit of work short.

Good shape:

```cpp
auto work = vix::orm::unit_of_work(db);

// database reads and writes

work.commit();
```

Bad shape:

```cpp
auto work = vix::orm::unit_of_work(db);

// database write
// network request
// large file processing
// slow external service call
// another database write

work.commit();
```

Avoid holding a database transaction open while doing unrelated work.

This is especially important for backend services where many requests may need database connections.

## UnitOfWork and connection pools

A unit of work acquires a connection from the database pool.

If many units of work are active at the same time, the pool can run out of idle connections.

When the pool reaches its maximum size, other operations may wait for a connection to be released.

This is not a bug. It is backpressure.

It prevents the application from creating unlimited database connections.

The fix is not always a bigger pool. Often the better fix is shorter transactions, fewer round trips, better indexes, or less work inside the transaction.

## UnitOfWork and error handling

Database operations can throw.

A unit of work is designed for this.

```cpp
#include <exception>
#include <vix.hpp>
#include <vix/db.hpp>
#include <vix/orm.hpp>

int main()
{
    try
    {
        auto db = vix::db::Database::sqlite("storage/app.db");
        auto work = vix::orm::unit_of_work(db);
        auto& conn = work.conn();

        auto stmt = conn.prepare(
            "INSERT INTO users (email, name) VALUES (?, ?)"
        );

        stmt->bind(1, "ada@example.com");
        stmt->bind(2, "Ada");
        stmt->exec();

        work.commit();

        vix::print("user created");

        return 0;
    }
    catch (const std::exception& e)
    {
        vix::print("unit of work error:", e.what());
        return 1;
    }
}
```

If a statement throws before `commit()`, stack unwinding destroys the unit of work, and the underlying transaction rolls back.

The error is then handled at the application boundary.

## Checking affected rows

For updates and deletes, check the number of affected rows before committing.

```cpp
auto work = vix::orm::unit_of_work(db);
auto& conn = work.conn();

auto stmt = conn.prepare(
    "UPDATE users SET name = ? WHERE email = ?"
);

stmt->bind(1, "Ada Lovelace");
stmt->bind(2, "ada@example.com");

auto affected = stmt->exec();

if (affected != 1)
{
    vix::print("unexpected updated rows:", affected);
    return 1;
}

work.commit();
```

If the affected count is wrong, the function returns before commit and the transaction rolls back.

This is useful for business operations where updating zero rows is not acceptable.

## SQLite and UnitOfWork

SQLite works well with units of work for local applications, tests, tools, and small services.

```cpp
auto db = vix::db::Database::sqlite("storage/app.db");
auto work = vix::orm::unit_of_work(db);

// SQLite statements through work.conn()

work.commit();
```

SQLite is file-based. Its concurrency behavior is different from a database server.

Keep transactions short and avoid long write transactions.

## MySQL and UnitOfWork

MySQL is a common target for backend services.

```cpp
auto db = vix::db::Database::mysql(
    "tcp://127.0.0.1:3306",
    "root",
    "secret",
    "app_db"
);

auto work = vix::orm::unit_of_work(db);

// MySQL statements through work.conn()

work.commit();
```

For transactional behavior, use transactional tables such as InnoDB.

Vix controls the transaction boundary through `begin`, `commit`, and `rollback`.

The database engine still controls isolation, locking, deadlocks, and durability behavior.

## UnitOfWork boundaries

A unit of work should match one application operation.

Good boundaries:

```txt
create user and profile
create order and order items
transfer money
apply one migration
write message and delivery record
reserve inventory and create checkout
```

Weak boundaries usually fall into two categories.

The unit of work is too small:

```txt
one simple update with no related operation
```

Or it is too large:

```txt
database write
external API call
file upload
database write
email sending
commit
```

A good unit of work protects the database consistency of one operation without holding resources longer than needed.

## Nested units of work

Do not assume nested units of work are supported automatically.

```cpp
auto outer = vix::orm::unit_of_work(db);

// avoid creating another unrelated unit of work here
// unless you deliberately want another connection and transaction

outer.commit();
```

A unit of work represents one transaction on one connection.

If the application needs savepoints or nested transaction semantics, implement that deliberately with database-specific SQL.

Do not rely on accidental nesting.

## Testing rollback behavior

You can test rollback behavior with SQLite.

```cpp
auto db = vix::db::Database::sqlite("storage/test_uow.db");

db.exec(
    "CREATE TABLE IF NOT EXISTS users ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "email TEXT NOT NULL UNIQUE"
    ")"
);

{
    auto work = vix::orm::unit_of_work(db);

    auto stmt = work.conn().prepare(
        "INSERT INTO users (email) VALUES (?)"
    );

    stmt->bind(1, "rollback@example.com");
    stmt->exec();

    // no commit
}

auto rows = db.query(
    "SELECT COUNT(*) FROM users WHERE email = ?",
    "rollback@example.com"
);

if (rows->next())
{
    vix::print("rows after rollback:", rows->row().getInt64(0));
}
```

The expected count is:

```txt
0
```

Because the unit of work was not committed.

## Common mistakes

### Calling repository methods and assuming they join the unit of work

Generic repository methods acquire connections from the pool.

Use `work.conn()` for transaction-bound statements.

### Forgetting commit

If you forget `commit()`, the operation rolls back.

This is safe, but it may surprise you during development.

### Holding the unit of work too long

A unit of work holds a database connection.

Keep it short.

### Doing external work inside the transaction

Avoid network calls, large file processing, waiting for user input, or slow external services while the transaction is open.

### Ignoring affected rows

If an update should affect one row, check it before committing.

### Creating nested units of work accidentally

Treat nested transaction behavior as an explicit design decision.

## Recommended style

Create the unit of work close to the first transactional database operation.

Use `auto& conn = work.conn()`.

Prepare all transaction-bound statements through that connection.

Check affected row counts when correctness depends on them.

Commit only at the successful end.

Let automatic rollback handle exceptions and early returns.

Keep the unit of work short.

Do not assume generic repository methods automatically join the transaction.

## Next steps

Read the With vix::db page to understand how ORM and direct database access should work together.

Read the repositories page if you want to wrap transaction-aware methods in project-specific repositories.

Read the query builder page if you need dynamic SQL inside a unit of work.
