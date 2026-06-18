# Mappers

A mapper defines how a C++ type is connected to database rows and columns.

In Vix ORM, mapping is explicit. The ORM does not inspect struct fields automatically, does not rely on runtime reflection, and does not guess column names from member names.

For every entity type used with a repository, you provide a specialization of:

```cpp
vix::orm::Mapper<T>
```

The public ORM header is:

```cpp
#include <vix/orm.hpp>
```

A mapper answers three questions:

```txt
How is an entity created from a database row?
Which fields are inserted?
Which fields are updated?
```

That is the core of Vix ORM.

## Minimal mapper

Given this entity:

```cpp
#include <cstdint>
#include <string>

struct User
{
    std::int64_t id{};
    std::string email;
    std::string name;
};
```

A mapper can be written like this:

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

This mapper says:

```txt
column 0 -> id
column 1 -> email
column 2 -> name
```

It also says that `email` and `name` are inserted and updated.

The `id` field is not inserted because the database owns it.

## Why mappers are explicit

A mapper is deliberately written by the developer.

This makes the database boundary visible.

The table schema is explicit.

The selected columns are explicit.

The inserted fields are explicit.

The updated fields are explicit.

This is more predictable than hidden mapping rules, especially in C++ projects where developers often need control over ownership, performance, and behavior.

A Vix mapper is not metadata magic. It is normal C++ code.

## The three mapper functions

A complete mapper usually defines:

```cpp
static T fromRow(const vix::db::ResultRow& row);

static vix::orm::FieldValues toInsertFields(const T& value);

static vix::orm::FieldValues toUpdateFields(const T& value);
```

`fromRow()` is used when reading rows from the database.

`toInsertFields()` is used by `Repository<T>::create()`.

`toUpdateFields()` is used by `Repository<T>::updateById()`.

If one of these functions is missing, repository operations that need it will fail to compile.

That is intentional. A missing mapper should be caught by the compiler.

## Mapping rows

`fromRow()` converts a `vix::db::ResultRow` into an entity.

```cpp
static User fromRow(const vix::db::ResultRow& row)
{
    return User{
        row.getInt64(0),
        row.getString(1),
        row.getString(2)
    };
}
```

The column order must match the SQL query.

The generic repository uses:

```sql
SELECT * FROM users
```

for `findAll()` and:

```sql
SELECT * FROM users WHERE id = ? LIMIT 1
```

for `findById()`.

Because of that, the mapper should match the physical column order returned by the table.

For example, with this table:

```sql
CREATE TABLE users (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  email TEXT NOT NULL UNIQUE,
  name TEXT NOT NULL
);
```

the row order is:

```txt
0 -> id
1 -> email
2 -> name
```

For custom queries, prefer explicit column lists and keep the mapper aligned with the selected columns.

```sql
SELECT id, email, name FROM users
```

## Insert fields

`toInsertFields()` defines which columns are inserted.

```cpp
static vix::orm::FieldValues toInsertFields(const User& user)
{
    return {
        {"email", user.email},
        {"name", user.name}
    };
}
```

The repository uses these fields to build an insert statement.

The generated SQL shape is:

```sql
INSERT INTO users (email,name) VALUES (?,?)
```

The values are bound as prepared statement parameters.

Do not include generated primary keys unless the application owns the id.

For an auto-increment id, this is correct:

```cpp
return {
    {"email", user.email},
    {"name", user.name}
};
```

This is usually wrong:

```cpp
return {
    {"id", user.id},
    {"email", user.email},
    {"name", user.name}
};
```

unless the application deliberately controls the id.

## Update fields

`toUpdateFields()` defines which columns are updated.

```cpp
static vix::orm::FieldValues toUpdateFields(const User& user)
{
    return {
        {"email", user.email},
        {"name", user.name}
    };
}
```

The repository uses these fields to build an update statement.

The generated SQL shape is:

```sql
UPDATE users SET email=?,name=? WHERE id=?
```

Primary keys and immutable fields should usually be excluded from update fields.

For example, if `created_at` is set by the database and should not change, do not return it in `toUpdateFields()`.

## FieldValues

A mapper returns field/value pairs.

```cpp
using FieldValue = std::pair<std::string, std::any>;
using FieldValues = std::vector<FieldValue>;
```

A field value looks like:

```cpp
{"email", user.email}
```

The first part is the database column name.

The second part is the C++ value to bind.

The ORM converts the `std::any` value into a `vix::db::DbValue` before binding it to the prepared statement.

Supported values include common C++ types such as integers, strings, booleans, floating point values, null values, and database values supported by `vix::db`.

## Supported value types

The mapper boundary uses `std::any`, but the value must still be a type that the ORM knows how to convert into a database value.

Common safe values include:

```txt
bool
int
unsigned
std::int64_t
std::uint64_t
double
float
std::string
std::string_view
const char*
std::nullptr_t
vix::db::DbValue
vix::db::Blob
```

Example:

```cpp
static vix::orm::FieldValues toInsertFields(const Product& product)
{
    return {
        {"name", product.name},
        {"price", product.price},
        {"active", product.active}
    };
}
```

If the mapper returns an unsupported type, the ORM throws a database error during binding.

Keep mapper values simple.

Convert custom domain values to database-ready values inside the mapper.

## Mapping nullable columns

Use `std::optional<T>` when a database column can be `NULL` and the absence matters in the domain.

```cpp
#include <optional>
#include <string>

struct Profile
{
    std::int64_t id{};
    std::string email;
    std::optional<std::string> display_name;
};
```

Read nullable values with `isNull()`:

```cpp
static Profile fromRow(const vix::db::ResultRow& row)
{
    return Profile{
        row.getInt64(0),
        row.getString(1),
        row.isNull(2)
            ? std::optional<std::string>{}
            : std::optional<std::string>{row.getString(2)}
    };
}
```

Write nullable values by returning an empty `std::any` when the value should be SQL `NULL`.

```cpp
static vix::orm::FieldValues toUpdateFields(const Profile& profile)
{
    return {
        {"display_name", profile.display_name
            ? std::any{*profile.display_name}
            : std::any{}}
    };
}
```

This maps a missing `display_name` to a database null.

## Mapping booleans

Many SQL engines store booleans as integer-like values.

If your entity uses `bool`, you can return it directly:

```cpp
struct FeatureFlag
{
    std::int64_t id{};
    std::string name;
    bool enabled{};
};

template <>
struct vix::orm::Mapper<FeatureFlag>
{
    static FeatureFlag fromRow(const vix::db::ResultRow& row)
    {
        return FeatureFlag{
            row.getInt64(0),
            row.getString(1),
            row.getInt64(2) != 0
        };
    }

    static vix::orm::FieldValues toInsertFields(const FeatureFlag& flag)
    {
        return {
            {"name", flag.name},
            {"enabled", flag.enabled}
        };
    }

    static vix::orm::FieldValues toUpdateFields(const FeatureFlag& flag)
    {
        return {
            {"name", flag.name},
            {"enabled", flag.enabled}
        };
    }
};
```

The mapper decides how the database representation becomes a C++ boolean.

## Mapping custom value objects

Domain types often contain value objects.

For example:

```cpp
struct Email
{
    std::string value;
};

struct User
{
    std::int64_t id{};
    Email email;
    std::string name;
};
```

The mapper should convert the value object to a database-ready type.

```cpp
static vix::orm::FieldValues toInsertFields(const User& user)
{
    return {
        {"email", user.email.value},
        {"name", user.name}
    };
}
```

And convert the database row back into the domain type:

```cpp
static User fromRow(const vix::db::ResultRow& row)
{
    return User{
        row.getInt64(0),
        Email{row.getString(1)},
        row.getString(2)
    };
}
```

Keep domain rules in domain types.

Keep database conversion in the mapper.

## Mapping private fields

If the entity is a class with private fields, the mapper can use public methods.

```cpp
class User
{
public:
    User() = default;

    User(std::int64_t id, std::string email, std::string name)
        : id_(id),
          email_(std::move(email)),
          name_(std::move(name))
    {
    }

    std::int64_t id() const noexcept
    {
        return id_;
    }

    const std::string& email() const noexcept
    {
        return email_;
    }

    const std::string& name() const noexcept
    {
        return name_;
    }

private:
    std::int64_t id_{};
    std::string email_;
    std::string name_;
};
```

Mapper:

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
            {"email", user.email()},
            {"name", user.name()}
        };
    }

    static vix::orm::FieldValues toUpdateFields(const User& user)
    {
        return {
            {"email", user.email()},
            {"name", user.name()}
        };
    }
};
```

If the mapper needs access to private fields, you can make it a friend, but prefer public methods when they express the domain clearly.

## Keeping SQL order stable

Because `fromRow()` reads columns by index, the selected column order matters.

For repository CRUD methods, the repository currently uses `SELECT *`.

That means the mapper should match the table column order.

For custom queries, write explicit column lists.

```cpp
auto rows = db.query(
    "SELECT id, email, name FROM users WHERE email = ?",
    email
);
```

Then the mapper can safely read:

```cpp
return User{
    row.getInt64(0),
    row.getString(1),
    row.getString(2)
};
```

Avoid changing table column order assumptions accidentally.

In larger applications, a project-specific repository can use explicit selects and call the same mapper.

## Complete example

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
    std::optional<std::string> display_name;
};

template <>
struct vix::orm::Mapper<User>
{
    static User fromRow(const vix::db::ResultRow& row)
    {
        return User{
            row.getInt64(0),
            row.getString(1),
            row.getString(2),
            row.isNull(3)
                ? std::optional<std::string>{}
                : std::optional<std::string>{row.getString(3)}
        };
    }

    static vix::orm::FieldValues toInsertFields(const User& user)
    {
        return {
            {"email", user.email},
            {"name", user.name},
            {"display_name", user.display_name
                ? std::any{*user.display_name}
                : std::any{}}
        };
    }

    static vix::orm::FieldValues toUpdateFields(const User& user)
    {
        return {
            {"email", user.email},
            {"name", user.name},
            {"display_name", user.display_name
                ? std::any{*user.display_name}
                : std::any{}}
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
        "name TEXT NOT NULL,"
        "display_name TEXT NULL"
        ")"
    );

    auto users = vix::orm::repository<User>(db, "users");

    auto id = users.create(User{
        0,
        "ada@example.com",
        "Ada",
        std::optional<std::string>{"Ada Lovelace"}
    });

    auto user = users.findById(static_cast<std::int64_t>(id));

    if (user)
    {
        vix::print(
            vix::options{.sep = " | "},
            user->id,
            user->email,
            user->name,
            user->display_name.value_or("no display name")
        );
    }

    return 0;
}
```

This example shows the full mapping path:

```txt
database row
-> Mapper<User>::fromRow
-> User

User
-> Mapper<User>::toInsertFields
-> prepared statement values
```

## Mapper and Repository

A repository depends on the mapper.

```cpp
auto users = vix::orm::repository<User>(db, "users");
```

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
```

the repository calls:

```cpp
Mapper<User>::fromRow(row);
```

So if repository behavior is wrong, inspect the mapper first.

## Backward-compatible names

The mapper primary API uses:

```cpp
toInsertFields
toUpdateFields
```

Older code may use:

```cpp
toInsertParams
toUpdateParams
```

These aliases still exist for compatibility.

For new code, prefer the field-based names:

```cpp
toInsertFields
toUpdateFields
```

They better describe what the mapper returns: column/value pairs.

## File organization

For small examples, keep the mapper near the entity.

For real applications, separate them.

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

`User.hpp` contains the entity type.

`UserMapper.hpp` contains the `Mapper<User>` specialization.

`UserRepository.hpp` contains repository composition and custom queries.

This keeps domain code clean and keeps database mapping in the persistence layer.

## Testing mappers

Mapper code is normal C++.

Test it like normal code.

For `fromRow()`, use integration tests with a small SQLite database.

For `toInsertFields()` and `toUpdateFields()`, simple unit-style checks are often enough.

Example:

```cpp
User user{
    0,
    "ada@example.com",
    "Ada"
};

auto fields = vix::orm::Mapper<User>::toInsertFields(user);

vix::print("insert fields:", fields.size());
```

In project tests, verify that the repository can create, read, update, and delete a mapped entity.

The mapper is the most common place where ORM bugs appear, because it is the boundary between database shape and C++ shape.

## Common mistakes

### Forgetting the mapper

A repository cannot work without `Mapper<T>`.

If you create:

```cpp
auto users = vix::orm::repository<User>(db, "users");
```

make sure `Mapper<User>` is specialized.

### Reading the wrong column order

If `fromRow()` reads:

```cpp
row.getString(1)
```

make sure column index `1` is actually the value you expect.

Use explicit `SELECT` lists for custom queries.

### Inserting generated ids

If the database owns the primary key, do not insert `id`.

### Updating immutable fields

Do not include fields such as `created_at` in `toUpdateFields()` unless the application deliberately updates them.

### Returning unsupported std::any values

Mapper field values must be convertible to database values.

Convert custom types to strings, integers, booleans, floating point values, blobs, or explicit database values before returning them.

### Hiding too much in the mapper

A mapper should convert data.

It should not perform database queries, call external services, or contain application workflows.

Keep it boring and predictable.

## Recommended style

Keep mappers explicit.

Use the same column order consistently.

Do not include generated ids in insert fields.

Exclude immutable fields from update fields.

Represent nullable database values deliberately.

Convert domain value objects inside the mapper.

Keep persistence mapping outside the entity.

Test mapper behavior through repository operations.

## Next steps

Read the repositories page next.

Once the mapper is correct, `Repository<T>` can provide create, find, update, delete, exists, and count operations with much less repeated SQL.
