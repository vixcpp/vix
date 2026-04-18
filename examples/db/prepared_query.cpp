#include <cstdint>
#include <iostream>
#include <vix/db/db.hpp>

int main()
{
  try
  {
    auto db = vix::db::Database::sqlite("vix.db");

    {
      vix::db::PooledConn conn(db.pool());

      conn->prepare(
              "CREATE TABLE IF NOT EXISTS users ("
              "id INTEGER PRIMARY KEY AUTOINCREMENT, "
              "name TEXT NOT NULL, "
              "age INTEGER NOT NULL)")
          ->exec();

      auto insert = conn->prepare(
          "INSERT INTO users (name, age) VALUES (?, ?)");

      insert->bind(1, std::string("Alice"));
      insert->bind(2, static_cast<std::int64_t>(20));
      insert->exec();
    }

    {
      vix::db::PooledConn conn(db.pool());

      auto st = conn->prepare("SELECT id, name FROM users WHERE age > ?");
      st->bind(1, static_cast<std::int64_t>(18));

      auto rs = st->query();

      while (rs->next())
      {
        const auto &row = rs->row();
        std::cout << row.getInt64(0) << " " << row.getString(1) << "\n";
      }
    }

    return 0;
  }
  catch (const std::exception &e)
  {
    std::cerr << "DB error: " << e.what() << "\n";
    return 1;
  }
}
