#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include <vix/db/Database.hpp>

void worker(vix::db::ConnectionPool &pool, int id)
{
  vix::db::PooledConn conn(pool);

  auto stmt = conn->prepare("INSERT INTO users (name) VALUES (?)");
  stmt->bind(1, std::string("worker_" + std::to_string(id)));
  stmt->exec();

  std::cout << "[thread] inserted " << id << "\n";
}

int main()
{
  try
  {
    auto db = vix::db::Database::sqlite("vix.db");

    auto &pool = db.pool();

    {
      vix::db::PooledConn conn(pool);

      conn->prepare(
              "CREATE TABLE IF NOT EXISTS users ("
              "id INTEGER PRIMARY KEY AUTOINCREMENT, "
              "name TEXT NOT NULL)")
          ->exec();
    }

    std::vector<std::thread> threads;

    for (int i = 0; i < 4; ++i)
    {
      threads.emplace_back(worker, std::ref(pool), i);
    }

    for (auto &t : threads)
    {
      t.join();
    }

    std::cout << "[OK] done\n";
    return 0;
  }
  catch (const std::exception &e)
  {
    std::cerr << "[ERR] " << e.what() << "\n";
    return 1;
  }
}
