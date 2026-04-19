#include <iostream>
#include <string>

#include <vix/orm/orm.hpp>

class CreateUsersTable final : public vix::db::Migration
{
public:
  std::string id() const override
  {
    return "2026-03-28-create-users";
  }

  void up(vix::db::Connection &c) override
  {
    c.prepare(
         "CREATE TABLE IF NOT EXISTS users ("
         "id INTEGER PRIMARY KEY AUTOINCREMENT, "
         "name TEXT NOT NULL)")
        ->exec();
  }

  void down(vix::db::Connection &c) override
  {
    c.prepare("DROP TABLE IF EXISTS users")->exec();
  }
};

int main()
{
  try
  {
    auto db = vix::db::Database::sqlite("orm_migrations_code.db");

    vix::orm::UnitOfWork uow(db);

    CreateUsersTable migration;
    vix::db::MigrationsRunner runner(uow.conn());

    runner.add(&migration);
    runner.runAll();

    uow.commit();

    std::cout << "[OK] code migrations applied\n";
    return 0;
  }
  catch (const std::exception &e)
  {
    std::cerr << "[ERR] " << e.what() << "\n";
    return 1;
  }
}
