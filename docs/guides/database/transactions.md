# Transactions

A transaction groups several database operations into one unit.

Either all operations are committed, or the database rolls them back. This is necessary when partial success would leave the application in an invalid state.

A simple example is a money transfer. Debiting one account and crediting another account must be treated as one operation. If the debit succeeds and the credit fails, the database is wrong. The application needs a transaction.

Vix provides `vix::db::Transaction` for this pattern.

```cpp
#include <vix.hpp>
#include <vix/db.hpp>
```

A transaction acquires a connection from the database pool, starts a transaction on that connection, and rolls back automatically if `commit()` is not called.

## Basic transaction

```cpp
#include <vix.hpp>
#include <vix/db.hpp>

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

    vix::db::Transaction tx(db.pool());

    auto& conn = tx.conn();

    auto create = conn.prepare(
        "INSERT OR IGNORE INTO accounts (name, balance) VALUES (?, ?)"
    );
    create->bind(1, "main");
    create->bind(2, 1000);
    create->exec();

    auto update = conn.prepare(
        "UPDATE accounts SET balance = balance - ? WHERE name = ?"
    );
    update->bind(1, 100);
    update->bind(2, "main");
    update->exec();

    tx.commit();

    vix::print("transaction committed");

    return 0;
}
```

The transaction begins when `tx` is created.

The transaction is committed when `tx.commit()` is called.

If the function exits before `commit()`, the transaction rolls back automatically.

## Why RAII matters

C++ has several ways to leave a scope.

A function can return early.

A statement can throw an exception.

A branch can skip the rest of the code.

A resource can be forgotten when code grows.

Transactions are a resource. If the transaction starts but does not finish correctly, the database connection can remain in an unexpected state or the database can contain partial changes.

Vix uses RAII for transaction lifetime.

The object owns the active transaction.

The destructor performs rollback if the transaction is still active.

That gives the code a safe default: uncommitted work is not kept.

## Commit

Call `commit()` only after every operation in the transaction has succeeded.

```cpp
vix::db::Transaction tx(db.pool());

auto& conn = tx.conn();

auto stmt = conn.prepare(
    "UPDATE users SET name = ? WHERE email = ?"
);
stmt->bind(1, "Ada Lovelace");
stmt->bind(2, "ada@example.com");
stmt->exec();

tx.commit();
```

After `commit()`, the transaction becomes inactive. The destructor will not roll it back.

Use `commit()` as the final step of the successful path.

## Automatic rollback

If `commit()` is not called, the destructor rolls the transaction back.

```cpp
void updateUser(vix::db::Database& db)
{
    vix::db::Transaction tx(db.pool());

    auto& conn = tx.conn();

    auto stmt = conn.prepare(
        "UPDATE users SET name = ? WHERE email = ?"
    );
    stmt->bind(1, "Ada Lovelace");
    stmt->bind(2, "ada@example.com");
    stmt->exec();

    return;

    tx.commit();
}
```

In this example, `commit()` is never reached. The transaction rolls back when `tx` leaves scope.

This is the behavior you want. A transaction should not commit by accident.

## Explicit rollback

You can also call `rollback()` manually.

```cpp
vix::db::Transaction tx(db.pool());

auto& conn = tx.conn();

auto stmt = conn.prepare(
    "UPDATE accounts SET balance = balance - ? WHERE name = ?"
);
stmt->bind(1, 100);
stmt->bind(2, "main");
stmt->exec();

tx.rollback();

vix::print("transaction rolled back");
```

After `rollback()`, the transaction becomes inactive.

The destructor will not roll it back again.

Explicit rollback is useful when the application detects a business condition and wants to cancel the transaction intentionally.

## Transfer example

This example shows a transaction where two updates must succeed together.

```cpp
#include <vix.hpp>
#include <vix/db.hpp>

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

    vix::db::Transaction tx(db.pool());

    auto& conn = tx.conn();

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

    tx.commit();

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

The debit and credit use the same transaction connection.

If one statement fails before `commit()`, the transaction rolls back.

## Validate before commit

A transaction should commit only when the application knows the state is correct.

For example, if the source account does not have enough balance, the transfer should stop before the update.

```cpp
vix::db::Transaction tx(db.pool());

auto& conn = tx.conn();

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

const auto balance = balanceRows->row().getInt64(0);

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

tx.commit();
```

The early returns are safe because the transaction rolls back automatically.

## Transactions and the connection pool

A transaction holds one pooled connection for its full lifetime.

This matters.

While the transaction is active, the connection cannot be reused by another query. If many transactions are open for a long time, the pool can run out of available connections and other operations may wait.

Keep transactions short.

Do the required database work.

Commit or roll back.

Then leave the scope.

Do not keep a transaction open while doing unrelated work such as network calls, large file processing, long computations, or waiting for user input.

## Good transaction shape

Prefer this structure:

```cpp
vix::db::Transaction tx(db.pool());

auto& conn = tx.conn();

// read required database state
// write required database changes

tx.commit();
```

The transaction starts late and ends early.

The code inside the transaction is only the code that needs transactional protection.

## Bad transaction shape

Avoid this structure:

```cpp
vix::db::Transaction tx(db.pool());

// database write
// call external API
// process a large file
// wait for another service
// database write

tx.commit();
```

This holds a database connection longer than necessary.

It can increase lock time, reduce pool availability, and create latency under load.

## Transactions with repositories

In a larger application, you may have repositories that usually use `Database::exec()` and `Database::query()` directly.

For transactional operations, pass a connection into the repository method, or create a transaction-specific method.

```cpp
#include <string>
#include <vix/db.hpp>

class AccountRepository
{
public:
    void debit(
        vix::db::Connection& conn,
        const std::string& name,
        std::int64_t amount
    )
    {
        auto stmt = conn.prepare(
            "UPDATE accounts SET balance = balance - ? WHERE name = ?"
        );

        stmt->bind(1, amount);
        stmt->bind(2, name);
        stmt->exec();
    }

    void credit(
        vix::db::Connection& conn,
        const std::string& name,
        std::int64_t amount
    )
    {
        auto stmt = conn.prepare(
            "UPDATE accounts SET balance = balance + ? WHERE name = ?"
        );

        stmt->bind(1, amount);
        stmt->bind(2, name);
        stmt->exec();
    }
};
```

Use it like this:

```cpp
AccountRepository accounts;

vix::db::Transaction tx(db.pool());

auto& conn = tx.conn();

accounts.debit(conn, "source", 100);
accounts.credit(conn, "target", 100);

tx.commit();
```

This keeps the operation on one connection and inside one transaction.

Do not start a new `Database::exec()` call inside the repository when the operation must be part of the active transaction. `Database::exec()` may acquire a different pooled connection, which would not be part of the transaction.

## Transaction boundaries

A transaction boundary should match a business operation.

Good examples:

```txt
create user and profile
create order and order items
move money between accounts
mark message as sent and write delivery record
apply one migration file
reserve inventory and create checkout record
```

Weak transaction boundaries usually come from wrapping too much or too little.

If the transaction contains only one simple statement, it may not be needed.

If the transaction contains half of a business operation, it is not protecting enough.

If the transaction contains unrelated work, it is doing too much.

## SQLite transactions

SQLite transactions work well for local applications, tools, tests, and small services.

```cpp
auto db = vix::db::Database::sqlite("storage/app.db");

vix::db::Transaction tx(db.pool());

// SQLite statements on tx.conn()

tx.commit();
```

SQLite is file-based. Its concurrency behavior is not the same as a server database.

For simple local workflows, this is usually fine.

For write-heavy concurrent services, measure the workload and keep transactions short.

## MySQL transactions

MySQL transactions are commonly used in backend services.

```cpp
auto db = vix::db::Database::mysql(
    "tcp://127.0.0.1:3306",
    "root",
    "secret",
    "app_db"
);

vix::db::Transaction tx(db.pool());

// MySQL statements on tx.conn()

tx.commit();
```

For MySQL, transactional behavior also depends on the storage engine and table definitions. Use transactional tables such as InnoDB.

The Vix transaction wrapper controls `begin`, `commit`, and `rollback` through the database connection. The database engine still defines isolation behavior, locking, deadlocks, and durability details.

## Error handling

A transaction is especially useful when exceptions can happen.

```cpp
#include <exception>
#include <vix.hpp>
#include <vix/db.hpp>

int main()
{
    try
    {
        auto db = vix::db::Database::sqlite("storage/app.db");

        vix::db::Transaction tx(db.pool());

        auto& conn = tx.conn();

        auto stmt = conn.prepare(
            "INSERT INTO users (email, name) VALUES (?, ?)"
        );
        stmt->bind(1, "ada@example.com");
        stmt->bind(2, "Ada");
        stmt->exec();

        tx.commit();

        vix::print("user created");

        return 0;
    }
    catch (const std::exception& e)
    {
        vix::print("database error:", e.what());
        return 1;
    }
}
```

If any statement throws before `commit()`, the transaction object rolls back while stack unwinding.

The destructor does not throw. Rollback failures during destruction are swallowed to preserve exception safety.

## Checking affected rows

For updates and deletes inside transactions, affected rows matter.

```cpp
vix::db::Transaction tx(db.pool());

auto& conn = tx.conn();

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

tx.commit();
```

If the statement did not update the expected row, the function returns before commit and the transaction rolls back.

Use this pattern for operations where updating zero rows would be a bug.

## Do not mix transaction and facade calls accidentally

Inside a transaction, use `tx.conn()` for all operations that must be part of the transaction.

This is correct:

```cpp
vix::db::Transaction tx(db.pool());

auto& conn = tx.conn();

auto a = conn.prepare("UPDATE accounts SET balance = balance - ? WHERE name = ?");
a->bind(1, 100);
a->bind(2, "source");
a->exec();

auto b = conn.prepare("UPDATE accounts SET balance = balance + ? WHERE name = ?");
b->bind(1, 100);
b->bind(2, "target");
b->exec();

tx.commit();
```

Avoid this:

```cpp
vix::db::Transaction tx(db.pool());

db.exec(
    "UPDATE accounts SET balance = balance - ? WHERE name = ?",
    100,
    "source"
);

db.exec(
    "UPDATE accounts SET balance = balance + ? WHERE name = ?",
    100,
    "target"
);

tx.commit();
```

The `db.exec()` calls may use different pooled connections. They are not guaranteed to be part of the transaction started by `tx`.

When a transaction is active, use the transaction connection.

## Nested transactions

The basic Vix transaction wrapper represents one transaction on one connection.

Do not assume nested transactions are supported by simply creating a second `Transaction` object.

```cpp
vix::db::Transaction outer(db.pool());

// avoid creating another unrelated transaction here
// unless you explicitly want another connection and another transaction

outer.commit();
```

If an application needs savepoints or nested transaction semantics, treat that as an engine-specific design decision and implement it deliberately.

Do not rely on accidental nesting.

## Migrations and transactions

Migration tools often use transactions to keep schema changes safe.

The exact behavior depends on the database engine and the SQL being executed. Some schema changes may auto-commit depending on the database.

For application migrations, the principle is simple: one migration should either apply correctly or stop the process.

Do not continue applying later migrations after a failed migration.

## Testing transactions

SQLite is useful for testing transaction behavior.

A test can create a database file, run a transaction, intentionally skip `commit()`, and verify that the data was not persisted.

```cpp
auto db = vix::db::Database::sqlite("storage/test_tx.db");

db.exec(
    "CREATE TABLE IF NOT EXISTS users ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "email TEXT NOT NULL UNIQUE"
    ")"
);

{
    vix::db::Transaction tx(db.pool());

    auto& conn = tx.conn();

    auto stmt = conn.prepare(
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

The expected count is `0`.

## Common mistakes

### Forgetting commit

```cpp
vix::db::Transaction tx(db.pool());

// writes

// missing tx.commit()
```

The transaction rolls back.

This is safe, but it may surprise you if you expected the writes to persist.

### Calling `db.exec()` inside a transaction

Use `tx.conn()` for transactional statements.

`db.exec()` is for normal operations that can acquire their own pooled connection.

### Holding the transaction too long

A transaction holds a pooled connection.

Keep it short.

### Doing external work inside the transaction

Avoid network calls, file processing, long computation, or waiting for user input inside an open transaction.

### Ignoring affected rows

If the operation expects one row to change, check it before committing.

### Assuming nested transactions work automatically

Do not create nested transactions casually.

Use explicit engine-specific savepoint logic if the project needs that behavior.

## Recommended style

Start the transaction close to the first database operation that needs it.

Use `auto& conn = tx.conn()`.

Prepare and execute statements through that connection.

Check affected rows when the operation depends on them.

Call `commit()` only at the successful end.

Let automatic rollback handle exceptions and early returns.

Keep the transaction short.

## Next steps

Read the migrations page to see how schema changes are applied and tracked.

Read the connection pool page to understand why a transaction holds one pooled connection.

Read the queries page if you need more detail on prepared statements, result sets, and row access.
