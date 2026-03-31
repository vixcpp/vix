#include <iostream>
#include <vix/db/Database.hpp>

int main()
{
  try
  {
    // Ultra simple
    auto db = vix::db::Database::sqlite("vix.db");

    auto &pool = db.pool();

    {
      vix::db::PooledConn conn(pool);

      conn->prepare(
              "CREATE TABLE IF NOT EXISTS users ("
              "id INTEGER PRIMARY KEY AUTOINCREMENT, "
              "name TEXT)")
          ->exec();

      conn->prepare(
              "INSERT INTO users (name) VALUES (?)")
          ->bind(1, std::string("Gaspard"));

      conn->prepare(
              "INSERT INTO users (name) VALUES (?)")
          ->bind(1, std::string("Adastra"));

      std::cout << "[OK] inserted users\n";
    }

    {
      vix::db::PooledConn conn(pool);

      auto stmt = conn->prepare("SELECT id, name FROM users");
      auto rs = stmt->query();

      while (rs->next())
      {
        const auto &row = rs->row();
        std::cout << "User: "
                  << row.getInt64(0) << " "
                  << row.getString(1) << "\n";
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
