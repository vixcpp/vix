# Entities

An entity is a C++ type that represents data from a database table.

In Vix ORM, an entity can be a plain struct, a class, or a type that inherits from `vix::orm::Entity`.

The important point is that Vix does not force your domain model to inherit from a framework base class.

This is valid:

```cpp
struct User
{
    std::int64_t id{};
    std::string email;
    std::string name;
};
```

This is also valid:

```cpp
struct User : vix::orm::Entity
{
    std::int64_t id_{};
    std::string email;
    std::string name;

    std::int64_t id() const noexcept override
    {
        return id_;
    }

    void setId(std::int64_t value) noexcept override
    {
        id_ = value;
    }
};
```

Use the simple form when you do not need a shared identity interface. Use `vix::orm::Entity` when the application benefits from treating different mapped objects through a common base type.

## Public header

For normal ORM usage, include:

```cpp
#include <vix/orm.hpp>
```

This exposes the public ORM API, including `Entity`, `Mapper<T>`, `Repository<T>`, `QueryBuilder`, and `UnitOfWork`.

## What an entity represents

An entity usually represents one row from one table.

For example, this table:

```sql
CREATE TABLE users (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  email TEXT NOT NULL UNIQUE,
  name TEXT NOT NULL
);
```

can be represented by:

```cpp
struct User
{
    std::int64_t id{};
    std::string email;
    std::string name;
};
```

The entity itself does not know how to load or save data.
That work belongs to the mapper and repository.
The entity is just the C++ object used by the application.

## Entity does not mean automatic persistence

Changing an entity does not update the database automatically.

```cpp
auto user = users.findById(1);

if (user)
{
    user->name = "Ada Lovelace";
}
```

This only changes the C++ object.

To update the database, call the repository explicitly:

```cpp
if (user)
{
    user->name = "Ada Lovelace";

    users.updateById(user->id, *user);
}
```

This is intentional.

Vix ORM does not use hidden dirty tracking. There is no invisible save operation. Database changes happen when the code asks for them.

## Plain structs

Plain structs are the simplest entity form.

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

This style is good for most examples and many real applications.
It keeps the domain type independent from the ORM.
It is easy to test.
It is easy to move between layers.
It does not require virtual functions.

For many Vix applications, this is the recommended first choice.

## Classes

A class can also be used as an entity.

```cpp
#include <cstdint>
#include <string>
#include <utility>

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

    void rename(std::string name)
    {
        name_ = std::move(name);
    }

private:
    std::int64_t id_{};
    std::string email_;
    std::string name_;
};
```

This style is useful when the entity has invariants or behavior.

For example, a class can expose methods such as:

```cpp
user.rename("Ada Lovelace");
user.changeEmail("ada@example.com");
user.activate();
user.disable();
```

If you use private fields, the mapper must use the public API or be declared as a friend.

## Inheriting from vix::orm::Entity

`vix::orm::Entity` provides a minimal identity interface.

```cpp
struct Entity
{
    virtual ~Entity() = default;

    virtual std::int64_t id() const noexcept;
    virtual void setId(std::int64_t value) noexcept;
};
```

A mapped type can inherit from it:

```cpp
#include <cstdint>
#include <string>
#include <vix/orm.hpp>

struct User : vix::orm::Entity
{
    std::int64_t id_{};
    std::string email;
    std::string name;

    std::int64_t id() const noexcept override
    {
        return id_;
    }

    void setId(std::int64_t value) noexcept override
    {
        id_ = value;
    }
};
```

This is useful when generic code needs an identity interface.

For example, a tracking layer, cache, or future identity map can work with objects that expose `id()` and `setId()` through a common base type.

Do not inherit from `Entity` only because the type is stored in a database. Inheritance is optional.

## Choosing the right style

Use a plain struct when the type is simple data.

```cpp
struct User
{
    std::int64_t id{};
    std::string email;
    std::string name;
};
```

Use a class when the type needs behavior or invariants.

```cpp
class Account
{
public:
    void withdraw(std::int64_t amount);
    void deposit(std::int64_t amount);

private:
    std::int64_t balance_{};
};
```

Use `vix::orm::Entity` when a shared identity interface is useful.

```cpp
struct User : vix::orm::Entity
{
    std::int64_t id_{};

    std::int64_t id() const noexcept override;
    void setId(std::int64_t value) noexcept override;
};
```

Do not make every entity inherit from `Entity` automatically. Start with the simplest model that fits the application.

## Entity and table shape

The entity should match the columns the application needs.

Given this table:

```sql
CREATE TABLE users (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  email TEXT NOT NULL UNIQUE,
  name TEXT NOT NULL,
  created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);
```

You can map all columns:

```cpp
struct User
{
    std::int64_t id{};
    std::string email;
    std::string name;
    std::string created_at;
};
```

Or you can map only the columns your application uses:

```cpp
struct User
{
    std::int64_t id{};
    std::string email;
    std::string name;
};
```

The mapper decides which columns are read and written.

The entity does not need to mirror every table column if the application does not use every column.

## Entity and `Mapper<T>`

An entity becomes usable by the ORM when it has a mapper.

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

The entity is the data type.
The mapper is the conversion rule.
The repository uses the mapper.

Without a mapper, `Repository<User>` cannot know how to create a `User` from a database row or which fields to insert.

## Entity and `Repository<T>`

A repository connects an entity type to a table.

```cpp
auto users = vix::orm::repository<User>(db, "users");
```

The table name is explicit.

The entity type is explicit.

The mapper is explicit.

This is the core Vix ORM model:

```txt
User
  -> Mapper<User>
  -> Repository<User>
  -> users table
  -> vix::db
```

The repository assumes the table has a primary key column named `id`.

For custom primary keys or advanced queries, use `vix::db` directly or write a project-specific repository wrapper.

## Generated ids

A common pattern is to let the database generate the primary key.

```sql
CREATE TABLE users (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  email TEXT NOT NULL UNIQUE,
  name TEXT NOT NULL
);
```

In that case, do not include `id` in `toInsertFields()`:

```cpp
static vix::orm::FieldValues toInsertFields(const User& user)
{
    return {
        {"email", user.email},
        {"name", user.name}
    };
}
```

The repository returns the generated id:

```cpp
auto id = users.create(User{
    0,
    "ada@example.com",
    "Ada"
});
```

You can then use the id to load the entity:

```cpp
auto user = users.findById(static_cast<std::int64_t>(id));
```

## Application-owned ids

Some applications own their identifiers.

For example, a table may use an external id, a snowflake id, or an id generated before insertion.

```sql
CREATE TABLE devices (
  id BIGINT PRIMARY KEY,
  name TEXT NOT NULL
);
```

In that case, including `id` in `toInsertFields()` can be correct.

```cpp
struct Device
{
    std::int64_t id{};
    std::string name;
};

template <>
struct vix::orm::Mapper<Device>
{
    static Device fromRow(const vix::db::ResultRow& row)
    {
        return Device{
            row.getInt64(0),
            row.getString(1)
        };
    }

    static vix::orm::FieldValues toInsertFields(const Device& device)
    {
        return {
            {"id", device.id},
            {"name", device.name}
        };
    }

    static vix::orm::FieldValues toUpdateFields(const Device& device)
    {
        return {
            {"name", device.name}
        };
    }
};
```

The rule is simple: the mapper should match the database ownership model.
If the database generates the id, do not insert it.
If the application owns the id, insert it deliberately.

## Nullable fields

A database column can be nullable.

In C++, represent optional data with `std::optional<T>` when the difference between “missing” and “present” matters.

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

The mapper can read nullable values explicitly:

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

For insert and update fields, use an empty `std::any` or a database null value when the column should be set to SQL `NULL`.

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

Use nullable fields only when the application truly has an absent state. Do not use `std::optional` just to avoid choosing a clear domain default.

## Entity behavior

Entities can contain behavior.

A domain type does not have to be only fields.

```cpp
class Account
{
public:
    Account(std::int64_t id, std::int64_t balance)
        : id_(id),
          balance_(balance)
    {
    }

    std::int64_t id() const noexcept
    {
        return id_;
    }

    std::int64_t balance() const noexcept
    {
        return balance_;
    }

    void deposit(std::int64_t amount)
    {
        if (amount <= 0)
        {
            return;
        }

        balance_ += amount;
    }

    bool withdraw(std::int64_t amount)
    {
        if (amount <= 0 || amount > balance_)
        {
            return false;
        }

        balance_ -= amount;
        return true;
    }

private:
    std::int64_t id_{};
    std::int64_t balance_{};
};
```

A mapper can then persist the state exposed by the entity.

The ORM does not require an anemic model. Use behavior when it makes the domain clearer.

## Keeping entities clean

Do not put database connection code inside the entity.

Avoid this:

```cpp
struct User
{
    std::int64_t id{};
    std::string email;
    std::string name;

    void save(vix::db::Database& db)
    {
        db.exec(
            "UPDATE users SET email = ?, name = ? WHERE id = ?",
            email,
            name,
            id
        );
    }
};
```

Prefer keeping persistence in a repository:

```cpp
auto users = vix::orm::repository<User>(db, "users");

users.updateById(user.id, user);
```

This keeps the entity focused on application data and behavior. The repository handles persistence.

## File organization

For small examples, keep everything in one file.

For real applications, separate domain types from persistence mappings.

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

This makes the code easier to read as the application grows.

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

This example uses a plain struct because that is enough.

The repository handles persistence.

The mapper handles conversion.

The entity remains simple.

## Common mistakes

### Thinking Entity is required

It is not required.

This is valid:

```cpp
struct User
{
    std::int64_t id{};
    std::string email;
    std::string name;
};
```

### Putting database code inside entities

Keep persistence in repositories and mappers.

### Including generated ids in insert fields by accident

If the database owns the id, leave it out of `toInsertFields()`.

### Forgetting optional fields

If a database column can be `NULL`, decide how the entity represents that.

Use `std::optional<T>` when absence matters.

### Making the entity mirror every column blindly

Map the fields the application needs.

Not every table column has to appear in every entity type.

### Expecting automatic persistence

Changing an entity does not update the database.

Call repository methods explicitly.

## Recommended style

Start with plain structs.

Use classes when behavior or invariants matter.

Use `vix::orm::Entity` only when a shared identity interface is useful.

Keep persistence code outside entities.

Use `Mapper<T>` to describe row and field mapping.

Use repositories for database operations.
Keep entity types easy to test.

## Next steps

Read the mappers page next.

The mapper is what makes an entity usable by the ORM. It defines how the entity is read from database rows and how it is converted into insert and update fields.
