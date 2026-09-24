#include <iostream>
#include <vix/db/Database.hpp>

int main()
{
  try
  {
    auto db = vix::db::Database::mysql(
        "tcp://127.0.0.1:3306",
        "root",
        "",
        "vixdb");

    db.exec(
        "CREATE TABLE IF NOT EXISTS users ("
        "id BIGINT PRIMARY KEY AUTO_INCREMENT, "
        "name VARCHAR(255) NOT NULL)");

    db.exec("INSERT INTO users (name) VALUES (?)", "Gaspard");

    std::cout << "[OK] inserted user\n";
    return 0;
  }
  catch (const std::exception &e)
  {
    std::cerr << "[ERR] " << e.what() << "\n";
    return 1;
  }
}
