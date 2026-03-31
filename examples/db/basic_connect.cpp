#include <iostream>
#include <vix/db/db.hpp>

int main()
{
  try
  {
    auto db = vix::db::Database::sqlite("vix.db");

    auto conn = db.pool().acquire();
    if (!conn->ping())
    {
      std::cerr << "DB ping failed\n";
      return 1;
    }

    std::cout << "DB connected successfully\n";
    return 0;
  }
  catch (const std::exception &e)
  {
    std::cerr << "DB error: " << e.what() << "\n";
    return 1;
  }
}
