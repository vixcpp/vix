#include <cstdint>
#include <iostream>
#include <string>

#include <vix/orm/orm.hpp>

int main()
{
  try
  {
    auto db = vix::db::Database::sqlite("orm_uow.db");

    db.exec(
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT NOT NULL, "
        "age INTEGER NOT NULL)");

    auto uow = vix::orm::unit_of_work(db);

    {
      auto st = uow.conn().prepare("INSERT INTO users (name, age) VALUES (?, ?)");
      st->bind(1, std::string("Alice"));
      st->bind(2, static_cast<std::int64_t>(20));
      st->exec();
    }

    {
      auto st = uow.conn().prepare("INSERT INTO users (name, age) VALUES (?, ?)");
      st->bind(1, std::string("Bob"));
      st->bind(2, static_cast<std::int64_t>(30));
      st->exec();
    }

    uow.commit();
    std::cout << "[OK] transaction committed\n";
    return 0;
  }
  catch (const std::exception &e)
  {
    std::cerr << "[ERR] " << e.what() << "\n";
    return 1;
  }
}
