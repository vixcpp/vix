# MySQL

MySQL support in Vix.cpp is intended for C++ applications that need to connect to a real database server.

SQLite is often the best first database for local development, examples, tests, and embedded storage. MySQL becomes useful when the application needs a shared database, remote access, production operations, backups, replication, existing MySQL infrastructure, or a database managed outside the application process.

Vix does not try to replace MySQL or hide SQL. It gives C++ applications a cleaner way to work with MySQL through the same database module used by the rest of Vix:

```cpp id="mudjri"
#include <vix/db.hpp>
```

The high-level entry point is:

```cpp id="my0vh1"
auto db = vix::db::Database::mysql(
    "tcp://127.0.0.1:3306",
    "root",
    "secret",
    "app_db"
);
```

From there, the application can use `exec()`, `query()`, prepared parameters, result sets, connection pooling, and transactions.

## When MySQL is the right choice

Use MySQL when the database is not just a local application file.

A backend service may have several application instances connecting to the same database. A production system may need backups, monitoring, users, permissions, replication, managed hosting, or operational tooling that already exists around MySQL. A company may already run MySQL and want the C++ application to fit into that environment.

In those cases, MySQL gives the application a server-backed persistence layer. Vix gives the C++ code a small and consistent interface for using it.

## How Vix uses MySQL

The MySQL driver is built behind the generic Vix database interfaces.

Application code should usually use:

```cpp id="e4k4xj"
vix::db::Database
```

The lower-level driver types exist for integration work, but most projects do not need to use them directly.

Internally, the MySQL driver adapts MySQL Connector/C++ to the Vix database model:

```txt id="h7so3z"
vix::db::Database
  -> ConnectionPool
  -> MySQLConnection
  -> MySQL Statement
  -> MySQL ResultSet
```

The value of this design is that the application code does not have to depend directly on MySQL Connector/C++ classes in every repository or service. The driver-specific code stays inside the database module.

## Minimal MySQL program

This program connects to MySQL, creates a table, inserts a user, and reads it back.

```cpp id="j9xhqh"
#include <vix.hpp>
#include <vix/db.hpp>
#include <exception>

int main()
{
    try
    {
        auto db = vix::db::Database::mysql(
            "tcp://127.0.0.1:3306",
            "root",
            "secret",
            "vix_app"
        );

        db.exec(
            "CREATE TABLE IF NOT EXISTS users ("
            "id BIGINT AUTO_INCREMENT PRIMARY KEY,"
            "email VARCHAR(255) NOT NULL UNIQUE,"
            "name VARCHAR(255) NOT NULL,"
            "created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP"
            ") ENGINE=InnoDB"
        );

        db.exec(
            "INSERT INTO users (email, name) VALUES (?, ?) "
            "ON DUPLICATE KEY UPDATE name = VALUES(name)",
            "ada@example.com",
            "Ada Lovelace"
        );

        auto rows = db.query(
            "SELECT id, email, name FROM users WHERE email = ?",
            "ada@example.com"
        );

        if (rows->next())
        {
            const auto& row = rows->row();

            vix::print(
              vix::options{.sep = " | "},
              row.getInt64(0),
              row.getString(1),
              row.getString(2)
            );
        }

        return 0;
    }
    catch (const std::exception& e)
    {
        vix::print("database error:", e.what());
        return 1;
    }
}
```

This is regular MySQL SQL. Vix is responsible for the C++ side: creating the database object, preparing statements, binding values, executing queries, and returning rows through a common result set interface.

## Build with MySQL support

Build the project with MySQL enabled:

```bash id="vyl2g5"
vix build --with-mysql
```

For a release build:

```bash id="m5qdw5"
vix build --preset release --with-mysql
```

For one-command build and run:

```bash id="p7lnx5"
vix run --with-mysql
```

The project must be built with MySQL support enabled. If the database code uses MySQL but the MySQL backend is not enabled at build time, the project can fail to compile or link depending on the build configuration.

## MySQL server requirements

Before running a MySQL-backed Vix application, make sure a MySQL-compatible server is available.

For local development, the usual connection values look like this:

```txt id="m3i1ga"
host: 127.0.0.1
port: 3306
user: root
database: vix_app
```

The C++ API expects the MySQL host string in this form:

```txt id="lmbgbm"
tcp://127.0.0.1:3306
```

Example:

```cpp id="l7e85n"
auto db = vix::db::Database::mysql(
    "tcp://127.0.0.1:3306",
    "root",
    "secret",
    "vix_app"
);
```

The database must exist before the application connects to it, unless your local workflow creates it separately.

For example:

```sql id="g82hgk"
CREATE DATABASE vix_app;
```

## Configure MySQL in vix.json

For real projects, prefer project configuration instead of hardcoding database values in source files.

```json id="m06z85"
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

This keeps database settings in one place.

The application can load configuration from the project.

The CLI can use the same project information for migration and database workflows.

Build scripts and deployment scripts can also read or generate the same shape.

## Host and port

In `vix.json`, write the host and port separately:

```json id="aqwg51"
{
  "host": "127.0.0.1",
  "port": 3306
}
```

Vix can build the MySQL Connector/C++ host string from those values.

For local development:

```json id="ve6vtp"
{
  "host": "127.0.0.1",
  "port": 3306
}
```

For a service on the same private network:

```json id="wl3ocv"
{
  "host": "mysql.internal",
  "port": 3306
}
```

For a managed database, use the host and port provided by the infrastructure.

## Credentials

A MySQL configuration needs a user, password, and database name.

```json id="x60h7n"
{
  "user": "app_user",
  "password": "local-secret",
  "database": "blog"
}
```

For local examples, simple credentials are acceptable.

For production, do not commit real credentials into the repository. Generate production configuration during deployment or load secrets through the infrastructure used by the application.

A safe repository should not expose production database passwords.

## Recommended local development setup

A practical local MySQL development setup looks like this:

```txt id="q4c9cn"
my-app/
├── vix.json
├── src/
│   └── main.cpp
└── migrations/
    ├── 2026_06_02_120000_create_users.up.sql
    └── 2026_06_02_120000_create_users.down.sql
```

The `vix.json` file describes the database connection.

The `migrations/` directory stores SQL schema changes.

The source code uses `vix::db::Database` instead of spreading MySQL Connector/C++ code across the application.

## Creating tables

Use MySQL syntax when writing MySQL migrations or setup SQL.

```cpp id="lwedwy"
db.exec(
    "CREATE TABLE IF NOT EXISTS users ("
    "id BIGINT AUTO_INCREMENT PRIMARY KEY,"
    "email VARCHAR(255) NOT NULL UNIQUE,"
    "name VARCHAR(255) NOT NULL,"
    "created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP"
    ") ENGINE=InnoDB"
);
```

For application tables, prefer explicit column types and constraints.

Use `BIGINT AUTO_INCREMENT` for identifiers when the table may grow.

Use `VARCHAR(255)` for bounded text such as email, slugs, names, and short labels.

Use `TEXT` for longer content.

Use `DATETIME` for timestamps when the application expects MySQL date and time behavior.

Use `NOT NULL` when a value is required by the application.

Use `UNIQUE` when the database must enforce uniqueness.

## Inserting rows

Use `exec()` for inserts.

```cpp id="xsun6c"
db.exec(
    "INSERT INTO users (email, name) VALUES (?, ?)",
    "grace@example.com",
    "Grace Hopper"
);
```

Use bind parameters for dynamic values.

```cpp id="ao9tgy"
db.exec(
    "INSERT INTO users (email, name) VALUES (?, ?)",
    email,
    name
);
```

Do not concatenate user input into SQL strings.

```cpp id="b9ef1a"
db.exec(
    "INSERT INTO users (email, name) VALUES ('" + email + "', '" + name + "')"
);
```

Prepared statements keep values separate from the SQL structure and let the driver bind values correctly.

## Handling duplicate keys

MySQL supports `ON DUPLICATE KEY UPDATE`.

This is useful when a column has a `UNIQUE` constraint and the application wants an upsert-style operation.

```cpp id="rqkiqu"
db.exec(
    "INSERT INTO users (email, name) VALUES (?, ?) "
    "ON DUPLICATE KEY UPDATE name = VALUES(name)",
    "ada@example.com",
    "Ada Lovelace"
);
```

Use this when updating an existing row is part of the intended behavior.

Do not use it to hide unexpected duplicate data unless that behavior is explicitly part of the application design.

## Reading rows

Use `query()` for statements that return rows.

```cpp id="sgqfmd"
auto rows = db.query(
    "SELECT id, email, name FROM users ORDER BY id"
);

while (rows->next())
{
    const auto& row = rows->row();

    auto id = row.getInt64(0);
    auto email = row.getString(1);
    auto name = row.getString(2);

    vix::print(vix::options{.sep = " | "}, id, email, name);
}
```

Column indexes start at `0`.

The order follows the `SELECT` clause.

```sql id="jn4u3j"
SELECT id, email, name FROM users
```

So the row access is:

```txt id="s90rma"
0 -> id
1 -> email
2 -> name
```

Use explicit column lists in application code. Avoid `SELECT *` outside quick experiments because it makes row access depend on table structure.

## Updating rows

Use `exec()` for updates.

```cpp id="h11v8c"
auto affected = db.exec(
    "UPDATE users SET name = ? WHERE email = ?",
    "Ada Byron",
    "ada@example.com"
);

vix::print("updated rows:", affected);
```

For critical operations, check the affected row count.

If the application expects one row to change and the result is zero, that can mean the target row does not exist.

If the application expects one row to change and the result is more than one, the `WHERE` clause may be too broad.

## Deleting rows

Use `exec()` for deletes.

```cpp id="ctyow2"
auto affected = db.exec(
    "DELETE FROM users WHERE email = ?",
    "grace@example.com"
);
```

Avoid broad delete statements in application code.

Prefer deletes that target a primary key, a unique column, or a clearly scoped condition.

## Last inserted id

For MySQL tables with `AUTO_INCREMENT`, the connection can expose the last inserted id.

At the high level, many applications can avoid relying on it directly by querying a unique value after insert.

```cpp id="e1k92k"
db.exec(
    "INSERT INTO users (email, name) VALUES (?, ?)",
    "linus@example.com",
    "Linus"
);

auto rows = db.query(
    "SELECT id FROM users WHERE email = ?",
    "linus@example.com"
);

if (rows->next())
{
    auto id = rows->row().getInt64(0);
    vix::print("created user id:", id);
}
```

Use the lower-level connection API when the exact driver insert id is needed immediately after an insert performed on the same connection.

## Transactions

MySQL transactions are important for multi-step writes.

The Vix `Transaction` type starts a transaction when it is created and rolls it back automatically if `commit()` is not called.

```cpp id="zc3x9r"
vix::db::Transaction tx(db.pool());

auto& conn = tx.conn();

auto debit = conn.prepare(
    "UPDATE accounts SET balance = balance - ? WHERE id = ?"
);
debit->bind(1, 100);
debit->bind(2, 1);
debit->exec();

auto credit = conn.prepare(
    "UPDATE accounts SET balance = balance + ? WHERE id = ?"
);
credit->bind(1, 100);
credit->bind(2, 2);
credit->exec();

tx.commit();
```

If an exception happens before `commit()`, the transaction object rolls back during destruction.

This is one of the most important patterns when writing database-backed C++ code. The rollback behavior is tied to C++ object lifetime, so cleanup happens even when control flow exits early.

## Connection pooling

MySQL connections are more expensive than local in-process operations. A backend service should reuse them.

Vix database objects use a connection pool.

```cpp id="yjgkiz"
vix::db::PoolConfig pool;
pool.min = 1;
pool.max = 8;

auto db = vix::db::Database::mysql(
    "tcp://127.0.0.1:3306",
    "root",
    "secret",
    "vix_app",
    pool
);
```

A pool gives the application a maximum number of live connections and avoids repeatedly opening new MySQL connections for every operation.

For local development, `max = 8` is usually enough.

For production, size the pool based on application concurrency, query latency, database capacity, and the total number of application instances.

Do not increase the pool size blindly. A larger pool can overload the database if several application instances are running.

## Connection health

The MySQL driver can check whether a connection is still valid before reuse.

This matters for long-running services. A database server can close idle connections. Network conditions can change. Credentials or permissions can be updated. A connection that was valid earlier may not remain usable forever.

When the pool acquires a connection, invalid connections can be discarded and replaced.

Application code should still handle database errors. A health check reduces common stale-connection problems, but it does not make the network impossible to fail.

## Migrations with MySQL

MySQL projects should keep schema changes as migration files.

```txt id="c0ufjj"
migrations/
├── 2026_06_02_120000_create_users.up.sql
└── 2026_06_02_120000_create_users.down.sql
```

Example `up` migration:

```sql id="s3hmmp"
CREATE TABLE users (
  id BIGINT AUTO_INCREMENT PRIMARY KEY,
  email VARCHAR(255) NOT NULL UNIQUE,
  name VARCHAR(255) NOT NULL,
  created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB;
```

Example `down` migration:

```sql id="basxvk"
DROP TABLE users;
```

Apply migrations with:

```bash id="dh664u"
vix orm migrate --db vix_app --dir migrations
```

Roll back one migration:

```bash id="ttxc50"
vix orm rollback --steps 1 --db vix_app --dir migrations
```

Check migration status:

```bash id="i670q4"
vix orm status --db vix_app --dir migrations
```

The `vix db` command is currently more focused on SQLite project workflows. For MySQL migration work, prefer `vix orm`.

## Generating migrations from schema snapshots

Vix can work with schema snapshots and generate migration files from schema changes.

A migration generation command can look like this:

```bash id="q6p0g6"
vix orm makemigrations \
  --new ./schema.new.json \
  --snapshot ./schema.json \
  --dir ./migrations \
  --name create_users \
  --dialect mysql
```

This can generate files such as:

```txt id="gf6vfl"
migrations/2026_06_02_120000_create_users.up.sql
migrations/2026_06_02_120000_create_users.down.sql
```

Use this workflow when the project wants schema changes to be reviewed as files before they are applied.

Generated migrations should still be inspected. A migration changes real data structures. Review the SQL before applying it to a shared or production database.

## MySQL and SQL dialect differences

Do not assume SQLite SQL and MySQL SQL are identical.

Examples:

| Concern           | SQLite                              | MySQL                               |
| ----------------- | ----------------------------------- | ----------------------------------- |
| Auto-increment id | `INTEGER PRIMARY KEY AUTOINCREMENT` | `BIGINT AUTO_INCREMENT PRIMARY KEY` |
| Text column       | `TEXT`                              | `VARCHAR(255)` or `TEXT`            |
| Boolean value     | flexible typing                     | often `TINYINT(1)`                  |
| Current timestamp | `CURRENT_TIMESTAMP`                 | `CURRENT_TIMESTAMP`                 |
| Engine option     | not used                            | `ENGINE=InnoDB`                     |

Keep engine-specific SQL in engine-specific migration files when needed.

For application queries, write SQL that matches the configured engine.

## Using MySQL in repositories

For a real backend, avoid putting SQL throughout route handlers.

A simple repository keeps database access in one place.

```cpp id="qsrnbu"
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

    void renameByEmail(std::string email, std::string name)
    {
        db_.exec(
            "UPDATE users SET name = ? WHERE email = ?",
            std::move(name),
            std::move(email)
        );
    }

private:
    vix::db::Database& db_;
};
```

This keeps the route layer focused on HTTP behavior and keeps SQL close to the data model.

## Production notes

A production MySQL deployment needs more than a connection string.

Use a dedicated database user for the application. Do not use `root` in production.

Use a strong password or infrastructure-managed secret.

Use migrations to change schema.

Back up the database before destructive migrations.

Monitor connection count, slow queries, deadlocks, storage usage, and replication health if replication is used.

Use transactions for multi-step writes.

Keep the pool size aligned with the database server capacity and the number of running application instances.

The Vix database module gives the C++ application a clean access layer, but operating MySQL correctly is still part of the production system.

## Common mistakes

### Building without MySQL support

If the application uses MySQL, build with:

```bash id="k7ccor"
vix build --with-mysql
```

or:

```bash id="nfxfuz"
vix run --with-mysql
```

### Using SQLite SQL in MySQL

This SQLite schema is not the right MySQL form:

```sql id="sp28cn"
CREATE TABLE users (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  email TEXT NOT NULL UNIQUE
);
```

Use MySQL syntax instead:

```sql id="gaz8u5"
CREATE TABLE users (
  id BIGINT AUTO_INCREMENT PRIMARY KEY,
  email VARCHAR(255) NOT NULL UNIQUE
) ENGINE=InnoDB;
```

### Committing production credentials

Do not commit production passwords in `vix.json`.

Use safe local credentials in examples only.

Generate or inject production configuration during deployment.

### Opening too many connections

Increasing the pool size can make the application faster only when the database is underused and queries spend time waiting for available connections.

If the database is already saturated, increasing the pool can make the system worse.

Measure first.

### Ignoring affected rows

For important updates and deletes, check how many rows changed.

```cpp id="aykbml"
auto affected = db.exec(
    "DELETE FROM users WHERE id = ?",
    userId
);

if (affected != 1)
{
    vix::print("expected to delete one user, deleted", affected);
}
```

## Recommended workflow

For a MySQL-backed Vix application, use this flow:

```txt id="ktdsnn"
1. Configure MySQL in vix.json.
2. Keep migrations in migrations/.
3. Build with --with-mysql.
4. Use vix::db::Database in the application.
5. Keep SQL inside repositories or data-access classes.
6. Use bind parameters for values.
7. Use transactions for multi-step writes.
8. Use vix orm for migration workflows.
9. Keep production credentials outside the repository.
```

This gives the project a clear database structure without hiding the database engine or turning SQL into magic.

## Next steps

Read the queries page for more detail on `exec()`, `query()`, parameter binding, and result sets.

Read the connection pool page before using MySQL in a concurrent backend service.

Read the transactions page before writing multi-step database operations.

Read the migrations page before changing schema in a shared database.
