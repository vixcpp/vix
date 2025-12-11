/**
 * @file tx_unit_of_work.cpp
 * @brief Example — Execute multiple statements atomically with Transaction / UnitOfWork.
 */

#include <vix/orm/orm.hpp>
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
        ConnectionPool pool{make_mysql_factory(host, user, pass, db)};

        UnitOfWork uow{pool};
        auto &c = uow.conn();

        {
            auto st = c.prepare("INSERT INTO users(name,email,age) VALUES(?,?,?)");
            st->bind(1, std::string("Alice"));
            st->bind(2, std::string("alice@example.com"));
            st->bind(3, 27);
            st->exec();
        }

        auto userId = c.lastInsertId();
        {
            auto st = c.prepare("INSERT INTO orders(user_id,total) VALUES(?,?)");
            st->bind(1, static_cast<std::int64_t>(userId));
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
