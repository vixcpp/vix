# vix orm

Database migrations and schema management.

---

## Usage

```bash
vix orm migrate   [options]
vix orm rollback  --steps <n> [options]
vix orm status    [options]
vix orm makemigrations --new <schema.json> [options]
```

---

## Description

`vix orm` manages:

- Database migrations
- Schema evolution
- Migration history
- Rollback operations

Supports MySQL and SQLite dialects.

---

## Commands

### Migrate

```bash
vix orm migrate
```

Apply pending migrations.

---

### Rollback

```bash
vix orm rollback --steps <n>
```

Rollback last N applied migrations.

Required:

```bash
--steps <n>
```

---

### Status

```bash
vix orm status
```

Show applied and pending migrations.

---

### Makemigrations

```bash
vix orm makemigrations --new <schema.json>
```

Generate migration from schema diff.

Options:

```bash
--new <path>          New schema (required)
--snapshot <path>     Previous schema snapshot (default: schema.json)
--name <label>        Migration label (default: auto)
--dialect <mysql|sqlite>  SQL dialect (default: mysql)
```

---

## Common Options

```bash
--db <name>           Database name
--dir <path>          Migrations directory
--host <uri>          MySQL URI
--user <name>         Database user
--pass <pass>         Database password
--project-dir <path>  Force project root detection
--tool <path>         Override migrator executable path
```

---

## Environment Defaults

```bash
VIX_ORM_HOST
VIX_ORM_USER
VIX_ORM_PASS
VIX_ORM_DB
VIX_ORM_DIR
VIX_ORM_TOOL
```

---

## Examples

Apply migrations:

```bash
vix orm migrate --db blog_db --dir ./migrations
```

Rollback:

```bash
vix orm rollback --steps 1 --db blog_db --dir ./migrations
```

Status:

```bash
vix orm status --db blog_db
```

Using environment variables:

```bash
VIX_ORM_DB=blog_db vix orm migrate --dir ./migrations
```

Generate migration:

```bash
vix orm makemigrations   --new ./schema.new.json   --snapshot ./schema.json   --dir ./migrations   --name create_users   --dialect mysql
```

---

`vix orm` keeps database schema evolution explicit and versioned.

