/**
 *
 *  @file users_crud.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.  All rights reserved.
 *  https://github.com/vixcpp/vix
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 */

#include <vix/orm/orm.hpp>

#include <iostream>
#include <string>
#include <vector>

using namespace vix::orm;

int main(int argc, char **argv)
{
  const std::string host = (argc > 1 ? argv[1] : "tcp://127.0.0.1:3306");
  const std::string user = (argc > 2 ? argv[2] : "root");
  const std::string pass = (argc > 3 ? argv[3] : "");
  const std::string db = (argc > 4 ? argv[4] : "vixdb");

  try
  {
    auto factory = make_mysql_factory(host, user, pass, db);

    PoolConfig cfg;
    cfg.min = 1;
    cfg.max = 8;

    ConnectionPool pool{factory, cfg};
    pool.warmup();

    Transaction tx(pool);
    auto &c = tx.conn();

    auto st = c.prepare("INSERT INTO users(name,email,age) VALUES(?,?,?)");

    struct Row
    {
      const char *name;
      const char *email;
      int age;
    };

    const std::vector<Row> rows = {
        {"Gaspard", "gaspardkirira@outlook.com", 23},
        {"Mina", "mina@example.com", 31},
        {"Omar", "omar@example.com", 35},
    };

    std::uint64_t total = 0;
    for (const auto &r : rows)
    {
      st->bind(1, r.name);
      st->bind(2, r.email);
      st->bind(3, r.age);
      total += st->exec();
    }

    tx.commit();
    std::cout << "[OK] inserted rows = " << total << "\n";
    return 0;
  }
  catch (const std::exception &e)
  {
    std::cerr << "[ERR] " << e.what() << "\n";
    return 1;
  }
}
