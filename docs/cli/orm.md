# vix orm

`vix orm` provides database migration and schema management tooling.

Use it when you want to apply migrations, roll back migrations, inspect migration status, or generate migration files from schema changes.

`vix orm` is a CLI wrapper around the Vix database migrator tool. It resolves your project, finds the migrations directory, locates the migrator executable, then forwards the command to the database migration engine.

## Usage

```bash
vix orm migrate [options]
vix orm rollback --steps <n> [options]
vix orm status [options]
vix orm makemigrations --new <schema.json> [options]
```

## Subcommands

| Subcommand       | Purpose                                       |
| ---------------- | --------------------------------------------- |
| `migrate`        | Apply pending migrations.                     |
| `rollback`       | Roll back applied migrations.                 |
| `status`         | Show migration status.                        |
| `makemigrations` | Generate migration files from schema changes. |

## Basic usage

```bash
vix orm migrate --db blog_db --dir ./migrations
vix orm status --db blog_db
vix orm rollback --steps 1 --db blog_db
vix orm makemigrations --new ./schema.new.json
```

More complete `makemigrations` example:

```bash
vix orm makemigrations \
  --new ./schema.new.json \
  --snapshot ./schema.json \
  --dir ./migrations \
  --name create_users \
  --dialect mysql
```

## What it does

`vix orm` helps manage database schema changes.

It can:

```txt
apply migration files
roll back migration files
inspect migration state
generate .up.sql and .down.sql files from schema JSON diffs
reuse environment defaults
detect the current project root
detect common migration directories
call the installed Vix database migrator tool
```

## Project detection

`vix orm` tries to detect the project root automatically.

It looks upward from the current directory and prefers:

```txt
a Vix repository root
an application root with CMakePresets.json
an application root with src/ and CMakeLists.txt
a directory with CMakeLists.txt
```

If detection fails, pass the root manually:

```bash
vix orm migrate --project-dir /path/to/project --db blog_db
```

## Migration directory detection

If `--dir` is not provided, Vix tries common migration locations:

```txt
migrations
db/migrations
database/migrations
sql/migrations
db/sql
migrations/mysql
migrations/sqlite
```

If none exists, Vix falls back to:

```txt
migrations
```

For `migrate`, `rollback`, and `status`, the migrations directory must already exist.

For `makemigrations`, Vix creates the migrations directory if it is missing.

## Migrator tool resolution

`vix orm` calls the Vix database migrator executable.

The preferred tool name is:

```txt
vix_db_migrator
```

Tool resolution order:

```txt
VIX_DB_TOOL
VIX_ORM_TOOL
installed libexec paths
same directory as the vix binary
development build paths
vix_db_migrator from PATH
```

`VIX_ORM_TOOL` is kept for backward compatibility.

Prefer `VIX_DB_TOOL` for new setups.

```bash
VIX_DB_TOOL=/path/to/vix_db_migrator vix orm migrate --db blog_db
```

## Migrate

```bash
vix orm migrate --db blog_db --dir ./migrations
```

`migrate` applies all pending migration files from the migrations directory.

With environment defaults:

```bash
VIX_ORM_DB=blog_db \
VIX_ORM_USER=root \
VIX_ORM_PASS=secret \
vix orm migrate --dir ./migrations
```

## Rollback

```bash
vix orm rollback --steps 1 --db blog_db --dir ./migrations
```

Rollback requires `--steps`.

```bash
vix orm rollback --steps 2 --db blog_db
```

`--steps` must be greater than or equal to `1`.

## Status

```bash
vix orm status --db blog_db --dir ./migrations
```

`status` checks the migration setup and prints migration information.

The current implementation prints the migrations directory and a status message from the migrator.

## Makemigrations

```bash
vix orm makemigrations --new ./schema.new.json
```

`makemigrations` compares a previous schema snapshot with a new schema JSON file.

Default previous snapshot:

```txt
schema.json
```

Default migration name:

```txt
auto
```

Default dialect:

```txt
mysql
```

Complete example:

```bash
vix orm makemigrations \
  --new ./schema.new.json \
  --snapshot ./schema.json \
  --dir ./migrations \
  --name create_users \
  --dialect mysql
```

This generates files like:

```txt
migrations/2026_05_28_130501_create_users.up.sql
migrations/2026_05_28_130501_create_users.down.sql
```

It also updates the snapshot file with the new schema.

## Schema workflow

A typical schema workflow looks like this:

```bash
# 1. Edit or generate a new schema file
vim schema.new.json

# 2. Generate migration files
vix orm makemigrations \
  --new ./schema.new.json \
  --snapshot ./schema.json \
  --dir ./migrations \
  --name add_users \
  --dialect mysql

# 3. Apply migrations
vix orm migrate --db blog_db --dir ./migrations

# 4. Check status
vix orm status --db blog_db --dir ./migrations

# 5. Validate the project
vix check --tests
```

## Dialects

Accepted dialect values:

```txt
mysql
sqlite
```

Current implementation note:

```txt
makemigrations currently generates SQL for mysql only.
```

If `--dialect sqlite` is passed to `makemigrations`, the CLI accepts the value, but the generator can still fail because SQLite SQL generation is not implemented yet.

Use MySQL for migration generation today:

```bash
vix orm makemigrations \
  --new ./schema.new.json \
  --dialect mysql
```

## Database drivers

The migrator uses the database drivers enabled when the tool was built.

If MySQL support is enabled, the migrator uses the MySQL driver.

If SQLite support is enabled instead, the migrator uses the SQLite driver.

If no database driver is enabled, the migrator reports that it was built without DB drivers.

## Common options

| Option                 | Description                                    |
| ---------------------- | ---------------------------------------------- |
| `--db <name>`          | Database name. Overrides `VIX_ORM_DB`.         |
| `--dir <path>`         | Migrations directory. Overrides `VIX_ORM_DIR`. |
| `--host <uri>`         | MySQL URI. Default is `tcp://127.0.0.1:3306`.  |
| `--user <name>`        | Database user. Default is `root`.              |
| `--pass <pass>`        | Database password.                             |
| `--project-dir <path>` | Force project root detection.                  |
| `--tool <path>`        | Override migrator executable path.             |
| `-h, --help`           | Show command help.                             |

## Makemigrations options

| Option              | Description                                  |                                  |
| ------------------- | -------------------------------------------- | -------------------------------- |
| `--new <path>`      | New schema JSON file. Required.              |                                  |
| `--snapshot <path>` | Previous snapshot. Default is `schema.json`. |                                  |
| `--name <label>`    | Migration label. Default is `auto`.          |                                  |
| `--dialect <mysql   | sqlite>`                                     | SQL dialect. Default is `mysql`. |

## Rollback options

| Option        | Description                                        |
| ------------- | -------------------------------------------------- |
| `--steps <n>` | Roll back the last N applied migrations. Required. |

## Environment variables

| Variable       | Description                                               |
| -------------- | --------------------------------------------------------- |
| `VIX_ORM_HOST` | Default database host. Default is `tcp://127.0.0.1:3306`. |
| `VIX_ORM_USER` | Default database user. Default is `root`.                 |
| `VIX_ORM_PASS` | Default database password.                                |
| `VIX_ORM_DB`   | Default database name. Default is `vixdb`.                |
| `VIX_ORM_DIR`  | Default migrations directory.                             |
| `VIX_DB_TOOL`  | Preferred migrator executable path.                       |
| `VIX_ORM_TOOL` | Legacy migrator executable path.                          |

## Common workflows

### Run migrations

```bash
vix orm migrate --db blog_db --dir ./migrations
```

### Check migration status

```bash
vix orm status --db blog_db --dir ./migrations
```

### Roll back one migration

```bash
vix orm rollback --steps 1 --db blog_db --dir ./migrations
```

### Generate a MySQL migration

```bash
vix orm makemigrations \
  --new ./schema.new.json \
  --snapshot ./schema.json \
  --dir ./migrations \
  --name create_users \
  --dialect mysql
```

### Use environment defaults

```bash
VIX_ORM_DB=blog_db \
VIX_ORM_USER=root \
VIX_ORM_PASS=secret \
vix orm migrate --dir ./migrations
```

### Use a custom migrator tool

```bash
VIX_DB_TOOL=./build/db_build/vix_db_migrator \
vix orm migrate --db blog_db --dir ./migrations
```

### Force project root

```bash
vix orm migrate \
  --project-dir /home/me/apps/blog \
  --db blog_db \
  --dir ./migrations
```

## Recommended migration workflow

```bash
# 1. Generate a migration
vix orm makemigrations \
  --new ./schema.new.json \
  --snapshot ./schema.json \
  --dir ./migrations \
  --name add_users \
  --dialect mysql

# 2. Review generated SQL
cat ./migrations/*.up.sql
cat ./migrations/*.down.sql

# 3. Apply migrations
vix orm migrate --db blog_db --dir ./migrations

# 4. Check migration status
vix orm status --db blog_db --dir ./migrations

# 5. Validate the project
vix check --tests
```

## CI usage

```bash
vix install
vix orm migrate --db blog_db --dir ./migrations
vix check --tests
```

With environment variables:

```bash
VIX_ORM_HOST=tcp://127.0.0.1:3306 \
VIX_ORM_USER=root \
VIX_ORM_PASS=secret \
VIX_ORM_DB=blog_db \
vix orm migrate --dir ./migrations
```

## Common mistakes

### Forgetting `--steps` during rollback

Wrong:

```bash
vix orm rollback --db blog_db
```

Correct:

```bash
vix orm rollback --steps 1 --db blog_db
```

### Forgetting `--new` during makemigrations

Wrong:

```bash
vix orm makemigrations --name create_users
```

Correct:

```bash
vix orm makemigrations --new ./schema.new.json --name create_users
```

### Expecting `makemigrations` to connect to the database

`makemigrations` does not need database connection arguments.

It compares schema files and writes migration files.

```bash
vix orm makemigrations --new ./schema.new.json
```

### Expecting SQLite migration generation to be complete

The CLI accepts:

```bash
vix orm makemigrations --new ./schema.new.json --dialect sqlite
```

But SQL generation is currently implemented for MySQL.

Use:

```bash
vix orm makemigrations --new ./schema.new.json --dialect mysql
```

### Passing passwords directly in shell history

Avoid this when possible:

```bash
vix orm migrate --db blog_db --user root --pass secret
```

Prefer environment variables:

```bash
VIX_ORM_PASS=secret vix orm migrate --db blog_db
```

### Running outside a project

If project detection fails, use:

```bash
vix orm migrate \
  --project-dir /path/to/project \
  --db blog_db \
  --dir ./migrations
```

### Forgetting to create migrations directory

For `migrate`, `rollback`, and `status`, the directory must exist.

```bash
mkdir -p migrations
vix orm migrate --db blog_db --dir ./migrations
```

`makemigrations` can create the directory automatically.

## Troubleshooting

### Migrator tool not found

Use:

```bash
VIX_DB_TOOL=/path/to/vix_db_migrator \
vix orm migrate --db blog_db --dir ./migrations
```

Or pass it directly:

```bash
vix orm migrate \
  --tool /path/to/vix_db_migrator \
  --db blog_db \
  --dir ./migrations
```

### Project directory not detected

Use:

```bash
vix orm migrate \
  --project-dir /path/to/project \
  --db blog_db
```

### Migrations directory not found

Create it:

```bash
mkdir -p migrations
```

Then run:

```bash
vix orm migrate --db blog_db --dir ./migrations
```

### Database driver missing

If the migrator says it was built without DB drivers, rebuild Vix with the required database driver enabled.

For MySQL:

```bash
vix build --with-mysql
```

For SQLite:

```bash
vix build --with-sqlite
```

## Related commands

| Command                   | Purpose                                                 |
| ------------------------- | ------------------------------------------------------- |
| `vix db`                  | Database checks, migrations, and SQLite backup tooling. |
| `vix build --with-mysql`  | Build with MySQL support.                               |
| `vix build --with-sqlite` | Build with SQLite support.                              |
| `vix check`               | Validate the project after migrations.                  |
| `vix task`                | Automate migration workflows.                           |
| `vix env check`           | Validate project environment files.                     |

## Next step

Continue with peer-to-peer nodes.

[Open the vix p2p guide](/cli/p2p)
