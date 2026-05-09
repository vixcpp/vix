# Production App

Structure a complete Vix application ready for local development and deployment.

```txt
configuration → routes → services → repositories → database → HTTP responses
```

## What you will build

```txt
GET  /             → app info
GET  /health       → health check
GET  /api/users    → list users
GET  /api/users/{id} → get one user
POST /api/users    → create user
```

With: `.env`, SQLite, config, route functions, repository layer, service layer, consistent JSON responses.

## Create a workspace

```bash
mkdir -p ~/tmp/vix-examples/production-app
cd ~/tmp/vix-examples/production-app
touch main.cpp .env
```

## .env

```dotenv
SERVER_PORT=8080
SERVER_TLS_ENABLED=false
DATABASE_ENGINE=sqlite
DATABASE_SQLITE_PATH=vix.db
LOGGING_ASYNC=true
LOGGING_QUEUE_MAX=20000
LOGGING_DROP_ON_OVERFLOW=true
WAF_MODE=basic
WAF_MAX_TARGET_LEN=4096
WAF_MAX_BODY_BYTES=1048576
```

## Full code

```cpp
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <vector>
#include <vix.hpp>
#include <vix/db.hpp>
using namespace vix;

struct User { std::int64_t id{}; std::string name; std::string email; std::string role; };

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

static json::Json user_to_json(const User &u)
{
  return json::obj({
    {"id", u.id},
    {"name", u.name},
    {"email", u.email},
    {"role", u.role
  }});
}

class UserRepository
{
public:
  explicit UserRepository(vix::db::Database &db) : db_(db) {}

  void initialize()
  {
    db_.exec("CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY AUTOINCREMENT, "
             "name TEXT NOT NULL, email TEXT NOT NULL UNIQUE, role TEXT NOT NULL DEFAULT 'user')");
  }

  std::vector<User> find_all()
  {
    std::vector<User> users;
    auto rows = db_.query("SELECT id, name, email, role FROM users ORDER BY id DESC");

    while (rows->next())
    {
      const auto &r = rows->row();
      users.push_back({r.getInt64(0), r.getString(1), r.getString(2), r.getString(3)});
    }
    return users;
  }

  std::optional<User> find_by_id(std::int64_t id)
  {
    vix::db::PooledConn conn(db_.pool());
    auto stmt = conn->prepare("SELECT id, name, email, role FROM users WHERE id = ?");
    stmt->bind(1, id);
    auto rows = stmt->query();

    if (!rows->next())
      return std::nullopt;

    const auto &r = rows->row();
    return User{r.getInt64(0), r.getString(1), r.getString(2), r.getString(3)};
  }

  std::int64_t create(const std::string &name, const std::string &email, const std::string &role)
  {
    vix::db::PooledConn conn(db_.pool());
    auto stmt = conn->prepare("INSERT INTO users (name, email, role) VALUES (?, ?, ?)");
    stmt->bind(1, name); stmt->bind(2, email); stmt->bind(3, role.empty() ? "user" : role);
    stmt->exec();
    auto rows = conn->prepare("SELECT last_insert_rowid()")->query();

    if (!rows->next())
      throw std::runtime_error("failed to read inserted user id");

    return rows->row().getInt64(0);
  }

private:
  vix::db::Database &db_;
};

class UserService
{
public:
  explicit UserService(UserRepository &users) : users_(users) {}

  json::Json list_users()
  {
    json::Json items = json::Json::array();
    for (const auto &u : users_.find_all())
      items.push_back(user_to_json(u));

    return items;
  }

  std::optional<json::Json> get_user(std::int64_t id)
  {
    const auto user = users_.find_by_id(id);
    if (!user)
      return std::nullopt;

    return user_to_json(*user);
  }

  std::int64_t create_user(const std::string &name, const std::string &email, const std::string &role)
  {
    if (name.empty())
      throw std::invalid_argument("name is required");

    if (email.empty())
      throw std::invalid_argument("email is required");

    return users_.create(name, email, role.empty() ? "user" : role);
  }

private:
  UserRepository &users_;
};

struct AppContext
{
  config::Config config;
  db::Database database;
  UserRepository user_repository;
  UserService user_service;

  explicit AppContext(const std::string &env_file)
      : config(env_file), database(config),
        user_repository(database), user_service(user_repository) {}
};

static void configure_middlewares(App &app)
{
  app.use([](Request &req, Response &, App::Next next){
    std::cout << "[request] " << req.method() << " " << req.path() << "\n";
    next();
  });

  app.use([](Request &, Response &res, App::Next next){
    res.header("X-Powered-By", "Vix.cpp"); res.header("X-Content-Type-Options", "nosniff");
    next();
  });
}

static void register_system_routes(App &app)
{
  app.get("/", [](Request &, Response &res){
    res.json({
      "ok", true,
      "name", "production-app",
      "framework", "Vix.cpp"
    });
  });

  app.get("/health", [](Request &, Response &res){
    res.json({
      "ok", true,
      "service", "production-app"
    });
  });
}

static void register_user_routes(App &app, UserService &users)
{
  app.get("/api/users", [&users](Request &, Response &res){
    res.json({
      "ok", true,
      "data", users.list_users()
    });
  });

  app.get("/api/users/{id}", [&users](Request &req, Response &res){
    const auto id = parse_id(req.param("id"));
    if (!id) {
      respond_error(res, 400, "invalid_id", "Invalid user id");
      return;
    }

    const auto user = users.get_user(*id);
    if (!user) {
      respond_error(res, 404, "user_not_found", "User not found");
      return;
    }

    res.json({
      "ok", true,
      "data", *user
    });
  });

  app.post("/api/users", [&users](Request &req, Response &res){
    const auto &body = req.json();
    if (!body.is_object()) {
      respond_error(res, 400, "invalid_body", "Expected JSON object");
      return;
    }

    const std::string name = body.value("name", "");
    const std::string email = body.value("email", "");
    const std::string role = body.value("role", "user");

    try{
      const auto id = users.create_user(name, email, role);
      const auto created = users.get_user(id);

      res.status(201).json({
        "ok", true,
        "message", "user created",
        "data", created ? *created : json::Json{}
      });

    }catch (const std::invalid_argument &e) {
      respond_error(res, 400, "validation_failed", e.what());
    }catch (const std::exception &) {
      respond_error(res, 409, "create_failed", "Could not create user");
    }

  });
}

int main()
{
  try
  {
    AppContext ctx{".env"};
    ctx.user_repository.initialize();

    App app;

    configure_middlewares(app);
    register_system_routes(app);
    register_user_routes(app, ctx.user_service);

    app.run(ctx.config);

    return 0;
  }
  catch (const std::exception &e)
  {
    std::cerr << "fatal error: " << e.what() << "\n";
    return 1;
  }
}
```

## Run and test

```bash
vix run main.cpp --with-sqlite
```

```bash
curl -i http://127.0.0.1:8080/
curl -i http://127.0.0.1:8080/health
curl -i http://127.0.0.1:8080/api/users
curl -i -X POST http://127.0.0.1:8080/api/users \
  -H "Content-Type: application/json" \
  -d '{"name":"Ada","email":"ada@example.com","role":"admin"}'
curl -i http://127.0.0.1:8080/api/users/1
```

## Architecture

```txt
Config → Database → Repository → Service → Routes
```

| Layer | Responsibility |
|-------|---------------|
| `routes/` | HTTP route registration |
| `services/` | Business logic |
| `repositories/` | Database access |
| `models/` | Data structures |
| `support/` | Shared helpers |
| `app/` | App wiring and dependencies |

## Real project structure

```bash
vix new api
cd api
```

```txt
api/
├── .env
├── CMakeLists.txt
├── vix.json
├── src/
│   ├── main.cpp
│   ├── routes/
│   ├── services/
│   ├── repositories/
│   └── models/
└── tests/
```

```bash
vix dev                          # development
vix build --with-sqlite          # build
vix tests                        # test
vix build --preset release --with-sqlite  # release
```

## systemd service

```ini
[Unit]
Description=Vix Production App
After=network.target

[Service]
Type=simple
WorkingDirectory=/opt/production-app
ExecStart=/opt/production-app/bin/production-app
Restart=always
RestartSec=3
Environment=SERVER_PORT=8080
Environment=SERVER_TLS_ENABLED=false
Environment=DATABASE_ENGINE=sqlite
Environment=DATABASE_SQLITE_PATH=/opt/production-app/data/vix.db

[Install]
WantedBy=multi-user.target
```

## Nginx reverse proxy

```nginx
server {
    listen 80;
    server_name example.com;
    location / {
        proxy_pass http://127.0.0.1:8080;
        proxy_http_version 1.1;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
    }
}
```

## Deployment checklist

- [ ] Release build works
- [ ] `.env` production values correct
- [ ] Database path exists and is writable
- [ ] systemd service starts and auto-restarts
- [ ] Nginx proxies to correct port
- [ ] `/health` returns ok
- [ ] Logs visible with `journalctl`
- [ ] Admin routes protected
- [ ] Secrets not committed to Git

## Common mistakes

```cpp
// Wrong — SQL in routes
app.post("/users", [&db](Request &req, Response &res) {
  db.exec("INSERT INTO users...");
  // + business logic
  // + JSON building
});

// Correct — thin routes
app.post("/users", [&service](Request &req, Response &res) {
  auto id = service.create_user(name, email, role);
  res.status(201).json({...});
});
```

```bash
vix run main.cpp --with-sqlite   # correct
vix run main.cpp                 # missing SQLite support
```

## What you should remember

```txt
Request → middleware → route → service → repository → database → JSON response
```

The core idea: **a production app is not only code that works — it is code that can be configured, monitored, deployed, and maintained.**

Back to [Examples](/examples/index)
