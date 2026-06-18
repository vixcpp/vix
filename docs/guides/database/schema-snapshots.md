# Schema snapshots

A schema snapshot is a structured description of the database schema at a given point in time.

Instead of only keeping SQL migration files, a project can also keep a machine-readable model of the current schema. Vix uses this model to compare two schema versions and generate migration operations.

A schema snapshot is useful when a project wants a clearer database history:

```txt
current schema
new schema
schema diff
generated migration files
reviewed SQL
applied migration
```

This workflow is not required for every project. Small applications can write migration files by hand. Schema snapshots become useful when the schema grows and the project wants more structure around database evolution.

## What a snapshot contains

A Vix schema snapshot describes tables, columns, indexes, types, defaults, and constraints.

At the C++ level, the schema model is built around these types:

```txt
vix::db::schema::Schema
vix::db::schema::Table
vix::db::schema::Column
vix::db::schema::Index
vix::db::schema::Type
vix::db::schema::DefaultValue
```

A `Schema` contains tables.

A `Table` contains columns and indexes.

A `Column` describes the column name, type, nullability, primary key flag, auto-increment flag, uniqueness flag, and optional default value.

An `Index` describes the index name, indexed columns, and whether the index is unique.

This gives Vix a database representation that can be serialized, compared, and used for migration generation.

## Snapshot JSON

A snapshot can be stored as JSON.

Example:

```json
{
  "version": 1,
  "tables": [
    {
      "name": "users",
      "columns": [
        {
          "name": "id",
          "type": {
            "base": "bigint"
          },
          "nullable": false,
          "primary_key": true,
          "auto_increment": true
        },
        {
          "name": "email",
          "type": {
            "base": "varchar",
            "size": 255
          },
          "nullable": false,
          "unique": true
        },
        {
          "name": "name",
          "type": {
            "base": "varchar",
            "size": 255
          },
          "nullable": false
        },
        {
          "name": "created_at",
          "type": {
            "base": "datetime"
          },
          "nullable": false,
          "default": "CURRENT_TIMESTAMP"
        }
      ],
      "indexes": [
        {
          "name": "idx_users_email",
          "columns": ["email"],
          "unique": true
        }
      ]
    }
  ]
}
```

This file is not a database. It is a description of the database structure.

The database still changes through migrations. The snapshot helps Vix understand what changed between two schema versions.

## Why snapshots are useful

SQL migration files are excellent for applying changes, but they describe change steps, not the full current shape of the database.

A snapshot describes the current target shape.

That makes it useful for tools.

With snapshots, Vix can compare:

```txt
schema.json
schema.new.json
```

and produce operations such as:

```txt
create table
drop table
add column
drop column
create index
drop index
```

Those operations can then be converted into SQL migration files.

This is helpful because the project can review generated SQL before applying it.

## The snapshot workflow

A typical schema snapshot workflow looks like this:

```txt
1. Keep schema.json as the current schema snapshot.
2. Create or edit schema.new.json.
3. Ask Vix to compare the two files.
4. Generate migration files.
5. Review the generated SQL.
6. Apply the migration locally.
7. Run tests.
8. Commit schema.json and migration files.
```

Example command:

```bash
vix orm makemigrations \
  --new ./schema.new.json \
  --snapshot ./schema.json \
  --dir ./migrations \
  --name add_users \
  --dialect mysql
```

This can generate files such as:

```txt
migrations/2026_06_02_120000_add_users.up.sql
migrations/2026_06_02_120000_add_users.down.sql
```

After generation, the migration should be reviewed like handwritten SQL.

## Current dialect support

The schema model is designed to support multiple SQL dialects.

The current migration SQL generator is focused on MySQL.

Use:

```bash
vix orm makemigrations \
  --new ./schema.new.json \
  --snapshot ./schema.json \
  --dir ./migrations \
  --name add_users \
  --dialect mysql
```

The CLI may accept `sqlite` as a dialect value, but migration generation for SQLite may not be available depending on the installed Vix version.

For now, use the schema snapshot generator primarily with MySQL.

For SQLite projects, handwritten migrations are usually simple and clear.

## Supported schema types

The schema model uses a portable set of base types.

```txt
int
bigint
double
bool
varchar
text
datetime
```

In JSON, a type is represented with a `base` field.

```json
{
  "base": "varchar",
  "size": 255
}
```

A `varchar` type should include a size.

```json
{
  "base": "varchar",
  "size": 255
}
```

A type without a size can be written like this:

```json
{
  "base": "bigint"
}
```

The SQL generator maps these schema types to the selected SQL dialect.

For MySQL, examples include:

```txt
bigint   -> BIGINT
bool     -> TINYINT(1)
text     -> TEXT
datetime -> DATETIME
varchar  -> VARCHAR(n)
```

## Columns

A column definition contains the column name, type, and constraints.

Example:

```json
{
  "name": "email",
  "type": {
    "base": "varchar",
    "size": 255
  },
  "nullable": false,
  "unique": true
}
```

A primary key column can be represented like this:

```json
{
  "name": "id",
  "type": {
    "base": "bigint"
  },
  "nullable": false,
  "primary_key": true,
  "auto_increment": true
}
```

A column with a default value can be represented like this:

```json
{
  "name": "created_at",
  "type": {
    "base": "datetime"
  },
  "nullable": false,
  "default": "CURRENT_TIMESTAMP"
}
```

Default values are raw SQL literals. That means the value should be written in the form expected by the target SQL dialect.

For example:

```json
{
  "default": "CURRENT_TIMESTAMP"
}
```

or:

```json
{
  "default": "'active'"
}
```

Because defaults are emitted as SQL literals, review them carefully.

## Indexes

Indexes are stored under the table definition.

Example:

```json
{
  "name": "idx_users_email",
  "columns": ["email"],
  "unique": true
}
```

A non-unique index can look like this:

```json
{
  "name": "idx_posts_author_id",
  "columns": ["author_id"],
  "unique": false
}
```

Indexes are important because schema changes are not only about columns. Query performance often depends on whether the database has indexes that match the application’s lookup patterns.

When a new query starts filtering by a column frequently, the schema snapshot should usually include the corresponding index.

## Full schema example

This schema contains `users` and `posts`.

```json
{
  "version": 1,
  "tables": [
    {
      "name": "users",
      "columns": [
        {
          "name": "id",
          "type": {
            "base": "bigint"
          },
          "nullable": false,
          "primary_key": true,
          "auto_increment": true
        },
        {
          "name": "email",
          "type": {
            "base": "varchar",
            "size": 255
          },
          "nullable": false,
          "unique": true
        },
        {
          "name": "name",
          "type": {
            "base": "varchar",
            "size": 255
          },
          "nullable": false
        },
        {
          "name": "created_at",
          "type": {
            "base": "datetime"
          },
          "nullable": false,
          "default": "CURRENT_TIMESTAMP"
        }
      ],
      "indexes": [
        {
          "name": "idx_users_email",
          "columns": ["email"],
          "unique": true
        }
      ]
    },
    {
      "name": "posts",
      "columns": [
        {
          "name": "id",
          "type": {
            "base": "bigint"
          },
          "nullable": false,
          "primary_key": true,
          "auto_increment": true
        },
        {
          "name": "user_id",
          "type": {
            "base": "bigint"
          },
          "nullable": false
        },
        {
          "name": "title",
          "type": {
            "base": "varchar",
            "size": 255
          },
          "nullable": false
        },
        {
          "name": "body",
          "type": {
            "base": "text"
          },
          "nullable": false
        },
        {
          "name": "created_at",
          "type": {
            "base": "datetime"
          },
          "nullable": false,
          "default": "CURRENT_TIMESTAMP"
        }
      ],
      "indexes": [
        {
          "name": "idx_posts_user_id",
          "columns": ["user_id"],
          "unique": false
        }
      ]
    }
  ]
}
```

This is the kind of file a project can keep as `schema.json`.

When the schema changes, create `schema.new.json`, then generate a migration from the difference.

## Generating a migration

Suppose the project has:

```txt
schema.json
schema.new.json
migrations/
```

Generate a migration:

```bash
vix orm makemigrations \
  --new ./schema.new.json \
  --snapshot ./schema.json \
  --dir ./migrations \
  --name add_posts \
  --dialect mysql
```

Vix compares the old snapshot with the new snapshot.

Then it writes migration files under `migrations/`.

The output can look like:

```txt
migrations/2026_06_02_120000_add_posts.up.sql
migrations/2026_06_02_120000_add_posts.down.sql
```

The generated `up` file applies the change.

The generated `down` file reverts it.

Review both files before applying the migration.

## Applying the generated migration

After generating and reviewing migration files, apply them.

```bash
vix orm migrate --db blog --dir ./migrations
```

Check status:

```bash
vix orm status --db blog --dir ./migrations
```

Run tests:

```bash
vix check --tests
```

If everything is correct, commit the schema snapshot and migration files.

```bash
git add schema.json migrations
git commit -m "docs: add database schema migration"
```

Use a project-specific commit message. The example above is only a placeholder.

## Using schema snapshots from C++

The schema model can also be used directly in C++.

```cpp
#include <vix.hpp>
#include <vix/db/schema/Json.hpp>
#include <vix/db/schema/Schema.hpp>

int main()
{
    vix::db::schema::Schema schema;

    vix::db::schema::Table users;
    users.name = "users";

    users.columns.push_back({
        "id",
        vix::db::schema::Type::BigInt(),
        false,
        true,
        true,
        false,
        std::nullopt
    });

    users.columns.push_back({
        "email",
        vix::db::schema::Type::VarChar(255),
        false,
        false,
        false,
        true,
        std::nullopt
    });

    schema.tables.push_back(std::move(users));

    auto json = vix::db::schema::to_json_string(schema, true);

    vix::print(json);

    return 0;
}
```

This creates a schema object and serializes it to JSON.

For most projects, editing JSON snapshots directly is simpler. The C++ API is useful for tools that generate schema from code.

## Reading a snapshot from JSON

```cpp
#include <vix.hpp>
#include <vix/db/schema/Json.hpp>

int main()
{
    const std::string text = R"json(
{
  "version": 1,
  "tables": [
    {
      "name": "users",
      "columns": [
        {
          "name": "id",
          "type": {
            "base": "bigint"
          },
          "nullable": false,
          "primary_key": true,
          "auto_increment": true
        }
      ],
      "indexes": []
    }
  ]
}
)json";

    auto schema = vix::db::schema::from_json_string_or_throw(text);

    if (schema.findTable("users"))
    {
        vix::print("users table found");
    }

    return 0;
}
```

Use `from_json_string_or_throw()` when the program expects the snapshot to be valid and should fail on invalid schema JSON.

## Computing a schema diff in C++

The diff engine compares two schema values.

```cpp
#include <vix.hpp>
#include <vix/db/mig/diff/Diff.hpp>
#include <vix/db/schema/Json.hpp>

int main()
{
    auto oldSchema = vix::db::schema::from_json_string_or_throw(R"json(
{
  "version": 1,
  "tables": []
}
)json");

    auto newSchema = vix::db::schema::from_json_string_or_throw(R"json(
{
  "version": 1,
  "tables": [
    {
      "name": "users",
      "columns": [
        {
          "name": "id",
          "type": {
            "base": "bigint"
          },
          "nullable": false,
          "primary_key": true,
          "auto_increment": true
        }
      ],
      "indexes": []
    }
  ]
}
)json");

    auto ops = vix::db::mig::diff::diff_or_throw(
        oldSchema,
        newSchema
    );

    vix::print("migration operations:", ops.size());

    return 0;
}
```

The diff result is a list of migration operations.

Supported operations include:

```txt
create table
drop table
add column
drop column
create index
drop index
```

## Generating MySQL SQL in C++

After computing diff operations, generate MySQL migration SQL.

```cpp
#include <vix.hpp>
#include <vix/db/mig/diff/Diff.hpp>
#include <vix/db/mig/sql/MySqlGenerator.hpp>
#include <vix/db/schema/Json.hpp>

int main()
{
    auto oldSchema = vix::db::schema::from_json_string_or_throw(R"json(
{
  "version": 1,
  "tables": []
}
)json");

    auto newSchema = vix::db::schema::from_json_string_or_throw(R"json(
{
  "version": 1,
  "tables": [
    {
      "name": "users",
      "columns": [
        {
          "name": "id",
          "type": {
            "base": "bigint"
          },
          "nullable": false,
          "primary_key": true,
          "auto_increment": true
        },
        {
          "name": "email",
          "type": {
            "base": "varchar",
            "size": 255
          },
          "nullable": false,
          "unique": true
        }
      ],
      "indexes": []
    }
  ]
}
)json");

    auto ops = vix::db::mig::diff::diff_or_throw(
        oldSchema,
        newSchema
    );

    auto upSql = vix::db::mig::sql::to_mysql_up(ops);
    auto downSql = vix::db::mig::sql::to_mysql_down(ops);

    vix::print("up migration:");
    vix::print(upSql);

    vix::print("down migration:");
    vix::print(downSql);

    return 0;
}
```

This is the lower-level version of what migration tooling can do for a project.

## Review generated SQL

Generated SQL should be reviewed before it is applied.

A generator can produce useful migration files, but it cannot know every production constraint, data migration requirement, rollback risk, or operational concern.

Review at least these points:

```txt
Does the migration target the correct SQL dialect?
Does the table name match the project convention?
Does each column have the intended type?
Are NOT NULL constraints safe for existing data?
Are default values correct?
Are unique constraints intentional?
Are indexes needed for expected queries?
Is any data lost?
Does the down migration make sense?
```

For shared or production databases, review is not optional.

## Snapshot updates

After a migration is generated and accepted, the current snapshot should move forward.

The usual pattern is:

```txt
schema.json      old current schema
schema.new.json  proposed schema
migration files  generated diff
schema.json      updated to the new schema after acceptance
```

Depending on the exact CLI workflow, the snapshot file may be updated automatically or manually. Check the generated files and the resulting snapshot before committing.

A commit should keep the migration files and the accepted schema snapshot together.

## Schema snapshots and handwritten migrations

Schema snapshots do not replace handwritten migrations.

Some schema changes are simple and can be generated.

Other changes need human judgment.

Examples:

```txt
split full_name into first_name and last_name
move data from one table to another
backfill values before adding NOT NULL
rename a column without losing data
change a column type safely
deduplicate data before adding UNIQUE
```

These operations often require custom SQL or application-specific migration steps.

In those cases, use the snapshot as a guide, but write the migration deliberately.

## Safe schema evolution

When changing a database used by real users, schema changes should be staged carefully.

A safer approach is often:

```txt
1. Add a nullable column.
2. Deploy code that writes both old and new shapes.
3. Backfill existing data.
4. Deploy code that reads the new shape.
5. Add NOT NULL or remove the old column later.
```

A schema diff can tell you what changed. It cannot decide the safest rollout strategy for production data.

Use generated migrations as a starting point, not as a substitute for engineering judgment.

## Common mistakes

### Treating the snapshot as the database

A snapshot is a description.

The database changes only when migrations are applied.

### Applying generated SQL without review

Always review generated migration files before applying them.

### Mixing dialects

A MySQL SQL generator produces MySQL SQL.

Do not apply generated MySQL SQL to SQLite.

### Forgetting indexes

If the new application code queries by a column, consider whether the schema needs an index.

### Making destructive changes too early

Dropping a table or column can destroy data.

Prefer staged changes when real data exists.

### Editing old snapshots without migration files

The snapshot and migration files must move together.

A changed snapshot without a migration does not change the database.

## Recommended workflow

Use this workflow for schema snapshot projects:

```txt
1. Keep schema.json committed.
2. Write schema.new.json when the schema changes.
3. Generate migrations with vix orm makemigrations.
4. Review the generated .up.sql and .down.sql files.
5. Apply migrations locally.
6. Run tests.
7. Update schema.json to the accepted schema.
8. Commit schema.json and migrations together.
```

For simple SQLite projects, handwritten migrations may be faster.

For larger MySQL projects, schema snapshots help keep migration generation and review more structured.

## Next steps

Read the migrations page if you want the file-based SQL migration workflow.

Read the CLI page for the exact `vix db` and `vix orm` commands.

Read the queries page if you want to use the schema in application code after migrations are applied.
