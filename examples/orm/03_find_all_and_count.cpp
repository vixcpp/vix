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
    auto db = vix::db::Database::sqlite("orm_find_all.db");

    {
      auto conn = db.pool().acquire();
      conn->prepare(
              "CREATE TABLE IF NOT EXISTS users ("
              "id INTEGER PRIMARY KEY AUTOINCREMENT, "
              "name TEXT NOT NULL)")
          ->exec();
      conn->prepare("DELETE FROM users")->exec();
    }

    auto repo = vix::orm::repository<User>(db, "users");
    repo.create(User{0, "Alice"});
    repo.create(User{0, "Bob"});
    repo.create(User{0, "Charlie"});

    std::cout << "[OK] count=" << repo.count() << "\n";

    for (const auto &u : repo.findAll())
    {
      std::cout << u.id << " " << u.name << "\n";
    }

    return 0;
  }
  catch (const std::exception &e)
  {
    std::cerr << "[ERR] " << e.what() << "\n";
    return 1;
  }
}
