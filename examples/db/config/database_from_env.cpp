#include <iostream>
#include <vix/config/Config.hpp>
#include <vix/db/Database.hpp>

int main()
{
  try
  {
    vix::config::Config cfg{".env"};

    std::cout << "database.engine = "
              << cfg.getString("database.engine", "sqlite") << "\n";

    std::cout << "database.default.host = "
              << cfg.getString("database.default.host", "127.0.0.1") << "\n";

    std::cout << "database.default.port = "
              << cfg.getInt("database.default.port", 3306) << "\n";

    std::cout << "database.default.user = "
              << cfg.getString("database.default.user", "root") << "\n";

    std::cout << "database.default.name = "
              << cfg.getString("database.default.name", "") << "\n";

    vix::db::Database db{cfg};

    db.exec(
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT NOT NULL)");

    db.exec("INSERT INTO users (name) VALUES (?)", "Gaspard");

    auto rows = db.query("SELECT id, name FROM users");

    while (rows->next())
    {
      const auto &row = rows->row();
      std::cout << row.getInt64(0) << " - " << row.getString(1) << "\n";
    }

    return 0;
  }
  catch (const std::exception &e)
  {
    std::cerr << "Config/DB error: " << e.what() << "\n";
    return 1;
  }
}
