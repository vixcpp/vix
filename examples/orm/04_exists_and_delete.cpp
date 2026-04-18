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
    auto db = vix::db::Database::sqlite("orm_exists.db");

    db.exec(
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT NOT NULL)");

    db.exec("DELETE FROM users");

    vix::orm::BaseRepository<User> repo(db.pool(), "users");
    const auto id = static_cast<std::int64_t>(repo.create(User{0, "Alice"}));

    std::cout << "[OK] exists before delete=" << (repo.existsById(id) ? "yes" : "no") << "\n";

    repo.removeById(id);

    std::cout << "[OK] exists after delete=" << (repo.existsById(id) ? "yes" : "no") << "\n";

    repo.create(User{0, "Bob"});
    repo.create(User{0, "Charlie"});

    std::cout << "[OK] count before removeAll=" << repo.count() << "\n";

    repo.removeAll();

    std::cout << "[OK] count after removeAll=" << repo.count() << "\n";

    return 0;
  }
  catch (const std::exception &e)
  {
    std::cerr << "[ERR] " << e.what() << "\n";
    return 1;
  }
}
