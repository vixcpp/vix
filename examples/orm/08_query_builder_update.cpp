#include <cstdint>
#include <iostream>
#include <string>

#include <vix/orm/orm.hpp>

int main()
{
  try
  {
    auto db = vix::db::Database::sqlite("orm_qb_update.db");

    {
      auto conn = db.pool().acquire();
      conn->prepare(
              "CREATE TABLE IF NOT EXISTS users ("
              "id INTEGER PRIMARY KEY AUTOINCREMENT, "
              "name TEXT NOT NULL, "
              "age INTEGER NOT NULL)")
          ->exec();
      conn->prepare("DELETE FROM users")->exec();

      auto insert = conn->prepare("INSERT INTO users (name, age) VALUES (?, ?)");
      insert->bind(1, std::string("Alice"));
      insert->bind(2, static_cast<std::int64_t>(20));
      insert->exec();
    }

    {
      auto conn = db.pool().acquire();

      vix::orm::QueryBuilder qb;
      qb.raw("UPDATE users SET age = ? WHERE name = ?")
          .param(std::int64_t(42))
          .param("Alice");

      auto st = conn->prepare(qb.sql());
      qb.bind(*st);
      st->exec();
    }

    {
      auto conn = db.pool().acquire();
      auto st = conn->prepare("SELECT name, age FROM users");
      auto rs = st->query();

      while (rs->next())
      {
        const auto &row = rs->row();
        std::cout << row.getString(0) << " " << row.getInt64(1) << "\n";
      }
    }

    return 0;
  }
  catch (const std::exception &e)
  {
    std::cerr << "[ERR] " << e.what() << "\n";
    return 1;
  }
}
