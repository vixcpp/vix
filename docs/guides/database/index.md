# Database

Vix.cpp includes a database module for C++ applications that need SQL access without building a full database layer from scratch.

The module is built around a small set of practical ideas: open a database, execute prepared statements, read result sets, reuse connections safely, wrap writes in transactions, and manage schema changes with migrations. It is designed for applications that need to move beyond toy examples while keeping the database code readable and explicit.

The main entry point is:

```cpp
#include <vix/db.hpp>
```

Most applications should start with `vix::db::Database`. Lower-level types such as connections, statements, result sets, pools, and migration runners exist when you need more control, but they are not required for the common path.

## Why this module exists

C++ gives you full control, but database code often becomes noisy quickly. A simple backend can end up mixing driver APIs, manual connection ownership, raw prepared statements, transaction cleanup, migration tracking, and error handling in the same files.

Vix keeps those concerns separated.

The database module provides a common interface over supported SQL engines. SQLite can be used for local development, embedded applications, tests, tools, and local-first storage. MySQL can be used for server applications and production systems that need a dedicated database server.

The goal is not to hide SQL. The goal is to make SQL usable from modern C++ with fewer repetitive mechanics around it.

## A first example

This example opens a SQLite database, creates a table, inserts a row, and reads it back.

```cpp
#include <vix.hpp>
#include <vix/db.hpp>

int main()
{
    auto db = vix::db::Database::sqlite("app.db");

    db.exec(
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT NOT NULL"
        ")"
    );

    db.exec(
        "INSERT INTO users (name) VALUES (?)",
        "Gaspard"
    );

    auto rows = db.query(
        "SELECT id, name FROM users ORDER BY id DESC LIMIT 1"
    );

    while (rows->next())
    {
        const auto& row = rows->row();

        vix::print(
            "user: {} {}",
            row.getInt64(0),
            row.getString(1)
        );
    }

    return 0;
}
```

The important part is not that Vix replaces SQL. It does not. The SQL stays visible, and the application remains clear. Vix handles the repetitive C++ side: preparing statements, binding values, returning a result set, and keeping the connection alive while the result is being read.

## What Vix provides

The database module is organized around a few core pieces.

`vix::db::Database` is the high-level facade. It selects the database engine, creates the connection factory, initializes the connection pool, and exposes convenience methods such as `exec()` and `query()`.

`vix::db::Connection` represents a live database connection. Drivers implement this interface for each backend.

`vix::db::Statement` represents a prepared SQL statement. It supports positional parameters and typed binding through Vix database values.

`vix::db::ResultSet` and `vix::db::ResultRow` represent query results. Result sets are forward-only. Rows expose typed accessors such as `getString()`, `getInt64()`, and `getDouble()`.

`vix::db::ConnectionPool` manages reusable database connections. It helps server applications avoid opening a new database connection for every request.

`vix::db::Transaction` provides RAII transaction handling. If a transaction is still active when the object is destroyed, it is rolled back automatically.

`vix::db::FileMigrationsRunner` applies SQL migrations from disk and records applied migrations in a tracking table.

The result is a database layer that can be used directly in small programs, but is also structured enough for backend applications, repositories, service layers, CLI tools, and production workflows.

## Supported engines

Vix currently focuses on SQLite and MySQL.

SQLite is the best default for local applications, development tools, test suites, embedded storage, small services, and local-first applications. It stores data in a file and works well when you want the application to run without requiring a separate database server.

MySQL is useful when the application needs a shared database server, remote access, established operational tooling, backups, replication, or deployment in a more traditional backend environment.

Both engines are accessed through the same high-level Vix API. Engine-specific behavior still exists, because SQL engines are not identical, but application code can stay close to the same structure.

## SQLite

SQLite support is useful when the database should live with the project or application.

```cpp
auto db = vix::db::Database::sqlite("storage/app.db");
```

A SQLite-backed application can use the same `exec()` and `query()` flow as any other Vix database application.

```cpp
db.exec(
    "CREATE TABLE IF NOT EXISTS posts ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "title TEXT NOT NULL,"
    "body TEXT NOT NULL"
    ")"
);

db.exec(
    "INSERT INTO posts (title, body) VALUES (?, ?)",
    "Hello Vix",
    "This post is stored in SQLite."
);
```

SQLite is a good fit for Vix examples because it makes the database workflow reproducible. A developer can clone a project, run it, inspect the local database file, apply migrations, and delete the storage directory without provisioning infrastructure.

## MySQL

MySQL support is intended for applications that connect to a running MySQL-compatible server.

```cpp
auto db = vix::db::Database::mysql(
    "tcp://127.0.0.1:3306",
    "root",
    "secret",
    "app_db"
);
```

The query API remains the same:

```cpp
db.exec(
    "CREATE TABLE IF NOT EXISTS users ("
    "id BIGINT AUTO_INCREMENT PRIMARY KEY,"
    "email VARCHAR(255) NOT NULL UNIQUE,"
    "name VARCHAR(255) NOT NULL"
    ")"
);

db.exec(
    "INSERT INTO users (email, name) VALUES (?, ?)",
    "gaspard@example.com",
    "Gaspard"
);
```

For production services, the value of MySQL support is not only the connection itself. The important part is the surrounding workflow: connection pooling, prepared statements, transactions, migration files, CLI inspection, and explicit configuration.

## Prepared statements and binding

Vix uses prepared statements for values passed to `exec()` and `query()`.

```cpp
db.exec(
    "UPDATE users SET name = ? WHERE id = ?",
    "Grace",
    1
);
```

Parameters are positional. The first value binds to the first placeholder, the second value binds to the second placeholder, and so on.

The database module supports common value types used in application code, including integers, floating point values, booleans, strings, null values, and binary values at the lower-level API.

Prepared statements keep values separate from SQL text. That makes application code easier to read and avoids manually building SQL strings with user data.

## Reading results

A query returns a result set.

```cpp
auto rows = db.query(
    "SELECT id, email, name FROM users ORDER BY id"
);

while (rows->next())
{
    const auto& row = rows->row();

    const auto id = row.getInt64(0);
    const auto email = row.getString(1);
    const auto name = row.getString(2);

    vix::print("{} {} {}", id, email, name);
}
```

Column access is index-based. Indexes start at `0`, following the order of the selected columns.

For nullable columns, use the `Or` helpers when a default value is acceptable:

```cpp
const auto displayName = row.getStringOr(2, "anonymous");
const auto score = row.getInt64Or(3, 0);
```

## Connection pooling

Backend applications should not open a new database connection for every request. A connection pool keeps a controlled number of connections available and reuses them across operations.

The high-level `Database` facade owns a pool internally. For advanced cases, the pool can be used directly.

```cpp
vix::db::PoolConfig pool;
pool.min = 1;
pool.max = 8;

auto db = vix::db::Database::sqlite("storage/app.db", pool);
```

A pool matters when a Vix HTTP server or worker system performs many database operations. It gives the application a clear upper bound on database connections and avoids repeating connection setup work unnecessarily.

## Transactions

A transaction groups multiple database changes into one unit.

```cpp
vix::db::Transaction tx(db.pool());

auto& conn = tx.conn();

auto create = conn.prepare(
    "INSERT INTO accounts (name, balance) VALUES (?, ?)"
);
create->bind(1, "main");
create->bind(2, 1000);
create->exec();

auto update = conn.prepare(
    "UPDATE accounts SET balance = balance - ? WHERE name = ?"
);
update->bind(1, 100);
update->bind(2, "main");
update->exec();

tx.commit();
```

If `commit()` is not called, the transaction rolls back when the `Transaction` object is destroyed. That behavior is useful in C++ because exceptions and early returns should not leave a transaction open.

## Migrations

Vix supports file-based SQL migrations.

A migration directory can contain files such as:

```txt
migrations/
├── 2026_06_02_120000_create_users.up.sql
└── 2026_06_02_120000_create_users.down.sql
```

The `up` file applies a schema change.

```sql
CREATE TABLE users (
  id BIGINT AUTO_INCREMENT PRIMARY KEY,
  email VARCHAR(255) NOT NULL UNIQUE,
  name VARCHAR(255) NOT NULL
);
```

The `down` file reverts it.

```sql
DROP TABLE users;
```

Applied migrations are tracked in a database table. The runner stores the migration id, checksum, and applied timestamp. This allows Vix to know which migrations have already been applied and to detect when an applied migration file has changed.

Migrations can be applied from C++ code through the migration runner, and they can also be managed from the Vix CLI.

## Schema snapshots

The database module also includes a schema model.

A schema can describe tables, columns, indexes, column types, defaults, primary keys, uniqueness, and nullability. It can be serialized to JSON, compared with another schema, and used to generate migration operations.

This is the foundation for workflows where a project keeps a schema snapshot and generates migrations from changes.

A simplified schema JSON can look like this:

```json
{
  "version": 1,
  "tables": [
    {
      "name": "users",
      "columns": [
        {
          "name": "id",
          "type": {
            "base": "bigint"
          },
          "nullable": false,
          "primary_key": true,
          "auto_increment": true
        },
        {
          "name": "email",
          "type": {
            "base": "varchar",
            "size": 255
          },
          "nullable": false,
          "unique": true
        }
      ],
      "indexes": []
    }
  ]
}
```

Schema snapshots are not required for simple applications. They become useful when a project wants a more structured migration workflow and a clear record of how the database shape changes over time.

## CLI workflow

Vix also exposes database tooling through the CLI.

Use `vix db` to inspect database state, check SQLite storage, apply SQLite migrations, and create local SQLite backups.

```bash
vix db status
vix db migrate
vix db backup
```

Use `vix orm` for migration-oriented workflows such as applying migrations, rolling back migrations, checking migration status, and generating migration files from schema snapshots.

```bash
vix orm migrate
vix orm rollback --steps 1
vix orm status
vix orm makemigrations --new ./schema.new.json
```

The CLI exists so database work is not trapped inside application code. A project can check database status in development, scripts, CI, and deployment workflows.

## Configuration

Database configuration can be stored in `vix.json`.

A SQLite project can use:

```json
{
  "name": "Blog",
  "database": {
    "engine": "sqlite",
    "sqlite": {
      "path": "storage/Blog.db"
    },
    "storage": "storage",
    "migrations": "migrations"
  }
}
```

A MySQL project can use:

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
      "database": "blog"
    },
    "migrations": "migrations"
  }
}
```

Configuration keeps database details out of source files and gives the CLI a consistent place to read project database settings.

## When to use the database module

Use the database module when your Vix application needs SQL and you want the database layer to remain explicit.

It is a good fit for:

| Use case               | Why it fits                                                                 |
| ---------------------- | --------------------------------------------------------------------------- |
| Local tools            | SQLite works without a separate server.                                     |
| HTTP APIs              | Queries, transactions, and pooling are available from C++.                  |
| Backend services       | MySQL support works with production database servers.                       |
| Test projects          | SQLite makes database tests easy to create and reset.                       |
| Migration workflows    | SQL migration files can be applied and tracked.                             |
| Schema-driven projects | JSON schema snapshots can be compared and used to generate migration files. |

For very small scripts, direct SQL through `Database::sqlite()` is usually enough. For larger projects, keep database access behind repositories or service classes, and let the Vix database module handle connections, prepared statements, transactions, and migration workflows.

## Related pages

Continue with the quick start if you want to write and run the smallest database example first.

For project setup, read the configuration page.

For backend usage, read the pages on queries, connection pooling, and transactions.

For schema evolution, read the pages on migrations and schema snapshots.
