# ORM quick start

This page shows the shortest useful path for using Vix ORM in a C++ application.

Vix ORM is built on top of `vix::db`. The database module opens the database, owns the connection pool, executes SQL, and returns rows. The ORM adds explicit entity mapping and a small repository API.

The public ORM header is:

```cpp
#include <vix/orm.hpp>
```

The example uses SQLite because it works without a separate database server.

## What you will build

You will build a small program that:

1. opens a SQLite database,
2. creates a `users` table,
3. defines a `User` type,
4. specializes `vix::orm::Mapper<User>`,
5. creates a repository,
6. inserts a user,
7. reads the user back,
8. updates the user,
9. lists all users,
10. deletes the user.

The goal is to show the ORM workflow without hiding the database.

## Create the project

Create a small project directory:

```bash
mkdir vix-orm-quick-start
cd vix-orm-quick-start
mkdir -p src storage
touch src/main.cpp
```

The SQLite database will be stored here:

```txt
storage/app.db
```

## Write the program

Open:

```txt
src/main.cpp
```

and add:

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
    try
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

        auto createdId = users.create(User{
            0,
            "ada@example.com",
            "Ada"
        });

        vix::print("created user id:", createdId);

        auto found = users.findById(static_cast<std::int64_t>(createdId));

        if (found)
        {
            vix::print_inline("found user: ");
            vix::print(
                vix::options{.sep = " | "},
                found->id,
                found->email,
                found->name
            );
        }

        User updated{
            static_cast<std::int64_t>(createdId),
            "ada@example.com",
            "Ada Lovelace"
        };

        auto updatedRows = users.updateById(
            static_cast<std::int64_t>(createdId),
            updated
        );

        vix::print("updated rows:", updatedRows);

        auto allUsers = users.findAll();

        vix::print("users after update:");

        for (const auto& user : allUsers)
        {
            vix::print(
                vix::options{.sep = " | "},
                user.id,
                user.email,
                user.name
            );
        }

        auto deletedRows = users.removeById(
            static_cast<std::int64_t>(createdId)
        );

        vix::print("deleted rows:", deletedRows);
        vix::print("remaining users:", users.count());

        return 0;
    }
    catch (const std::exception& e)
    {
        vix::print("orm error:", e.what());
        return 1;
    }
}
```

This is a complete ORM example, but nothing is hidden.

The table is created with SQL.

The `User` type is a normal C++ struct.

The mapper defines how rows and fields are converted.

The repository provides the CRUD operations.

## Build with SQLite support

Build the project with SQLite enabled:

```bash
vix build --with-sqlite
```

Or build and run in one command:

```bash
vix run --with-sqlite
```

The ORM depends on `vix::db`, so the selected database backend must be enabled at build time.

For MySQL, use:

```bash
vix build --with-mysql
```

## Expected output

The output should look close to this:

```txt
created user id: 1
found user: 1 | ada@example.com | Ada
updated rows: 1
users after update:
1 | ada@example.com | Ada Lovelace
deleted rows: 1
remaining users: 0
```

The generated id can be different if the database file already existed before running the example.

## Reset the example

To reset the SQLite database:

```bash
rm -f storage/app.db storage/app.db-wal storage/app.db-shm
```

Then run the program again.

SQLite may create `-wal` and `-shm` sidecar files depending on how the database is used.

## The entity

The example uses a normal C++ struct:

```cpp
struct User
{
    std::int64_t id{};
    std::string email;
    std::string name;
};
```

This type does not inherit from `vix::orm::Entity`.

That is allowed.

Vix ORM does not force your domain objects to inherit from a framework base class. If you want a shared identity interface, you can inherit from `vix::orm::Entity`, but it is optional.

For many applications, plain structs are enough.

## The mapper

The mapper is the most important part of the ORM.

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

`fromRow()` converts a database row into a `User`.

`toInsertFields()` returns the fields used by `Repository<User>::create()`.

`toUpdateFields()` returns the fields used by `Repository<User>::updateById()`.

The `id` field is not inserted or updated here because the database owns it through `AUTOINCREMENT`.

This is explicit by design. Vix ORM does not guess which fields should be stored.

## The repository

Create a repository with:

```cpp
auto users = vix::orm::repository<User>(db, "users");
```

The first argument is the database facade.

The second argument is the table name.

The repository assumes that the table has a primary key column named:

```txt
id
```

A repository gives you common operations:

```cpp
users.create(user);
users.findById(id);
users.findAll();
users.existsById(id);
users.count();
users.updateById(id, user);
users.removeById(id);
users.removeAll();
```

The repository uses the mapper to convert between C++ objects and database rows.

## Create

`create()` inserts a new entity and returns the generated id.

```cpp
auto id = users.create(User{
    0,
    "grace@example.com",
    "Grace Hopper"
});
```

The fields inserted into the SQL statement come from:

```cpp
Mapper<User>::toInsertFields(user)
```

For the `User` example, the generated insert shape is equivalent to:

```sql
INSERT INTO users (email,name) VALUES (?,?)
```

The values are bound as prepared statement parameters.

## Find by id

`findById()` returns `std::optional<T>`.

```cpp
auto user = users.findById(1);

if (user)
{
    vix::print("email:", user->email);
}
```

If no row exists, the result is `std::nullopt`.

This makes missing rows explicit in the type system.

## Find all

`findAll()` returns a vector of entities.

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

Use `findAll()` for small tables, admin tools, examples, and tests.

For large tables, add custom queries with pagination instead of loading everything at once.

## Update by id

`updateById()` updates one row by primary key.

```cpp
User user{
    1,
    "ada@example.com",
    "Ada Lovelace"
};

auto affected = users.updateById(1, user);

vix::print("updated rows:", affected);
```

The updated fields come from:

```cpp
Mapper<User>::toUpdateFields(user)
```

Check the affected row count when the application expects exactly one row to change.

```cpp
if (affected != 1)
{
    vix::print("unexpected updated rows:", affected);
}
```

## Remove by id

`removeById()` deletes one row by primary key.

```cpp
auto affected = users.removeById(1);

vix::print("deleted rows:", affected);
```

For important deletes, check the affected row count.

## Count

`count()` returns the number of rows in the table.

```cpp
auto total = users.count();

vix::print("users:", total);
```

This is useful in examples, admin tools, tests, and simple checks.

## Exists by id

`existsById()` checks whether a row exists.

```cpp
if (users.existsById(1))
{
    vix::print("user exists");
}
```

Use this when the application only needs existence, not the full entity.

## Why the mapper is required

The repository cannot know your struct layout automatically.

C++ does not provide the kind of runtime reflection that would let Vix safely inspect arbitrary fields and column names.

Vix chooses explicit mapping instead.

That means every mapped type needs a `Mapper<T>` specialization.

If you try to use `Repository<User>` without a mapper, the compiler will report that `Mapper<User>` is not implemented.

This is intentional. A missing mapper should fail at compile time, not silently at runtime.

## Using a custom repository wrapper

For a real application, you may wrap the generic repository in a domain-specific repository.

```cpp
class UserRepository
{
public:
    explicit UserRepository(vix::db::Database& db)
        : users_(vix::orm::repository<User>(db, "users"))
    {
    }

    std::optional<User> findById(std::int64_t id)
    {
        return users_.findById(id);
    }

    std::uint64_t create(User user)
    {
        return users_.create(user);
    }

private:
    vix::orm::Repository<User> users_;
};
```

This gives you a place to add application-specific methods later.

For example:

```cpp
std::optional<User> findByEmail(const std::string& email);
std::vector<User> findRecentUsers(std::size_t limit);
std::uint64_t removeInactiveUsers();
```

Those methods can use `vix::db` directly or use `QueryBuilder`.

## Adding a custom query

The generic repository covers simple CRUD. It does not cover every query.

For custom SQL, use the database facade directly.

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

This keeps SQL clear and reuses the same mapper.

## Using UnitOfWork

Use `UnitOfWork` when several operations must commit or roll back together.

```cpp
auto work = vix::orm::unit_of_work(db);

auto& conn = work.conn();

auto insert = conn.prepare(
    "INSERT INTO users (email, name) VALUES (?, ?)"
);

insert->bind(1, "linus@example.com");
insert->bind(2, "Linus");
insert->exec();

auto audit = conn.prepare(
    "INSERT INTO audit_log (message) VALUES (?)"
);

audit->bind(1, "created user linus@example.com");
audit->exec();

work.commit();
```

If `commit()` is not called, the underlying transaction rolls back.

Use the connection returned by `work.conn()` for all statements that must be part of the same transaction.

Do not call unrelated `db.exec()` operations inside the unit of work if they must be part of the transaction, because `db.exec()` can acquire another connection from the pool.

## Using QueryBuilder

Use `QueryBuilder` when a query has optional filters.

```cpp
vix::orm::QueryBuilder qb;

qb.raw("SELECT id, email, name FROM users WHERE 1=1");

std::string email = "ada@example.com";

if (!email.empty())
{
    qb.raw(" AND email = ?");
    qb.param(email);
}

vix::db::PooledConn conn(db.pool());

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
```

`QueryBuilder` keeps SQL visible and stores parameters separately for binding.

It is not a replacement for SQL.

## Recommended file organization

For a small project, one file is fine.

For a larger project, split the code:

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

`User.hpp` contains the domain type.

`UserMapper.hpp` contains the `Mapper<User>` specialization.

`UserRepository.hpp` contains application-specific repository methods.

This keeps persistence logic away from route handlers and command handlers.

## Common mistakes

### Forgetting the mapper

A repository needs `Mapper<T>`.

```cpp
auto users = vix::orm::repository<User>(db, "users");
```

This only works if `Mapper<User>` is specialized.

### Inserting the id field by accident

If the database owns the primary key, do not include `id` in `toInsertFields()`.

Good:

```cpp
return {
    {"email", user.email},
    {"name", user.name}
};
```

Avoid:

```cpp
return {
    {"id", user.id},
    {"email", user.email},
    {"name", user.name}
};
```

unless the application deliberately controls ids.

### Assuming the ORM creates tables

The repository does not create tables automatically.

Create tables with migrations or explicit SQL.

```cpp
db.exec(
    "CREATE TABLE IF NOT EXISTS users ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "email TEXT NOT NULL UNIQUE,"
    "name TEXT NOT NULL"
    ")"
);
```

### Using `findAll()` for large tables

`findAll()` loads all rows.

For large tables, write custom queries with `LIMIT`, pagination, filters, and indexes.

### Expecting hidden persistence

Changing a C++ object does not update the database automatically.

You must call:

```cpp
users.updateById(id, user);
```

This is deliberate.

## Recommended next step

After this quick start, read the mappers page.

The mapper is the part that decides whether ORM usage stays clean or becomes confusing.

Once mapping is clear, the repository, query builder, and unit of work pages will be easier to understand.
