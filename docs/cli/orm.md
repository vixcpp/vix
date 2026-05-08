# vix orm

`vix orm` provides database migration and schema management tooling.

Use it when you want to apply migrations, rollback migrations, inspect migration status, or generate migration files from schema changes.

## Usage

```bash
vix orm migrate   [options]
vix orm rollback  --steps <n> [options]
vix orm status    [options]
vix orm makemigrations --new <schema.json> [options]
```

## Subcommands

| Subcommand | Purpose |
|------------|---------|
| `migrate` | Apply pending migrations |
| `rollback` | Roll back applied migrations |
| `status` | Show migration status |
| `makemigrations` | Generate migration files from schema changes |

## Basic usage

```bash
vix orm migrate --db blog_db --dir ./migrations
vix orm status --db blog_db
vix orm rollback --steps 1 --db blog_db
vix orm makemigrations \
  --new ./schema.new.json \
  --snapshot ./schema.json \
  --dir ./migrations \
  --name create_users \
  --dialect mysql
```

## Migrate

```bash
vix orm migrate --db blog_db --dir ./migrations
```

## Rollback

```bash
vix orm rollback --steps 1 --db blog_db --dir ./migrations
vix orm rollback --steps 2 --db blog_db
```

`--steps` is required.

## Status

```bash
vix orm status --db blog_db
```

## Makemigrations

```bash
vix orm makemigrations --new ./schema.new.json

vix orm makemigrations \
  --new ./schema.new.json \
  --snapshot ./schema.json \
  --dir ./migrations \
  --name create_users \
  --dialect mysql
```

Supported dialects: `mysql` (default), `sqlite`

## Common options

| Option | Description |
|--------|-------------|
| `--db <name>` | Database name. Overrides `VIX_ORM_DB`. |
| `--dir <path>` | Migrations directory. Overrides `VIX_ORM_DIR`. |
| `--host <uri>` | MySQL URI. Default is `tcp://127.0.0.1:3306`. |
| `--user <name>` | Database user. Default is `root`. |
| `--pass <pass>` | Database password. |
| `--project-dir <path>` | Force project root detection. |
| `--tool <path>` | Override migrator executable path. |
| `-h, --help` | Show command help. |

## Makemigrations options

| Option | Description |
|--------|-------------|
| `--new <path>` | New schema JSON file. Required. |
| `--snapshot <path>` | Previous snapshot. Default is `schema.json`. |
| `--name <label>` | Migration label. Default is automatic. |
| `--dialect <mysql\|sqlite>` | SQL dialect. Default is `mysql`. |

## Rollback options

| Option | Description |
|--------|-------------|
| `--steps <n>` | Roll back the last N applied migrations. Required. |

## Environment variables

| Variable | Description |
|----------|-------------|
| `VIX_ORM_HOST` | Default database host. |
| `VIX_ORM_USER` | Default database user. |
| `VIX_ORM_PASS` | Default database password. |
| `VIX_ORM_DB` | Default database name. |
| `VIX_ORM_DIR` | Default migrations directory. |
| `VIX_ORM_TOOL` | Default migrator executable path. |

## Common workflows

```bash
# Run migrations
vix orm migrate --db blog_db --dir ./migrations

# Check status
vix orm status --db blog_db

# Roll back one migration
vix orm rollback --steps 1 --db blog_db --dir ./migrations

# Generate MySQL migration
vix orm makemigrations \
  --new ./schema.new.json \
  --snapshot ./schema.json \
  --dir ./migrations \
  --name create_users \
  --dialect mysql

# Use environment defaults
VIX_ORM_DB=blog_db \
VIX_ORM_USER=root \
VIX_ORM_PASS=secret \
vix orm migrate --dir ./migrations
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

# 2. Apply migrations
vix orm migrate --db blog_db --dir ./migrations

# 3. Check status
vix orm status --db blog_db --dir ./migrations

# 4. Validate
vix check --tests
```

## Common mistakes

### Forgetting `--steps` during rollback

```bash
# Wrong
vix orm rollback --db blog_db

# Correct
vix orm rollback --steps 1 --db blog_db
```

### Forgetting `--new` during makemigrations

```bash
# Wrong
vix orm makemigrations --name create_users

# Correct
vix orm makemigrations --new ./schema.new.json --name create_users
```

### Passing password directly in shell history

```bash
# Prefer environment variables
VIX_ORM_PASS=secret vix orm migrate --db blog_db
```

## Related commands

| Command | Purpose |
|---------|---------|
| `vix build --with-mysql` | Build with MySQL support |
| `vix build --with-sqlite` | Build with SQLite support |
| `vix check` | Validate project after migrations |
| `vix task` | Add migration workflows to `vix.json` |

## Next step

Continue with peer-to-peer nodes.

[Open the vix p2p guide](/cli/p2p)
