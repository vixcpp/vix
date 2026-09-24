# vix db

`vix db` inspects and manages project database state.

Use it when you want to check database configuration, inspect SQLite storage status, apply migrations, or create a local database backup.

```bash
vix db
```

## Overview

`vix db` is the database utility command for Vix projects.

It can:

- inspect database configuration
- detect SQLite usage
- check whether the storage directory exists
- check whether the storage directory is writable
- check whether the database file exists
- show SQLite WAL and SHM sidecar paths
- check whether migration files exist
- apply file-based SQL migrations
- create timestamped SQLite backups
- copy SQLite WAL and SHM sidecar files when present
- print status as JSON for scripts and CI

The command is currently focused on SQLite project workflows.

MySQL can be detected from configuration, but migration and backup actions currently support SQLite only.

## Usage

```bash
vix db [action] [options]
```

## Actions

| Action    | Purpose                                                          |
| --------- | ---------------------------------------------------------------- |
| `status`  | Inspect database and storage status. This is the default action. |
| `migrate` | Apply pending file-based SQL migrations.                         |
| `backup`  | Create a SQLite database backup.                                 |

## Basic examples

```bash
# Inspect database status
vix db

# Same as vix db
vix db status

# Print status as JSON
vix db status --json

# Apply migrations
vix db migrate

# Create a database backup
vix db backup

# Show verbose diagnostics
vix db status --verbose
```

## Default action

If no action is passed, Vix runs:

```bash
vix db status
```

So these are equivalent:

```bash
vix db
vix db status
```

## Configuration source

`vix db` reads database configuration from:

```txt
vix.json
```

under:

```txt
database
```

Example:

```json
{
  "name": "PulseGrid",
  "database": {
    "engine": "sqlite",
    "sqlite": {
      "path": "storage/PulseGrid.db"
    },
    "storage": "storage",
    "migrations": "migrations"
  }
}
```

## Database config fields

| Field                  | Purpose                                               |
| ---------------------- | ----------------------------------------------------- |
| `database.engine`      | Database engine. Supported values: `sqlite`, `mysql`. |
| `database.sqlite.path` | SQLite database file path.                            |
| `database.sqlite_path` | Flat alternative for SQLite path.                     |
| `database.storage`     | Storage directory.                                    |
| `database.migrations`  | Migrations directory.                                 |

## Default configuration

If no database configuration exists, Vix uses SQLite defaults based on the project name.

For a project named:

```txt
PulseGrid
```

the defaults are:

```txt
engine: sqlite
database: storage/PulseGrid.db
storage: storage
migrations: migrations
wal: storage/PulseGrid.db-wal
shm: storage/PulseGrid.db-shm
```

Project name is detected from:

```txt
vix.json name
*.vix file name
current directory name
```

## SQLite config example

```json
{
  "name": "PulseGrid",
  "database": {
    "engine": "sqlite",
    "sqlite": {
      "path": "storage/PulseGrid.db"
    },
    "storage": "storage",
    "migrations": "migrations"
  }
}
```

## Flat SQLite path example

You can also use the flat `sqlite_path` field:

```json
{
  "name": "PulseGrid",
  "database": {
    "engine": "sqlite",
    "sqlite_path": "storage/PulseGrid.db",
    "storage": "storage",
    "migrations": "migrations"
  }
}
```

## MySQL config detection

Vix can parse:

```json
{
  "database": {
    "engine": "mysql"
  }
}
```

But current `migrate` and `backup` actions support SQLite only.

If you run migration with MySQL configured, Vix reports:

```txt
vix db migrate currently supports SQLite only.
Fix: set database.engine to sqlite
```

If you run backup with MySQL configured, Vix reports:

```txt
vix db backup currently supports SQLite only.
Fix: set database.engine to sqlite before using vix db backup
```

## Status

Run:

```bash
vix db status
```

or:

```bash
vix db
```

This prints the database status.

Example output shape:

```txt
Database
Project: PulseGrid
Engine: sqlite
Path: storage/PulseGrid.db
Storage: storage
Database exists: yes
Storage exists: yes
Storage writable: yes

SQLite
Detected: yes
WAL path: storage/PulseGrid.db-wal
WAL file: not present
SHM path: storage/PulseGrid.db-shm
SHM file: not present

Migrations
Directory: migrations
Exists: yes

Status
Result: ok
database status looks good
```

## What status checks

`vix db status` checks:

| Check                | Meaning                                           |
| -------------------- | ------------------------------------------------- |
| engine               | Whether the database engine is known.             |
| SQLite detection     | Whether the project looks like a SQLite project.  |
| storage directory    | Whether the storage directory exists.             |
| storage writable     | Whether Vix can write into the storage directory. |
| database file        | Whether the database file exists.                 |
| WAL file             | Whether the SQLite WAL sidecar file exists.       |
| SHM file             | Whether the SQLite SHM sidecar file exists.       |
| migrations directory | Whether the migrations directory exists.          |

## Status levels

`vix db status` can produce three statuses:

| Status    | Meaning                                     | Exit code |
| --------- | ------------------------------------------- | --------- |
| `ok`      | Database status looks good.                 | `0`       |
| `warning` | Something is missing, but it may be normal. | `0`       |
| `error`   | A required condition failed.                | `1`       |

A missing database file is a warning because the database may not exist before the first migration.

A missing storage directory is an error because migrations and backups need it.

## JSON status

Use:

```bash
vix db status --json
```

Example output shape:

```json
{
  "project": "PulseGrid",
  "engine": "sqlite",
  "status": "ok",
  "database": {
    "path": "storage/PulseGrid.db",
    "exists": true
  },
  "storage": {
    "path": "storage",
    "exists": true,
    "writable": true
  },
  "sqlite": {
    "detected": true,
    "wal_path": "storage/PulseGrid.db-wal",
    "wal_exists": false,
    "shm_path": "storage/PulseGrid.db-shm",
    "shm_exists": false
  },
  "migrations": {
    "path": "migrations",
    "exists": true
  },
  "warnings": [],
  "errors": []
}
```

Use JSON output for:

- CI
- scripts
- deployment checks
- dashboards
- automated diagnostics

## Migrations

Run:

```bash
vix db migrate
```

This applies file-based SQL migrations from the configured migrations directory.

Example config:

```json
{
  "database": {
    "engine": "sqlite",
    "sqlite": {
      "path": "storage/PulseGrid.db"
    },
    "storage": "storage",
    "migrations": "migrations"
  }
}
```

Expected output shape:

```txt
Database Migrations
Engine: sqlite
Database: storage/PulseGrid.db
Directory: migrations
migrations applied successfully
```

## Migration requirements

`vix db migrate` requires:

```txt
database.engine = sqlite
storage directory exists
migrations directory exists
Vix CLI built with db module support
```

If the CLI was built without database support, Vix reports:

```txt
vix db migrate is not available in this build.
Fix: rebuild the Vix CLI with the db module enabled
```

## Migration files

Migrations are loaded from the configured migrations directory.

Default:

```txt
migrations
```

A typical structure can look like:

```txt
migrations/
├── 001_create_users.up.sql
├── 002_create_posts.up.sql
└── 003_add_indexes.up.sql
```

The migrator expects `.up.sql` migration files.

## Create storage and migrations folders

Before running migrations, create the required directories:

```bash
mkdir -p storage migrations
```

Then add migration files:

```bash
touch migrations/001_init.up.sql
```

Then run:

```bash
vix db migrate
```

## Backup

Run:

```bash
vix db backup
```

This creates a timestamped SQLite backup under:

```txt
backups/
```

Example output shape:

```txt
Database Backup
Database: storage/PulseGrid.db
Backup: backups/PulseGrid-20260528-120501.db
WAL copied: yes
SHM copied: yes
database backup created
```

## Backup file naming

Backup files use the database filename and a timestamp.

For:

```txt
storage/PulseGrid.db
```

Vix creates a backup like:

```txt
backups/PulseGrid-20260528-120501.db
```

If the database has no extension, Vix uses:

```txt
.db
```

## WAL and SHM backups

SQLite may create sidecar files:

```txt
storage/PulseGrid.db-wal
storage/PulseGrid.db-shm
```

When present, `vix db backup` copies them too.

For a backup:

```txt
backups/PulseGrid-20260528-120501.db
```

the sidecar backups become:

```txt
backups/PulseGrid-20260528-120501.db-wal
backups/PulseGrid-20260528-120501.db-shm
```

If WAL or SHM files are not present, Vix simply reports:

```txt
WAL copied: no
SHM copied: no
```

That is not automatically an error.

## Backup requirements

`vix db backup` requires:

```txt
database.engine = sqlite
database file exists
```

If the database file does not exist, Vix reports:

```txt
database file not found: storage/PulseGrid.db
Fix: run vix db migrate or create the database before backing it up
```

## Full SQLite workflow

```bash
mkdir -p storage migrations
```

Create a migration:

```sql
-- migrations/001_init.up.sql
CREATE TABLE IF NOT EXISTS users (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  name TEXT NOT NULL,
  created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);
```

Check status:

```bash
vix db status
```

Apply migrations:

```bash
vix db migrate
```

Check again:

```bash
vix db status
```

Create a backup:

```bash
vix db backup
```

## Full `vix.json` example

```json
{
  "name": "PulseGrid",
  "database": {
    "engine": "sqlite",
    "sqlite": {
      "path": "storage/PulseGrid.db"
    },
    "storage": "storage",
    "migrations": "migrations"
  },
  "production": {
    "service": {
      "name": "pulsegrid",
      "health_local": "http://127.0.0.1:8080/"
    },
    "health": {
      "service": "pulsegrid.service",
      "local": "http://127.0.0.1:8080/"
    },
    "logs": {
      "service": "pulsegrid",
      "lines": 120
    }
  }
}
```

## Relationship with `vix build`

Some projects need database support at build time.

For SQLite support:

```bash
vix build --with-sqlite
```

For MySQL support:

```bash
vix build --with-mysql
```

For scripts:

```bash
vix run main.cpp --with-sqlite
```

or:

```bash
vix dev main.cpp --with-sqlite
```

## Relationship with `vix service`

In production, the service should run with the same database path expected by your project.

Before installing or restarting the service:

```bash
vix db status
vix db migrate
vix build --preset release --with-sqlite
vix service restart
```

## Relationship with `vix health`

Database problems often appear as app health failures.

If health fails:

```bash
vix health local
vix logs app --errors
vix db status
```

## Relationship with `vix logs`

When migrations fail or the app cannot open the database, inspect logs:

```bash
vix logs app --errors
```

If production errors repeat:

```bash
vix logs errors --repeated
```

## Relationship with `vix doctor production`

`vix doctor production` checks broader production readiness.

`vix db` focuses on database readiness.

Before deployment:

```bash
vix db status
vix db backup
vix doctor production
```

## Options

| Option       | Description                                                    |
| ------------ | -------------------------------------------------------------- |
| `--json`     | Print supported output as JSON. Currently useful for `status`. |
| `--verbose`  | Show verbose diagnostic output.                                |
| `-v`         | Alias for `--verbose`.                                         |
| `-h, --help` | Show help.                                                     |

## Commands reference

| Command                | Description                              |
| ---------------------- | ---------------------------------------- |
| `vix db`               | Same as `vix db status`.                 |
| `vix db status`        | Inspect database and storage status.     |
| `vix db status --json` | Print status as JSON.                    |
| `vix db migrate`       | Apply pending file-based SQL migrations. |
| `vix db backup`        | Create a SQLite backup.                  |
| `vix db --help`        | Show help.                               |

## Common workflows

### Inspect database status

```bash
vix db
```

### Inspect database status as JSON

```bash
vix db status --json
```

### Prepare SQLite folders

```bash
mkdir -p storage migrations
```

### Apply migrations

```bash
vix db migrate
```

### Create backup before deployment

```bash
vix db backup
```

### Deploy with SQLite

```bash
vix db status
vix db backup
vix build --preset release --with-sqlite
vix service restart
vix health local
```

### Debug database-related production failure

```bash
vix health local
vix logs app --errors
vix db status
```

## Common mistakes

### Running migrate before creating storage directory

Wrong:

```bash
vix db migrate
```

when `storage/` does not exist.

Correct:

```bash
mkdir -p storage
vix db migrate
```

### Running migrate before creating migrations directory

Wrong:

```bash
vix db migrate
```

when `migrations/` does not exist.

Correct:

```bash
mkdir -p migrations
vix db migrate
```

### Running backup before database exists

Wrong:

```bash
vix db backup
```

before the database file exists.

Correct:

```bash
vix db migrate
vix db backup
```

### Expecting backup to support MySQL

Current backup support is SQLite-only.

For MySQL, use your database server’s backup tools until Vix adds native MySQL backup support.

### Expecting migrate to support MySQL

Current migration support is SQLite-only.

For MySQL, use an external migration workflow until Vix adds native MySQL migration support.

### Ignoring WAL and SHM files

If your SQLite database uses WAL mode, sidecar files may exist.

`vix db backup` copies them when present.

Do not manually copy only the `.db` file while the database is actively writing unless you understand SQLite backup safety.

### Running from the wrong directory

Wrong:

```bash
cd ~
vix db status
```

Correct:

```bash
cd /path/to/myapp
vix db status
```

`vix db` reads `vix.json` from the current project directory.

## Troubleshooting

### Unknown database engine

Check:

```json
{
  "database": {
    "engine": "sqlite"
  }
}
```

Supported parsed values are:

```txt
sqlite
mysql
```

Any other value is treated as unknown.

### Storage directory is missing

Create it:

```bash
mkdir -p storage
```

Then run:

```bash
vix db status
```

### Storage directory is not writable

Check permissions:

```bash
ls -ld storage
```

Fix ownership or permissions:

```bash
sudo chown -R "$USER":"$USER" storage
```

Then run:

```bash
vix db status
```

### Database file does not exist yet

This can be normal before the first migration.

Run:

```bash
vix db migrate
```

or create the database from your app.

### Migrations directory is missing

Create it:

```bash
mkdir -p migrations
```

Add `.up.sql` files, then run:

```bash
vix db migrate
```

### Migration support is unavailable

If Vix reports:

```txt
vix db migrate is not available in this build.
```

rebuild the Vix CLI with the database module enabled.

### Migration failed

Inspect the SQL error, then check the migration file.

Run status again:

```bash
vix db status
```

If the app is running in production, inspect logs:

```bash
vix logs app --errors
```

### Backup failed

Check:

```bash
vix db status
```

Make sure:

- engine is SQLite
- database file exists
- `backups/` can be created
- current user can read the database file
- current user can write to the project directory

## Best practices

Keep database configuration in `vix.json`.

Use SQLite for simple local-first and small production apps.

Create `storage/` explicitly.

Keep migrations in `migrations/`.

Use `.up.sql` migration files.

Run `vix db status` before migrations.

Run `vix db backup` before production deployments.

Run `vix db status --json` in CI when useful.

Use `vix build --with-sqlite` for apps that need SQLite support.

Use `vix logs app --errors` when database errors appear in production.

## Related commands

| Command                   | Purpose                                  |
| ------------------------- | ---------------------------------------- |
| `vix build --with-sqlite` | Build with SQLite support.               |
| `vix build --with-mysql`  | Build with MySQL support.                |
| `vix run --with-sqlite`   | Run a script or app with SQLite support. |
| `vix dev --with-sqlite`   | Develop with SQLite support.             |
| `vix service`             | Manage the production app service.       |
| `vix health`              | Check whether the app responds.          |
| `vix logs`                | Inspect app and production errors.       |
| `vix doctor production`   | Inspect production readiness.            |

## Next step

Inspect production logs.

[Open the logs guide](/cli/logs)
