# SQLite

SQLite is the simplest database engine to use with Vix.cpp.

It does not require a separate server, a running daemon, a network connection, or database provisioning before the application starts. The database is stored in a local file, which makes SQLite a strong default for examples, local tools, tests, embedded applications, prototypes, and local-first software.

In Vix, SQLite is accessed through the same database facade used by the rest of the database module:

```cpp id="xe11ub"
#include <vix/db.hpp>
```

Most applications should use:

```cpp id="vxxh3g"
auto db = vix::db::Database::sqlite("storage/app.db");
```

That gives the application a high-level database object with prepared statements, typed parameter binding, result sets, connection pooling, and transaction support.

## When SQLite is the right choice

SQLite is a good fit when the application should run immediately without external infrastructure.

A local developer can clone a project, build it, run it, and get a working database from a file inside the project directory. A test suite can create a temporary database, run queries, and remove it afterwards. A CLI tool can keep its own state without requiring a MySQL or PostgreSQL server. A local-first application can write data locally before any remote synchronization exists.

This matters for Vix because many C++ applications do not need a database server on day one. They need a reliable local database workflow that can later grow into a larger architecture.

SQLite gives that path.

## Opening a SQLite database

The shortest form is:

```cpp id="ux6mxn"
auto db = vix::db::Database::sqlite("storage/app.db");
```

If the file does not exist, SQLite can create it.

The parent directory must exist:

```bash id="nbh0ho"
mkdir -p storage
```

A common project layout is:

```txt id="d3d8js"
my-app/
├── src/
│   └── main.cpp
├── storage/
│   └── app.db
├── migrations/
└── vix.json
```

Keep the database under `storage/` instead of putting it at the project root. It keeps runtime data separate from source files and makes cleanup easier.

## Minimal program

This program opens a SQLite database, creates a table, inserts a row, and reads it back.

```cpp id="cbom0d"
#include <vix.hpp>
#include <vix/db.hpp>
#include <exception>

int main()
{
    try
    {
        auto db = vix::db::Database::sqlite("storage/app.db");

        db.exec(
            "CREATE TABLE IF NOT EXISTS notes ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "title TEXT NOT NULL,"
            "body TEXT NOT NULL"
            ")"
        );

        db.exec(
            "INSERT INTO notes (title, body) VALUES (?, ?)",
            "First note",
            "SQLite is running through Vix."
        );

        auto rows = db.query(
            "SELECT id, title, body FROM notes ORDER BY id DESC LIMIT 1"
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

The SQL remains normal SQLite SQL. Vix handles the database object, prepared statement flow, value binding, result set ownership, and typed row access.

## Build with SQLite support

Build the project with SQLite enabled:

```bash id="d3dj6l"
vix build --with-sqlite
```

For a release build:

```bash id="gkyvml"
vix build --preset release --with-sqlite
```

For one-command build and run:

```bash id="fs7ovr"
vix run --with-sqlite
```

The `--with-sqlite` flag enables the SQLite-related database build options. If SQLite support is not enabled at build time, a project that uses `vix::db::Database::sqlite()` can fail to compile or link depending on how the project is configured.

## Configure SQLite in vix.json

For a real project, describe the SQLite database in `vix.json`.

```json id="wdz8f2"
{
  "name": "Notes",
  "database": {
    "engine": "sqlite",
    "sqlite": {
      "path": "storage/Notes.db"
    },
    "storage": "storage",
    "migrations": "migrations"
  }
}
```

This configuration gives Vix one place to find the database path, storage directory, and migration directory.

The application can use the same project-level configuration as the CLI tooling. That avoids hardcoding one path in the application and another path in scripts.

## SQLite path

The recommended form is:

```json id="af6pkg"
{
  "database": {
    "engine": "sqlite",
    "sqlite": {
      "path": "storage/app.db"
    }
  }
}
```

The flat form may also be accepted by some tooling:

```json id="tr4d5l"
{
  "database": {
    "engine": "sqlite",
    "sqlite_path": "storage/app.db"
  }
}
```

Prefer the nested `sqlite.path` form for new projects.

## Storage files

A SQLite database normally appears as one file:

```txt id="ude44x"
storage/app.db
```

Depending on the journal mode and active connections, SQLite may also create sidecar files:

```txt id="ih8sio"
storage/app.db-wal
storage/app.db-shm
```

These files are normal.

The `-wal` file is used by SQLite write-ahead logging.

The `-shm` file is shared memory metadata used with WAL mode.

Do not treat these files as source files. They are runtime database artifacts.

A typical `.gitignore` should contain:

```txt id="kicrsl"
storage/*.db
storage/*.db-wal
storage/*.db-shm
```

If the project needs fixture databases for tests, keep them in a dedicated directory and document that choice explicitly.

## Creating tables

Use `exec()` for SQL statements that do not return rows.

```cpp id="g881ed"
db.exec(
    "CREATE TABLE IF NOT EXISTS users ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "email TEXT NOT NULL UNIQUE,"
    "name TEXT NOT NULL,"
    "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
    ")"
);
```

SQLite accepts a flexible type system, but it is still better to write schemas deliberately.

Use `INTEGER` for identifiers.

Use `TEXT` for strings and timestamps stored as text.

Use `REAL` for floating point values.

Use `BLOB` for binary values.

Use `NOT NULL` when the application expects the value to exist.

Use `UNIQUE` when the database should enforce uniqueness.

## Inserting data

Pass values as bind parameters.

```cpp id="nc66n8"
db.exec(
    "INSERT INTO users (email, name) VALUES (?, ?)",
    "gaspard@example.com",
    "Gaspard"
);
```

The first value after the SQL string binds to the first `?`.

The second value binds to the second `?`.

Do not concatenate user data into SQL strings.

Prefer this:

```cpp id="eftunm"
db.exec(
    "INSERT INTO users (email, name) VALUES (?, ?)",
    email,
    name
);
```

Avoid this:

```cpp id="gi9o5i"
db.exec(
    "INSERT INTO users (email, name) VALUES ('" + email + "', '" + name + "')"
);
```

Keeping values separate from SQL text makes the code clearer and lets the driver handle value binding.

## Reading data

Use `query()` for statements that return rows.

```cpp id="gbshpi"
auto rows = db.query(
    "SELECT id, email, name FROM users ORDER BY id"
);

while (rows->next())
{
    const auto& row = rows->row();

    const auto id = row.getInt64(0);
    const auto email = row.getString(1);
    const auto name = row.getString(2);

    vix::print(vix::options{.sep = " | "}, id, email, name);
}
```

Column indexes start at `0`.

The order follows the `SELECT` clause:

```sql id="wf0cjo"
SELECT id, email, name FROM users
```

So:

```txt id="pfw8li"
0 -> id
1 -> email
2 -> name
```

Use explicit column order in queries instead of `SELECT *` in application code. It makes row reading stable and easier to review.

## Updating data

Use `exec()` for updates.

```cpp id="vz82md"
db.exec(
    "UPDATE users SET name = ? WHERE email = ?",
    "Ada Lovelace",
    "ada@example.com"
);
```

The return value is the number of affected rows when supported by the driver.

```cpp id="nddpri"
auto affected = db.exec(
    "UPDATE users SET name = ? WHERE email = ?",
    "Ada Lovelace",
    "ada@example.com"
);

vix::print("updated rows:", affected);
```

For important updates, check the affected row count when the application expects exactly one row to change.

## Deleting data

Use `exec()` for deletes.

```cpp id="p07tbs"
db.exec(
    "DELETE FROM users WHERE email = ?",
    "gaspard@example.com"
);
```

For destructive operations, always use a clear `WHERE` clause.

In application code, avoid building generic delete helpers that make it easy to run a delete without a condition.

## Last inserted id

SQLite tables often use `INTEGER PRIMARY KEY AUTOINCREMENT`.

After an insert, the underlying connection can expose the last inserted row id.

When using the high-level `Database` helpers, most application code can avoid reading it directly by querying the row with a stable unique value.

```cpp id="fy3q9o"
db.exec(
    "INSERT INTO users (email, name) VALUES (?, ?)",
    "grace@example.com",
    "Grace Hopper"
);

auto rows = db.query(
    "SELECT id FROM users WHERE email = ?",
    "grace@example.com"
);

if (rows->next())
{
    auto id = rows->row().getInt64(0);
    vix::print("created user id:", id);
}
```

Use unique application values such as email, slug, or external id when that makes the code clearer.

## Transactions

Use a transaction when several writes must succeed or fail together.

```cpp id="u7iwkx"
vix::db::Transaction tx(db.pool());

auto& conn = tx.conn();

auto insertUser = conn.prepare(
    "INSERT INTO users (email, name) VALUES (?, ?)"
);
insertUser->bind(1, "linus@example.com");
insertUser->bind(2, "Linus");
insertUser->exec();

auto insertProfile = conn.prepare(
    "INSERT INTO profiles (user_email, bio) VALUES (?, ?)"
);
insertProfile->bind(1, "linus@example.com");
insertProfile->bind(2, "Created inside the same transaction.");
insertProfile->exec();

tx.commit();
```

If `commit()` is not called, the transaction rolls back when the `Transaction` object is destroyed.

That is the behavior you want for code that can return early or throw an exception. The database should not be left with half of a logical operation.

## Pooling with SQLite

The high-level `Database` object owns a connection pool.

For SQLite, a small pool is usually enough.

```cpp id="gxgf0w"
vix::db::PoolConfig pool;
pool.min = 1;
pool.max = 4;

auto db = vix::db::Database::sqlite("storage/app.db", pool);
```

SQLite is a local file database. It can serve many read-heavy local workflows well, but it is not the same operational model as a database server.

For local tools, tests, and small services, a small pool keeps the design simple.

For write-heavy server workloads, measure the application and understand SQLite’s concurrency behavior before increasing the pool size.

## Migrations with SQLite

SQLite projects can use file-based migrations.

A migration directory can look like this:

```txt id="cs5z1v"
migrations/
├── 2026_06_02_120000_create_users.up.sql
└── 2026_06_02_120000_create_users.down.sql
```

Example `up` migration:

```sql id="cnqfge"
CREATE TABLE users (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  email TEXT NOT NULL UNIQUE,
  name TEXT NOT NULL,
  created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);
```

Example `down` migration:

```sql id="sq5apf"
DROP TABLE users;
```

Apply migrations with:

```bash id="e4h08a"
vix db migrate
```

or with the ORM migration command when that is the workflow used by the project:

```bash id="brxjdd"
vix orm migrate
```

For SQLite project workflows, `vix db` can inspect database status, apply migrations, and create local backups.

## Inspect SQLite status

Run:

```bash id="dzct4d"
vix db status
```

Vix can report the configured engine, database path, storage directory, database file presence, WAL path, SHM path, and migration directory.

For scripts and CI, use JSON output:

```bash id="o6wikf"
vix db status --json
```

This makes SQLite workflows easier to automate. A script can check whether the storage directory exists, whether it is writable, and whether the configured database path is consistent with the project.

## Create a backup

For SQLite projects, `vix db backup` can create a timestamped backup of the database file.

```bash id="stt6e8"
vix db backup
```

If WAL or SHM sidecar files are present, the backup workflow can copy them as well.

Backups are useful before destructive migrations, local experiments, or manual database inspection.

## Reset a local SQLite database

For development, the fastest reset is often to remove the local database files.

```bash id="hxrw6p"
rm -f storage/app.db storage/app.db-wal storage/app.db-shm
```

Then recreate the database by running migrations or starting the application.

```bash id="oyyzhp"
vix db migrate
vix run --with-sqlite
```

Only do this for local development data. Do not reset production data this way.

## Use SQLite for tests

SQLite is useful for tests because each test run can use a separate file.

```cpp id="fdo6k0"
auto db = vix::db::Database::sqlite("storage/test.db");
```

A test can create the schema, run operations, check results, and delete the file afterwards.

For isolated tests, use a unique path per test run:

```cpp id="ervjfz"
auto db = vix::db::Database::sqlite("storage/test_users_001.db");
```

This avoids test cases sharing state accidentally.

## Error handling

Database operations can throw exceptions when something fails.

```cpp id="g2qc12"
try
{
    auto db = vix::db::Database::sqlite("storage/app.db");

    db.exec(
        "INSERT INTO users (email, name) VALUES (?, ?)",
        "ada@example.com",
        "Ada"
    );
}
catch (const std::exception& e)
{
    vix::print("database error:", e.what());
}
```

Handle errors at a level where the application can make a meaningful decision.

A CLI may print the error and return a non-zero exit code.

An HTTP API may convert the error into a response.

A migration tool may stop immediately to avoid applying later changes on a broken schema.

## Practical recommendations

Use SQLite first when the project can be local, embedded, or self-contained.

Keep the database file under `storage/`.

Commit migrations, not runtime database files.

Use bind parameters for all dynamic values.

Use explicit column lists in `SELECT` queries.

Use transactions for multi-step writes.

Use `vix db status` to inspect project database state.

Use MySQL later when the application needs a shared database server or production infrastructure built around MySQL.

## Next steps

Read the MySQL page when the application needs to connect to a database server.

Read the queries page for more detail on `exec()`, `query()`, result sets, and row access.

Read the transactions page when the application needs stronger write safety.

Read the migrations page when the database schema must evolve over time.
