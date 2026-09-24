#include <filesystem>
#include <iostream>

#include <vix/orm/orm.hpp>

int main()
{
  try
  {
    auto db = vix::db::Database::sqlite("orm_migrations_files.db");

    vix::db::PooledConn conn(db.pool());

    vix::db::FileMigrationsRunner runner(
        *conn,
        std::filesystem::path{"migrations"});

    runner.setTable("schema_migrations");
    runner.applyAll();

    std::cout << "[OK] file migrations applied\n";
    return 0;
  }
  catch (const std::exception &e)
  {
    std::cerr << "[ERR] " << e.what() << "\n";
    return 1;
  }
}
