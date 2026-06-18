# Connection pool

A database connection pool keeps database connections open and reuses them across operations.

Without a pool, an application can end up opening a new database connection for every query, request, command, or background job. That is expensive for server databases such as MySQL, and it can also make local database code harder to control when multiple parts of the application run at the same time.

Vix provides a connection pool in the database module.

Most applications do not need to create the pool manually. The high-level `vix::db::Database` facade owns a pool internally and uses it when you call `exec()` or `query()`.

```cpp
#include <vix.hpp>
#include <vix/db.hpp>

int main()
{
    auto db = vix::db::Database::sqlite("storage/app.db");

    db.exec(
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "email TEXT NOT NULL UNIQUE"
        ")"
    );

    db.exec(
        "INSERT OR IGNORE INTO users (email) VALUES (?)",
        "ada@example.com"
    );

    auto rows = db.query("SELECT id, email FROM users");

    while (rows->next())
    {
        const auto& row = rows->row();

        vix::print(
            vix::options{.sep = " | "},
            row.getInt64(0),
            row.getString(1)
        );
    }

    return 0;
}
```

In this example, the application does not acquire or release connections manually. `Database` handles the common path.

## Why pooling exists

A database connection is a resource.

For MySQL, a connection means a live connection to a database server. Creating it repeatedly adds latency, consumes server resources, and can become a bottleneck under concurrent load.

For SQLite, a connection is local and lighter, but it is still a resource tied to a database file and driver state. Keeping connection usage explicit helps the application remain predictable.

A pool gives the application two useful properties.

First, connections can be reused.

Second, the application can limit how many connections exist at the same time.

That limit matters. An application should not be able to accidentally create hundreds of database connections just because many requests arrive at once.

## The default path

Use the `Database` facade first.

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

The facade creates a connection factory, initializes a connection pool, and uses that pool for high-level operations.

```cpp
db.exec(
    "INSERT INTO users (email, name) VALUES (?, ?)",
    "grace@example.com",
    "Grace Hopper"
);

auto rows = db.query(
    "SELECT id, email, name FROM users ORDER BY id"
);
```

This is the recommended style for normal application queries.

Use the lower-level pool API only when you need direct connection ownership.

## Pool configuration

A pool is configured with `vix::db::PoolConfig`.

```cpp
vix::db::PoolConfig pool;
pool.min = 1;
pool.max = 8;
```

`min` is the number of connections Vix should create during warmup.

`max` is the maximum number of live connections the pool should allow.

You can pass the pool configuration when creating a database.

```cpp
#include <vix.hpp>
#include <vix/db.hpp>

int main()
{
    vix::db::PoolConfig pool;
    pool.min = 1;
    pool.max = 8;

    auto db = vix::db::Database::sqlite(
        "storage/app.db",
        pool
    );

    vix::print("database ready");

    return 0;
}
```

For MySQL:

```cpp
#include <vix.hpp>
#include <vix/db.hpp>

int main()
{
    vix::db::PoolConfig pool;
    pool.min = 1;
    pool.max = 8;

    auto db = vix::db::Database::mysql(
        "tcp://127.0.0.1:3306",
        "root",
        "secret",
        "app_db",
        pool
    );

    vix::print("mysql database ready");

    return 0;
}
```

A good local default is usually:

```txt
min: 1
max: 8
```

Do not increase `max` without a reason. More connections do not automatically make an application faster. If the database server is already saturated, more connections can make latency worse.

## How acquisition works

When the application needs a connection, the pool tries to reuse an idle connection.

If an idle connection exists and is still valid, it is returned.

If no idle connection exists and the pool has not reached `max`, the pool creates a new connection.

If the pool has already reached `max`, the caller waits until another operation releases a connection.

That behavior gives the application backpressure. Instead of creating unlimited database connections, the application waits at the pool boundary.

This is important for backend services. It keeps database pressure controlled when concurrent requests arrive.

## Connection health

Connections can become invalid.

A MySQL server can close an idle connection. A network connection can fail. A database process can restart. Even local database state can become unavailable if files move or permissions change.

Vix connections expose `ping()`.

The pool can check a connection before reusing it. If the connection is invalid, the pool can discard it and create another one when allowed by the pool limit.

This does not remove the need for error handling. Database operations can still fail. The point of `ping()` is to avoid reusing obviously broken connections.

## Manual pool usage

Most code should use `Database::exec()` and `Database::query()`.

Use manual pool access when several low-level operations need to share a connection and you are not using `Transaction`.

```cpp
#include <vix.hpp>
#include <vix/db.hpp>

int main()
{
    auto db = vix::db::Database::sqlite("storage/app.db");

    vix::db::PooledConn conn(db.pool());

    auto stmt = conn->prepare(
        "INSERT INTO users (email, name) VALUES (?, ?)"
    );

    stmt->bind(1, "bjarne@example.com");
    stmt->bind(2, "Bjarne");
    stmt->exec();

    vix::print("user inserted");

    return 0;
}
```

`PooledConn` acquires a connection when it is constructed.

When the `PooledConn` object is destroyed, the connection is released back to the pool.

That is the recommended manual style because it uses RAII and avoids forgotten releases.

## Avoid manual acquire/release when possible

The pool also has lower-level `acquire()` and `release()` operations.

They are useful internally and for advanced integration code, but application code should prefer `PooledConn`.

This is more error-prone:

```cpp
auto conn = db.pool().acquire();

auto stmt = conn->prepare(
    "INSERT INTO users (email, name) VALUES (?, ?)"
);

stmt->bind(1, "ada@example.com");
stmt->bind(2, "Ada");
stmt->exec();

db.pool().release(std::move(conn));
```

If an exception happens before `release()`, the connection may not return to the pool.

This is safer:

```cpp
vix::db::PooledConn conn(db.pool());

auto stmt = conn->prepare(
    "INSERT INTO users (email, name) VALUES (?, ?)"
);

stmt->bind(1, "ada@example.com");
stmt->bind(2, "Ada");
stmt->exec();
```

When `conn` leaves scope, the connection returns to the pool automatically.

## Pooling and queries

A query result may depend on the connection that produced it.

The high-level `Database::query()` returns a result set that keeps the pooled connection alive while the result is being read.

```cpp
auto rows = db.query(
    "SELECT id, email FROM users ORDER BY id"
);

while (rows->next())
{
    const auto& row = rows->row();

    vix::print(
        vix::options{.sep = " | "},
        row.getInt64(0),
        row.getString(1)
    );
}
```

This is why you can safely read from the result set without manually keeping a connection variable next to it.

Do not store row references after the result set moves forward.

Do not return `const ResultRow&` from a function.

Copy values into your own types.

```cpp
struct User
{
    std::int64_t id{};
    std::string email;
};

User readUser(const vix::db::ResultRow& row)
{
    return User{
        row.getInt64(0),
        row.getString(1)
    };
}
```

## Pooling and transactions

A transaction needs one connection for the full duration of the transaction.

`vix::db::Transaction` acquires a pooled connection and starts a transaction on it.

```cpp
#include <vix.hpp>
#include <vix/db.hpp>

int main()
{
    auto db = vix::db::Database::sqlite("storage/app.db");

    vix::db::Transaction tx(db.pool());

    auto& conn = tx.conn();

    auto insertUser = conn.prepare(
        "INSERT INTO users (email, name) VALUES (?, ?)"
    );
    insertUser->bind(1, "linus@example.com");
    insertUser->bind(2, "Linus");
    insertUser->exec();

    auto insertProfile = conn.prepare(
        "INSERT INTO profiles (email, bio) VALUES (?, ?)"
    );
    insertProfile->bind(1, "linus@example.com");
    insertProfile->bind(2, "Created inside one transaction.");
    insertProfile->exec();

    tx.commit();

    vix::print("transaction committed");

    return 0;
}
```

The connection is released when the transaction object is destroyed.

If `commit()` is not called, the transaction rolls back automatically.

This combines pooling with C++ lifetime management.

## Pool size for SQLite

SQLite is local and file-based.

For local tools, examples, tests, and small applications, keep the pool small.

```cpp
vix::db::PoolConfig pool;
pool.min = 1;
pool.max = 4;

auto db = vix::db::Database::sqlite("storage/app.db", pool);
```

A large pool is rarely useful for a simple SQLite application. SQLite can support concurrent access, but its concurrency model is different from a server database.

For write-heavy workloads, measure before increasing pool size.

A small pool is easier to reason about.

## Pool size for MySQL

MySQL is server-backed.

A pool is more important because opening a connection can be expensive and because backend services may handle many requests.

```cpp
vix::db::PoolConfig pool;
pool.min = 1;
pool.max = 8;

auto db = vix::db::Database::mysql(
    "tcp://127.0.0.1:3306",
    "root",
    "secret",
    "app_db",
    pool
);
```

For production, choose `max` based on the whole deployment, not just one process.

If you run 10 application instances and each has a pool of 20 connections, the database may see up to 200 application connections.

That may be too many.

Start with a conservative pool size. Measure query latency, request concurrency, database CPU, connection count, and slow queries before increasing it.

## Pool configuration in vix.json

A project can describe database settings in `vix.json`.

A SQLite project:

```json
{
  "name": "Blog",
  "database": {
    "engine": "sqlite",
    "sqlite": {
      "path": "storage/Blog.db",
      "pool": {
        "min": 1,
        "max": 4
      }
    },
    "storage": "storage",
    "migrations": "migrations"
  }
}
```

A MySQL project:

```json
{
  "name": "Blog",
  "database": {
    "engine": "mysql",
    "mysql": {
      "host": "127.0.0.1",
      "port": 3306,
      "user": "root",
      "password": "secret",
      "database": "blog",
      "pool": {
        "min": 1,
        "max": 8
      }
    },
    "migrations": "migrations"
  }
}
```

Keep pool configuration near the selected engine. That makes it clear which backend the settings apply to.

If your current project configuration parser supports only a top-level database pool, keep the same values there and document that convention inside the project.

## Blocking behavior

When all connections are busy and the pool has reached `max`, acquiring another connection waits.

This behavior is intentional.

It prevents the application from creating unlimited database connections during load spikes.

In an HTTP server, this can mean a request waits for a database connection. That is usually better than overwhelming the database server.

For latency-sensitive applications, the right solution is not always a larger pool. The real fix may be shorter transactions, faster queries, better indexes, fewer database round trips, or moving non-critical work outside the request path.

## Keep transactions short

A transaction holds a connection until it commits or rolls back.

This means long transactions reduce the number of connections available to the rest of the application.

Keep transactions short.

Do the database work inside the transaction.

Avoid slow network calls, filesystem work, or expensive computation while the transaction is open.

Prefer this shape:

```cpp
vix::db::Transaction tx(db.pool());

// database writes here

tx.commit();
```

Avoid this shape:

```cpp
vix::db::Transaction tx(db.pool());

// database write
// call remote API
// process large file
// database write

tx.commit();
```

A connection pool protects the database from unlimited connections. It does not make long-held connections free.

## Repositories and pooling

In larger applications, pass `vix::db::Database&` to repositories.

```cpp
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vix.hpp>
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

private:
    vix::db::Database& db_;
};
```

The repository does not own the pool.

The application owns the `Database` object.

Each repository method uses the database facade, and the facade uses the pool.

That keeps ownership simple.

## Application lifetime

Create the database object near the application startup.

Keep it alive for as long as the application needs database access.

Do not create a new `Database` object for every query.

Prefer this:

```cpp
int main()
{
    auto db = vix::db::Database::sqlite("storage/app.db");

    UserRepository users(db);

    // application starts here

    return 0;
}
```

Avoid this:

```cpp
void createUser()
{
    auto db = vix::db::Database::sqlite("storage/app.db");

    db.exec(
        "INSERT INTO users (email, name) VALUES (?, ?)",
        "ada@example.com",
        "Ada"
    );
}
```

Creating the database object repeatedly can recreate pool state and defeat the point of pooling.

## Error handling

Pool operations and database operations can throw exceptions.

Handle them at the application boundary where you can make a useful decision.

```cpp
#include <exception>
#include <vix.hpp>
#include <vix/db.hpp>

int main()
{
    try
    {
        vix::db::PoolConfig pool;
        pool.min = 1;
        pool.max = 8;

        auto db = vix::db::Database::sqlite(
            "storage/app.db",
            pool
        );

        vix::print("database ready");

        return 0;
    }
    catch (const std::exception& e)
    {
        vix::print("database error:", e.what());
        return 1;
    }
}
```

If pool warmup fails, the application should fail early. A backend service should not report itself ready when it cannot create the required database connections.

## Common mistakes

### Creating one database object per query

Do not do this:

```cpp
void saveUser(std::string email)
{
    auto db = vix::db::Database::sqlite("storage/app.db");

    db.exec(
        "INSERT INTO users (email) VALUES (?)",
        std::move(email)
    );
}
```

Create the database object once and pass it where it is needed.

### Using manual acquire/release in normal code

Prefer `PooledConn`.

```cpp
vix::db::PooledConn conn(db.pool());
```

This gives automatic release when the object leaves scope.

### Holding a connection too long

Do not keep a pooled connection in a long-lived object unless that object truly needs exclusive connection ownership.

A repository should usually store `Database&`, not `PooledConn`.

### Increasing the pool size blindly

A larger pool is not always better.

First check query speed, indexes, transaction length, database capacity, and the number of application instances.

### Returning row references

A row reference belongs to the result set iteration.

Copy values into application types instead.

## Recommended style

Use `Database::exec()` and `Database::query()` for normal queries.

Configure pool size when creating the `Database`.

Use `PooledConn` for manual connection access.

Use `Transaction` for multi-step writes.

Keep transactions short.

Keep the database object alive for the application lifetime.

Use small pools for SQLite.

Use measured, conservative pools for MySQL.

## Next steps

Read the transactions page to understand how Vix keeps a pooled connection inside a transaction.

Read the migrations page to understand how schema changes are applied safely.

Read the CLI page to see how database configuration and migrations are used from `vix db` and `vix orm`.
