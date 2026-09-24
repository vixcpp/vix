#include <cstdint>
#include <iostream>
#include <optional>
#include <string>

#include <vix/orm/orm.hpp>

struct User
{
  std::int64_t id{};
  std::string name;
  std::string email;
};

template <>
struct vix::orm::Mapper<User>
{
  static User fromRow(const vix::db::ResultRow &row)
  {
    return User{
        row.getInt64Or(0, 0),
        row.getStringOr(1, ""),
        row.getStringOr(2, ""),
    };
  }

  static vix::orm::FieldValues toInsertFields(const User &u)
  {
    return {
        {"name", u.name},
        {"email", u.email},
    };
  }

  static vix::orm::FieldValues toUpdateFields(const User &u)
  {
    return {
        {"name", u.name},
        {"email", u.email},
    };
  }
};

class UserRepository : public vix::orm::BaseRepository<User>
{
public:
  using vix::orm::BaseRepository<User>::BaseRepository;

  std::optional<User> findByEmail(const std::string &email)
  {
    vix::db::PooledConn conn(this->pool());
    auto st = conn->prepare("SELECT id, name, email FROM users WHERE email = ? LIMIT 1");
    st->bind(1, email);

    auto rs = st->query();
    if (!rs || !rs->next())
    {
      return std::nullopt;
    }

    return vix::orm::Mapper<User>::fromRow(rs->row());
  }
};

int main()
{
  try
  {
    auto db = vix::db::Database::sqlite("orm_custom_repo.db");

    db.exec(
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT NOT NULL, "
        "email TEXT NOT NULL)");

    UserRepository repo(db.pool(), "users");
    repo.create(User{0, "Alice", "alice@example.com"});

    if (auto u = repo.findByEmail("alice@example.com"))
    {
      std::cout << "[OK] found by email: " << u->name << "\n";
    }

    return 0;
  }
  catch (const std::exception &e)
  {
    std::cerr << "[ERR] " << e.what() << "\n";
    return 1;
  }
}
