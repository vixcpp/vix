# Repositories

A repository is the main way to use Vix ORM for common CRUD operations.

In Vix ORM, a repository connects one C++ type to one database table.

```cpp
auto users = vix::orm::repository<User>(db, "users");
```

The repository uses:

```txt
User
Mapper<User>
users table
vix::db::Database
vix::db::ConnectionPool
```

The repository does not replace the database. It sits above `vix::db` and uses the database pool, prepared statements, result sets, and mapper rules.

The public ORM header is:

```cpp
#include <vix/orm.hpp>
```

## What a repository does

A repository provides a small CRUD API:

```txt
create
findById
findAll
existsById
count
updateById
removeById
removeAll
```

The repository assumes that the table has a primary key column named:

```txt
id
```

It also assumes that the entity type has a `Mapper<T>` specialization.

The repository does not create tables automatically. Use SQL or migrations for schema creation.

## Minimal example

```cpp
#include <cstdint>
#include <string>
#include <vix.hpp>
#include <vix/db.hpp>
#include <vix/orm.hpp>

struct User
{
    std::int64_t id{};
    std::string email;
    std::string name;
};

template <>
struct vix::orm::Mapper<User>
{
    static User fromRow(const vix::db::ResultRow& row)
    {
        return User{
            row.getInt64(0),
            row.getString(1),
            row.getString(2)
        };
    }

    static vix::orm::FieldValues toInsertFields(const User& user)
    {
        return {
            {"email", user.email},
            {"name", user.name}
        };
    }

    static vix::orm::FieldValues toUpdateFields(const User& user)
    {
        return {
            {"email", user.email},
            {"name", user.name}
        };
    }
};

int main()
{
    auto db = vix::db::Database::sqlite("storage/app.db");

    db.exec(
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "email TEXT NOT NULL UNIQUE,"
        "name TEXT NOT NULL"
        ")"
    );

    auto users = vix::orm::repository<User>(db, "users");

    auto id = users.create(User{
        0,
        "ada@example.com",
        "Ada"
    });

    auto user = users.findById(static_cast<std::int64_t>(id));

    if (user)
    {
        vix::print(
            vix::options{.sep = " | "},
            user->id,
            user->email,
            user->name
        );
    }

    return 0;
}
```

This example shows the whole repository flow.

The database owns the connection pool.

The repository uses the pool.

The mapper controls conversion.

The table schema remains explicit.

## Creating a repository

The preferred high-level form is:

```cpp
auto users = vix::orm::repository<User>(db, "users");
```

The first argument is a `vix::db::Database`.

The second argument is the table name.

You can also create a repository from a connection pool:

```cpp
auto users = vix::orm::repository<User>(db.pool(), "users");
```

The public alias is:

```cpp
vix::orm::Repository<User>
```

which maps to the default repository implementation.

```cpp
vix::orm::Repository<User> users(db.pool(), "users");
```

For most application code, prefer the helper:

```cpp
auto users = vix::orm::repository<User>(db, "users");
```

It is shorter and keeps the database facade as the main object.

## Table name

The table name is passed explicitly.

```cpp
auto users = vix::orm::repository<User>(db, "users");
```

The repository does not infer table names from C++ types.

This is intentional.

A type named `User` may map to a table called `users`, `app_users`, `accounts`, or something project-specific. Vix does not guess.

If the table name is empty, repository construction fails.

Use clear table names and keep them close to the persistence code.

## create

`create()` inserts a new entity.

```cpp
auto id = users.create(User{
    0,
    "grace@example.com",
    "Grace Hopper"
});
```

The repository asks the mapper for insert fields:

```cpp
Mapper<User>::toInsertFields(user)
```

For the `User` example:

```cpp
return {
    {"email", user.email},
    {"name", user.name}
};
```

The repository builds an insert statement from those fields.

The SQL shape is:

```sql
INSERT INTO users (email,name) VALUES (?,?)
```

The values are bound as prepared statement parameters.

`create()` returns the last inserted id reported by the connection.

When the database owns the primary key, do not include `id` in `toInsertFields()`.

## findById

`findById()` reads one row by primary key.

```cpp
auto user = users.findById(1);
```

The return type is:

```cpp
std::optional<User>
```

If a row exists, the repository returns the mapped entity.

If no row exists, it returns `std::nullopt`.

```cpp
auto user = users.findById(1);

if (!user)
{
    vix::print("user not found");
    return;
}

vix::print("email:", user->email);
```

The repository uses `Mapper<User>::fromRow()` to materialize the entity.

## findAll

`findAll()` loads all rows from the table.

```cpp
auto allUsers = users.findAll();

for (const auto& user : allUsers)
{
    vix::print(
        vix::options{.sep = " | "},
        user.id,
        user.email,
        user.name
    );
}
```

Use `findAll()` for:

```txt
small tables
examples
tests
admin screens
local tools
```

Do not use `findAll()` blindly on large tables.

For large tables, write a custom query with a `LIMIT`, pagination, filters, and indexes.

## existsById

`existsById()` checks whether a row exists.

```cpp
if (users.existsById(1))
{
    vix::print("user exists");
}
```

Use this when the application only needs to know if a row exists and does not need to load the full entity.

This can be cleaner than calling `findById()` and ignoring the entity.

## count

`count()` returns the number of rows in the table.

```cpp
auto total = users.count();

vix::print("users:", total);
```

This is useful for simple checks, tests, admin tools, and local examples.

For filtered counts, write a custom query with `vix::db`:

```cpp
auto rows = db.query(
    "SELECT COUNT(*) FROM users WHERE email LIKE ?",
    "%@example.com"
);

if (rows->next())
{
    vix::print("example.com users:", rows->row().getInt64(0));
}
```

The generic repository count is intentionally simple.

## updateById

`updateById()` updates one row by primary key.

```cpp
User user{
    1,
    "ada@example.com",
    "Ada Lovelace"
};

auto affected = users.updateById(1, user);
```

The repository asks the mapper for update fields:

```cpp
Mapper<User>::toUpdateFields(user)
```

For the `User` example:

```cpp
return {
    {"email", user.email},
    {"name", user.name}
};
```

The SQL shape is:

```sql
UPDATE users SET email=?,name=? WHERE id=?
```

Check the affected row count when the application expects exactly one row to change.

```cpp
auto affected = users.updateById(user.id, user);

if (affected != 1)
{
    vix::print("unexpected updated rows:", affected);
}
```

Do not include immutable fields in `toUpdateFields()` unless the application deliberately updates them.

## removeById

`removeById()` deletes one row by primary key.

```cpp
auto affected = users.removeById(1);

vix::print("deleted rows:", affected);
```

For important deletes, check the affected count.

```cpp
if (affected != 1)
{
    vix::print("unexpected deleted rows:", affected);
}
```

A result of `0` usually means the row did not exist.

## removeAll

`removeAll()` deletes every row in the table.

```cpp
auto affected = users.removeAll();

vix::print("deleted rows:", affected);
```

Use this carefully.

It is useful for tests, local examples, and controlled admin tools.

It is dangerous in production application code unless the operation is deliberate and protected.

Avoid exposing `removeAll()` behind a normal user-facing route.

## Repository and `Mapper<T>`

The repository depends on `Mapper<T>` for all object conversion.

When you call:

```cpp
users.create(user);
```

the repository calls:

```cpp
Mapper<User>::toInsertFields(user);
```

When you call:

```cpp
users.updateById(id, user);
```

the repository calls:

```cpp
Mapper<User>::toUpdateFields(user);
```

When you call:

```cpp
users.findById(id);
users.findAll();
```

the repository calls:

```cpp
Mapper<User>::fromRow(row);
```

If repository behavior looks wrong, inspect the mapper first.

Most ORM mistakes are mapping mistakes.

## Repository and table schema

The repository assumes an `id` primary key.

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

If your table uses a different primary key name, the generic repository is not the right abstraction for that table.

Use `vix::db` directly or write a project-specific repository.

## Repository and `SELECT \*`

The generic repository reads rows using the table shape.

That means `Mapper<T>::fromRow()` should match the order returned by the table.

For a table defined as:

```sql
CREATE TABLE users (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  email TEXT NOT NULL UNIQUE,
  name TEXT NOT NULL
);
```

the mapper can read:

```cpp
return User{
    row.getInt64(0),
    row.getString(1),
    row.getString(2)
};
```

For custom queries, prefer explicit column lists:

```cpp
auto rows = db.query(
    "SELECT id, email, name FROM users WHERE email = ?",
    email
);
```

and reuse the mapper only when the selected order matches the mapper.

## Custom repository wrapper

In real applications, wrap the generic repository in a project-specific class.

```cpp
#include <cstdint>
#include <optional>
#include <string>
#include <vix/db.hpp>
#include <vix/orm.hpp>

class UserRepository
{
public:
    explicit UserRepository(vix::db::Database& db)
        : db_(db),
          users_(vix::orm::repository<User>(db, "users"))
    {
    }

    std::uint64_t create(User user)
    {
        return users_.create(user);
    }

    std::optional<User> findById(std::int64_t id)
    {
        return users_.findById(id);
    }

    std::uint64_t updateById(std::int64_t id, User user)
    {
        return users_.updateById(id, user);
    }

    std::uint64_t removeById(std::int64_t id)
    {
        return users_.removeById(id);
    }

private:
    vix::db::Database& db_;
    vix::orm::Repository<User> users_;
};
```

This gives you a place to add custom queries without putting SQL into route handlers.

## Custom query in a repository

The generic repository should not try to cover every query.

For application-specific queries, use `vix::db` inside your wrapper.

```cpp
std::optional<User> findByEmail(const std::string& email)
{
    auto rows = db_.query(
        "SELECT id, email, name FROM users WHERE email = ? LIMIT 1",
        email
    );

    if (!rows->next())
    {
        return std::nullopt;
    }

    return vix::orm::Mapper<User>::fromRow(rows->row());
}
```

This keeps the custom SQL explicit and still reuses the mapper.

## Pagination example

For tables that can grow, use pagination.

```cpp
std::vector<User> findPage(std::int64_t limit, std::int64_t offset)
{
    auto rows = db_.query(
        "SELECT id, email, name FROM users ORDER BY id LIMIT ? OFFSET ?",
        limit,
        offset
    );

    std::vector<User> out;

    while (rows->next())
    {
        out.push_back(vix::orm::Mapper<User>::fromRow(rows->row()));
    }

    return out;
}
```

This is better than calling `findAll()` on a large table.

## Filter example

Use custom SQL for filtered queries.

```cpp
std::vector<User> findByDomain(const std::string& domain)
{
    auto rows = db_.query(
        "SELECT id, email, name FROM users WHERE email LIKE ? ORDER BY id",
        "%@" + domain
    );

    std::vector<User> out;

    while (rows->next())
    {
        out.push_back(vix::orm::Mapper<User>::fromRow(rows->row()));
    }

    return out;
}
```

Add an index when a filter becomes important for performance.

## Using QueryBuilder inside a repository

Use `QueryBuilder` when filters are optional.

```cpp
std::vector<User> search(
    const std::string& email,
    const std::string& name
)
{
    vix::orm::QueryBuilder qb;

    qb.raw("SELECT id, email, name FROM users WHERE 1=1");

    if (!email.empty())
    {
        qb.raw(" AND email = ?");
        qb.param(email);
    }

    if (!name.empty())
    {
        qb.raw(" AND name = ?");
        qb.param(name);
    }

    vix::db::PooledConn conn(db_.pool());

    auto stmt = conn->prepare(qb.sql());
    qb.bind(*stmt);

    auto rows = stmt->query();

    std::vector<User> out;

    while (rows->next())
    {
        out.push_back(vix::orm::Mapper<User>::fromRow(rows->row()));
    }

    return out;
}
```

The SQL remains visible.

The parameters stay separate from the SQL string.

The connection returns to the pool automatically.

## Repositories and transactions

Repository methods acquire their own pooled connection.

That is fine for normal operations.

For a transaction, all operations that must be part of the transaction need to use the same connection.

A simple `Repository<T>` call may not be part of your active transaction if it acquires another connection.

For transactional workflows, use `UnitOfWork` and the transaction connection directly.

```cpp
auto work = vix::orm::unit_of_work(db);

auto& conn = work.conn();

auto insertUser = conn.prepare(
    "INSERT INTO users (email, name) VALUES (?, ?)"
);
insertUser->bind(1, "grace@example.com");
insertUser->bind(2, "Grace Hopper");
insertUser->exec();

auto insertAudit = conn.prepare(
    "INSERT INTO audit_log (message) VALUES (?)"
);
insertAudit->bind(1, "created user grace@example.com");
insertAudit->exec();

work.commit();
```

This ensures both statements use one connection and one transaction.

Do not assume that generic repository calls automatically join the current `UnitOfWork`.

## Transaction-aware repository methods

A project-specific repository can offer methods that accept a connection.

```cpp
void createWithConnection(
    vix::db::Connection& conn,
    const User& user
)
{
    const auto fields = vix::orm::Mapper<User>::toInsertFields(user);

    auto stmt = conn.prepare(
        "INSERT INTO users (email, name) VALUES (?, ?)"
    );

    stmt->bind(1, std::any_cast<std::string>(fields[0].second));
    stmt->bind(2, std::any_cast<std::string>(fields[1].second));
    stmt->exec();
}
```

In many cases, writing the SQL directly is clearer:

```cpp
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
```

Use the simpler version unless generic field binding is truly needed.

## Error handling

Repository operations can throw database errors or standard exceptions.

Handle errors at the application boundary.

```cpp
try
{
    auto id = users.create(User{
        0,
        "ada@example.com",
        "Ada"
    });

    vix::print("created user id:", id);
}
catch (const std::exception& e)
{
    vix::print("repository error:", e.what());
}
```

For a CLI, print the error and return a non-zero exit code.

For an HTTP API, convert the error into a response.

For service code, decide which errors should be retried, reported, or treated as business failures.

## Build requirements

Repositories depend on the database backend.

For SQLite:

```bash
vix build --with-sqlite
```

For MySQL:

```bash
vix build --with-mysql
```

For running directly:

```bash
vix run --with-sqlite
```

or:

```bash
vix run --with-mysql
```

The ORM is not a standalone persistence engine. It uses `vix::db`.

## Common mistakes

### Using a repository without a mapper

This will not work unless `Mapper<User>` is specialized:

```cpp
auto users = vix::orm::repository<User>(db, "users");
```

### Expecting the repository to create tables

The repository does not create schema.

Use migrations or explicit SQL.

### Using findAll on large tables

Use pagination for tables that can grow.

### Assuming repository methods join UnitOfWork automatically

Generic repository methods acquire connections from the pool.

Use `work.conn()` for transaction-bound operations.

### Including id in insert fields when the database generates it

Leave generated ids out of `toInsertFields()`.

### Exposing removeAll carelessly

`removeAll()` deletes all rows in the table.

Keep it away from normal user-facing routes.

### Hiding important SQL behind too many wrappers

Repositories should make application code clearer, not make SQL impossible to find.

## Recommended style

Use `vix::orm::repository<T>(db, "table")` for simple CRUD.

Keep `Mapper<T>` close to the persistence layer.

Wrap generic repositories in project-specific repositories for real applications.

Use direct `vix::db` queries for custom SQL.

Use `QueryBuilder` for dynamic filters.

Use `UnitOfWork` for transaction boundaries.

Avoid `findAll()` on large tables.

Review all destructive operations.

## Next steps

Read the Query Builder page for dynamic SQL with bind parameters.

Read the Unit of Work page before writing transaction-sensitive code.

Read the With vix::db page to understand when to use ORM and when to use direct database access.
