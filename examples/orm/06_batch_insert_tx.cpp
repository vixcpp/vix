#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include <vix/orm/orm.hpp>

int main()
{
  try
  {
    auto db = vix::db::Database::sqlite("orm_batch.db");

    db.exec(
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT NOT NULL, "
        "age INTEGER NOT NULL)");

    const std::vector<std::pair<std::string, std::int64_t>> users = {
        {"Alice", 20},
        {"Bob", 25},
        {"Charlie", 30},
        {"Diane", 35},
    };

    auto uow = vix::orm::UnitOfWork(db);

    for (const auto &[name, age] : users)
    {
      auto st = uow.conn().prepare("INSERT INTO users (name, age) VALUES (?, ?)");
      st->bind(1, name);
      st->bind(2, age);
      st->exec();
    }

    uow.commit();
    std::cout << "[OK] inserted " << users.size() << " users\n";
    return 0;
  }
  catch (const std::exception &e)
  {
    std::cerr << "[ERR] " << e.what() << "\n";
    return 1;
  }
}
