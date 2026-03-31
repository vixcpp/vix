#include <cstdint>
#include <iostream>
#include <string>

#include <vix/orm/orm.hpp>

struct User : vix::orm::Entity
{
  std::int64_t userId{};
  std::string name;

  std::int64_t id() const noexcept override
  {
    return userId;
  }

  void setId(std::int64_t value) noexcept override
  {
    userId = value;
  }
};

template <>
struct vix::orm::Mapper<User>
{
  static User fromRow(const vix::db::ResultRow &row)
  {
    User u{};
    u.setId(row.getInt64Or(0, 0));
    u.name = row.getStringOr(1, "");
    return u;
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
    auto db = vix::db::Database::sqlite("orm_entity.db");

    {
      auto conn = db.pool().acquire();
      conn->prepare(
              "CREATE TABLE IF NOT EXISTS users ("
              "id INTEGER PRIMARY KEY AUTOINCREMENT, "
              "name TEXT NOT NULL)")
          ->exec();
    }

    auto repo = vix::orm::repository<User>(db, "users");

    User alice{};
    alice.setId(0);
    alice.name = "Alice";

    auto id = static_cast<std::int64_t>(repo.create(alice));

    if (auto u = repo.findById(id))
    {
      std::cout << "[OK] entity id=" << u->id() << " name=" << u->name << "\n";
    }

    return 0;
  }
  catch (const std::exception &e)
  {
    std::cerr << "[ERR] " << e.what() << "\n";
    return 1;
  }
}
