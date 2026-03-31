#include <iostream>

#include <vix/orm/orm.hpp>

int main()
{
  try
  {
    auto db = vix::db::Database::sqlite("orm_errors.db");
    auto conn = db.pool().acquire();

    auto st = conn->prepare("SELECT * FROM table_that_does_not_exist");
    auto rs = st->query();

    while (rs->next())
    {
      (void)rs->row();
    }

    std::cout << "Unexpected success\n";
    return 0;
  }
  catch (const vix::db::DBError &e)
  {
    std::cerr << "[DBError] " << e.what() << "\n";
    return 1;
  }
  catch (const std::exception &e)
  {
    std::cerr << "[ERR] " << e.what() << "\n";
    return 1;
  }
}
