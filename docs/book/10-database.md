# Database

In the previous chapter, you learned errors and logging.
Now you will connect a Vix application to a database.

```txt
Request → validation → database query → JSON Response
```

Memory disappears when the app restarts.
A database gives your application durable state.

## Public header

```cpp
#include <vix/db.hpp>
```

## SQLite or MySQL?

| Criteria    | SQLite                               | MySQL                             |
| ----------- | ------------------------------------ | --------------------------------- |
| Best for    | Local development, small apps, MVPs. | Multi-user production APIs.       |
| Setup       | Very simple, no server required.     | Requires a database server.       |
| Persistence | Stores data in a local file.         | Stores data in a server database. |

Start with SQLite for learning.

## Build flags

```bash
vix build --with-sqlite
vix run main.cpp --with-sqlite

vix build --with-mysql
vix run main.cpp --with-mysql
```

## First SQLite connection

```cpp
#include <vix/db.hpp>

auto db = vix::db::Database::sqlite("vix.db");
db.exec("CREATE TABLE IF NOT EXISTS healthcheck (id INTEGER PRIMARY KEY)");
```

## First MySQL connection

```cpp
auto db = vix::db::Database::mysql("tcp://127.0.0.1:3306", "root", "", "vixdb");
```

## Database from .env

```dotenv
DATABASE_ENGINE=sqlite
DATABASE_DEFAULT_NAME=vix.db
```

```cpp
vix::config::Config cfg{".env"};
vix::db::Database db{cfg};
```

## Create a table

```cpp
// SQLite
db.exec(
    "CREATE TABLE IF NOT EXISTS users ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
    "name TEXT NOT NULL, "
    "role TEXT NOT NULL)");

// MySQL
db.exec(
    "CREATE TABLE IF NOT EXISTS users ("
    "id BIGINT PRIMARY KEY AUTO_INCREMENT, "
    "name VARCHAR(255) NOT NULL, "
    "role VARCHAR(64) NOT NULL)");
```

## Insert data

```cpp
db.exec("INSERT INTO users (name, role) VALUES (?, ?)", "Alice", "admin");
```

Always use parameterized queries.
Never build SQL with string concatenation.

## Query data

```cpp
auto rows = db.query("SELECT id, name, role FROM users");
while (rows->next())
{
  const auto &row = rows->row();
  std::cout << row.getInt64(0) << " " << row.getString(1) << " " << row.getString(2) << "\n";
}
```

## Prepared statements

```cpp
vix::db::PooledConn conn(db.pool());
auto stmt = conn->prepare("SELECT id, name FROM users WHERE id = ?");
stmt->bind(1, static_cast<std::int64_t>(1));
auto rows = stmt->query();
```

Use prepared statements for:
user input,
route parameters,
query filters,
inserts,
updates,
deletes.

## Connection pool

```cpp
vix::db::PooledConn conn(db.pool());
// connection returns automatically when PooledConn is destroyed (RAII)
```

## Transactions

```cpp
db.transaction([&](vix::db::Connection &conn){
  conn.prepare("INSERT INTO users (name, role) VALUES (?, ?)")
      ->bind(1, "Alice")->bind(2, "admin")->exec();
  conn.prepare("INSERT INTO users (name, role) VALUES (?, ?)")
      ->bind(1, "Bob")->bind(2, "user")->exec();
});
```

Use transactions for:
orders + items,
user + profile,
money transfers,
any multi-step write.

## Complete database API

```cpp
#include <vix.hpp>
#include <vix/db.hpp>
#include <vix/log.hpp>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>

using namespace vix;

struct User {
  std::int64_t id{};
  std::string name;
  std::string role;
};

static json::Json user_to_json(const User &u)
{
  return json::kv({
    {"id", json::Json(u.id)},
    {"name", json::Json(u.name)},
    {"role", json::Json(u.role)}
  });
}

static void respond_error(Response &res,
                          int status,
                          const std::string &code,
                          const std::string &msg)
{
  res.status(status).json(json::kv({
    {"ok", json::Json(false)},
    {"error", json::Json(code)},
    {"message", json::Json(msg)}
  }));
}

static std::optional<std::int64_t> parse_id(const std::string &text)
{
  try {
    return std::stoll(text);
  } catch (...) {
    return std::nullopt;
  }
}

static void initialize_schema(vix::db::Database &db)
{
  db.exec("CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL, role TEXT NOT NULL)");
}

static void seed_users(vix::db::Database &db)
{
  auto rows = db.query("SELECT COUNT(*) FROM users");

  if (rows->next() && rows->row().getInt64(0) > 0)
    return;

  db.exec("INSERT INTO users (name, role) VALUES (?, ?)", "Alice", "admin");
  db.exec("INSERT INTO users (name, role) VALUES (?, ?)", "Bob", "user");
}

static json::Json list_users(vix::db::Database &db)
{
  json::Json items = json::Json::array();
  auto rows = db.query("SELECT id, name, role FROM users ORDER BY id ASC");

  while (rows->next())
  {
    const auto &row = rows->row();
    items.push_back(user_to_json({row.getInt64(0), row.getString(1), row.getString(2)}));
  }
  return items;
}

static std::optional<User> find_user_by_id(vix::db::Database &db, std::int64_t id)
{
  vix::db::PooledConn conn(db.pool());
  auto stmt = conn->prepare("SELECT id, name, role FROM users WHERE id = ?");
  stmt->bind(1, id);
  auto rows = stmt->query();

  if (!rows->next())
    return std::nullopt;

  const auto &row = rows->row();
  return User{row.getInt64(0), row.getString(1), row.getString(2)};
}

static User create_user(vix::db::Database &db, const std::string &name, const std::string &role)
{
  db.exec("INSERT INTO users (name, role) VALUES (?, ?)", name, role);
  auto rows = db.query("SELECT id, name, role FROM users ORDER BY id DESC LIMIT 1");

  if (!rows->next())
    throw std::runtime_error("failed to load created user");

  const auto &row = rows->row();

  return {
    row.getInt64(0),
    row.getString(1),
    row.getString(2)
  };
}

static void register_user_routes(App &app, vix::db::Database &db)
{
  app.get("/api/users", [&db](Request &, Response &res){
    try {
      res.json(json::kv({
        {"ok", json::Json(true)},
        {"data", list_users(db)}
      }));

    }catch (const std::exception &e) {
      vix::log::error("failed to list users", "details", e.what()); respond_error(res, 500, "internal_error", "Internal server error");
    }
  });

  app.get("/api/users/{id}", [&db](Request &req, Response &res){
    const auto id = parse_id(req.param("id"));

    if (!id) {
      respond_error(res, 400, "invalid_id", "Invalid user id");
      return;
    }

    const auto user = find_user_by_id(db, *id);
    if (!user) {
      respond_error(res, 404, "user_not_found", "User not found");
      return;
    }

    res.json(json::kv({
      {"ok", json::Json(true)},
      {"data", user_to_json(*user)}
    }));
  });

  app.post("/api/users", [&db](Request &req, Response &res){
    try{
      const auto &body = req.json();
      if (!body.is_object()) {
        respond_error(res, 400, "invalid_request", "Expected JSON object body");
        return;
      }

      const std::string name = body.value("name", "");
      const std::string role = body.value("role", "user");
      if (name.empty()) {
        respond_error(res, 400, "validation_failed", "Field 'name' is required");
        return;
      }

      const User user = create_user(db, name, role.empty() ? "user" : role);
      res.status(201).json(json::kv({
        {"ok", json::Json(true)},
        {"message", json::Json("user created")},
        {"data", user_to_json(user)}
      }));

    }catch (const std::exception &e) {
      vix::log::error("failed to create user", "details", e.what()); respond_error(res, 500, "internal_error", "Internal server error");
    }
  });
}

int main()
{
  vix::log::set_level(vix::log::LogLevel::Info);
  try{
    auto db = vix::db::Database::sqlite("vix.db");
    initialize_schema(db);
    seed_users(db);

    App app;

    app.get("/health", [](Request &, Response &res) {
      res.json({"ok", true, "service", "database-api"});
    });

    register_user_routes(app, db);

    app.run(8080);

    return 0;
  }catch (const std::exception &e){
    vix::log::critical("application startup failed", "details", e.what());
    return 1;
  }
}
```

## Test

```bash
curl -i http://127.0.0.1:8080/health
curl -i http://127.0.0.1:8080/api/users
curl -i http://127.0.0.1:8080/api/users/1
curl -i http://127.0.0.1:8080/api/users/999
curl -i -X POST http://127.0.0.1:8080/api/users \
  -H "Content-Type: application/json" \
  -d '{"name":"Charlie","role":"user"}'
```

Restart the app — unlike the memory API, the new user should still exist.

## Migrations

For real projects, use migrations instead of `CREATE TABLE IF NOT EXISTS` at startup.

```cpp
class CreateUsersTable final : public vix::db::Migration
{
public:
  std::string id() const override { return "2026-01-22-create-users"; }

  void up(vix::db::Connection &conn) override
  {
    conn.prepare("CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL, role TEXT NOT NULL)")->exec();
  }

  void down(vix::db::Connection &conn) override
  {
    conn.prepare("DROP TABLE IF EXISTS users")->exec();
  }
};
```

## Common mistakes

### Building SQL with string concatenation

```cpp
// Wrong
std::string sql = "SELECT * FROM users WHERE name = '" + name + "'";

// Correct — use prepared statements
auto stmt = conn->prepare("SELECT * FROM users WHERE name = ?");
stmt->bind(1, name);
```

### Returning raw database errors

Log internal details, return safe client errors.

### Not validating before insert

Always validate input before database writes.

## What you should remember

The Vix DB model is explicit:
connect → prepare → bind → query → read rows → commit when needed.

Use prepared statements for user input.
Use transactions for multi-step writes.
Routes should validate input, call database logic, and return safe JSON responses.

## Next chapter

[Next: Real-time WebSocket](/book/11-realtime-websocket)
