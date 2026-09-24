# Queries

Vix database queries are intentionally close to SQL.

The database module does not invent a new query language and does not hide the database behind heavy abstractions. You write SQL, pass values as bind parameters, and read rows through a small C++ API.

For most application code, the two main operations are:

```cpp
db.exec(...);
db.query(...);
```

Use `exec()` when the SQL statement does not return rows.

Use `query()` when the SQL statement returns rows.

Both APIs use prepared statements internally, so dynamic values should be passed as arguments instead of being concatenated into the SQL string.

## Include the database API

Use:

```cpp
#include <vix/db.hpp>
```

If the example also prints output, include the common Vix header or the print header:

```cpp
#include <vix.hpp>
#include <vix/db.hpp>
```

For direct print usage:

```cpp
#include <vix/print.hpp>
```

## Open a database

The examples on this page use SQLite because it is easy to run locally.

```cpp
auto db = vix::db::Database::sqlite("storage/app.db");
```

The same `exec()` and `query()` style is used with MySQL:

```cpp
auto db = vix::db::Database::mysql(
    "tcp://127.0.0.1:3306",
    "root",
    "secret",
    "app_db"
);
```

The SQL dialect still belongs to the selected engine. SQLite SQL and MySQL SQL are not always identical.

## `exec()`

Use `exec()` for statements that change database state or do not return rows.

Typical examples:

```txt
CREATE TABLE
INSERT
UPDATE
DELETE
DROP TABLE
ALTER TABLE
```

Example:

```cpp
db.exec(
    "CREATE TABLE IF NOT EXISTS users ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "email TEXT NOT NULL UNIQUE,"
    "name TEXT NOT NULL"
    ")"
);
```

The return value is the number of affected rows when the driver supports it.

```cpp
auto affected = db.exec(
    "UPDATE users SET name = ? WHERE email = ?",
    "Ada Lovelace",
    "ada@example.com"
);

vix::print("updated rows:", affected);
```

For schema statements such as `CREATE TABLE`, affected rows may not be meaningful. For `UPDATE` and `DELETE`, it is often useful.

## `query()`

Use `query()` for statements that return rows.

```cpp
auto rows = db.query(
    "SELECT id, email, name FROM users ORDER BY id"
);
```

A result set is forward-only. Call `next()` to move to the next row.

```cpp
while (rows->next())
{
    const auto& row = rows->row();

    auto id = row.getInt64(0);
    auto email = row.getString(1);
    auto name = row.getString(2);

    vix::print(vix::options{.sep = " | "}, id, email, name);
}
```

Column indexes start at `0`.

The indexes follow the order in the `SELECT` list:

```sql
SELECT id, email, name FROM users
```

So the row is read as:

```txt
0 -> id
1 -> email
2 -> name
```

Prefer explicit column lists in application queries. Avoid `SELECT *` in application code because it makes row access depend on table structure.

## Bind parameters

Values passed after the SQL string are bound as positional parameters.

```cpp
db.exec(
    "INSERT INTO users (email, name) VALUES (?, ?)",
    "grace@example.com",
    "Grace Hopper"
);
```

The first value binds to the first `?`.

The second value binds to the second `?`.

This also works with `query()`:

```cpp
auto rows = db.query(
    "SELECT id, email, name FROM users WHERE email = ?",
    "grace@example.com"
);
```

Do not concatenate user input into SQL strings.

Prefer this:

```cpp
db.exec(
    "DELETE FROM users WHERE email = ?",
    email
);
```

Avoid this:

```cpp
db.exec(
    "DELETE FROM users WHERE email = '" + email + "'"
);
```

Prepared statements keep SQL structure separate from values. The code is easier to read and the driver handles value binding.

## Supported bind values

At the lower level, Vix database values are represented by `vix::db::DbValue`.

Common application values can be bound directly:

```cpp
db.exec("INSERT INTO metrics (name, value) VALUES (?, ?)", "requests", 42);
db.exec("INSERT INTO prices (label, amount) VALUES (?, ?)", "plan", 19.99);
db.exec("INSERT INTO flags (name, enabled) VALUES (?, ?)", "beta", true);
```

For explicit null values at the lower-level statement API, use `bindNull()`:

```cpp
auto conn = db.pool().acquire();
auto stmt = conn->prepare("INSERT INTO users (email, name) VALUES (?, ?)");

stmt->bind(1, "anonymous@example.com");
stmt->bindNull(2);
stmt->exec();

db.pool().release(std::move(conn));
```

Most application code should use the high-level `Database` helpers unless it needs direct control over the connection lifetime.

## Insert rows

Use `exec()` for inserts.

```cpp
db.exec(
    "INSERT INTO users (email, name) VALUES (?, ?)",
    "ada@example.com",
    "Ada"
);
```

For SQLite examples where the same file may be reused across runs, `INSERT OR IGNORE` can be useful:

```cpp
db.exec(
    "INSERT OR IGNORE INTO users (email, name) VALUES (?, ?)",
    "ada@example.com",
    "Ada"
);
```

For MySQL, the equivalent pattern is often `ON DUPLICATE KEY UPDATE`:

```cpp
db.exec(
    "INSERT INTO users (email, name) VALUES (?, ?) "
    "ON DUPLICATE KEY UPDATE name = VALUES(name)",
    "ada@example.com",
    "Ada Lovelace"
);
```

Use these patterns only when duplicate handling is intended by the application.

## Read one row

A query may return zero or more rows.

When the application expects one optional row, use `if`.

```cpp
auto rows = db.query(
    "SELECT id, email, name FROM users WHERE email = ?",
    "ada@example.com"
);

if (rows->next())
{
    const auto& row = rows->row();

    vix::print_inline("User: ");
    vix::print(
        vix::options{.sep = " | "},
        row.getInt64(0),
        row.getString(1),
        row.getString(2)
    );
}
else
{
    vix::print("User not found");
}
```

If the application expects exactly one row, decide what should happen when no row exists.

For a repository method, returning `std::optional<User>` is often better than printing directly.

## Read many rows

Use `while`.

```cpp
auto rows = db.query(
    "SELECT id, email, name FROM users ORDER BY id"
);

while (rows->next())
{
    const auto& row = rows->row();

    auto id = row.getInt64(0);
    auto email = row.getString(1);
    auto name = row.getString(2);

    vix::print(vix::options{.sep = " | "}, id, email, name);
}
```

The result set keeps the database resources needed for the query. Read rows while the result set is alive.

Do not store references to `row` beyond the current iteration.

## Read counts and scalar values

A scalar query returns one row with one column.

```cpp
auto count = db.query("SELECT COUNT(*) FROM users");

if (count->next())
{
    vix::print("Remaining users:", count->row().getInt64(0));
}
```

For clarity, keep the SQL simple when reading counts, sums, or existence checks.

```cpp
auto rows = db.query(
    "SELECT COUNT(*) FROM users WHERE email = ?",
    "ada@example.com"
);

if (rows->next())
{
    auto exists = rows->row().getInt64(0) > 0;
    vix::print("exists:", exists);
}
```

## Update rows

Use `exec()` for updates.

```cpp
auto affected = db.exec(
    "UPDATE users SET name = ? WHERE email = ?",
    "Ada Lovelace",
    "ada@example.com"
);

vix::print("updated rows:", affected);
```

For important updates, check the affected row count.

```cpp
if (affected != 1)
{
    vix::print("expected one updated row, got:", affected);
}
```

A result of `0` can mean the target row does not exist.

A result greater than `1` can mean the `WHERE` clause is too broad.

## Delete rows

Use `exec()` for deletes.

```cpp
auto affected = db.exec(
    "DELETE FROM users WHERE email = ?",
    "grace@example.com"
);

vix::print("deleted rows:", affected);
```

For destructive operations, prefer primary keys or unique columns in the condition.

```cpp
db.exec(
    "DELETE FROM users WHERE id = ?",
    userId
);
```

Avoid application code that can accidentally produce a delete without a `WHERE` clause.

## Nullable columns

Use `isNull()` when `NULL` has meaning.

```cpp
auto rows = db.query(
    "SELECT id, nickname FROM users WHERE id = ?",
    1
);

if (rows->next())
{
    const auto& row = rows->row();

    if (row.isNull(1))
    {
        vix::print("nickname: null");
    }
    else
    {
        vix::print("nickname:", row.getString(1));
    }
}
```

Use the `Or` helpers when a default value is acceptable.

```cpp
auto nickname = row.getStringOr(1, "anonymous");
auto score = row.getInt64Or(2, 0);
auto ratio = row.getDoubleOr(3, 0.0);
```

This keeps simple read paths clean without losing the ability to handle `NULL` explicitly where needed.

## Row accessor methods

`ResultRow` exposes typed accessors.

```cpp
row.getString(0);
row.getInt64(1);
row.getDouble(2);
```

And nullable helpers:

```cpp
row.getStringOr(0, "default");
row.getInt64Or(1, 0);
row.getDoubleOr(2, 0.0);
```

Use the accessor that matches the shape you expect from the SQL query.

For booleans, many SQL engines store values as integers. Read them as integers and convert in application code when needed.

```cpp
auto enabled = row.getInt64(0) != 0;
```

## Statement API

The high-level `Database` helpers are enough for most queries.

When you need direct control over the connection or transaction, use the lower-level prepared statement API.

```cpp
vix::db::PooledConn conn(db.pool());

auto stmt = conn->prepare(
    "INSERT INTO users (email, name) VALUES (?, ?)"
);

stmt->bind(1, "bjarne@example.com");
stmt->bind(2, "Bjarne");
stmt->exec();
```

Use this style when several statements must share the same connection, especially inside a transaction.

## Queries inside a transaction

A transaction gives several operations one shared database connection.

```cpp
vix::db::Transaction tx(db.pool());

auto& conn = tx.conn();

auto insert = conn.prepare(
    "INSERT INTO accounts (name, balance) VALUES (?, ?)"
);
insert->bind(1, "main");
insert->bind(2, 1000);
insert->exec();

auto update = conn.prepare(
    "UPDATE accounts SET balance = balance - ? WHERE name = ?"
);
update->bind(1, 100);
update->bind(2, "main");
update->exec();

tx.commit();
```

If `commit()` is not called, the transaction rolls back when the `Transaction` object is destroyed.

Use this for multi-step writes where partial success would corrupt the application state.

## Query result lifetime

A result set may depend on the statement and connection that produced it.

The high-level `Database::query()` returns a result set that keeps the needed pooled connection alive while rows are being read.

That is why this is safe:

```cpp
auto rows = db.query(
    "SELECT id, email FROM users"
);

while (rows->next())
{
    const auto& row = rows->row();
    vix::print(vix::options{.sep = " | "}, row.getInt64(0), row.getString(1));
}
```

Do not return row references from a function.

Do not store `const ResultRow&` in a container.

Read the values you need and copy them into your own domain type.

## Mapping rows to application types

For real application code, map database rows to your own types.

```cpp
struct User
{
    std::int64_t id{};
    std::string email;
    std::string name;
};

User readUser(const vix::db::ResultRow& row)
{
    User user;

    user.id = row.getInt64(0);
    user.email = row.getString(1);
    user.name = row.getString(2);

    return user;
}
```

Use it like this:

```cpp
auto rows = db.query(
    "SELECT id, email, name FROM users WHERE email = ?",
    "ada@example.com"
);

if (rows->next())
{
    auto user = readUser(rows->row());

    vix::print(
        vix::options{.sep = " | "},
        user.id,
        user.email,
        user.name
    );
}
```

This keeps SQL row access in one place.

## Repository example

A repository keeps database access out of HTTP handlers or application commands.

```cpp
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
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
    explicit UserRepository(vix::db::Database& db)
        : db_(db)
    {
    }

    void create(std::string email, std::string name)
    {
        db_.exec(
            "INSERT INTO users (email, name) VALUES (?, ?)",
            std::move(email),
            std::move(name)
        );
    }

    std::optional<User> findByEmail(const std::string& email)
    {
        auto rows = db_.query(
            "SELECT id, email, name FROM users WHERE email = ?",
            email
        );

        if (!rows->next())
        {
            return std::nullopt;
        }

        const auto& row = rows->row();

        return User{
            row.getInt64(0),
            row.getString(1),
            row.getString(2)
        };
    }

    std::uint64_t renameByEmail(
        const std::string& email,
        const std::string& name
    )
    {
        return db_.exec(
            "UPDATE users SET name = ? WHERE email = ?",
            name,
            email
        );
    }

private:
    vix::db::Database& db_;
};
```

The route layer can then call repository methods instead of carrying SQL directly.

## Error handling

Database operations can throw exceptions.

Handle errors at the boundary where the application can make a useful decision.

```cpp
try
{
    db.exec(
        "INSERT INTO users (email, name) VALUES (?, ?)",
        "ada@example.com",
        "Ada"
    );
}
catch (const std::exception& e)
{
    vix::print("database error:", e.what());
}
```

For a CLI tool, printing the error and returning a non-zero exit code may be enough.

For an HTTP API, convert the error into a response.

For a migration command, stop immediately rather than continuing after a failed statement.

## SQLite and MySQL query differences

The Vix C++ API is consistent, but SQL dialects still differ.

SQLite example:

```sql
CREATE TABLE users (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  email TEXT NOT NULL UNIQUE,
  name TEXT NOT NULL
);
```

MySQL example:

```sql
CREATE TABLE users (
  id BIGINT AUTO_INCREMENT PRIMARY KEY,
  email VARCHAR(255) NOT NULL UNIQUE,
  name VARCHAR(255) NOT NULL
) ENGINE=InnoDB;
```

Application queries such as simple `SELECT`, `INSERT`, `UPDATE`, and `DELETE` can often look similar, but schema SQL and advanced features can differ.

Keep migrations engine-specific when needed.

## Printing query results

`vix::print` does not use `{}` placeholders like `fmt::print`.

Use multiple arguments:

```cpp
vix::print("updated rows:", affected);
```

Use a custom separator when printing columns:

```cpp
vix::print(vix::options{.sep = " | "}, id, email, name);
```

Use `print_inline()` when you want a label before a custom-separated row:

```cpp
vix::print_inline("Updated user: ");
vix::print(vix::options{.sep = " | "}, id, email, name);
```

Do not write:

```cpp
vix::print("{} | {} | {}", id, email, name);
```

That is not the Vix print style.

## Common mistakes

### Concatenating values into SQL

Avoid this:

```cpp
db.query(
    "SELECT id FROM users WHERE email = '" + email + "'"
);
```

Use bind parameters:

```cpp
db.query(
    "SELECT id FROM users WHERE email = ?",
    email
);
```

### Using `SELECT *`

Avoid this in application code:

```cpp
auto rows = db.query("SELECT * FROM users");
```

Prefer explicit columns:

```cpp
auto rows = db.query(
    "SELECT id, email, name FROM users"
);
```

This keeps row indexes stable.

### Reading a row before calling `next()`

Do not call `row()` before `next()` succeeds.

Wrong:

```cpp
auto rows = db.query("SELECT id FROM users");
auto id = rows->row().getInt64(0);
```

Correct:

```cpp
auto rows = db.query("SELECT id FROM users");

if (rows->next())
{
    auto id = rows->row().getInt64(0);
    vix::print("id:", id);
}
```

### Keeping row references too long

Do not store a `ResultRow` reference beyond the current result-set iteration.

Wrong:

```cpp
const vix::db::ResultRow* saved = nullptr;

while (rows->next())
{
    saved = &rows->row();
}
```

Copy values into your own type instead.

### Ignoring affected rows

For important updates and deletes, check the result.

```cpp
auto affected = db.exec(
    "DELETE FROM users WHERE id = ?",
    userId
);

if (affected != 1)
{
    vix::print("unexpected deleted rows:", affected);
}
```

### Mixing dialects

Do not use SQLite table syntax in a MySQL migration, or MySQL-specific syntax in a SQLite migration.

Keep SQL aligned with the selected engine.

## Recommended style

Use `exec()` for statements that do not return rows.

Use `query()` for statements that return rows.

Use bind parameters for dynamic values.

Use explicit column lists.

Use `if` when reading one optional row.

Use `while` when reading many rows.

Copy row values into application types.

Use transactions for multi-step writes.

Keep SQL inside repositories or data-access classes in larger applications.

## Next steps

Read the connection pool page to understand how Vix reuses database connections.

Read the transactions page before writing multi-step database changes.

Read the migrations page when your schema needs to evolve over time.
