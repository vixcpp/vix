# ORM Guide

This document covers the Vix ORM module at a practical level.

It is split in two parts:

1) ORM C++ API (entities, mappers, repositories, unit of work)
2) `vix orm` CLI (migrations and schema evolution)

The ORM is intentionally minimal:
- explicit SQL
- predictable behavior
- small abstractions only where they remove boilerplate

---

## Include

Most users include the umbrella header:

```cpp
#include <vix/orm/orm.hpp>
using namespace vix::orm;
```

Internally, it includes:
- `QueryBuilder.hpp`
- `Entity.hpp`
- `Mapper.hpp`
- `Repository.hpp`
- `UnitOfWork.hpp`
- `db_compat.hpp`

---

# 1) ORM C++ API

## Core building blocks

### Entity

`Entity` is a semantic base class for ORM-managed types.

```cpp
struct Entity
{
  virtual ~Entity() = default;
};
```

It contains no data and no behavior.
It exists to:
- provide a clear extension point
- allow polymorphic handling when needed

You do not need to inherit from it for the ORM to work, but it can be useful for consistency.

---

## Mapper

`Mapper<T>` defines how an entity is mapped to and from the database.

It is a template that you fully specialize per entity type.

It has three responsibilities:

- `fromRow(row)`
  Build `T` from a database result row

- `toInsertParams(v)`
  Produce column/value pairs for INSERT

- `toUpdateParams(v)`
  Produce column/value pairs for UPDATE

All values are stored as `std::any` to keep the mapper flexible.
The ORM later converts them into `vix::db::DbValue`.

Example specialization:

```cpp
struct User
{
  std::int64_t id = 0;
  std::string name;
  std::int64_t age = 0;
};

template<>
struct vix::orm::Mapper<User>
{
  static User fromRow(const vix::db::ResultRow& row)
  {
    User u;
    u.id = row.i64("id").value_or(0);
    u.name = row.str("name").value_or("");
    u.age = row.i64("age").value_or(0);
    return u;
  }

  static std::vector<std::pair<std::string, std::any>>
  toInsertParams(const User& u)
  {
    return {
      {"name", u.name},
      {"age",  u.age}
    };
  }

  static std::vector<std::pair<std::string, std::any>>
  toUpdateParams(const User& u)
  {
    return {
      {"name", u.name},
      {"age",  u.age}
    };
  }
};
```

Notes:
- `toInsertParams` usually excludes `"id"` (auto-increment).
- `toUpdateParams` usually excludes immutable fields.

---

## Repository: BaseRepository

`BaseRepository<T>` is a minimal generic CRUD repository for a single table.

Assumptions:
- the table primary key column is named `"id"`
- `Mapper<T>` is specialized for your entity

### Constructor

```cpp
BaseRepository(vix::db::ConnectionPool& pool, std::string table);
```

### Create

```cpp
std::uint64_t create(const T& v);
```

- Uses `Mapper<T>::toInsertParams`
- Builds:

```sql
INSERT INTO table (col1,col2,...) VALUES (?,?,...)
```

- Returns last insert id

### Find by id

```cpp
std::optional<T> findById(std::int64_t id);
```

- Executes:

```sql
SELECT * FROM table WHERE id = ? LIMIT 1
```

- Returns `std::nullopt` if not found

### Update by id

```cpp
std::uint64_t updateById(std::int64_t id, const T& v);
```

- Uses `Mapper<T>::toUpdateParams`
- Builds:

```sql
UPDATE table SET a=?,b=?,... WHERE id=?
```

- Returns affected rows

### Delete by id

```cpp
std::uint64_t removeById(std::int64_t id);
```

- Executes:

```sql
DELETE FROM table WHERE id = ?
```

---

## UnitOfWork

`UnitOfWork` groups operations in a transaction, using RAII:

- transaction begins at construction
- rollback happens on destruction unless committed

```cpp
vix::orm::UnitOfWork uow(pool);

try
{
  // do work using uow.conn() or repositories
  uow.commit();
}
catch (...)
{
  // optional explicit rollback
  uow.rollback();
  throw;
}
```

### Access the transaction connection

```cpp
vix::db::Connection& c = uow.conn();
```

Use this for:
- multiple repositories in one transaction
- custom SQL in the same transaction scope

---

## QueryBuilder

`QueryBuilder` is a tiny helper to build SQL + parameter list.

It keeps:
- `sql_` as a string
- `params_` as `std::vector<vix::db::DbValue>`

Example:

```cpp
QueryBuilder q;
q.raw("SELECT * FROM users WHERE age > ?").param(18);

auto st = conn.prepare(q.sql());
const auto& ps = q.params();
for (std::size_t i = 0; i < ps.size(); ++i)
  st->bind(i + 1, ps[i]);

auto rs = st->query();
```

This is intentionally minimal. It does not try to validate SQL.

---

## db_compat.hpp

This header is a compatibility layer that re-exports the DB module types into `vix::orm`.

It also provides:

### `any_to_dbvalue_or_throw(const std::any&)`

The ORM uses `std::any` in mappers. This function converts runtime values into `vix::db::DbValue`.

Supported types include:
- empty `std::any` / `nullptr_t` -> NULL
- `vix::db::DbValue` -> passthrough
- bool
- integral types -> int64 (best-effort narrowing)
- float/double -> double
- std::string / std::string_view / const char* -> string
- `vix::db::Blob` -> blob

If the type is not supported, it throws `vix::db::DBError`.

Practical rule:
- keep mapper fields to basic scalar types and strings
- use `vix::db::Blob` for binary data
- avoid custom structs inside `std::any`

---

# 2) CLI: vix orm

`vix orm` manages:
- database migrations
- schema evolution
- migration history
- rollback operations

It keeps schema changes explicit and versioned.

## Usage

```bash
vix orm migrate   [options]
vix orm rollback  --steps <n> [options]
vix orm status    [options]
vix orm makemigrations --new <schema.json> [options]
```

---

## Commands

### migrate

Apply pending migrations.

```bash
vix orm migrate
```

---

### rollback

Rollback last N applied migrations.

```bash
vix orm rollback --steps 1
```

Required:
- `--steps <n>`

---

### status

Show applied and pending migrations.

```bash
vix orm status
```

---

### makemigrations

Generate a migration from a schema diff.

```bash
vix orm makemigrations --new ./schema.new.json
```

Options:
- `--new <path>`          New schema (required)
- `--snapshot <path>`     Previous schema snapshot (default: `schema.json`)
- `--name <label>`        Migration label (default: auto)
- `--dialect <mysql|sqlite>`  SQL dialect (default: mysql)

---

## Common options

```bash
--db <name>           Database name
--dir <path>          Migrations directory
--host <uri>          MySQL URI
--user <name>         Database user
--pass <pass>         Database password
--project-dir <path>  Force project root detection
--tool <path>         Override migrator executable path
```

---

## Environment defaults

If you prefer env configuration:

```bash
VIX_ORM_HOST
VIX_ORM_USER
VIX_ORM_PASS
VIX_ORM_DB
VIX_ORM_DIR
VIX_ORM_TOOL
```

Example:

```bash
VIX_ORM_DB=blog_db VIX_ORM_DIR=./migrations vix orm migrate
```

---

## Examples

Apply migrations:

```bash
vix orm migrate --db blog_db --dir ./migrations
```

Rollback:

```bash
vix orm rollback --steps 1 --db blog_db --dir ./migrations
```

Status:

```bash
vix orm status --db blog_db
```

Generate migration:

```bash
vix orm makemigrations \
  --new ./schema.new.json \
  --snapshot ./schema.json \
  --dir ./migrations \
  --name create_users \
  --dialect mysql
```

---

# Summary

The Vix ORM is a thin, explicit layer over `vix::db`:

- `Mapper<T>` defines the mapping
- `BaseRepository<T>` gives minimal CRUD
- `UnitOfWork` groups operations transactionally
- `QueryBuilder` builds SQL + params with no magic
- `vix orm` CLI manages migrations and schema evolution

