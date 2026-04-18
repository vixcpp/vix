#include <cstdint>
#include <iostream>
#include <string>

#include <vix/orm/orm.hpp>

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

int main()
{
  try
  {
    auto db = vix::db::Database::sqlite("orm_crud.db");

    db.exec(
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT NOT NULL, "
        "email TEXT NOT NULL, "
        "age INTEGER NOT NULL)");

    vix::orm::BaseRepository<User> repo(db.pool(), "users");

    const auto id = static_cast<std::int64_t>(
        repo.create(User{0, "Alice", "alice@example.com", 25}));

    std::cout << "[OK] create id=" << id << "\n";

    if (auto u = repo.findById(id))
    {
      std::cout << "[OK] find " << u->name << " " << u->email << "\n";
    }

    repo.updateById(id, User{id, "Alice Updated", "alice.updated@example.com", 26});
    std::cout << "[OK] update id=" << id << "\n";

    repo.removeById(id);
    std::cout << "[OK] delete id=" << id << "\n";

    return 0;
  }
  catch (const std::exception &e)
  {
    std::cerr << "[ERR] " << e.what() << "\n";
    return 1;
  }
}
