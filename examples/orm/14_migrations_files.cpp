#include <filesystem>
#include <iostream>

#include <vix/orm/orm.hpp>

int main()
{
  try
  {
    auto db = vix::db::Database::sqlite("orm_migrations_files.db");
    auto uow = vix::orm::unit_of_work(db);

    vix::db::FileMigrationsRunner runner(
        uow.conn(),
        std::filesystem::path{"examples/migrations"});

    runner.setTable("schema_migrations");
    runner.applyAll();

    uow.commit();
    std::cout << "[OK] file migrations applied\n";
    return 0;
  }
  catch (const std::exception &e)
  {
    std::cerr << "[ERR] " << e.what() << "\n";
    return 1;
  }
}
