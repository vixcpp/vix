/**
 * @file tx_unit_of_work.cpp
 * @brief Example — Execute multiple statements atomically with Transaction / UnitOfWork.
 *
 * Note: BaseRepository<T> uses ConnectionPool internally; for a single atomic
 * unit, we show direct SQL on uow.conn() so everything shares the same tx.
 */

#include <vix/orm/orm.hpp>
#include <iostream>

using namespace Vix::orm;

int main(int argc, char **argv)
{
    std::string host = (argc > 1 ? argv[1] : "tcp://127.0.0.1:3306");
    std::string user = (argc > 2 ? argv[2] : "root");
    std::string pass = (argc > 3 ? argv[3] : "");
    std::string db = (argc > 4 ? argv[4] : "vixdb");

    try
    {
        ConnectionPool pool{host, user, pass, db};

        // Group two operations in one atomic unit
        UnitOfWork uow{pool};
        auto &c = uow.conn();

        // Insert a user
        {
            auto st = c.prepare("INSERT INTO users(name,email,age) VALUES(?,?,?)");
            st->bind(1, std::string("Alice"));
            st->bind(2, std::string("alice@example.com"));
            st->bind(3, 27);
            st->exec();
        }

        // Insert an order for that user (assume id=LAST_INSERT_ID())
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
