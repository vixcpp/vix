#include <filesystem>
#include <iostream>
#include <string>

#include <vix/db/db.hpp>

class CreateUsersTable final : public vix::db::Migration
{
public:
  std::string id() const override
  {
    return "2026-01-22-create-users";
  }

  void up(vix::db::Connection &c) override
  {
    auto st = c.prepare(
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT NOT NULL, "
        "age INTEGER NOT NULL"
        ");");
    st->exec();
  }

  void down(vix::db::Connection &c) override
  {
    auto st = c.prepare("DROP TABLE IF EXISTS users;");
    st->exec();
  }
};

static void runCodeMigrations(vix::db::Database &db)
{
  std::cout << "[migrations] running code migrations...\n";

  db.transaction([&](vix::db::Connection &conn)
                 {
    CreateUsersTable migration;

    vix::db::MigrationsRunner runner(conn);
    runner.add(&migration);
    runner.runAll(); });

  std::cout << "[migrations] done (code)\n";
}

static void runFileMigrations(vix::db::Database &db,
                              std::filesystem::path dir)
{
  std::cout << "[migrations] running file migrations from: "
            << dir.string() << "\n";

  db.transaction([&](vix::db::Connection &conn)
                 {
    vix::db::FileMigrationsRunner runner(conn, std::move(dir));
    runner.setTable("schema_migrations");
    runner.applyAll(); });

  std::cout << "[migrations] done (files)\n";
}

int main()
{
  try
  {
    auto db = vix::db::Database::sqlite("vix.db");

    runCodeMigrations(db);
    runFileMigrations(db, std::filesystem::path{"migrations"});

    std::cout << "OK\n";
    return 0;
  }
  catch (const std::exception &e)
  {
    std::cerr << "ERROR: " << e.what() << "\n";
    return 1;
  }
}
