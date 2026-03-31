
# Users CRUD over HTTP (ORM + Repository)

This page shows a minimal HTTP CRUD API for a `users` table using:

-   Vix.cpp HTTP server (`App`)
-   Vix ORM (`ConnectionPool`, `BaseRepository<T>`, `Mapper<T>`)
-   JSON request and JSON responses
-   One simple auth layer: API key on `/api/`

Rules of this doc: - one concept - one minimal main() - quick curl tests

## What you get

Routes (all under `/api`):

-   POST `/api/users` create user
-   GET `/api/users/:id` read user
-   PUT `/api/users/:id` update user
-   DELETE `/api/users/:id` delete user

Auth: - Requires header `x-api-key: dev_key_123` for all `/api/*`

## Database schema

Create the table first:

``` sql
CREATE TABLE IF NOT EXISTS users (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  name VARCHAR(255) NOT NULL,
  email VARCHAR(255) NOT NULL,
  age INT NOT NULL
);
```

Recommended in production:

``` sql
CREATE UNIQUE INDEX users_email_uq ON users(email);
```

## Single file example: users_crud_http.cpp

Save as `users_crud_http.cpp`:

``` cpp
#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>
#include <vix/orm/orm.hpp>

#include <vix/json/Simple.hpp>
#include <vix/json/convert.hpp>
#include <vix/json/json.hpp>

#include <any>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace vix;
using namespace vix::orm;
namespace J = vix::json;

// Model
struct User
{
  std::int64_t id{};
  std::string name;
  std::string email;
  int age{};
};

// Mapper<User>
namespace vix::orm
{
  template <>
  struct Mapper<User>
  {
    static User fromRow(const ResultRow &row)
    {
      User u{};
      u.id = row.getInt64Or(0, 0);
      u.name = row.getStringOr(1, "");
      u.email = row.getStringOr(2, "");
      u.age = static_cast<int>(row.getInt64Or(3, 0));
      return u;
    }

    static std::vector<std::pair<std::string, std::any>> toInsertParams(const User &u)
    {
      return { {"name", u.name}, {"email", u.email}, {"age", u.age}};
    }

    static std::vector<std::pair<std::string, std::any>> toUpdateParams(const User &u)
    {
      return { {"name", u.name}, {"email", u.email}, {"age", u.age}};
    }
  };
} // namespace vix::orm

// Simple JSON helpers (Simple.hpp)
static J::kvs user_to_kvs(const User &u)
{
  using J::obj;
  return obj({
      "id",
      u.id,
      "name",
      u.name,
      "email",
      u.email,
      "age",
      u.age,
  });
}

static void json_send(Response &res, const J::kvs &payload)
{
  // convert Simple -> nlohmann::json
  res.json(J::to_json(payload));
}

static void json_send(Response &res, int status, const J::kvs &payload)
{
  res.status(status);
  // convert Simple -> nlohmann::json
  res.json(J::to_json(payload));
}

// Parse helpers (nlohmann::json)
static std::optional<std::string> json_string(const J::Json &body, const char *k)
{
  if (!body.is_object() || !body.contains(k) || !body[k].is_string())
    return std::nullopt;
  return body[k].get<std::string>();
}

static std::optional<std::int64_t> json_i64(const J::Json &body, const char *k)
{
  if (!body.is_object() || !body.contains(k))
    return std::nullopt;

  if (body[k].is_number_integer())
    return static_cast<std::int64_t>(body[k].get<long long>());

  return std::nullopt;
}

static void register_routes(App &app, ConnectionPool &pool)
{
  auto repo = std::make_shared<BaseRepository<User>>(pool, "users");

  app.get("/", [](Request &, Response &res)
          { res.send(
                "Users CRUD HTTP example:\n"
                "  POST   /api/users\n"
                "  GET    /api/users/{id}\n"
                "  PUT    /api/users/{id}\n"
                "  DELETE /api/users/{id}\n"
                "All /api requires: x-api-key: dev_key_123\n"); });

  // Create
  app.post("/api/users", [repo](Request &req, Response &res)
           {
    const auto body = req.json();

    const auto name  = json_string(body, "name");
    const auto email = json_string(body, "email");
    const auto age64 = json_i64(body, "age");

    if (!name || !email || !age64)
    {
      json_send(res, 400, J::obj({
        "ok", false,
        "error", "bad_request",
        "hint", "expected JSON: {name:string, email:string, age:int}"
      }));
      return;
    }

    User u{};
    u.name = *name;
    u.email = *email;
    u.age = static_cast<int>(*age64);

    u.id = static_cast<std::int64_t>(repo->create(u));

    json_send(res, 201, J::obj({
      "ok", true,
      "data", user_to_kvs(u)
    })); });

  // Read
  app.get("/api/users/{id}", [repo](Request &req, Response &res)
          {
    const auto id = std::stoll(req.param("id"));

    auto u = repo->findById(static_cast<std::int64_t>(id));
    if (!u)
    {
      json_send(res, 404, J::obj({
        "ok", false,
        "error", "not_found"
      }));
      return;
    }

    json_send(res, J::obj({
      "ok", true,
      "data", user_to_kvs(*u)
    })); });

  // Update
  app.put("/api/users/{id}", [repo](Request &req, Response &res)
          {
    const auto id = std::stoll(req.param("id"));
    const auto body = req.json();

    const auto name  = json_string(body, "name");
    const auto email = json_string(body, "email");
    const auto age64 = json_i64(body, "age");

    if (!name || !email || !age64)
    {
      json_send(res, 400, J::obj({
        "ok", false,
        "error", "bad_request",
        "hint", "expected JSON: {name:string, email:string, age:int}"
      }));
      return;
    }

    User u{};
    u.id = static_cast<std::int64_t>(id);
    u.name = *name;
    u.email = *email;
    u.age = static_cast<int>(*age64);

    const auto affected = repo->updateById(u.id, u);
    if (affected == 0)
    {
      json_send(res, 404, J::obj({
        "ok", false,
        "error", "not_found"
      }));
      return;
    }

    json_send(res, J::obj({
      "ok", true,
      "affected", static_cast<long long>(affected),
      "data", user_to_kvs(u)
    })); });

  // Delete
  app.del("/api/users/{id}", [repo](Request &req, Response &res)
          {
    const auto id = std::stoll(req.param("id"));

    const auto affected = repo->removeById(static_cast<std::int64_t>(id));
    if (affected == 0)
    {
      json_send(res, 404, J::obj({
        "ok", false,
        "error", "not_found"
      }));
      return;
    }

    json_send(res, J::obj({
      "ok", true,
      "affected", static_cast<long long>(affected)
    })); });
}

int main()
{
  try
  {
    // DB
    auto factory = make_mysql_factory("tcp://127.0.0.1:3306", "root", "", "vixdb");

    PoolConfig pcfg;
    pcfg.min = 1;
    pcfg.max = 8;

    ConnectionPool pool{factory, pcfg};
    pool.warmup();

    // HTTP
    App app;

    middleware::app::install(app, "/api/", middleware::app::api_key_dev("dev_key_123"));

    register_routes(app, pool);

    app.run(8080);
    return 0;
  }
  catch (const DBError &e)
  {
    std::cerr << "[DBError] " << e.what() << "\n";
    return 1;
  }
  catch (const std::exception &e)
  {
    std::cerr << "[ERR] " << e.what() << "\n";
    return 1;
  }
}
```

## Run

``` bash
vix run users_crud_http.cpp
```

## Try with curl

Set the key once:

``` bash
K="x-api-key: dev_key_123"
```

Create:

``` bash
curl -i -H "$K" -H "Content-Type: application/json" \
  -d '{"name":"Bob","email":"bob@example.com","age":30}' \
  http://127.0.0.1:8080/api/users
```

Read (replace 1):

``` bash
curl -i -H "$K" http://127.0.0.1:8080/api/users/1
```

Update (replace 1):

``` bash
curl -i -X PUT -H "$K" -H "Content-Type: application/json" \
  -d '{"name":"Bob Updated","email":"bob@example.com","age":31}' \
  http://127.0.0.1:8080/api/users/1
```

Delete (replace 1):

``` bash
curl -i -X DELETE -H "$K" http://127.0.0.1:8080/api/users/1
```

Missing key (should fail):

``` bash
curl -i http://127.0.0.1:8080/api/users/1
```

## What this teaches

-   A clean ORM mapping (`Mapper<User>`)
-   A stable data layer (`BaseRepository<User>`)
-   A simple pool for scalability (`ConnectionPool`)
-   Minimal HTTP CRUD with predictable JSON
-   Prefix middleware install for API protection
