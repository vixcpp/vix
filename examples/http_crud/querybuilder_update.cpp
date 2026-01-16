/**
 *
 *  @file examples/http_crud/querybuilder_update.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.  All rights reserved.
 *  https://github.com/vixcpp/vix
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 *
 */
#include <vix/orm/orm.hpp>
#include <vix/orm/ConnectionPool.hpp>
#include <vix/orm/MySQLDriver.hpp>

#include <iostream>
#include <string>

using namespace vix::orm;

int main(int argc, char **argv)
{
  std::string host = (argc > 1 ? argv[1] : "tcp://127.0.0.1:3306");
  std::string user = (argc > 2 ? argv[2] : "root");
  std::string pass = (argc > 3 ? argv[3] : "");
  std::string db = (argc > 4 ? argv[4] : "vixdb");

  try
  {
    auto factory = make_mysql_factory(host, user, pass, db);

    PoolConfig cfg;
    cfg.min = 1;
    cfg.max = 8;

    ConnectionPool pool{factory, cfg};
    pool.warmup();

    QueryBuilder qb;
    qb.raw("UPDATE users SET age=? WHERE email=?")
        .param(29)
        .param(std::string("gaspardkirira@example.com"));

    PooledConn pc(pool);
    auto st = pc.get().prepare(qb.sql());

    const auto &ps = qb.params();
    for (std::size_t i = 0; i < ps.size(); ++i)
      st->bind(i + 1, ps[i]);

    auto affected = st->exec();
    std::cout << "[OK] affected rows = " << affected << "\n";
    return 0;
  }
  catch (const std::exception &e)
  {
    std::cerr << "[ERR] " << e.what() << "\n";
    return 1;
  }
}
