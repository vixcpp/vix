#include <cstdint>
#include <iostream>
#include <string>

#include <vix/orm/orm.hpp>

int main()
{
  try
  {
    auto db = vix::db::Database::sqlite("orm_qb_update.db");

    db.exec(
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT NOT NULL, "
        "age INTEGER NOT NULL)");

    db.exec("DELETE FROM users");

    db.exec("INSERT INTO users (name, age) VALUES (?, ?)",
            std::string("Alice"),
            static_cast<std::int64_t>(20));

    {
      vix::db::PooledConn conn(db.pool());

      vix::orm::QueryBuilder qb;
      qb.raw("UPDATE users SET age = ? WHERE name = ?")
          .param(std::int64_t(42))
          .param("Alice");

      auto st = conn->prepare(qb.sql());
      qb.bind(*st);
      st->exec();
    }

    {
      auto rs = db.query("SELECT name, age FROM users");

      while (rs && rs->next())
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
