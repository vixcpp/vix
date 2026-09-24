/**
 *
 *  @file http_db_env_showcase.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira.
 *  All rights reserved.
 *  https://github.com/vixcpp/vix
 *
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 *
 *  ----------------------------------------------------------------------------
 *  GOAL
 *  ----
 *  A clean HTTP + DB showcase for Vix.
 *
 *  It demonstrates:
 *    - loading database configuration from .env
 *    - bootstrapping a SQLite/MySQL database with Config::Config cfg(".env")
 *    - simple HTTP routes
 *    - DB reads with db.query(...)
 *    - DB writes with db.exec(...)
 *    - transaction usage with db.transaction(...)
 *    - path params
 *    - JSON body reading
 *    - clear route organization
 *
 *  QUICK START
 *  -----------
 *    vix run examples/http/06_http_db_env_showcase.cpp
 *
 *  QUICK TESTS
 *  -----------
 *    curl -i http://127.0.0.1:8080/
 *    curl -i http://127.0.0.1:8080/health
 *    curl -i http://127.0.0.1:8080/users
 *    curl -i http://127.0.0.1:8080/users/1
 *    curl -i -X POST http://127.0.0.1:8080/users \
 *      -H "Content-Type: application/json" \
 *      -d '{"name":"Ada","email":"ada@example.com","age":22}'
 *
 */

#include <vix.hpp>
#include <vix/config/Config.hpp>
#include <vix/db/db.hpp>

#include <cstdint>
#include <exception>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

using namespace vix;
namespace J = vix::json;

// -----------------------------------------------------------------------------
// Small helpers
// -----------------------------------------------------------------------------

static int to_int_or(std::string_view s, int fallback)
{
  if (s.empty())
    return fallback;

  int sign = 1;
  std::size_t i = 0;

  if (s[0] == '-')
  {
    sign = -1;
    i = 1;
  }

  if (i >= s.size())
    return fallback;

  long long value = 0;
  for (; i < s.size(); ++i)
  {
    const char c = s[i];
    if (c < '0' || c > '9')
      return fallback;
    value = (value * 10) + (c - '0');
    if (value > 2000000000LL)
      return fallback;
  }

  value *= sign;
  if (value < -2147483648LL || value > 2147483647LL)
    return fallback;

  return static_cast<int>(value);
}

static J::kvs ok_message(std::string_view message)
{
  return J::obj({
      "ok",
      true,
      "message",
      std::string(message),
  });
}

static J::kvs error_message(std::string_view message, int status = 400)
{
  return J::obj({
      "ok",
      false,
      "status",
      static_cast<long long>(status),
      "error",
      std::string(message),
  });
}

// -----------------------------------------------------------------------------
// App state
// -----------------------------------------------------------------------------

struct AppState
{
  std::shared_ptr<vix::db::Database> db;
};

// -----------------------------------------------------------------------------
// Database bootstrap
// -----------------------------------------------------------------------------

static void bootstrap_database(vix::db::Database &db)
{
  db.exec("DROP TABLE IF EXISTS users");

  db.exec(
      "CREATE TABLE users ("
      "id INTEGER PRIMARY KEY AUTOINCREMENT, "
      "name TEXT NOT NULL, "
      "email TEXT NOT NULL UNIQUE, "
      "age INTEGER NOT NULL)");

  db.transaction([&](vix::db::Connection &conn)
                 {
    auto st1 = conn.prepare(
        "INSERT INTO users (name, email, age) VALUES (?, ?, ?)");
    st1->bind(1, "Gaspard");
    st1->bind(2, "gaspard@vix.dev");
    st1->bind(3, static_cast<std::int64_t>(24));
    st1->exec();

    auto st2 = conn.prepare(
        "INSERT INTO users (name, email, age) VALUES (?, ?, ?)");
    st2->bind(1, "Ada");
    st2->bind(2, "ada@vix.dev");
    st2->bind(3, static_cast<std::int64_t>(22));
    st2->exec(); });
}

static std::shared_ptr<vix::db::Database> create_database(const vix::config::Config &cfg)
{
  auto db = std::make_shared<vix::db::Database>(cfg);

  bootstrap_database(*db);
  return db;
}
// -----------------------------------------------------------------------------
// Route registration
// -----------------------------------------------------------------------------

static void register_public_routes(
    App &app,
    const AppState &state,
    const vix::config::Config &cfg)
{
  app.get("/", [state, &cfg](Request &, Response &res)
          { res.json(J::obj({
                "message",
                "Vix HTTP + DB + .env showcase",
                "database_engine",
                cfg.getString("database.engine", "sqlite"),
                "hint",
                "Try /health, /users, /users/1, POST /users",
            })); });

  app.get("/health", [state](Request &, Response &res)
          {
            try
            {
              auto rows = state.db->query("SELECT COUNT(*) FROM users");
              std::int64_t total = 0;

              if (rows->next())
              {
                total = rows->row().getInt64(0);
              }

              res.json(J::obj({
                  "ok", true,
                  "service", "vix",
                  "database", true,
                  "users_count", static_cast<long long>(total),
              }));
            }
            catch (const std::exception &e)
            {
              res.status(500).json(J::obj({
                  "ok", false,
                  "database", false,
                  "error", e.what(),
              }));
            } });
}

static void register_user_routes(App &app, const AppState &state)
{
  app.get("/users", [state](Request &, Response &res)
          {
            auto rows = state.db->query(
                "SELECT id, name, email, age FROM users ORDER BY id ASC");

            std::vector<J::token> items;

            while (rows->next())
            {
              const auto &row = rows->row();

              items.emplace_back(J::obj({
                  "id", row.getInt64(0),
                  "name", row.getString(1),
                  "email", row.getString(2),
                  "age", row.getInt64(3),
              }));
            }

            res.json(J::obj({
                "ok", true,
                "count", static_cast<long long>(items.size()),
                "data", J::array(std::move(items)),
            })); });

  app.get("/users/{id}", [state](Request &req, Response &res)
          {
            const int id = to_int_or(req.param("id", "0"), 0);

            if (id <= 0)
            {
              res.status(400).json(error_message("invalid user id", 400));
              return;
            }

            auto rows = state.db->query(
                "SELECT id, name, email, age FROM users WHERE id = ?",
                id);

            if (!rows->next())
            {
              res.status(404).json(error_message("user not found", 404));
              return;
            }

            const auto &row = rows->row();

            res.json(J::obj({
                "ok", true,
                "data", J::obj({
                    "id", row.getInt64(0),
                    "name", row.getString(1),
                    "email", row.getString(2),
                    "age", row.getInt64(3),
                }),
            })); });

  app.post("/users", [state](Request &req, Response &res)
           {
             const auto &j = req.json();

             if (!j.is_object())
             {
               res.status(400).json(error_message("expected JSON object body", 400));
               return;
             }

             if (!j.contains("name") || !j["name"].is_string())
             {
               res.status(400).json(error_message("field 'name' is required", 400));
               return;
             }

             if (!j.contains("email") || !j["email"].is_string())
             {
               res.status(400).json(error_message("field 'email' is required", 400));
               return;
             }

             if (!j.contains("age") || !j["age"].is_number_integer())
             {
               res.status(400).json(error_message("field 'age' is required", 400));
               return;
             }

             const std::string name = j["name"].get<std::string>();
             const std::string email = j["email"].get<std::string>();
             const std::int64_t age = j["age"].get<std::int64_t>();

             if (name.empty())
             {
               res.status(400).json(error_message("field 'name' cannot be empty", 400));
               return;
             }

             if (email.empty())
             {
               res.status(400).json(error_message("field 'email' cannot be empty", 400));
               return;
             }

             if (age < 0)
             {
               res.status(400).json(error_message("field 'age' must be >= 0", 400));
               return;
             }

             try
             {
               state.db->exec(
                   "INSERT INTO users (name, email, age) VALUES (?, ?, ?)",
                   name,
                   email,
                   age);

               auto rows = state.db->query(
                   "SELECT id, name, email, age "
                   "FROM users WHERE email = ? "
                   "ORDER BY id DESC LIMIT 1",
                   email);

               if (!rows->next())
               {
                 res.status(201).json(ok_message("user created"));
                 return;
               }

               const auto &row = rows->row();

               res.status(201).json(J::obj({
                   "ok", true,
                   "message", "user created",
                   "data", J::obj({
                       "id", row.getInt64(0),
                       "name", row.getString(1),
                       "email", row.getString(2),
                       "age", row.getInt64(3),
                   }),
               }));
             }
             catch (const std::exception &e)
             {
               res.status(400).json(J::obj({
                   "ok", false,
                   "error", "failed to create user",
                   "details", e.what(),
               }));
             } });
}

static void register_debug_routes(App &app, const AppState &state)
{
  app.get("/_routes", [](Request &, Response &res)
          {
            std::vector<J::token> routes;

            auto push = [&](std::string_view method,
                            std::string_view path,
                            std::string_view note)
            {
              routes.emplace_back(J::obj({
                  "method", std::string(method),
                  "path", std::string(path),
                  "note", std::string(note),
              }));
            };

            push("GET", "/", "home route with db engine info from .env");
            push("GET", "/health", "database health + users count");
            push("GET", "/users", "list all users");
            push("GET", "/users/{id}", "get one user by id");
            push("POST", "/users", "create a user from JSON body");

            res.json(J::obj({
                "ok", true,
                "count", static_cast<long long>(routes.size()),
                "routes", J::array(std::move(routes)),
            })); });
}

static void register_all_routes(
    App &app,
    const AppState &state,
    const vix::config::Config &cfg)
{
  register_public_routes(app, state, cfg);
  register_user_routes(app, state);
  register_debug_routes(app, state);
}

// -----------------------------------------------------------------------------
// Bootstrap
// -----------------------------------------------------------------------------

static int run_server()
{
  vix::config::Config cfg{".env"};

  App app;

  const AppState state{
      create_database(cfg),
  };

  register_all_routes(app, state, cfg);
  app.run(cfg.getServerPort());
  return 0;
}

int main()
{
  return run_server();
}
