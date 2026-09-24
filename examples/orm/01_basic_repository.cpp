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
    User u{};
    u.id = row.getInt64Or(0, 0);
    u.name = row.getStringOr(1, "");
    return u;
  }

  static vix::orm::FieldValues toInsertFields(const User &u)
  {
    return {
        {"name", u.name},
    };
  }

  static vix::orm::FieldValues toUpdateFields(const User &u)
  {
    return {
        {"name", u.name},
    };
  }
};

int main()
{
  try
  {
    auto db = vix::db::Database::sqlite("orm_basic.db");

    db.exec(
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT NOT NULL)");

    vix::orm::BaseRepository<User> repo(db.pool(), "users");
    const auto id = repo.create(User{0, "Alice"});

    std::cout << "[OK] created id=" << id << "\n";
    return 0;
  }
  catch (const std::exception &e)
  {
    std::cerr << "[ERR] " << e.what() << "\n";
    return 1;
  }
}
