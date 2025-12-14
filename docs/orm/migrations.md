# ORM Migrations — Overview

This page explains how **Vix ORM migrations** work and how to use the CLI to apply or rollback SQL files **from your application**, not from the ORM module itself.

---

## What is a migration?

A migration is a versioned SQL change stored as **two files**:

- `<id>.up.sql` → apply changes
- `<id>.down.sql` → rollback changes

Example IDs are typically timestamp-like so ordering is natural:

- `0001_init.up.sql`
- `0001_init.down.sql`

Vix uses the **base id** (`0001_init`) to track applied migrations in the database.

---

## Folder layout (recommended)

In your application:

```
apps/blog/
└── migrations/
    ├── 0001_init.up.sql
    └── 0001_init.down.sql
```

You can also use:

- `db/migrations/`
- `database/migrations/`

But the simplest and recommended approach is `./migrations`.

---

## How migrations are tracked

When you run migrations, Vix creates a metadata table:

- `schema_migrations`

It stores:

- `id` — migration identifier
- `checksum` — SHA-256 hash of the `.up.sql` file
- `applied_at` — string timestamp marker

This allows Vix to:

- detect already-applied migrations
- detect modified migration files (checksum mismatch)
- rollback the last applied migration(s) in the correct order

---

## CLI usage

### Apply all pending migrations

```bash
vix orm migrate --db blog_db --dir ./migrations
```

### Rollback last N migrations

```bash
vix orm rollback --steps 1 --db blog_db --dir ./migrations
```

### Show migration status (basic)

```bash
vix orm status --db blog_db --dir ./migrations
```

---

## Flags and environment variables

### CLI flags

- `--db <name>`
- `--dir <path>`
- `--host <uri>`
- `--user <name>`
- `--pass <pass>`
- `--steps <n>` (rollback only)

### Environment variables

- `VIX_ORM_HOST` (default: `tcp://127.0.0.1:3306`)
- `VIX_ORM_USER` (default: `root`)
- `VIX_ORM_PASS` (default: `""`)
- `VIX_ORM_DB` (default: `vixdb`)
- `VIX_ORM_DIR` (default: `migrations`)

Example:

```bash
export VIX_ORM_DB=blog_db
vix orm migrate --dir ./migrations
```

---

## Important behavior: database creation

Migrations **do not create the database itself**.

```bash
vix orm migrate --db blog_db
```

- `blog_db` must already exist
- migrations only create tables inside `blog_db`

```sql
CREATE DATABASE blog_db;
```

---

## Writing migrations

### Example: init migration

**migrations/0001_init.up.sql**

```sql
CREATE TABLE IF NOT EXISTS users (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    name VARCHAR(120) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

**migrations/0001_init.down.sql**

```sql
DROP TABLE IF EXISTS users;
```

---

## Troubleshooting

### Cannot rollback: migration files not found

- Ensure the migration files still exist
- Do not delete old migrations
- Point `--dir` to the correct folder

### Migrations applied but DB unchanged

- Wrong database selected
- Forgot `--db` or `VIX_ORM_DB`

```bash
vix orm migrate --db blog_db --dir ./migrations
```

---

## Example — migrate.cpp

```cpp
#include <vix/orm/orm.hpp>
#include <vix/orm/FileMigrationsRunner.hpp>

#include <iostream>

static int run_migrations()
{
    auto raw = vix::orm::make_mysql_conn(
        "tcp://127.0.0.1:3306",
        "root",
        "",
        "blog_db"
    );

    vix::orm::MySQLConnection conn{raw};
    vix::orm::FileMigrationsRunner runner{conn, "./migrations"};
    runner.applyAll();
    return 0;
}

int main()
{
    try
    {
        run_migrations();
        std::cout << "[OK] migrations applied\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[ERR] " << e.what() << "\n";
        return 1;
    }
}
```

---

## Next improvements (optional roadmap)

- Show applied vs pending migrations
- SQLite runner support
- Custom schema table
- Dry-run mode
