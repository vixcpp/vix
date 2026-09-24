#include <cstdint>
#include <iostream>
#include <string>

#include <vix/orm/orm.hpp>

int main()
{
  try
  {
    auto db = vix::db::Database::sqlite("orm_qb_select.db");

    db.exec(
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT NOT NULL, "
        "age INTEGER NOT NULL)");

    db.exec("DELETE FROM users");

    db.exec("INSERT INTO users (name, age) VALUES (?, ?)",
            std::string("Alice"),
            static_cast<std::int64_t>(20));

    db.exec("INSERT INTO users (name, age) VALUES (?, ?)",
            std::string("Bob"),
            static_cast<std::int64_t>(16));

    vix::db::PooledConn conn(db.pool());

    vix::orm::QueryBuilder qb;
    qb.raw("SELECT id, name, age FROM users WHERE age >= ?")
        .param(std::int64_t(18));

    auto st = conn->prepare(qb.sql());
    qb.bind(*st);

    auto rs = st->query();
    while (rs && rs->next())
    {
      const auto &row = rs->row();
      std::cout << row.getInt64(0) << " "
                << row.getString(1) << " "
                << row.getInt64(2) << "\n";
    }

    return 0;
  }
  catch (const std::exception &e)
  {
    std::cerr << "[ERR] " << e.what() << "\n";
    return 1;
  }
}
