# Database

Build a persistent JSON API with Vix and SQLite.

```txt
HTTP request → validate input → database query → JSON response
```

## What you will build

```txt
GET    /health       → health check
GET    /users        → list users from database
GET    /users/{id}   → find one user
POST   /users        → create a user
PATCH  /users/{id}   → update a user
DELETE /users/{id}   → delete a user
```

## Create a workspace

```bash
mkdir -p ~/tmp/vix-examples/database
cd ~/tmp/vix-examples/database
touch main.cpp
```

## Full code

```cpp
#include <cstdint>
#include <optional>
#include <string>
#include <vix.hpp>
#include <vix/db.hpp>
using namespace vix;

static void respond_error(Response &res, int status,
                           const std::string &code, const std::string &message)
{
  res.status(status).json({
    "ok", false,
    "error", code,
    "message", message
  });
}

static std::optional<std::int64_t> parse_id(const std::string &text)
{
  try
  {
    std::size_t pos = 0;
    const auto value = std::stoll(text, &pos);
    if (pos != text.size() || value <= 0) return std::nullopt;
    return value;
  }
  catch (...) { return std::nullopt; }
}

static void initialize_database(vix::db::Database &db)
{
  db.exec(
      "CREATE TABLE IF NOT EXISTS users ("
      "id INTEGER PRIMARY KEY AUTOINCREMENT, "
      "name TEXT NOT NULL, "
      "email TEXT NOT NULL UNIQUE, "
      "role TEXT NOT NULL DEFAULT 'user', "
      "created_at INTEGER NOT NULL DEFAULT (strftime('%s','now')))");
}

static json::Json list_users(vix::db::Database &db)
{
  json::Json items = json::Json::array();
  auto rows = db.query("SELECT id, name, email, role, created_at FROM users ORDER BY id DESC");

  while (rows->next())
  {
    const auto &row = rows->row();
    items.push_back(json::obj({
      {"id", row.getInt64(0)},
      {"name", row.getString(1)},
      {"email", row.getString(2)},
      {"role", row.getString(3)},
      {"created_at", row.getInt64(4)
    }}));
  }
  return items;
}

static std::optional<json::Json> find_user_by_id(vix::db::Database &db, std::int64_t id)
{
  vix::db::PooledConn conn(db.pool());
  auto stmt = conn->prepare("SELECT id, name, email, role, created_at FROM users WHERE id = ?");
  stmt->bind(1, id);
  auto rows = stmt->query();

  if (!rows->next())
    return std::nullopt;

  const auto &row = rows->row();

  return json::obj({
    {"id", row.getInt64(0)},
    {"name", row.getString(1)},
    {"email", row.getString(2)}, {"role", row.getString(3)},
    {"created_at", row.getInt64(4)
  }});
}

static std::int64_t create_user(vix::db::Database &db, const std::string &name,
                                  const std::string &email, const std::string &role)
{
  vix::db::PooledConn conn(db.pool());
  auto stmt = conn->prepare("INSERT INTO users (name, email, role) VALUES (?, ?, ?)");
  stmt->bind(1, name); stmt->bind(2, email); stmt->bind(3, role.empty() ? "user" : role);
  stmt->exec();

  auto rows = conn->prepare("SELECT last_insert_rowid()")->query();

  if (!rows->next())
    throw std::runtime_error("failed to read inserted user id");

  return rows->row().getInt64(0);
}

static void register_routes(App &app, vix::db::Database &db)
{
  app.get("/health", [](Request &, Response &res){
    res.json({
      "ok", true,
      "service", "database-example"
    });
  });

  app.get("/users", [&db](Request &, Response &res){
    res.json({
      "ok", true,
      "data", list_users(db)
    });
  });

  app.get("/users/{id}", [&db](Request &req, Response &res){
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

    res.json({
      "ok", true,
      "data", *user
    });
  });

  app.post("/users", [&db](Request &req, Response &res){
    const auto &body = req.json();
    if (!body.is_object()) {
      respond_error(res, 400, "invalid_body", "Expected JSON object");
      return;
    }

    const std::string name = body.value("name", "");
    const std::string email = body.value("email", "");
    const std::string role = body.value("role", "user");

    if (name.empty()) {
      respond_error(res, 400, "validation_failed", "name is required");
      return;
    }

    if (email.empty()) {
      respond_error(res, 400, "validation_failed", "email is required");
      return;
    }

    try{
      const auto id = create_user(db, name, email, role);
      const auto user = find_user_by_id(db, id);

      res.status(201).json({
        "ok", true,
        "message", "user created",
        "data", user ? *user : json::Json{}
      });

    }catch (const std::exception &) {
      respond_error(res, 409, "create_failed", "Could not create user");
    }

  });

  app.del("/users/{id}", [&db](Request &req, Response &res){
    const auto id = parse_id(req.param("id"));
    if (!id) {
      respond_error(res, 400, "invalid_id", "Invalid user id");
      return;
    }

    if (!find_user_by_id(db, *id)) {
      respond_error(res, 404, "user_not_found", "User not found");
      return;
    }
    try{
      vix::db::PooledConn conn(db.pool());
      auto stmt = conn->prepare("DELETE FROM users WHERE id = ?");
      stmt->bind(1, *id); stmt->exec();

      res.json({
        "ok", true,
        "message", "user deleted",
        "id", *id
      });
    }catch (const std::exception &) {
      respond_error(res, 500, "delete_failed", "Could not delete user");
    }

  });
}

int main()
{
  try
  {
    vix::db::Database db = vix::db::Database::sqlite("vix.db");
    initialize_database(db);

    App app;

    register_routes(app, db);

    app.run(8080);

    return 0;
  }
  catch (const std::exception &e)
  {
    std::cerr << "database example error: " << e.what() << "\n";
    return 1;
  }
}
```

## Run and test

```bash
vix run main.cpp --with-sqlite
```

```bash
curl -i http://127.0.0.1:8080/health
curl -i http://127.0.0.1:8080/users
curl -i -X POST http://127.0.0.1:8080/users \
  -H "Content-Type: application/json" \
  -d '{"name":"Ada","email":"ada@example.com","role":"admin"}'
curl -i http://127.0.0.1:8080/users/1
curl -i -X DELETE http://127.0.0.1:8080/users/1
```

Restart the server — unlike in-memory storage, users persist in `vix.db`.

## Key database patterns

```cpp
// Simple query
auto rows = db.query("SELECT id, name FROM users ORDER BY id DESC");
while (rows->next()) {
  const auto &row = rows->row();
  row.getInt64(0);
  row.getString(1);
}

// Prepared statement (always use for user input)
vix::db::PooledConn conn(db.pool());
auto stmt = conn->prepare("SELECT * FROM users WHERE id = ?");
stmt->bind(1, id);
auto rows = stmt->query();

// Insert
auto stmt = conn->prepare("INSERT INTO users (name, email) VALUES (?, ?)");
stmt->bind(1, name);
stmt->bind(2, email);
stmt->exec();

// Last insert id (SQLite)
conn->prepare("SELECT last_insert_rowid()")->query()->next();
```

## Add .env configuration

```dotenv
DATABASE_ENGINE=sqlite
DATABASE_SQLITE_PATH=vix.db
SERVER_PORT=8080
SERVER_TLS_ENABLED=false
```

```cpp
vix::config::Config cfg{".env"};
vix::db::Database db{cfg};
app.run(cfg);
```

## Transactions

```cpp
db.transaction([&](vix::db::Connection &conn){
  conn.prepare("INSERT INTO users (name, email, role) VALUES (?, ?, ?)")
    ->bind(1, "Alice")->bind(2, "alice@example.com")->bind(3, "admin")->exec();

  conn.prepare("INSERT INTO users (name, email, role) VALUES (?, ?, ?)")
    ->bind(1, "Bob")->bind(2, "bob@example.com")->bind(3, "user")->exec();
});
```

## SQLite vs MySQL

| | SQLite | MySQL |
|-|--------|-------|
| Setup | Very simple | Requires server |
| Run flag | `--with-sqlite` | `--with-mysql` |
| Best for | Local apps, MVPs | Multi-user production |

## Common mistakes

```bash
vix run main.cpp          # Wrong — SQLite support missing
vix run main.cpp --with-sqlite  # Correct
```

```cpp
// Wrong — SQL injection risk
auto rows = db.query("SELECT * FROM users WHERE id = " + id);

// Correct — prepared statement
auto stmt = conn->prepare("SELECT * FROM users WHERE id = ?");
stmt->bind(1, id);
```

```cpp
// Wrong — exposes internals to client
respond_error(res, 500, "database_error", e.what());

// Correct — safe message, log internally
respond_error(res, 500, "database_error", "Database operation failed");
```

## What you should remember

```cpp
vix::db::Database db = vix::db::Database::sqlite("vix.db");
// Run with: vix run main.cpp --with-sqlite
```

Use prepared statements for user input. Use transactions for multi-step writes. The core idea: **persistence means state survives process restarts.**

Next: [WebSocket](/examples/websocket)
