#include <iostream>
#include <vix/db/db.hpp>

int main()
{
  try
  {
    auto db = vix::db::Database::sqlite("vix.db");

    db.exec("CREATE TABLE IF NOT EXISTS healthcheck (id INTEGER PRIMARY KEY)");

    std::cout << "DB connected successfully\n";
    return 0;
  }
  catch (const std::exception &e)
  {
    std::cerr << "DB error: " << e.what() << "\n";
    return 1;
  }
}
