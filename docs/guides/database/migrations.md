# Migrations

A database migration is a controlled change to the database schema.

Instead of editing the database manually, a project keeps schema changes in files. Those files are committed with the source code, reviewed like code, and applied in order.

Vix supports file-based SQL migrations through the database module and exposes migration workflows through the CLI.

A typical migration directory looks like this:

```txt
migrations/
├── 2026_06_02_120000_create_users.up.sql
└── 2026_06_02_120000_create_users.down.sql
```

The `up` file applies the change.

The `down` file reverts the change.

This gives the project a database history that can be inspected, reviewed, applied, rolled back, and automated.

## Why migrations matter

A database schema changes over time.

A first version may only need a `users` table. Later, the application may need profiles, sessions, orders, payments, indexes, constraints, audit logs, or migration tracking tables.

Without migrations, developers often end up with manual instructions such as:

```txt
Run this SQL in production.
Then run this other SQL locally.
Then update this table by hand.
```

That does not scale.

Migrations make schema changes explicit.

They answer simple but important questions:

```txt
Which schema changes exist?
Which changes have already been applied?
In what order should they run?
Can the last change be rolled back?
Did someone edit a migration after it was applied?
```

The Vix database module is designed to make those questions part of the normal project workflow.

## Migration file naming

Vix file-based migrations use paired SQL files.

```txt
<id>.up.sql
<id>.down.sql
```

Example:

```txt
2026_06_02_120000_create_users.up.sql
2026_06_02_120000_create_users.down.sql
```

The migration id is the shared base name:

```txt
2026_06_02_120000_create_users
```

Use sortable names. Timestamp prefixes are recommended because they keep migration order stable across machines and repositories.

A good migration id describes the schema change:

```txt
2026_06_02_120000_create_users
2026_06_02_121500_add_user_profiles
2026_06_02_123000_add_email_index_to_users
```

Avoid vague names such as:

```txt
change_db
update_table
new_migration
fix_schema
```

A migration name should help a developer understand the database history without opening every file.

## Create a migration directory

Create the directory at the project root:

```bash
mkdir -p migrations
```

A common Vix project layout is:

```txt
my-app/
├── vix.json
├── src/
│   └── main.cpp
├── storage/
│   └── .gitkeep
└── migrations/
    ├── 2026_06_02_120000_create_users.up.sql
    └── 2026_06_02_120000_create_users.down.sql
```

The `migrations/` directory should be committed to Git.

The local database files under `storage/` usually should not be committed.

## Configure migrations in vix.json

Database tooling reads the migration directory from `vix.json`.

For SQLite:

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

For MySQL:

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

The important field is:

```json
{
  "database": {
    "migrations": "migrations"
  }
}
```

That tells Vix where migration files are stored.

## A first SQLite migration

Create:

```txt
migrations/2026_06_02_120000_create_users.up.sql
```

Add:

```sql
CREATE TABLE users (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  email TEXT NOT NULL UNIQUE,
  name TEXT NOT NULL,
  created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);
```

Create:

```txt
migrations/2026_06_02_120000_create_users.down.sql
```

Add:

```sql
DROP TABLE users;
```

The `up` file creates the table.

The `down` file removes it.

For local SQLite projects, this is enough to start using migrations.

## A first MySQL migration

For MySQL, use MySQL syntax.

Create:

```txt
migrations/2026_06_02_120000_create_users.up.sql
```

Add:

```sql
CREATE TABLE users (
  id BIGINT AUTO_INCREMENT PRIMARY KEY,
  email VARCHAR(255) NOT NULL UNIQUE,
  name VARCHAR(255) NOT NULL,
  created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB;
```

Create:

```txt
migrations/2026_06_02_120000_create_users.down.sql
```

Add:

```sql
DROP TABLE users;
```

Do not assume SQLite and MySQL schema SQL are identical. Keep migration SQL aligned with the selected engine.

## Apply migrations with vix db

For SQLite project workflows, use:

```bash
vix db migrate
```

This applies pending SQL migrations from the configured migration directory.

Before applying migrations, you can inspect database status:

```bash
vix db status
```

For JSON output:

```bash
vix db status --json
```

This is useful in scripts and CI because the command can report the configured engine, database path, storage directory, migration directory, warnings, and errors.

The `vix db` command is currently focused on SQLite project workflows. It can detect MySQL configuration, but migration and backup actions are primarily SQLite-oriented.

## Apply migrations with vix orm

For migration-oriented workflows, use `vix orm`.

Apply migrations:

```bash
vix orm migrate --db blog_db --dir ./migrations
```

Check migration status:

```bash
vix orm status --db blog_db --dir ./migrations
```

Roll back one migration:

```bash
vix orm rollback --steps 1 --db blog_db --dir ./migrations
```

The `--dir` option points to the migration directory.

The `--db` option identifies the target database used by the migration tool.

A typical MySQL workflow uses `vix orm` for migrations:

```bash
vix orm migrate --db blog --dir migrations
vix orm status --db blog --dir migrations
vix orm rollback --steps 1 --db blog --dir migrations
```

## What Vix tracks

When file-based migrations run, Vix tracks applied migrations in a database table.

The default table name is:

```txt
schema_migrations
```

The table stores migration metadata such as:

```txt
id
checksum
applied_at
```

The `id` identifies the migration.

The `checksum` is computed from the `up` migration contents.

The `applied_at` value records when the migration was applied.

This matters because a migration file should not be silently edited after it has already been applied. If a migration was applied and the file later changes, the checksum can reveal that the migration history is no longer stable.

## Why checksums matter

Migration files are part of the project history.

After a migration has been applied to a database, changing the old migration file is dangerous. Another developer or environment may have already applied the original version.

The safe approach is usually to create a new migration.

Bad workflow:

```txt
1. Apply 2026_06_02_120000_create_users.up.sql
2. Edit that same migration later
3. Hope every database stays consistent
```

Better workflow:

```txt
1. Apply 2026_06_02_120000_create_users.up.sql
2. Need another schema change
3. Create 2026_06_02_130000_add_user_status.up.sql
```

Checksums help detect when a migration file no longer matches the version that was applied.

## Apply migrations from C++

The database module also exposes a C++ file migration runner.

```cpp
#include <vix.hpp>
#include <vix/db.hpp>

int main()
{
    try
    {
        auto db = vix::db::Database::sqlite("storage/app.db");

        vix::db::PooledConn conn(db.pool());

        vix::db::FileMigrationsRunner runner(
            conn.get(),
            "migrations"
        );

        runner.applyAll();

        vix::print("migrations applied");

        return 0;
    }
    catch (const std::exception& e)
    {
        vix::print("migration error:", e.what());
        return 1;
    }
}
```

This style is useful when the application or a dedicated tool wants to apply migrations directly from C++.

For most project workflows, prefer the CLI command because it keeps migration operations outside the application runtime.

## Roll back migrations from C++

Use `rollback(steps)` to revert the last applied migrations.

```cpp
#include <vix.hpp>
#include <vix/db.hpp>

int main()
{
    try
    {
        auto db = vix::db::Database::sqlite("storage/app.db");

        vix::db::PooledConn conn(db.pool());

        vix::db::FileMigrationsRunner runner(
            conn.get(),
            "migrations"
        );

        runner.rollback(1);

        vix::print("rollback complete");

        return 0;
    }
    catch (const std::exception& e)
    {
        vix::print("rollback error:", e.what());
        return 1;
    }
}
```

Rollback requires a matching `.down.sql` file.

If the down migration is missing, the migration should not be considered safely reversible.

## Custom migration table

The file migration runner uses `schema_migrations` by default.

For advanced projects, you can override the table name:

```cpp
vix::db::FileMigrationsRunner runner(
    conn.get(),
    "migrations"
);

runner.setTable("app_schema_migrations");
runner.applyAll();
```

Most projects should keep the default name unless there is a clear reason to change it.

## SQL statement splitting

A migration file can contain multiple SQL statements separated by semicolons.

Example:

```sql
CREATE TABLE users (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  email TEXT NOT NULL UNIQUE,
  name TEXT NOT NULL
);

CREATE INDEX idx_users_email ON users(email);
```

The file migration runner reads the migration file and executes the statements in order.

Keep migration files simple. Complex SQL parsing is hard across dialects. Avoid clever SQL formatting that makes statement boundaries ambiguous.

## Transaction behavior

A migration should leave the database in a clear state.

If a migration fails, the migration process should stop.

Some engines and some schema statements can auto-commit or have engine-specific transactional behavior. SQLite and MySQL do not behave identically for every schema operation.

The safest project habit is:

```txt
one migration
one clear schema change
review SQL before applying
stop on failure
do not continue after a broken migration
```

For application data changes, use transactions where the engine supports them and where the operation must be atomic.

## Code-based migrations

The database module also supports code-based migrations through the `Migration` interface.

A migration object provides:

```cpp
std::string id() const;
void up(vix::db::Connection& c);
void down(vix::db::Connection& c);
```

Example:

```cpp
#include <vix/db.hpp>

class CreateUsers final : public vix::db::Migration
{
public:
    std::string id() const override
    {
        return "2026_06_02_120000_create_users";
    }

    void up(vix::db::Connection& conn) override
    {
        conn.prepare(
            "CREATE TABLE IF NOT EXISTS users ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "email TEXT NOT NULL UNIQUE,"
            "name TEXT NOT NULL"
            ")"
        )->exec();
    }

    void down(vix::db::Connection& conn) override
    {
        conn.prepare("DROP TABLE users")->exec();
    }
};
```

Run code-based migrations with `MigrationsRunner`:

```cpp
auto db = vix::db::Database::sqlite("storage/app.db");

vix::db::PooledConn conn(db.pool());

CreateUsers createUsers;

vix::db::MigrationsRunner runner(conn.get());
runner.add(&createUsers);
runner.runAll();
```

File-based SQL migrations are usually easier to review and operate.

Code-based migrations are useful when the migration needs C++ logic or when a project deliberately wants migrations compiled into a tool.

## File-based vs code-based migrations

Use file-based migrations for most projects.

They are easy to review.

They work well with Git.

They can be inspected without compiling the application.

They map naturally to CLI workflows.

Use code-based migrations when a migration needs C++ logic or when the migration runner is part of a dedicated compiled tool.

A practical rule:

```txt
schema changes -> SQL migration files
special migration logic -> code-based migration
```

Do not use code-based migrations just to avoid writing SQL.

## Generate migrations from schema snapshots

Vix also supports a schema snapshot workflow through `vix orm makemigrations`.

A project can keep a current schema snapshot:

```txt
schema.json
```

Then write or generate a new schema:

```txt
schema.new.json
```

Generate migration files:

```bash
vix orm makemigrations \
  --new ./schema.new.json \
  --snapshot ./schema.json \
  --dir ./migrations \
  --name create_users \
  --dialect mysql
```

This can generate files such as:

```txt
migrations/2026_06_02_120000_create_users.up.sql
migrations/2026_06_02_120000_create_users.down.sql
```

The generated SQL should still be reviewed.

Migration generation is a productivity tool. It does not remove responsibility for checking the database change before applying it.

## Schema snapshot workflow

A practical schema workflow looks like this:

```txt
1. Edit schema.new.json
2. Generate migration files
3. Review the generated SQL
4. Apply migrations locally
5. Run the application tests
6. Commit schema snapshot and migration files
```

Example commands:

```bash
vix orm makemigrations \
  --new ./schema.new.json \
  --snapshot ./schema.json \
  --dir ./migrations \
  --name add_users \
  --dialect mysql

vix orm migrate --db blog --dir ./migrations

vix orm status --db blog --dir ./migrations

vix check --tests
```

This keeps schema evolution explicit and testable.

## Migration status

Use status commands to inspect migration state.

For SQLite project status:

```bash
vix db status
```

For migration status:

```bash
vix orm status --db blog --dir ./migrations
```

A status command is useful before deployment, before rollback, and when debugging a local environment.

## Rollback strategy

Rollback is useful during development and controlled deployments.

```bash
vix orm rollback --steps 1 --db blog --dir ./migrations
```

Rollback depends on the quality of the `.down.sql` file.

A down migration should undo the up migration as safely as possible.

For simple table creation:

```sql
DROP TABLE users;
```

For adding a column:

```sql
ALTER TABLE users DROP COLUMN status;
```

For data migrations, rollback may not be fully safe or possible. In that case, document the limitation and consider a forward-only corrective migration instead.

## Destructive migrations

A destructive migration removes data or changes data in a way that cannot be easily restored.

Examples:

```txt
DROP TABLE
DROP COLUMN
DELETE without a recoverable copy
changing a column type with possible data loss
rewriting production values
```

Treat destructive migrations carefully.

Before applying one:

```txt
create a backup
review the SQL
run it locally
test rollback if rollback is supported
understand which data will be lost
```

For production systems, destructive migrations should be deliberate and documented.

## Migration review checklist

Before committing a migration, check:

```txt
Does the file name have a stable sortable id?
Does the up migration do one clear thing?
Does the down migration revert it when possible?
Is the SQL written for the correct engine?
Are indexes added for new lookup patterns?
Are NOT NULL and UNIQUE constraints intentional?
Is data loss possible?
Was the migration tested locally?
```

This checklist catches many schema mistakes before they reach shared environments.

## SQLite migration notes

SQLite is a strong fit for local development and tests.

Use:

```bash
vix db migrate
```

for simple SQLite migration workflows.

Use `vix db status` before and after migration when you want to inspect the configured database state.

SQLite schema changes can have limitations depending on the operation. For complex changes, create a new table, copy data, verify it, and then replace the old table carefully.

## MySQL migration notes

MySQL is the natural choice when the application uses a database server.

Use:

```bash
vix orm migrate --db blog --dir migrations
```

and:

```bash
vix orm rollback --steps 1 --db blog --dir migrations
```

For generated migrations, use:

```bash
vix orm makemigrations \
  --new ./schema.new.json \
  --snapshot ./schema.json \
  --dir ./migrations \
  --name add_users \
  --dialect mysql
```

Use MySQL syntax in MySQL migration files.

Prefer transactional tables such as InnoDB.

Review generated SQL before applying it to a shared database.

## Common mistakes

### Editing an applied migration

Do not edit a migration after it has been applied to a shared database.

Create a new migration instead.

### Missing down migration

If rollback matters, create the `.down.sql` file with the `.up.sql` file.

A migration without a down file may still be usable, but it is not safely reversible.

### Mixing SQL dialects

Do not put SQLite schema syntax in a MySQL migration.

Do not put MySQL-only syntax in a SQLite migration.

### Using vague migration names

Prefer:

```txt
2026_06_02_120000_create_users
```

Avoid:

```txt
update_db
```

### Running destructive migrations without backup

Back up the database before destructive schema changes.

### Ignoring status

Run status commands when debugging migration state.

```bash
vix db status
vix orm status --db blog --dir migrations
```

## Recommended workflow

For a new Vix project using SQLite:

```txt
1. Configure database.engine as sqlite.
2. Create storage/ and migrations/.
3. Write .up.sql and .down.sql files.
4. Run vix db status.
5. Run vix db migrate.
6. Run the application.
7. Commit vix.json and migrations/.
```

For a MySQL-backed project:

```txt
1. Configure database.engine as mysql.
2. Create migrations/.
3. Write MySQL migration files.
4. Build with MySQL support.
5. Run vix orm migrate.
6. Check vix orm status.
7. Commit vix.json and migrations/.
```

For schema snapshot projects:

```txt
1. Keep schema.json as the current snapshot.
2. Write schema.new.json.
3. Generate migrations with vix orm makemigrations.
4. Review generated SQL.
5. Apply migrations locally.
6. Run tests.
7. Commit schema.json and migrations/.
```

## Next steps

Read the schema snapshots page if you want to generate migrations from schema JSON files.

Read the CLI page if you want the exact `vix db` and `vix orm` command workflow.

Read the transactions page if your migration or application logic needs atomic multi-step writes.
