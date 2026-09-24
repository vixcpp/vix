# With vix::db

Vix ORM is built on top of `vix::db`.

That relationship is important. The ORM is not a separate persistence engine. It does not replace the database module. It uses the same database facade, connection pool, prepared statements, result sets, and transactions.

Use `vix::db` when SQL is the clearest tool.

Use `vix::orm` when you want entity mapping, repositories, query building, and unit-of-work transaction structure.

A good Vix application can use both.

```cpp
#include <vix/db.hpp>
#include <vix/orm.hpp>
```

## The layers

The database layer provides the foundation.

```cpp
vix::db::Database
vix::db::ConnectionPool
vix::db::Connection
vix::db::Statement
vix::db::ResultSet
vix::db::ResultRow
vix::db::Transaction
```

The ORM layer builds on top of that foundation.

```cpp
vix::orm::Mapper<T>
vix::orm::Repository<T>
vix::orm::QueryBuilder
vix::orm::UnitOfWork
```

The relationship looks like this:

```txt
application code
  -> vix::orm
      -> Mapper<T>
      -> Repository<T>
      -> QueryBuilder
      -> UnitOfWork
          -> vix::db
              -> Database
              -> ConnectionPool
              -> Connection
              -> Statement
              -> ResultSet
              -> driver
```

The database module owns the actual database work.

The ORM organizes application access to it.

## Start with vix::db

Every ORM-backed application still starts with a database.

```cpp
auto db = vix::db::Database::sqlite("storage/app.db");
```

or:

```cpp
auto db = vix::db::Database::mysql(
    "tcp://127.0.0.1:3306",
    "root",
    "secret",
    "app_db"
);
```

The ORM does not open a database by itself. It receives a database facade or a connection pool.

```cpp
auto users = vix::orm::repository<User>(db, "users");
```

This makes ownership clear.

The application owns the database.

The repository uses the database pool.

## Use vix::db for schema

Vix ORM repositories do not create tables automatically.

Create schema with migrations or explicit SQL.

```cpp
db.exec(
    "CREATE TABLE IF NOT EXISTS users ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "email TEXT NOT NULL UNIQUE,"
    "name TEXT NOT NULL"
    ")"
);
```

For real projects, prefer migrations.

```txt
migrations/
├── 2026_06_02_120000_create_users.up.sql
└── 2026_06_02_120000_create_users.down.sql
```

The ORM assumes the table already exists.

This is deliberate. Schema changes should be explicit and reviewable.

## Use ORM for repeated CRUD

After the table exists and a mapper is defined, use the repository for common operations.

```cpp
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
```

This is the main value of the ORM.

It removes repeated CRUD boilerplate while keeping mapping explicit.

## Use vix::db for custom SQL

Not every query should go through a generic repository.

For custom queries, direct SQL is often clearer.

```cpp
std::optional<User> findByEmail(
    vix::db::Database& db,
    const std::string& email
)
{
    auto rows = db.query(
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

This uses `vix::db` for the query and `vix::orm::Mapper<User>` for conversion.

That combination is often the cleanest approach.

SQL stays visible.

Mapping stays reused.

## Use QueryBuilder for dynamic SQL

When SQL has optional filters, use `QueryBuilder`.

```cpp
std::vector<User> searchUsers(
    vix::db::Database& db,
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
        qb.raw(" AND name LIKE ?");
        qb.param("%" + name + "%");
    }

    qb.raw(" ORDER BY id");

    vix::db::PooledConn conn(db.pool());

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

`QueryBuilder` does not replace SQL. It keeps SQL construction and bind values organized.

## Use UnitOfWork for transactions

When several statements must commit or roll back together, use `UnitOfWork`.

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

A transaction belongs to one connection.

That is why `work.conn()` matters.

Every statement that must participate in the transaction should use that connection.

## Do not mix transaction connections accidentally

This looks like it should be transactional, but it is not guaranteed to be:

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

The repository method may acquire its own connection from the pool. That connection may not be the transaction connection owned by `work`.

For transaction-bound work, use:

```cpp
auto& conn = work.conn();
```

and prepare statements from that connection.

```cpp
auto stmt = conn.prepare(
    "INSERT INTO users (email, name) VALUES (?, ?)"
);

stmt->bind(1, "ada@example.com");
stmt->bind(2, "Ada");
stmt->exec();

work.commit();
```

This keeps the operation inside the unit of work.

## Transaction-aware repositories

For larger projects, write repository methods that accept a connection.

```cpp
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

private:
    vix::db::Database& db_;
    vix::orm::Repository<User> users_;
};
```

Use it inside a unit of work:

```cpp
UserRepository users(db);
auto work = vix::orm::unit_of_work(db);

users.createWithConnection(
    work.conn(),
    User{
        0,
        "linus@example.com",
        "Linus"
    }
);

work.commit();
```

This style keeps normal CRUD convenient and keeps transaction-bound operations correct.

## Complete example

This example uses `vix::db` for schema, `vix::orm::Mapper<User>` for mapping, `Repository<User>` for CRUD, and direct `vix::db` for a custom query.

```cpp
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
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

std::optional<User> findByEmail(
    vix::db::Database& db,
    const std::string& email
)
{
    auto rows = db.query(
        "SELECT id, email, name FROM users WHERE email = ? LIMIT 1",
        email
    );

    if (!rows->next())
    {
        return std::nullopt;
    }

    return vix::orm::Mapper<User>::fromRow(rows->row());
}

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

    auto byId = users.findById(static_cast<std::int64_t>(id));

    if (byId)
    {
        vix::print_inline("by id: ");
        vix::print(
            vix::options{.sep = " | "},
            byId->id,
            byId->email,
            byId->name
        );
    }

    auto byEmail = findByEmail(db, "ada@example.com");

    if (byEmail)
    {
        vix::print_inline("by email: ");
        vix::print(
            vix::options{.sep = " | "},
            byEmail->id,
            byEmail->email,
            byEmail->name
        );
    }

    return 0;
}
```

This is the intended style.

Use the ORM for the common path.

Use `vix::db` when custom SQL is the better tool.

## When to use only vix::db

Use only `vix::db` when the application has a few direct SQL operations and no real entity layer.

Examples:

```txt
small CLI tool
simple migration helper
one-off database script
custom reporting query
low-level database utility
```

A direct database query can be clearer than creating entities, mappers, and repositories.

```cpp
auto rows = db.query(
    "SELECT COUNT(*) FROM users"
);

if (rows->next())
{
    vix::print("users:", rows->row().getInt64(0));
}
```

Do not use ORM just because it exists.

## When to add ORM

Add ORM when repeated mapping code starts to appear.

Signs that ORM helps:

```txt
the same table is read in several places
the same row-to-object conversion is repeated
CRUD code is duplicated
route handlers contain too much SQL
application services need cleaner persistence boundaries
```

At that point, move row conversion into `Mapper<T>` and standard operations into `Repository<T>`.

## When to use both

Most serious applications use both.

Use `Repository<T>` for standard entity operations.

Use direct `vix::db` for custom SQL.

Use `QueryBuilder` for optional filters.

Use `UnitOfWork` for transactional boundaries.

This gives the project a flexible database layer without forcing every query into the same abstraction.

## Example service using both

```cpp
class UserService
{
public:
    explicit UserService(vix::db::Database& db)
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

This keeps common CRUD short and keeps custom SQL explicit.

## Build flags

Because ORM depends on the database module, build with the selected database backend.

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

The include path for ORM is:

```cpp
#include <vix/orm.hpp>
```

The include path for direct database access is:

```cpp
#include <vix/db.hpp>
```

## Project structure

A clean project can separate domain, persistence, and application logic.

```txt
src/
├── domain/
│   └── User.hpp
├── infrastructure/
│   └── persistence/
│       ├── UserMapper.hpp
│       └── UserRepository.hpp
├── application/
│   └── UserService.hpp
└── main.cpp
```

`domain/User.hpp` contains the entity.

`UserMapper.hpp` contains the `Mapper<User>` specialization.

`UserRepository.hpp` wraps `Repository<User>` and custom SQL.

`UserService.hpp` uses the repository and database operations to implement application behavior.

This structure keeps SQL out of route handlers and keeps domain objects independent from database mechanics.

## Common mistakes

### Treating ORM as a replacement for SQL

Vix ORM is not designed to hide SQL completely.

Use SQL where SQL is clearer.

### Expecting repositories to create schema

Repositories do not create tables.

Use migrations or direct `vix::db` schema SQL.

### Assuming repository methods are automatically transactional

Generic repository methods can acquire their own connections.

Use `UnitOfWork::conn()` for transaction-bound statements.

### Mapping every query through `Repository<T>`

Some queries are better as direct SQL.

Reports, joins, aggregates, and filtered searches often read better with `vix::db` or `QueryBuilder`.

### Duplicating row mapping everywhere

If the same row-to-object conversion appears in several places, move it into `Mapper<T>`.

### Hiding database behavior behind too many wrappers

Abstractions should make the code easier to understand.

If a wrapper hides the SQL and makes behavior harder to trace, it is probably the wrong abstraction.

## Recommended style

Start with `vix::db`.

Add `vix::orm` when mapping and repository structure reduce repetition.

Keep schema in migrations.

Keep row conversion in `Mapper<T>`.

Use `Repository<T>` for basic CRUD.

Use direct `vix::db` for custom SQL.

Use `QueryBuilder` for dynamic SQL.

Use `UnitOfWork` for transaction boundaries.

Use transaction-aware repository methods when repository code must participate in a unit of work.

Keep ownership simple: the application owns `vix::db::Database`.

## Next steps

Read the database guide if you need the lower-level SQL API.

Read the repositories page if you want to wrap CRUD operations cleanly.

Read the unit of work page before implementing transaction-sensitive workflows.

Read the query builder page for dynamic SQL.
