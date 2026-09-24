#include <iostream>

#include <vix/orm/orm.hpp>

int main()
{
  try
  {
    auto db = vix::db::Database::sqlite("orm_errors.db");

    auto rs = db.query("SELECT * FROM table_that_does_not_exist");

    while (rs && rs->next())
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
