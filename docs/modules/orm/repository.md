# Vix ORM Repository Guide

This guide documents the core repository layer of Vix ORM:

-   `vix::orm::BaseRepository<T>` (generic CRUD for one table)
-   `vix::db::ConnectionPool` + `vix::db::PooledConn` (thread-safe
    pooled connections)

It is written for beginners, but it also includes design notes for
production and advanced usage.

## Who is this for?

### Beginner

You want CRUD quickly without writing SQL everywhere.

### Intermediate

You want a clean data layer (Mapper + Repository) and predictable SQL.

### Advanced / Production

You want to understand how SQL is built, how pooling behaves, and how to
extend the repository safely.

## Quick mental model

-   **Mapper`<T>`{=html}**: teaches Vix how to convert between database
    rows and your C++ type.
-   **BaseRepository`<T>`{=html}**: builds small, explicit SQL (INSERT,
    SELECT by id, UPDATE by id, DELETE by id).
-   **ConnectionPool**: owns reusable connections, shared across
    threads.
-   **PooledConn**: RAII wrapper that acquires a connection and returns
    it automatically.

# 1. BaseRepository`<T>`{=html} overview

`BaseRepository<T>` is a minimal generic repository bound to:

-   a connection pool
-   a single table name
-   a primary key column named **id**
-   a `Mapper<T>` specialization

It is intentionally simple:

-   No magic query language
-   No hidden schema inference
-   Predictable SQL strings

This makes behavior easy to debug and stable in production.

## 1.1 Assumptions and guarantees

### Assumptions

-   Your table primary key column is named `id`
-   `Mapper<T>` defines:
    -   `fromRow(const ResultRow&)`
    -   `toInsertParams(const T&)`
    -   `toUpdateParams(const T&)`
-   Insert uses `lastInsertId()` from the same connection

### Guarantees

-   One connection per call (through `PooledConn`)
-   Prepared statements for safe binding
-   No caching inside the repository (you control caching at a higher
    layer)

# 2. Repository public API

The repository gives you 4 core operations:

-   `create(const T&) -> std::uint64_t`
-   `findById(std::int64_t) -> std::optional<T>`
-   `updateById(std::int64_t, const T&) -> std::uint64_t`
-   `removeById(std::int64_t) -> std::uint64_t`

They are designed to be used as building blocks for business logic.

# 3. How SQL is generated

The repository builds SQL at runtime using small helper builders:

-   `build_insert_cols(params)` -\> `"name,email,age"`
-   `build_insert_qs(n)` -\> `"?,?,?"`
-   `build_update_set(params)` -\> `"name=?,email=?,age=?"`

This matches the repository philosophy: - build simple SQL - bind values
safely - do not hide database behavior

## 3.1 create(): INSERT

Signature:

``` cpp
std::uint64_t create(const T& v);
```

Flow:

1.  `Mapper<T>::toInsertParams(v)` returns vector of `(column, any)`
2.  Build SQL: `INSERT INTO <table> (<cols>) VALUES (<qs>)`
3.  Acquire pooled connection
4.  Prepare + bind + exec
5.  Return `lastInsertId()`

Key implementation detail:

``` cpp
vix::db::PooledConn pc(pool_);
auto st = pc.get().prepare(sql);

for (std::size_t i = 0; i < params.size(); ++i)
  st->bind(i + 1, any_to_dbvalue_or_throw(params[i].second));

st->exec();
return pc.get().lastInsertId();
```

Why this matters: - `lastInsertId()` is correct only if called on the
same connection. - Using `PooledConn` guarantees that.

## 3.2 findById(): SELECT

Signature:

``` cpp
std::optional<T> findById(std::int64_t id);
```

SQL:

``` sql
SELECT * FROM <table> WHERE id = ? LIMIT 1
```

Behavior: - Returns `std::nullopt` if not found - Uses
`Mapper<T>::fromRow()` to build `T`

## 3.3 updateById(): UPDATE

Signature:

``` cpp
std::uint64_t updateById(std::int64_t id, const T& v);
```

Flow: 1. `Mapper<T>::toUpdateParams(v)` returns vector of
`(column, any)` 2. SQL:
`UPDATE <table> SET <col1>=?,<col2>=? WHERE id=?` 3. Bind update params
first, then bind id last 4. Return affected row count

Implementation pattern:

``` cpp
std::size_t idx = 1;
for (const auto& kv : params)
  st->bind(idx++, any_to_dbvalue_or_throw(kv.second));

st->bind(idx, id);
return st->exec();
```

## 3.4 removeById(): DELETE

Signature:

``` cpp
std::uint64_t removeById(std::int64_t id);
```

SQL:

``` sql
DELETE FROM <table> WHERE id = ?
```

Returns: - affected row count (often 0 or 1)

# 4. Mapper`<T>`{=html} contract (important)

Repository works only because `Mapper<T>` provides two directions:

### DB -\> C++

`fromRow(row)` builds an object from a result row.

### C++ -\> DB

`toInsertParams(entity)` returns the insert columns and values.
`toUpdateParams(entity)` returns the update columns and values.

Example skeleton:

``` cpp
namespace vix::orm {
  template <>
  struct Mapper<MyEntity>
  {
    static MyEntity fromRow(const ResultRow& row);

    static std::vector<std::pair<std::string, std::any>>
    toInsertParams(const MyEntity& e);

    static std::vector<std::pair<std::string, std::any>>
    toUpdateParams(const MyEntity& e);
  };
}
```

Beginner rule: - Keep mapping explicit and boring. - Avoid hidden
conversions. - Document your column order if you rely on `SELECT *`.

Production tip: - Prefer explicit select column lists instead of
`SELECT *` when schema evolves frequently.

# 5. ConnectionPool and PooledConn

The repository is safe and scalable because it relies on a real pool.

## 5.1 ConnectionPool guarantees

`ConnectionPool` is thread-safe:

-   It uses a mutex + condition_variable
-   It holds idle connections in a queue
-   It tracks `total_` connections
-   It enforces `cfg.max`

### PoolConfig

``` cpp
struct PoolConfig
{
  std::size_t min = 1;
  std::size_t max = 8;
};
```

Rules of thumb: - CLI tools: min=1 max=2 - small web API: min=2 max=8 -
bigger API: max=16+ (tune by load testing)

## 5.2 warmup()

`warmup()` pre-creates at least `cfg.min` connections.

Why: - avoids a slow first request - validates DB credentials early

## 5.3 PooledConn (RAII)

`PooledConn` acquires a connection in its constructor and releases it in
its destructor.

This means: - no leaks - no "forgot to release" bugs - safe early
returns and exceptions

Key idea:

``` cpp
explicit PooledConn(ConnectionPool& p)
  : pool_(p), c_(p.acquire()) {}

~PooledConn() noexcept
{
  if (c_)
    pool_.release(std::move(c_));
}
```

# 6. Transactions and repositories

BaseRepository methods are single calls. They do not start transactions
for you.

For multi-step business logic (user + order, etc.), prefer:

-   `Transaction` (low level)
-   `UnitOfWork` (business-oriented grouping)

Pattern: - Do not call `repo.create()` many times if you need atomic
behavior across tables. - Instead, use a UnitOfWork and run all
statements on the same connection, then commit.

This is why Vix provides the UnitOfWork example.

# 7. Extending BaseRepository safely

You will eventually want more than CRUD-by-id.

Best pattern: - Keep BaseRepository as a simple primitive - Add a typed
repository for your domain, and implement explicit queries

Example idea (pseudocode):

``` cpp
class UsersRepository : public BaseRepository<User>
{
public:
  using BaseRepository<User>::BaseRepository;

  std::optional<User> findByEmail(const std::string& email)
  {
    // Build explicit SQL, bind, map with Mapper<User>::fromRow
  }
};
```

Why this is best: - You keep generic code stable - You add domain
queries where they belong - You avoid turning BaseRepository into a huge
abstraction

# 8. Performance notes

### Prepared statements

Repository uses prepared statements and parameter binding, which is
good.

### String building

SQL strings are built per call. This is fine for most apps, but if you
need extreme throughput: - cache common SQL strings in your typed
repository - keep column lists stable

### std::any

`toInsertParams()` returns `std::any` values. This is flexible, but it
can add overhead in hot paths. If you need maximum performance: - add
typed bind APIs, or - add specialized mapper helpers, or - keep ORM for
productivity and use raw SQL for hotspots

# 9. Common beginner mistakes

1.  Forgetting the table has `id` as primary key
2.  Wrong column order assumptions with `SELECT *`
3.  Putting `id` into insert params (auto increment should generate it)
4.  Using the repository for multi-step operations without a transaction

# 10. Minimal example (CRUD)

``` cpp
BaseRepository<User> repo{pool, "users"};

// Create
auto id = repo.create(User{0, "Bob", "bob@example.com", 30});

// Read
auto u = repo.findById(static_cast<std::int64_t>(id));

// Update
repo.updateById(static_cast<std::int64_t>(id), User{static_cast<std::int64_t>(id), "Bob2", "bob2@example.com", 31});

// Delete
repo.removeById(static_cast<std::int64_t>(id));
```

# Summary

-   `Mapper<T>` defines mapping rules.
-   `BaseRepository<T>` provides minimal CRUD for one table.
-   `ConnectionPool` provides scalable reusable connections.
-   `PooledConn` ensures RAII safety.
-   For multi-step operations, use transactions or UnitOfWork.
