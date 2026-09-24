# Database quick start

This page shows the shortest useful path for using the Vix database module in a C++ application.

The example uses SQLite because it does not require a separate database server. You can create a database file, create a table, insert data, query it, and remove the file when you want to reset the example.

The same high-level API is also used with MySQL. The connection setup changes, but the application code for executing statements, binding values, and reading rows stays close to the same shape.

## What you will build

You will create a small C++ program that:

1. opens a SQLite database file,
2. creates a `users` table,
3. inserts two users,
4. reads the users back,
5. updates one row,
6. reads the updated row,
7. deletes one row.

The goal is not to hide SQL. The goal is to show the basic Vix database workflow without introducing migrations, repositories, connection pools, or production configuration too early.

## Create the project

Create a small Vix project:

```bash id="ad761c"
mkdir vix-db-quick-start
cd vix-db-quick-start
mkdir -p src storage
touch src/main.cpp
```

For this guide, the SQLite database will be stored under:

```txt id="x9bura"
storage/app.db
```

Keeping the database file under `storage/` makes it easy to inspect or delete during development.

## Write the program

Open:

```txt id="xbzv6n"
src/main.cpp
```

and add:

```cpp id="88ea2a"
#include <vix.hpp>
#include <vix/db.hpp>
#include <exception>

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

        db.exec(
            "INSERT OR IGNORE INTO users (email, name) VALUES (?, ?)",
            "gaspard@example.com",
            "Gaspard"
        );

        db.exec(
            "INSERT OR IGNORE INTO users (email, name) VALUES (?, ?)",
            "ada@example.com",
            "Ada"
        );

        vix::print("Users after insert:");

        auto users = db.query(
            "SELECT id, email, name FROM users ORDER BY id"
        );

        while (users->next())
        {
            const auto& row = users->row();

            vix::print(
                vix::options{.sep = " | "},
                row.getInt64(0),
                row.getString(1),
                row.getString(2)
            );
        }

        db.exec(
            "UPDATE users SET name = ? WHERE email = ?",
            "Ada Lovelace",
            "ada@example.com"
        );

        auto updated = db.query(
            "SELECT id, email, name FROM users WHERE email = ?",
            "ada@example.com"
        );

        if (updated->next())
        {
            const auto& row = updated->row();

            vix::print_inline("Updated user: ");
            vix::print(
                vix::options{.sep = " | "},
                row.getInt64(0),
                row.getString(1),
                row.getString(2)
            );
        }

        db.exec(
            "DELETE FROM users WHERE email = ?",
            "gaspard@example.com"
        );

        auto count = db.query(
            "SELECT COUNT(*) FROM users"
        );

        if (count->next())
        {
            vix::print("Remaining users:", count->row().getInt64(0));
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

This example keeps all the logic in one file so the database flow is visible. In a real application, you would usually move database access into repository or service classes.

## Build with SQLite support

Build the project with SQLite enabled:

```bash id="i3ob91"
vix build --with-sqlite
```

For a release build:

```bash id="qz1td3"
vix build --preset release --with-sqlite
```

`--with-sqlite` enables the database module with SQLite support for the build.

## Run the program

Run the generated binary from the build directory:

```bash id="x0bl51"
./build-ninja/app
```

Depending on your project target name, the binary name can be different. If you use `vix run`, Vix builds and starts the application in one command:

```bash id="zjvn59"
vix run --with-sqlite
```

The output should look close to this:

| Section           |  ID | Email               | Name         |
| ----------------- | --: | ------------------- | ------------ |
| User after insert |   1 | gaspard@example.com | Gaspard      |
| User after insert |   2 | ada@example.com     | Ada          |
| Updated user      |   2 | ada@example.com     | Ada Lovelace |
| Remaining users   |   1 |                     |              |

The exact IDs can differ if the database file already existed before running the program.

## Reset the example

SQLite stores the data in:

```txt id="ckmdm7"
storage/app.db
```

To reset the example:

```bash id="zlp6et"
rm -f storage/app.db storage/app.db-wal storage/app.db-shm
```

Then run the program again.

The `-wal` and `-shm` files are SQLite sidecar files. They may appear depending on the SQLite journal mode used by the database.

## What happened

The program starts by creating a `vix::db::Database` instance:

```cpp id="u651x3"
auto db = vix::db::Database::sqlite("storage/app.db");
```

That creates a SQLite-backed database object. Internally, Vix selects the SQLite driver, creates a connection factory, initializes the connection pool, and exposes the high-level `exec()` and `query()` API.

`exec()` is used for statements that do not return rows:

```cpp id="ljmmmr"
db.exec(
    "INSERT OR IGNORE INTO users (email, name) VALUES (?, ?)",
    "ada@example.com",
    "Ada"
);
```

`query()` is used for statements that return rows:

```cpp id="1r9h77"
auto users = db.query(
    "SELECT id, email, name FROM users ORDER BY id"
);
```

Rows are read using a forward-only result set:

```cpp id="6ugfj4"
while (users->next())
{
    const auto& row = users->row();

    const auto id = row.getInt64(0);
    const auto email = row.getString(1);
    const auto name = row.getString(2);
}
```

Column indexes start at `0`, in the order used by the `SELECT` statement.

## Bind parameters

Values passed after the SQL string are bound as positional parameters.

```cpp id="qo5v45"
db.exec(
    "UPDATE users SET name = ? WHERE email = ?",
    "Ada Lovelace",
    "ada@example.com"
);
```

The first value binds to the first `?`.

The second value binds to the second `?`.

Do not build SQL by manually concatenating user input into the SQL string. Keep values separate and pass them as bind parameters.

Prefer this:

```cpp id="app7x2"
db.exec(
    "DELETE FROM users WHERE email = ?",
    email
);
```

Do not do this:

```cpp id="mu4nrr"
db.exec(
    "DELETE FROM users WHERE email = '" + email + "'"
);
```

Prepared statements keep the SQL structure separate from values. The code is easier to read, and values are handled by the driver.

## Reading nullable values

If a column can be `NULL`, you can check it explicitly:

```cpp id="hnnz39"
if (!row.isNull(2))
{
    auto name = row.getString(2);
}
```

Or you can use a default value:

```cpp id="hg5tft"
auto name = row.getStringOr(2, "anonymous");
auto score = row.getInt64Or(3, 0);
```

Use explicit null checks when `NULL` has business meaning. Use the `Or` helpers when a default value is acceptable.

## Add a transaction

For multiple related writes, use a transaction.

```cpp id="zj7cv6"
vix::db::Transaction tx(db.pool());

auto& conn = tx.conn();

auto insert = conn.prepare(
    "INSERT INTO users (email, name) VALUES (?, ?)"
);
insert->bind(1, "grace@example.com");
insert->bind(2, "Grace Hopper");
insert->exec();

auto update = conn.prepare(
    "UPDATE users SET name = ? WHERE email = ?"
);
update->bind(1, "Ada Byron");
update->bind(2, "ada@example.com");
update->exec();

tx.commit();
```

If `commit()` is not called, the transaction rolls back when `tx` is destroyed.

That behavior is important in C++ because exceptions and early returns should not leave the database in a partially updated state.

## Use configuration later

For a quick start, creating the database directly in code is fine:

```cpp id="74fwqy"
auto db = vix::db::Database::sqlite("storage/app.db");
```

For a real project, move the database settings into `vix.json`:

```json id="8j0i75"
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

Then the application and the Vix CLI can read the same project database configuration.

## Use the CLI to inspect the database

After running the program, you can inspect the project database state:

```bash id="d7h44w"
vix db status
```

For JSON output:

```bash id="3bgb5v"
vix db status --json
```

This is useful in scripts, CI, and deployment checks.

## Next steps

The quick start intentionally avoids advanced structure. It shows the basic database loop:

```txt id="97m9eo"
open database
create table
execute prepared statements
read result sets
handle errors
```

Continue with the configuration page when you want the database settings to live in `vix.json`.

Read the SQLite and MySQL pages when you want engine-specific setup.

Read the transactions and migrations pages when your application needs safer writes and schema evolution.
