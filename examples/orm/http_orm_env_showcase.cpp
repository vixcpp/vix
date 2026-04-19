/**
 *
 *  @file http_orm_env_showcase.cpp
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
 *  A clean HTTP + ORM showcase for Vix.
 *
 *  It demonstrates:
 *    - loading database configuration from .env
 *    - bootstrapping a SQLite/MySQL database with Config::Config cfg(".env")
 *    - explicit ORM mapping with Mapper<T>
 *    - repository-based reads and writes
 *    - custom repository queries
 *    - simple HTTP routes
 *    - path params
 *    - JSON body reading
 *    - clear route organization
 *
 *  QUICK START
 *  -----------
 *    vix run examples/http/07_http_orm_env_showcase.cpp
 *
 *  QUICK TESTS
 *  -----------
 *    curl -i http://127.0.0.1:8080/
 *    curl -i http://127.0.0.1:8080/health
 *    curl -i http://127.0.0.1:8080/users
 *    curl -i http://127.0.0.1:8080/users/1
 *    curl -i http://127.0.0.1:8080/users/email/ada@vix.dev
 *    curl -i -X POST http://127.0.0.1:8080/users \
 *      -H "Content-Type: application/json" \
 *      -d '{"name":"Grace","email":"grace@vix.dev","age":29}'
 *    curl -i -X PUT http://127.0.0.1:8080/users/1 \
 *      -H "Content-Type: application/json" \
 *      -d '{"name":"Gaspard Updated","email":"gaspard.updated@vix.dev","age":25}'
 *    curl -i -X DELETE http://127.0.0.1:8080/users/1
 *
 */

#include <vix.hpp>
#include <vix/config/Config.hpp>
#include <vix/db/db.hpp>
#include <vix/orm/orm.hpp>

#include <cstdint>
#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

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
// ORM model
// -----------------------------------------------------------------------------

struct User
{
  std::int64_t id{};
  std::string name;
  std::string email;
  int age{};
};

template <>
struct vix::orm::Mapper<User>
{
  static User fromRow(const vix::db::ResultRow &row)
  {
    User u{};
    u.id = row.getInt64Or(0, 0);
    u.name = row.getStringOr(1, "");
    u.email = row.getStringOr(2, "");
    u.age = static_cast<int>(row.getInt64Or(3, 0));
    return u;
  }

  static vix::orm::FieldValues toInsertFields(const User &u)
  {
    return {
        {"name", u.name},
        {"email", u.email},
        {"age", u.age},
    };
  }

  static vix::orm::FieldValues toUpdateFields(const User &u)
  {
    return {
        {"name", u.name},
        {"email", u.email},
        {"age", u.age},
    };
  }
};

// -----------------------------------------------------------------------------
// Custom repository
// -----------------------------------------------------------------------------

class UserRepository : public vix::orm::BaseRepository<User>
{
public:
  using vix::orm::BaseRepository<User>::BaseRepository;

  std::optional<User> findByEmail(const std::string &email)
  {
    vix::db::PooledConn conn(this->pool());
    auto st = conn->prepare(
        "SELECT id, name, email, age "
        "FROM users WHERE email = ? LIMIT 1");

    st->bind(1, email);

    auto rs = st->query();
    if (!rs || !rs->next())
    {
      return std::nullopt;
    }

    return vix::orm::Mapper<User>::fromRow(rs->row());
  }
};

// -----------------------------------------------------------------------------
// App state
// -----------------------------------------------------------------------------

struct AppState
{
  std::shared_ptr<vix::db::Database> db;
  std::shared_ptr<UserRepository> users;
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

  db.exec(
      "INSERT INTO users (name, email, age) VALUES (?, ?, ?)",
      std::string("Gaspard"),
      std::string("gaspard@vix.dev"),
      static_cast<std::int64_t>(24));

  db.exec(
      "INSERT INTO users (name, email, age) VALUES (?, ?, ?)",
      std::string("Ada"),
      std::string("ada@vix.dev"),
      static_cast<std::int64_t>(22));
}

static AppState create_app_state(const vix::config::Config &cfg)
{
  auto db = std::make_shared<vix::db::Database>(cfg);

  bootstrap_database(*db);

  auto users = std::make_shared<UserRepository>(db->pool(), "users");

  return AppState{
      db,
      users,
  };
}

// -----------------------------------------------------------------------------
// JSON helpers
// -----------------------------------------------------------------------------

static J::kvs user_to_json(const User &u)
{
  return J::obj({
      "id",
      static_cast<long long>(u.id),
      "name",
      u.name,
      "email",
      u.email,
      "age",
      static_cast<long long>(u.age),
  });
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
                "Vix HTTP + ORM + .env showcase",
                "database_engine",
                cfg.getString("database.engine", "sqlite"),
                "hint",
                "Try /health, /users, /users/1, /users/email/{email}, POST /users",
            })); });

  app.get("/health", [state](Request &, Response &res)
          {
            try
            {
              res.json(J::obj({
                  "ok", true,
                  "service", "vix",
                  "database", true,
                  "users_count", static_cast<long long>(state.users->count()),
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
            const auto users = state.users->findAll();

            std::vector<J::token> items;
            items.reserve(users.size());

            for (const auto &u : users)
            {
              items.emplace_back(user_to_json(u));
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

            auto user = state.users->findById(id);
            if (!user)
            {
              res.status(404).json(error_message("user not found", 404));
              return;
            }

            res.json(J::obj({
                "ok", true,
                "data", user_to_json(*user),
            })); });

  app.get("/users/email/{email}", [state](Request &req, Response &res)
          {
            const std::string email = req.param("email", "");

            if (email.empty())
            {
              res.status(400).json(error_message("email is required", 400));
              return;
            }

            auto user = state.users->findByEmail(email);
            if (!user)
            {
              res.status(404).json(error_message("user not found", 404));
              return;
            }

            res.json(J::obj({
                "ok", true,
                "data", user_to_json(*user),
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

             User user{};
             user.name = j["name"].get<std::string>();
             user.email = j["email"].get<std::string>();
             user.age = static_cast<int>(j["age"].get<std::int64_t>());

             if (user.name.empty())
             {
               res.status(400).json(error_message("field 'name' cannot be empty", 400));
               return;
             }

             if (user.email.empty())
             {
               res.status(400).json(error_message("field 'email' cannot be empty", 400));
               return;
             }

             if (user.age < 0)
             {
               res.status(400).json(error_message("field 'age' must be >= 0", 400));
               return;
             }

             try
             {
               const auto id = state.users->create(user);
               auto created = state.users->findById(static_cast<std::int64_t>(id));

               if (!created)
               {
                 res.status(201).json(ok_message("user created"));
                 return;
               }

               res.status(201).json(J::obj({
                   "ok", true,
                   "message", "user created",
                   "data", user_to_json(*created),
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

  app.put("/users/{id}", [state](Request &req, Response &res)
          {
            const int id = to_int_or(req.param("id", "0"), 0);

            if (id <= 0)
            {
              res.status(400).json(error_message("invalid user id", 400));
              return;
            }

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

            if (!state.users->existsById(id))
            {
              res.status(404).json(error_message("user not found", 404));
              return;
            }

            User user{};
            user.id = id;
            user.name = j["name"].get<std::string>();
            user.email = j["email"].get<std::string>();
            user.age = static_cast<int>(j["age"].get<std::int64_t>());

            if (user.name.empty())
            {
              res.status(400).json(error_message("field 'name' cannot be empty", 400));
              return;
            }

            if (user.email.empty())
            {
              res.status(400).json(error_message("field 'email' cannot be empty", 400));
              return;
            }

            if (user.age < 0)
            {
              res.status(400).json(error_message("field 'age' must be >= 0", 400));
              return;
            }

            try
            {
              const auto changed = state.users->updateById(id, user);

              if (changed == 0)
              {
                res.status(404).json(error_message("user not updated", 404));
                return;
              }

              auto updated = state.users->findById(id);

              res.json(J::obj({
                  "ok", true,
                  "message", "user updated",
                  "data", updated ? user_to_json(*updated) : user_to_json(user),
              }));
            }
            catch (const std::exception &e)
            {
              res.status(400).json(J::obj({
                  "ok", false,
                  "error", "failed to update user",
                  "details", e.what(),
              }));
            } });

  app.del("/users/{id}", [state](Request &req, Response &res)
          {
               const int id = to_int_or(req.param("id", "0"), 0);

               if (id <= 0)
               {
                 res.status(400).json(error_message("invalid user id", 400));
                 return;
               }

               if (!state.users->existsById(id))
               {
                 res.status(404).json(error_message("user not found", 404));
                 return;
               }

               const auto removed = state.users->removeById(id);

               if (removed == 0)
               {
                 res.status(404).json(error_message("user not deleted", 404));
                 return;
               }

               res.json(J::obj({
                   "ok", true,
                   "message", "user deleted",
                   "id", static_cast<long long>(id),
               })); });
}

static void register_debug_routes(App &app, const AppState &)
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
            push("GET", "/health", "database health + ORM repository count");
            push("GET", "/users", "list all users via repository");
            push("GET", "/users/{id}", "get one user by id via repository");
            push("GET", "/users/email/{email}", "custom repository lookup by email");
            push("POST", "/users", "create a user via repository");
            push("PUT", "/users/{id}", "update a user via repository");
            push("DELETE", "/users/{id}", "delete a user via repository");

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
  const AppState state = create_app_state(cfg);

  register_all_routes(app, state, cfg);
  app.run(cfg.getServerPort());
  return 0;
}

int main()
{
  return run_server();
}
