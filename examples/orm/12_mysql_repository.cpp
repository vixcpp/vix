#include <cstdint>
#include <iostream>
#include <string>

#include <vix/orm/orm.hpp>

struct User
{
  std::int64_t id{};
  std::string name;
};

template <>
struct vix::orm::Mapper<User>
{
  static User fromRow(const vix::db::ResultRow &row)
  {
    return User{
        row.getInt64Or(0, 0),
        row.getStringOr(1, ""),
    };
  }

  static vix::orm::FieldValues toInsertFields(const User &u)
  {
    return {{"name", u.name}};
  }

  static vix::orm::FieldValues toUpdateFields(const User &u)
  {
    return {{"name", u.name}};
  }
};

int main()
{
  try
  {
    auto db = vix::db::Database::mysql(
        "tcp://127.0.0.1:3306",
        "root",
        "",
        "vixdb");

    {
      auto conn = db.pool().acquire();
      conn->prepare(
              "CREATE TABLE IF NOT EXISTS users ("
              "id BIGINT AUTO_INCREMENT PRIMARY KEY, "
              "name TEXT NOT NULL)")
          ->exec();
    }

    auto repo = vix::orm::repository<User>(db, "users");
    const auto id = repo.create(User{0, "Hello MySQL ORM"});

    std::cout << "[OK] created id=" << id << "\n";
    return 0;
  }
  catch (const std::exception &e)
  {
    std::cerr << "[ERR] " << e.what() << "\n";
    return 1;
  }
}
