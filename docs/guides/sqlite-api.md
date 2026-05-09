# SQLite API

This guide shows how to build a Vix REST API backed by SQLite.

## Public headers

```cpp
#include <vix.hpp>
#include <vix/db.hpp>
```

## Setup

```bash
vix new sqlite-api
cd sqlite-api
vix build --with-sqlite
```

## Minimal SQLite connection

```cpp
auto db = vix::db::Database::sqlite("vix.db");
db.exec("CREATE TABLE IF NOT EXISTS healthcheck (id INTEGER PRIMARY KEY)");
```

## Initialize the database

```cpp
static void initialize_database(vix::db::Database &db)
{
  db.exec(
      "CREATE TABLE IF NOT EXISTS users ("
      "id INTEGER PRIMARY KEY AUTOINCREMENT, "
      "name TEXT NOT NULL, "
      "email TEXT NOT NULL UNIQUE, "
      "role TEXT NOT NULL DEFAULT 'user'"
      ")");
}
```

## Query all users

```cpp
static json::Json list_users(vix::db::Database &db)
{
  json::Json items = json::Json::array();
  auto rows = db.query("SELECT id, name, email, role FROM users ORDER BY id DESC");
  while (rows->next())
  {
    const auto &row = rows->row();
    items.push_back(json::kv({
        {"id", json::Json(row.getInt64(0))},
        {"name", json::Json(row.getString(1))},
        {"email", json::Json(row.getString(2))},
        {"role", json::Json(row.getString(3))},
    }));
  }
  return items;
}
```

## Find one user by id (prepared statement)

```cpp
static std::optional<json::Json> find_user_by_id(vix::db::Database &db, std::int64_t id)
{
  vix::db::PooledConn conn(db.pool());
  auto stmt = conn->prepare("SELECT id, name, email, role FROM users WHERE id = ?");
  stmt->bind(1, id);
  auto rows = stmt->query();

  if (!rows->next())
    return std::nullopt;

  const auto &row = rows->row();
  return json::kv({
      {"id", json::Json(row.getInt64(0))},
      {"name", json::Json(row.getString(1))},
      {"email", json::Json(row.getString(2))},
      {"role", json::Json(row.getString(3))},
  });
}
```

## Create a user

```cpp
static std::int64_t create_user(vix::db::Database &db,
    const std::string &name, const std::string &email, const std::string &role)
{
  vix::db::PooledConn conn(db.pool());
  auto stmt = conn->prepare("INSERT INTO users (name, email, role) VALUES (?, ?, ?)");
  stmt->bind(1, name);
  stmt->bind(2, email); stmt->bind(3, role);
  stmt->exec();
  auto rows = conn->prepare("SELECT last_insert_rowid()")->query();
  if (!rows->next())
    throw std::runtime_error("failed to read inserted id");

  return rows->row().getInt64(0);
}
```

## Use transactions

```cpp
db.transaction([&](vix::db::Connection &conn){
  conn.prepare("INSERT INTO users (name, email, role) VALUES (?, ?, ?)")
    ->bind(1, "Alice")->bind(2, "alice@example.com")->bind(3, "admin")->exec();

  conn.prepare("INSERT INTO users (name, email, role) VALUES (?, ?, ?)")
    ->bind(1, "Bob")->bind(2, "bob@example.com")->bind(3, "user")->exec();
});
```

## Test the API

```bash
curl -i http://127.0.0.1:8080/health
curl -i http://127.0.0.1:8080/users
curl -i http://127.0.0.1:8080/users/1
curl -i -X POST http://127.0.0.1:8080/users \
  -H "Content-Type: application/json" \
  -d '{"name":"Ada","email":"ada@example.com","role":"admin"}'
```

## Common SQLite types

| SQLite type | Use for                          |
|-------------|----------------------------------|
| `INTEGER`   | IDs, counters, and timestamps.   |
| `TEXT`      | Strings, emails, and names.      |
| `REAL`      | Floating-point numeric values.   |
| `BLOB`      | Binary data.                     |

## Common mistakes

### Building SQL with string concatenation

```cpp
// Wrong
auto rows = db.query("SELECT * FROM users WHERE id = " + id);

// Correct
auto stmt = conn->prepare("SELECT * FROM users WHERE id = ?");
stmt->bind(1, id);
```

### Forgetting SQLite build support

```bash
vix build --with-sqlite
vix run main.cpp --with-sqlite  # for script mode
```

## What to use next

- [MySQL API guide](/guides/mysql-api)
- [Validation guide](/guides/validation)
- [Production Nginx + systemd guide](/guides/production-nginx-systemd)
