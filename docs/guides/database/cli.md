# Database CLI

Vix provides database-related CLI commands for inspecting project database state, applying migrations, creating backups, and working with schema-driven migration workflows.

The database CLI is not a replacement for the C++ API.

The C++ API is used by the application.

The CLI is used by developers, scripts, CI jobs, and deployment workflows.

A typical project uses both:

```txt
application code
  -> vix::db
  -> vix::orm

developer workflow
  -> vix db
  -> vix orm
```

## Command groups

Vix database tooling is split into two command groups.

```bash
vix db
```

is focused on project database inspection and practical database operations such as SQLite status, SQLite migrations, and local backups.

```bash
vix orm
```

is focused on migration workflows, rollback, migration status, and schema snapshot based migration generation.

Use `vix db` when you want to inspect or manage the configured project database.

Use `vix orm` when you are working with migration files and schema snapshots.

## Project configuration

The CLI reads project database information from `vix.json`.

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
      "password": "",
      "database": "blog"
    },
    "migrations": "migrations"
  }
}
```

The important fields for database tooling are:

```txt
database.engine
database.sqlite.path
database.storage
database.migrations
database.mysql.host
database.mysql.port
database.mysql.user
database.mysql.database
```

Do not commit production credentials in `vix.json`.

Use environment files, deployment secrets, or runtime configuration for real passwords.

## Recommended project layout

A database-enabled Vix project can use this layout:

```txt
my-app/
├── vix.json
├── .env.example
├── src/
│   └── main.cpp
├── storage/
│   └── .gitkeep
└── migrations/
    ├── 2026_06_02_120000_create_users.up.sql
    └── 2026_06_02_120000_create_users.down.sql
```

Commit:

```txt
vix.json
.env.example
migrations/
storage/.gitkeep
```

Do not commit local database runtime files:

```txt
storage/*.db
storage/*.db-wal
storage/*.db-shm
```

A typical `.gitignore` contains:

```txt
storage/*.db
storage/*.db-wal
storage/*.db-shm
.env
.env.local
.env.production.local
```

## vix db status

Use `vix db status` to inspect the configured database state.

```bash
vix db status
```

For a SQLite project, the command can show information such as:

```txt
engine
database path
storage directory
migration directory
database file presence
WAL file path
SHM file path
warnings
errors
```

This is useful before running migrations, debugging local setup, or checking whether the project database path is correct.

Example:

```bash
vix db status
```

Expected style of output:

```txt
Database
engine: sqlite
path: storage/Blog.db
storage: storage
migrations: migrations
```

The exact output can evolve with the CLI, but the purpose remains the same: show the project database configuration and local database state.

## JSON output

Use JSON output when scripts or CI jobs need to read database status.

```bash
vix db status --json
```

This is useful when a script needs to check whether the database path exists, whether the storage directory is configured, or whether warnings are present.

Example workflow:

```bash
vix db status --json
```

A CI job can parse the output and fail early if the project database configuration is invalid.

## vix db migrate

Use `vix db migrate` to apply pending migrations for a configured SQLite project.

```bash
vix db migrate
```

The command reads the migration directory from `vix.json`.

```json
{
  "database": {
    "migrations": "migrations"
  }
}
```

Migration files use paired names:

```txt
2026_06_02_120000_create_users.up.sql
2026_06_02_120000_create_users.down.sql
```

The `up` file applies the migration.

The `down` file reverts the migration.

For SQLite projects, this gives a simple local workflow:

```bash
vix db status
vix db migrate
vix run --with-sqlite
```

## vix db backup

Use `vix db backup` to create a local SQLite database backup.

```bash
vix db backup
```

This is useful before destructive migrations, experiments, or manual database changes.

If SQLite sidecar files exist, such as WAL and SHM files, the backup workflow can include them depending on the implementation.

Common SQLite runtime files are:

```txt
storage/app.db
storage/app.db-wal
storage/app.db-shm
```

Backups are useful for local development, but they are not a complete production backup strategy.

For production, use database-specific backup tooling and infrastructure-level backups.

## vix db and MySQL

`vix db` is primarily useful for SQLite-oriented project workflows.

A project can still configure MySQL in `vix.json`, but MySQL migration workflows should usually use `vix orm`.

For MySQL:

```bash
vix orm migrate --db blog --dir migrations
vix orm status --db blog --dir migrations
vix orm rollback --steps 1 --db blog --dir migrations
```

Use `vix db` for project inspection and SQLite-local workflows.

Use `vix orm` for migration-oriented workflows, especially when working with MySQL.

## vix orm migrate

Use `vix orm migrate` to apply migration files.

```bash
vix orm migrate --db blog --dir migrations
```

The `--dir` option points to the migration directory.

The `--db` option identifies the target database used by the migration tool.

A typical migration directory looks like this:

```txt
migrations/
├── 2026_06_02_120000_create_users.up.sql
└── 2026_06_02_120000_create_users.down.sql
```

When migrations are applied, Vix tracks them in a migration table.

The default migration table is:

```txt
schema_migrations
```

It stores metadata such as:

```txt
id
checksum
applied_at
```

This allows Vix to know which migrations have already been applied and whether an applied migration file has changed.

## vix orm status

Use `vix orm status` to inspect migration state.

```bash
vix orm status --db blog --dir migrations
```

Status is useful before applying migrations, before rollback, and when debugging a local or shared database.

A typical workflow is:

```bash
vix orm status --db blog --dir migrations
vix orm migrate --db blog --dir migrations
vix orm status --db blog --dir migrations
```

This makes migration state visible before and after changes.

## vix orm rollback

Use `vix orm rollback` to revert applied migrations.

```bash
vix orm rollback --steps 1 --db blog --dir migrations
```

`--steps 1` means rollback the last applied migration.

Rollback depends on the matching `.down.sql` file.

Example:

```txt
2026_06_02_120000_create_users.up.sql
2026_06_02_120000_create_users.down.sql
```

If the down migration is missing or unsafe, rollback should not be treated as reliable.

For destructive or production changes, review the down migration carefully before depending on it.

## vix orm makemigrations

Use `vix orm makemigrations` to generate migration files from schema snapshots.

Example:

```bash
vix orm makemigrations \
  --new ./schema.new.json \
  --snapshot ./schema.json \
  --dir ./migrations \
  --name add_users \
  --dialect mysql
```

This compares:

```txt
schema.json
schema.new.json
```

and writes migration files under:

```txt
migrations/
```

The output can look like:

```txt
migrations/2026_06_02_120000_add_users.up.sql
migrations/2026_06_02_120000_add_users.down.sql
```

Generated migration files must still be reviewed.

Migration generation helps with repetitive schema changes, but it cannot understand every production constraint, data migration risk, or rollout strategy.

## Schema snapshot workflow

A schema snapshot workflow usually looks like this:

```txt
1. Keep schema.json as the current accepted schema.
2. Create schema.new.json with the desired schema.
3. Generate migrations with vix orm makemigrations.
4. Review the generated .up.sql and .down.sql files.
5. Apply migrations locally.
6. Run tests.
7. Update schema.json to the accepted new schema.
8. Commit schema.json and migrations together.
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

Use this workflow when the project wants schema changes to be structured and reviewable.

## SQLite workflow

For a local SQLite-backed Vix project:

```bash
mkdir -p storage migrations
```

Create `vix.json`:

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

Create a migration:

```txt
migrations/2026_06_02_120000_create_users.up.sql
migrations/2026_06_02_120000_create_users.down.sql
```

Apply:

```bash
vix db status
vix db migrate
vix db status
```

Build and run:

```bash
vix run --with-sqlite
```

This is the simplest complete database CLI workflow.

## MySQL workflow

For a MySQL-backed project, create `vix.json`:

```json
{
  "name": "Blog",
  "database": {
    "engine": "mysql",
    "mysql": {
      "host": "127.0.0.1",
      "port": 3306,
      "user": "root",
      "password": "",
      "database": "blog"
    },
    "migrations": "migrations"
  }
}
```

Create migration files with MySQL SQL:

```txt
migrations/2026_06_02_120000_create_users.up.sql
migrations/2026_06_02_120000_create_users.down.sql
```

Apply migrations:

```bash
vix orm migrate --db blog --dir migrations
```

Check status:

```bash
vix orm status --db blog --dir migrations
```

Rollback one migration:

```bash
vix orm rollback --steps 1 --db blog --dir migrations
```

Build with MySQL support:

```bash
vix build --with-mysql
```

Run:

```bash
vix run --with-mysql
```

## Migration file examples

SQLite `up` migration:

```sql
CREATE TABLE users (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  email TEXT NOT NULL UNIQUE,
  name TEXT NOT NULL,
  created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);
```

SQLite `down` migration:

```sql
DROP TABLE users;
```

MySQL `up` migration:

```sql
CREATE TABLE users (
  id BIGINT AUTO_INCREMENT PRIMARY KEY,
  email VARCHAR(255) NOT NULL UNIQUE,
  name VARCHAR(255) NOT NULL,
  created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB;
```

MySQL `down` migration:

```sql
DROP TABLE users;
```

Keep migration SQL aligned with the selected database engine.

Do not use SQLite schema syntax in a MySQL migration.

Do not use MySQL-only syntax in a SQLite migration.

## Migration tracking

Vix file-based migrations are tracked in the database.

The tracking table is usually:

```txt
schema_migrations
```

Each applied migration records its id and checksum.

The checksum matters because migration files should not be silently edited after they have already been applied.

If a migration has already been applied and the schema needs another change, create a new migration.

Do not rewrite old migration history in shared environments.

## Review before applying

Before applying migrations, check:

```txt
Is the target database correct?
Is the migration directory correct?
Is the SQL dialect correct?
Does the up migration do one clear thing?
Does the down migration safely revert it?
Can data be lost?
Is there a backup before destructive changes?
Were generated migrations reviewed?
```

The CLI helps run migrations. It does not replace engineering review.

## Destructive migrations

A destructive migration removes or rewrites data.

Examples:

```txt
DROP TABLE
DROP COLUMN
DELETE existing data
change column type with possible data loss
deduplicate rows before adding UNIQUE
```

Before running destructive migrations:

```txt
create a backup
review SQL carefully
test locally
check rollback
understand what data can be lost
```

For production systems, prefer staged schema changes when possible.

## CI usage

Database CLI commands can be used in CI.

For example:

```bash
vix db status --json
vix orm status --db blog --dir migrations
vix check --tests
```

A CI pipeline can verify that:

```txt
database configuration is present
migration files are visible
schema snapshots generate expected migrations
tests pass against a test database
```

Use a temporary database for CI.

For SQLite, CI can create and delete a database file.

For MySQL, CI should use a test database, container, or service instance.

## Local development habits

For SQLite local development:

```bash
rm -f storage/*.db storage/*.db-wal storage/*.db-shm
vix db migrate
vix run --with-sqlite
```

For MySQL local development:

```bash
vix orm migrate --db blog --dir migrations
vix run --with-mysql
```

Before changing migrations:

```bash
vix orm status --db blog --dir migrations
```

After generating migrations:

```bash
git diff migrations
```

Review migration files before applying them.

## Common mistakes

### Running the wrong command group

Use `vix db` for SQLite project inspection and local database operations.

Use `vix orm` for migration workflows, rollback, status, and schema snapshot generation.

### Missing migration directory

If `vix.json` says:

```json
{
  "database": {
    "migrations": "migrations"
  }
}
```

create it:

```bash
mkdir -p migrations
```

### Missing storage directory

If SQLite uses:

```json
{
  "sqlite": {
    "path": "storage/app.db"
  }
}
```

create the parent directory:

```bash
mkdir -p storage
```

### Editing applied migrations

Do not edit migrations already applied to shared databases.

Create a new migration.

### Missing down migration

Rollback needs a `.down.sql` file.

If rollback matters, write the down migration with the up migration.

### Applying generated SQL without review

Review generated migration files before applying them.

### Mixing dialects

Keep SQLite migrations SQLite-compatible.

Keep MySQL migrations MySQL-compatible.

### Committing local database files

Commit migrations.

Do not commit local `.db`, `.db-wal`, or `.db-shm` files.

## Recommended workflow

For SQLite:

```txt
1. Configure database.engine as sqlite in vix.json.
2. Create storage/ and migrations/.
3. Write .up.sql and .down.sql files.
4. Run vix db status.
5. Run vix db migrate.
6. Build with --with-sqlite.
7. Run the application.
```

For MySQL:

```txt
1. Configure database.engine as mysql in vix.json.
2. Create migrations/.
3. Write MySQL migration files.
4. Run vix orm migrate.
5. Run vix orm status.
6. Build with --with-mysql.
7. Run the application.
```

For schema snapshots:

```txt
1. Keep schema.json committed.
2. Write schema.new.json.
3. Run vix orm makemigrations.
4. Review generated SQL.
5. Apply migrations locally.
6. Run tests.
7. Commit schema.json and migrations together.
```

## Related pages

Read the configuration page to understand `vix.json` and `.env`.

Read the migrations page to understand migration files and `schema_migrations`.

Read the schema snapshots page to understand `vix orm makemigrations`.

Read the ORM guide if you want to use repositories and mappers on top of the database layer.
