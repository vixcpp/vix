# Query Builder

`vix::orm::QueryBuilder` is a small helper for building SQL incrementally while keeping parameters separate from the SQL string.

It is useful when a query has optional filters, optional ordering, or conditional clauses.
It is not a full ORM query language.
It does not replace SQL.
It does not inspect entities.
It does not generate joins automatically.
It simply helps you build prepared statements without manually managing a separate vector of parameters.

The public ORM header is:

```cpp
#include <vix/orm.hpp>
```

## Why QueryBuilder exists

Many SQL queries are static.

For example:

```cpp
auto rows = db.query(
    "SELECT id, email, name FROM users WHERE email = ?",
    email
);
```

This is already clear. You do not need a query builder for that.

The problem appears when a query depends on optional filters.

```txt
filter by email only if email is provided
filter by name only if name is provided
filter by active status only if active is provided
apply LIMIT and OFFSET only in paginated queries
```

Without a helper, code often starts concatenating SQL and manually tracking bind values.

`QueryBuilder` keeps that code explicit while making it harder to forget the parameter list.

## Basic usage

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
```

At the end, the builder contains:

```cpp
qb.sql();
qb.params();
```

`sql()` returns the SQL string.

`params()` returns the collected bind parameters.

You can then prepare the SQL and bind the collected values.

```cpp
vix::db::PooledConn conn(db.pool());

auto stmt = conn->prepare(qb.sql());

qb.bind(*stmt);

auto rows = stmt->query();
```

## A complete example

```cpp
#include <cstdint>
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
        qb.raw(" AND name = ?");
        qb.param(name);
    }

    qb.raw(" ORDER BY id");

    vix::db::PooledConn conn(db.pool());

    auto stmt = conn->prepare(qb.sql());
    qb.bind(*stmt);

    auto rows = stmt->query();

    std::vector<User> users;

    while (rows->next())
    {
        users.push_back(
            vix::orm::Mapper<User>::fromRow(rows->row())
        );
    }

    return users;
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

    auto users = searchUsers(db, "ada@example.com", "");

    for (const auto& user : users)
    {
        vix::print(
            vix::options{.sep = " | "},
            user.id,
            user.email,
            user.name
        );
    }

    return 0;
}
```

This example keeps SQL visible and keeps values bindable.

The query builder does not hide the query. It only helps assemble it safely.

## raw

`raw()` appends SQL text exactly as provided.

```cpp
vix::orm::QueryBuilder qb;

qb.raw("SELECT id, email FROM users");
qb.raw(" WHERE email = ?");
qb.param("ada@example.com");
```

The SQL becomes:

```sql
SELECT id, email FROM users WHERE email = ?
```

Use `raw()` for normal fragments where you control spacing.

## rawSpace

`rawSpace()` appends SQL text followed by one space.

```cpp
vix::orm::QueryBuilder qb;

qb.rawSpace("SELECT id, email, name");
qb.rawSpace("FROM users");
qb.raw("WHERE id = ?");
qb.param(1);
```

This can make long SQL construction easier to read.

Be careful with spacing. `QueryBuilder` does not parse SQL or fix malformed SQL.

It appends what you tell it to append.

## space and newline

Use `space()` when you want to add a single space.

```cpp
qb.raw("SELECT id, email");
qb.space();
qb.raw("FROM users");
```

Use `newline()` when you want generated SQL to be easier to inspect.

```cpp
qb.raw("SELECT id, email, name");
qb.newline();
qb.raw("FROM users");
qb.newline();
qb.raw("WHERE email = ?");
```

Newlines are optional. They are mainly useful when debugging generated SQL.

## Parameters

Use `param()` to add a bind value.

```cpp
qb.raw("WHERE email = ?");
qb.param(email);
```

Each call to `param()` adds one value to the parameter list.

When you call:

```cpp
qb.bind(*stmt);
```

the builder binds all collected values to the statement starting at index `1`.

The first parameter is bound to the first placeholder.

The second parameter is bound to the second placeholder.

The order matters.

## Supported parameter types

`QueryBuilder` supports common database value types.

Examples:

```cpp
qb.param(42);
qb.param(std::int64_t{42});
qb.param(std::uint64_t{42});
qb.param(19.99);
qb.param(true);
qb.param("ada@example.com");
qb.param(std::string{"Ada"});
qb.param(std::string_view{"Ada"});
qb.paramNull();
```

For binary values, use a database blob value.

```cpp
vix::db::Blob blob;
blob.bytes = {1, 2, 3, 4};

qb.param(std::move(blob));
```

Use `paramNull()` when the SQL value should be `NULL`.

```cpp
qb.raw("display_name IS ?");
qb.paramNull();
```

For normal optional filters, it is usually better to omit the condition entirely instead of comparing with `NULL`.

## Binding parameters

Once the SQL is built, prepare the statement and bind the parameters.

```cpp
vix::db::PooledConn conn(db.pool());

auto stmt = conn->prepare(qb.sql());

qb.bind(*stmt);
```

Then execute:

```cpp
auto rows = stmt->query();
```

or:

```cpp
auto affected = stmt->exec();
```

Use `query()` for SQL that returns rows.

Use `exec()` for SQL that does not return rows.

## Reading results

`QueryBuilder` only builds SQL and parameters.

It does not read rows.

Use `vix::db::ResultSet` and `vix::db::ResultRow` like normal.

```cpp
auto rows = stmt->query();

while (rows->next())
{
    const auto& row = rows->row();

    vix::print(
        vix::options{.sep = " | "},
        row.getInt64(0),
        row.getString(1),
        row.getString(2)
    );
}
```

If the query maps to an entity, reuse the mapper.

```cpp
auto user = vix::orm::Mapper<User>::fromRow(rows->row());
```

## Optional filters

The most common use case is optional filters.

```cpp
std::vector<User> search(
    vix::db::Database& db,
    const std::string& email,
    const std::string& name,
    bool onlyActive
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

    if (onlyActive)
    {
        qb.raw(" AND active = ?");
        qb.param(true);
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

The `WHERE 1=1` pattern is not required, but it makes conditional `AND` clauses simple.

## Pagination

Use bind parameters for `LIMIT` and `OFFSET` when the database supports it.

```cpp
vix::orm::QueryBuilder qb;

qb.raw("SELECT id, email, name FROM users ORDER BY id LIMIT ? OFFSET ?");
qb.param(static_cast<std::int64_t>(limit));
qb.param(static_cast<std::int64_t>(offset));
```

Then bind and execute:

```cpp
vix::db::PooledConn conn(db.pool());

auto stmt = conn->prepare(qb.sql());
qb.bind(*stmt);

auto rows = stmt->query();
```

Pagination should be used instead of `findAll()` for tables that can grow.

## Dynamic ordering

Be careful with dynamic ordering.

Column names and sort directions cannot be bound as normal SQL parameters. Bind parameters are for values, not identifiers.

This is valid:

```cpp
qb.raw("WHERE email = ?");
qb.param(email);
```

This is not valid:

```cpp
qb.raw("ORDER BY ?");
qb.param(sortColumn);
```

For dynamic ordering, whitelist allowed column names.

```cpp
std::string orderBy = "id";

if (requestedOrder == "email")
{
    orderBy = "email";
}
else if (requestedOrder == "name")
{
    orderBy = "name";
}

qb.raw(" ORDER BY ");
qb.raw(orderBy);
```

Do not insert unchecked user input into `ORDER BY`.

Use a whitelist.

## Dynamic sort direction

Sort direction must also be whitelisted.

```cpp
std::string direction = "ASC";

if (requestedDirection == "desc")
{
    direction = "DESC";
}

qb.raw(" ");
qb.raw(direction);
```

Do not pass arbitrary user text into SQL fragments.

Values can be bound.

SQL structure must be controlled by the application.

## Dynamic table names

Avoid dynamic table names in application code.

If you must choose between a small set of known tables, whitelist them.

```cpp
std::string table = "users";

if (requestedTable == "admins")
{
    table = "admins";
}

qb.raw("SELECT id, email, name FROM ");
qb.raw(table);
```

Never append untrusted table names directly.

`QueryBuilder` does not sanitize raw SQL fragments.

That is not its job.

## Inserts with QueryBuilder

Most inserts should use `Repository<T>::create()` or direct `db.exec()`.

But `QueryBuilder` can build insert statements when the field list is dynamic.

```cpp
vix::orm::QueryBuilder qb;

qb.raw("INSERT INTO audit_log (message, level) VALUES (?, ?)");
qb.param("user created");
qb.param("info");

vix::db::PooledConn conn(db.pool());

auto stmt = conn->prepare(qb.sql());
qb.bind(*stmt);

stmt->exec();
```

Use direct SQL when the statement is simple.

Use `QueryBuilder` when conditional SQL construction is clearer than manual string and parameter handling.

## Updates with QueryBuilder

For conditional updates, a query builder can help.

```cpp
vix::orm::QueryBuilder qb;

qb.raw("UPDATE users SET updated_at = CURRENT_TIMESTAMP");

if (!name.empty())
{
    qb.raw(", name = ?");
    qb.param(name);
}

if (!email.empty())
{
    qb.raw(", email = ?");
    qb.param(email);
}

qb.raw(" WHERE id = ?");
qb.param(userId);

vix::db::PooledConn conn(db.pool());

auto stmt = conn->prepare(qb.sql());
qb.bind(*stmt);

auto affected = stmt->exec();

vix::print("updated rows:", affected);
```

Make sure the generated SQL is valid when optional fields are absent.

In this example, `updated_at` is always updated, so the `SET` clause is never empty.

## Deletes with QueryBuilder

Use a clear `WHERE` clause for deletes.

```cpp
vix::orm::QueryBuilder qb;

qb.raw("DELETE FROM sessions WHERE expires_at < ?");
qb.param(expirationTime);

vix::db::PooledConn conn(db.pool());

auto stmt = conn->prepare(qb.sql());
qb.bind(*stmt);

auto affected = stmt->exec();

vix::print("deleted sessions:", affected);
```

Be careful with optional filters in delete statements.

A missing filter can delete more rows than intended.

When building deletes dynamically, validate that the final SQL has the expected condition.

## Checking generated SQL

During development, print the generated SQL if a dynamic query behaves unexpectedly.

```cpp
vix::print("sql:", qb.sql());
vix::print("params:", qb.params().size());
```

Do not log sensitive parameter values in production.

Logging SQL structure is often useful.

Logging secrets, tokens, passwords, or personal data is not.

## clear

Use `clear()` to reuse the same builder object.

```cpp
vix::orm::QueryBuilder qb;

qb.raw("SELECT id FROM users WHERE email = ?");
qb.param("ada@example.com");

// use query

qb.clear();

qb.raw("SELECT id FROM users WHERE name = ?");
qb.param("Ada");
```

Most code can simply create a new builder. Use `clear()` only when reuse improves the code.

## reserve

Use `reserve()` when a builder will create a larger query and you know the approximate size.

```cpp
vix::orm::QueryBuilder qb;

qb.reserve(512, 8);
```

This reserves storage for SQL text and parameters.

It is a performance hint, not a requirement.

Most code does not need it.

## takeSql and takeParams

`takeSql()` moves the SQL string out of the builder.

`takeParams()` moves the parameter list out of the builder.

Use these only when integrating with lower-level code that wants to take ownership of the generated SQL and parameters.

Normal code should use:

```cpp
qb.sql();
qb.params();
qb.bind(*stmt);
```

Do not call `takeSql()` and then expect `qb.sql()` to still contain the original SQL.

## QueryBuilder with repositories

The generic repository covers simple CRUD.

Use `QueryBuilder` inside a project-specific repository for custom queries.

```cpp
class UserRepository
{
public:
    explicit UserRepository(vix::db::Database& db)
        : db_(db),
          users_(vix::orm::repository<User>(db, "users"))
    {
    }

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
            qb.raw(" AND name LIKE ?");
            qb.param("%" + name + "%");
        }

        qb.raw(" ORDER BY id");

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

private:
    vix::db::Database& db_;
    vix::orm::Repository<User> users_;
};
```

This keeps standard CRUD in the generic repository and custom search logic in the project repository.

## QueryBuilder with UnitOfWork

Use the transaction connection when a query must be part of a unit of work.

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

Do not create a separate pooled connection if the query must participate in the transaction.

Use `work.conn()`.

## QueryBuilder and SQL injection

`QueryBuilder` helps keep values separate from SQL, but it does not make raw SQL fragments safe automatically.

This is safe:

```cpp
qb.raw("WHERE email = ?");
qb.param(email);
```

This is not safe:

```cpp
qb.raw("WHERE email = '" + email + "'");
```

Values should be passed with `param()`.

SQL identifiers such as table names, column names, and sort directions cannot be passed with `param()`. They must be controlled by the application and whitelisted when dynamic.

## Common mistakes

### Forgetting to bind parameters

Wrong:

```cpp
auto stmt = conn->prepare(qb.sql());
auto rows = stmt->query();
```

Correct:

```cpp
auto stmt = conn->prepare(qb.sql());
qb.bind(*stmt);
auto rows = stmt->query();
```

### Adding a placeholder without a parameter

Wrong:

```cpp
qb.raw("WHERE email = ?");
```

Correct:

```cpp
qb.raw("WHERE email = ?");
qb.param(email);
```

### Adding a parameter without a placeholder

Wrong:

```cpp
qb.raw("SELECT id FROM users");
qb.param(email);
```

The SQL has no `?` placeholder for the parameter.

### Passing untrusted input to raw

Wrong:

```cpp
qb.raw(" ORDER BY " + userInput);
```

Correct:

```cpp
std::string orderBy = "id";

if (userInput == "email")
{
    orderBy = "email";
}
else if (userInput == "name")
{
    orderBy = "name";
}

qb.raw(" ORDER BY ");
qb.raw(orderBy);
```

### Assuming QueryBuilder validates SQL

It does not validate SQL.

It builds the string and stores parameters.

The database driver validates the SQL when the statement is prepared or executed.

### Using QueryBuilder when static SQL is clearer

This is enough:

```cpp
auto rows = db.query(
    "SELECT id, email, name FROM users WHERE email = ?",
    email
);
```

Do not use a builder when the query is simple.

## Recommended style

Use plain `db.query()` or `db.exec()` for static SQL.

Use `QueryBuilder` for conditional SQL.

Keep values in `param()`.

Keep raw SQL fragments controlled by the application.

Whitelist dynamic identifiers.

Use `PooledConn` for normal prepared statement execution.

Use `UnitOfWork::conn()` for transaction-bound queries.

Reuse `Mapper<T>::fromRow()` when the selected column order matches the mapper.

Print generated SQL only during development and avoid logging sensitive values.

## Next steps

Read the Unit of Work page to understand how to run builder-generated statements inside a transaction.

Read the With vix::db page to understand when direct database access is better than ORM helpers.

Read the repositories page if you want to use `QueryBuilder` inside project-specific repositories.
