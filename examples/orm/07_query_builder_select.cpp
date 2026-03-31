#include <cstdint>
#include <iostream>
#include <string>

#include <vix/orm/orm.hpp>

int main()
{
  try
  {
    auto db = vix::db::Database::sqlite("orm_qb_select.db");

    {
      auto conn = db.pool().acquire();
      conn->prepare(
              "CREATE TABLE IF NOT EXISTS users ("
              "id INTEGER PRIMARY KEY AUTOINCREMENT, "
              "name TEXT NOT NULL, "
              "age INTEGER NOT NULL)")
          ->exec();
      conn->prepare("DELETE FROM users")->exec();

      auto st1 = conn->prepare("INSERT INTO users (name, age) VALUES (?, ?)");
      st1->bind(1, std::string("Alice"));
      st1->bind(2, static_cast<std::int64_t>(20));
      st1->exec();

      auto st2 = conn->prepare("INSERT INTO users (name, age) VALUES (?, ?)");
      st2->bind(1, std::string("Bob"));
      st2->bind(2, static_cast<std::int64_t>(16));
      st2->exec();
    }

    auto conn = db.pool().acquire();

    vix::orm::QueryBuilder qb;
    qb.raw("SELECT id, name, age FROM users WHERE age >= ?")
        .param(std::int64_t(18));

    auto st = conn->prepare(qb.sql());
    qb.bind(*st);

    auto rs = st->query();
    while (rs->next())
    {
      const auto &row = rs->row();
      std::cout << row.getInt64(0) << " "
                << row.getString(1) << " "
                << row.getInt64(2) << "\n";
    }

    return 0;
  }
  catch (const std::exception &e)
  {
    std::cerr << "[ERR] " << e.what() << "\n";
    return 1;
  }
}
