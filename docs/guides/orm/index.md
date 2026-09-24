# ORM

Vix ORM is an explicit C++ ORM layer built on top of `vix::db`.

It is designed for applications that want structured database access without hiding SQL, database connections, transactions, or mapping rules behind runtime magic.

The public header is:

```cpp
#include <vix/orm.hpp>
```

The ORM module provides a small set of building blocks:

```txt
Entity
Mapper<T>
Repository<T>
QueryBuilder
UnitOfWork
```

These pieces work together with the Vix database module. `vix::db` provides the database connections, prepared statements, result sets, transactions, and connection pool. `vix::orm` adds a clear application-level structure on top.

## What Vix ORM is

Vix ORM is a lightweight mapping and repository layer for C++ applications.

It helps you write code like this:

```cpp
auto users = vix::orm::repository<User>(db, "users");

auto id = users.create(User{
    0,
    "ada@example.com",
    "Ada"
});

auto user = users.findById(static_cast<std::int64_t>(id));
```

But the mapping remains explicit. You decide how a `User` is read from a database row. You decide which fields are inserted. You decide which fields are updated.

There is no runtime reflection.
There is no hidden metadata system.
There is no automatic persistence.
There is no invisible dirty tracking.
The ORM exists to reduce repetitive repository code while keeping the database model readable.

## What Vix ORM is not

Vix ORM is not trying to turn C++ into a dynamic Active Record framework.
It does not inspect your struct fields automatically.
It does not generate schema from classes at runtime.
It does not silently save objects when they change.
It does not hide transactions.
It does not make SQL disappear.
That design is intentional.

C++ developers often need predictable behavior, explicit ownership, and clear failure boundaries. Vix ORM follows that style. It gives you repository helpers and mapping conventions, but it keeps the important parts visible.

## Relationship with vix::db

The ORM depends on `vix::db`.

A normal Vix database application can start with only the database module:

```cpp
#include <vix.hpp>
#include <vix/db.hpp>

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

    db.exec(
        "INSERT INTO users (email, name) VALUES (?, ?)",
        "ada@example.com",
        "Ada"
    );

    return 0;
}
```

When the application grows, repeating SQL mapping code in every route or service becomes noisy. That is where `vix::orm` helps.

The ORM still uses the same database pool, statements, result rows, and transactions underneath.

```txt
vix::db::Database
  -> ConnectionPool
  -> Connection
  -> Statement
  -> ResultSet

vix::orm
  -> Mapper<T>
  -> Repository<T>
  -> UnitOfWork
```

The database remains the foundation.

The ORM organizes application access to it.

## Main concepts

### Entity

`Entity` is an optional base type for domain objects.

It provides a minimal identity interface:

```cpp
virtual std::int64_t id() const noexcept;
virtual void setId(std::int64_t value) noexcept;
```

You can inherit from it when you want a common identity model.

You can also ignore it and use plain C++ structs.

```cpp
struct User
{
    std::int64_t id{};
    std::string email;
    std::string name;
};
```

Vix ORM does not force your domain objects to inherit from a framework type.

### `Mapper<T>`

`Mapper<T>` defines how an entity maps to database rows and fields.

A mapper answers three questions:

```txt
How do I create T from a ResultRow?
Which fields are inserted?
Which fields are updated?
```

Example:

```cpp
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
```

This is the most important idea in the module.

The mapper is explicit. The developer controls the mapping.

### `Repository<T>`

`Repository<T>` provides a small CRUD API for one entity type and one table.

```cpp
auto users = vix::orm::repository<User>(db, "users");
```

A repository supports common operations:

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

The repository assumes the table has a primary key column named `id`.

It uses `Mapper<T>` to convert between database rows and C++ values.

### QueryBuilder

`QueryBuilder` is a lightweight SQL builder.

It helps construct SQL incrementally while keeping parameters separate from the SQL text.

It is not a full query DSL.

It does not try to replace SQL.

It is useful when a query has optional filters.

```cpp
vix::orm::QueryBuilder qb;

qb.raw("SELECT id, email, name FROM users WHERE 1=1");

if (!email.empty())
{
    qb.raw(" AND email = ?");
    qb.param(email);
}
```

The SQL stays visible. The values stay bindable.

### UnitOfWork

`UnitOfWork` groups several operations into one transaction.

It is built on top of `vix::db::Transaction`.

```cpp
auto work = vix::orm::unit_of_work(db);

auto& conn = work.conn();

// operations using the same transaction connection

work.commit();
```

If the work is not committed, the underlying transaction rolls back.

This makes transaction boundaries explicit and safe.

## A small complete example

This example creates a `User` type, maps it with `Mapper<User>`, and uses a repository.

```cpp
#include <cstdint>
#include <optional>
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

The repository generates the basic SQL for the operation. The mapper controls how the entity is converted.

The table schema is still explicit.

The database connection is still owned by `vix::db`.

The output is still ordinary Vix output.

## Why mapping is explicit

C++ does not have built-in runtime reflection for normal structs in the same way dynamic languages do.

Some frameworks work around that with macros, code generation, metadata registration, or runtime type systems.

Vix ORM does not start there.

The default model is explicit specialization:

```cpp
template <>
struct vix::orm::Mapper<User>
{
    // mapping code
};
```

This is more verbose than automatic reflection, but it has important advantages.

The compiler sees the mapping.
The developer controls every field.
There is no hidden column name inference.
There is no surprising persistence behavior.
The mapping can be reviewed like normal C++ code.

That fits the design of Vix: simple abstractions, explicit control, and predictable runtime behavior.

## Repository design

`Repository<T>` is intentionally small.

It handles common CRUD operations for a table with an `id` primary key.

It does not try to cover every query shape.

For custom queries, use `vix::db` directly or use `QueryBuilder`.

This is a good design because real applications often need queries that do not fit generic CRUD.

A repository can use both approaches:

```cpp
class UserService
{
public:
    explicit UserService(vix::db::Database& db)
        : db_(db),
          users_(vix::orm::repository<User>(db, "users"))
    {
    }

    std::optional<User> findById(std::int64_t id)
    {
        return users_.findById(id);
    }

    std::uint64_t countByDomain(const std::string& domain)
    {
        auto rows = db_.query(
            "SELECT COUNT(*) FROM users WHERE email LIKE ?",
            "%@" + domain
        );

        if (!rows->next())
        {
            return 0;
        }

        return static_cast<std::uint64_t>(
            rows->row().getInt64(0)
        );
    }

private:
    vix::db::Database& db_;
    vix::orm::Repository<User> users_;
};
```

Use the repository for standard operations.

Use direct SQL for queries where SQL is clearer.

## Unit of work and transactions

A `UnitOfWork` starts a transaction and exposes the underlying connection.

This is important because every operation inside a transaction must use the same connection.

```cpp
auto work = vix::orm::unit_of_work(db);

auto& conn = work.conn();

auto insert = conn.prepare(
    "INSERT INTO users (email, name) VALUES (?, ?)"
);

insert->bind(1, "grace@example.com");
insert->bind(2, "Grace Hopper");
insert->exec();

work.commit();
```

The unit of work is explicit.

The transaction starts when the object is created.

The transaction commits when `commit()` is called.

If `commit()` is not called, the underlying transaction rolls back.

This gives the ORM a safe transactional pattern without hiding the database connection.

## QueryBuilder and explicit SQL

`QueryBuilder` is useful when SQL is partly dynamic.

Example:

```cpp
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

auto conn = db.pool().acquire();
auto stmt = conn->prepare(qb.sql());

qb.bind(*stmt);

auto rows = stmt->query();

while (rows->next())
{
    auto user = vix::orm::Mapper<User>::fromRow(rows->row());

    vix::print(
        vix::options{.sep = " | "},
        user.id,
        user.email,
        user.name
    );
}

db.pool().release(std::move(conn));
```

For normal code, prefer `PooledConn` instead of manual `acquire()` and `release()`:

```cpp
vix::db::PooledConn conn(db.pool());

auto stmt = conn->prepare(qb.sql());
qb.bind(*stmt);
```

That way the connection returns to the pool automatically.

## Build requirements

Because Vix ORM depends on `vix::db`, the selected database backend must be enabled at build time.

For SQLite:

```bash
vix build --with-sqlite
```

For MySQL:

```bash
vix build --with-mysql
```

For one-command build and run:

```bash
vix run --with-sqlite
```

or:

```bash
vix run --with-mysql
```

The ORM layer does not replace the database backend. It uses it.

## Recommended project structure

A project using ORM can keep domain types and database mappings close, but not mixed randomly throughout the codebase.

One possible structure is:

```txt
src/
├── domain/
│   └── User.hpp
├── infrastructure/
│   └── persistence/
│       ├── UserMapper.hpp
│       └── UserRepository.hpp
└── main.cpp
```

`domain/User.hpp` contains the domain type.

`UserMapper.hpp` contains the `Mapper<User>` specialization.

`UserRepository.hpp` can wrap `vix::orm::Repository<User>` and add application-specific queries.

Keep SQL and mapping near the persistence layer.

Keep route handlers or command handlers focused on application behavior.

## When to use Vix ORM

Use Vix ORM when a project has repeated CRUD code and clear entity types.

It is a good fit for:

| Use case                | Why it fits                                               |
| ----------------------- | --------------------------------------------------------- |
| Backend APIs            | Repositories keep route handlers smaller.                 |
| Admin tools             | CRUD operations map naturally to repositories.            |
| Local apps              | SQLite plus repositories gives a simple local data layer. |
| Service layers          | Mapping stays explicit and testable.                      |
| Transactional workflows | UnitOfWork gives one clear transaction boundary.          |

Do not use the ORM just because it exists.

For one or two queries, `vix::db` may be simpler.

For complex reporting queries, direct SQL may be clearer.

For custom joins, aggregates, or engine-specific SQL, use `vix::db` directly or combine it with `QueryBuilder`.

## Recommended style

Use `#include <vix/orm.hpp>` as the public ORM include.

Keep entities simple.

Specialize `Mapper<T>` explicitly.

Use repositories for standard CRUD.

Use `vix::db` directly for custom SQL.

Use `QueryBuilder` for dynamic SQL with bind parameters.

Use `UnitOfWork` for operations that must commit or roll back together.

Do not hide transaction boundaries.

Do not rely on automatic persistence.

Do not spread SQL across route handlers.

## Related pages

Read the quick start to build a complete first ORM example.

Read the mappers page before using repositories seriously.

Read the repositories page for CRUD operations.

Read the query builder page for dynamic SQL.

Read the unit of work page for transactions.

Read the database guide if you need to understand the lower-level `vix::db` layer.
