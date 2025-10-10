/**
 * @file error_handling.cpp
 * @brief Example — Minimal error handling with DBError.
 */

#include <vix/orm/orm.hpp>
#include <iostream>

using namespace Vix::orm;

int main()
{
    try
    {
        auto raw = make_mysql_conn("tcp://127.0.0.1:3306", "root", "", "db_does_not_exist");
        MySQLConnection c{raw};
        auto st = c.prepare("SELECT 1");
        st->exec();
        std::cout << "[INFO] This message may not be reached if connection fails.\n";
    }
    catch (const DBError &e)
    {
        std::cerr << "[DBError] " << e.what() << "\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << "[std::exception] " << e.what() << "\n";
    }
    return 0;
}
