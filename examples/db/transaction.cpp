#include <cstdint>
#include <iostream>
#include <string>
#include <vix/db/db.hpp>

int main()
{
  try
  {
    auto db = vix::db::Database::sqlite("vix.db");

    vix::db::Transaction tx(db.pool());

    tx.conn().prepare(
                 "CREATE TABLE IF NOT EXISTS users ("
                 "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                 "name TEXT NOT NULL, "
                 "age INTEGER NOT NULL)")
        ->exec();

    {
      auto st = tx.conn().prepare(
          "INSERT INTO users (name, age) VALUES (?, ?)");
      st->bind(1, std::string("Alice"));
      st->bind(2, static_cast<std::int64_t>(20));
      st->exec();
    }

    {
      auto st = tx.conn().prepare(
          "SELECT id, name, age FROM users WHERE age >= ?");
      st->bind(1, static_cast<std::int64_t>(18));

      auto rs = st->query();
      while (rs->next())
      {
        const auto &row = rs->row();
        std::cout
            << row.getInt64(0) << " "
            << row.getString(1) << " "
            << row.getInt64(2) << "\n";
      }
    }

    tx.commit();
    std::cout << "Committed.\n";
    return 0;
  }
  catch (const std::exception &e)
  {
    std::cerr << "DB error: " << e.what() << "\n";
    return 1;
  }
}
