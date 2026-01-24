/**
 *
 *  @file error_handling.hpp
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

using namespace vix::orm;

int main(int argc, char **argv)
{
  (void)argc;
  (void)argv;

  try
  {
    // Intentionally wrong DB name to show error handling
    const std::string host = "tcp://127.0.0.1:3306";
    const std::string user = "root";
    const std::string pass = "";
    const std::string db = "db_does_not_exist";

    auto factory = make_mysql_factory(host, user, pass, db);

    PoolConfig cfg;
    cfg.min = 1;
    cfg.max = 8;

    ConnectionPool pool{factory, cfg};

    // will throw if factory returns invalid connection (recommended after our warmup fix),
    // or later when first query fails.
    pool.warmup();

    UnitOfWork uow{pool};
    auto &con = uow.conn();

    auto st = con.prepare("SELECT 1");
    (void)st->exec();

    std::cout << "[INFO] This message may not be reached if connection fails.\n";
    return 0;
  }
  catch (const DBError &e)
  {
    std::cerr << "[DBError] " << e.what() << "\n";
    return 1;
  }
  catch (const std::exception &e)
  {
    std::cerr << "[std::exception] " << e.what() << "\n";
    return 1;
  }
}
