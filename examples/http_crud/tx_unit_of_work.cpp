/**
 *
 *  @file tx_unit_of_work.hpp
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

#include <cstdint>
#include <iostream>
#include <string>

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

    UnitOfWork uow{pool};
    auto &c = uow.conn();

    {
      auto st = c.prepare("INSERT INTO users(name,email,age) VALUES(?,?,?)");
      st->bind(1, "Alice");
      st->bind(2, "alice@example.com");
      st->bind(3, 27);
      st->exec();
    }

    const std::uint64_t userId = c.lastInsertId();

    {
      auto st = c.prepare("INSERT INTO orders(user_id,total) VALUES(?,?)");
      st->bind(1, userId);
      st->bind(2, 199.99);
      st->exec();
    }

    uow.commit();
    std::cout << "[OK] user+order committed. user_id=" << userId << "\n";
    return 0;
  }
  catch (const std::exception &e)
  {
    std::cerr << "[ERR] " << e.what() << "\n";
    return 1;
  }
}
