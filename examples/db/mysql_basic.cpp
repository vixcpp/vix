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

    auto &pool = db.pool();

    {
      vix::db::PooledConn conn(pool);

      conn->prepare(
              "CREATE TABLE IF NOT EXISTS users ("
              "id BIGINT AUTO_INCREMENT PRIMARY KEY, "
              "name TEXT)")
          ->exec();

      conn->prepare(
              "INSERT INTO users (name) VALUES (?)")
          ->bind(1, std::string("Gaspard"));

      std::cout << "[OK] inserted user\n";
    }

    return 0;
  }
  catch (const std::exception &e)
  {
    std::cerr << "[ERR] " << e.what() << "\n";
    return 1;
  }
}
