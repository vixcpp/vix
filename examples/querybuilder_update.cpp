/**
 * @file querybuilder_update.cpp
 * @brief Example — Build an UPDATE query with QueryBuilder and execute it.
 *
 * NOTE: ResultSet is not implemented yet, so we demonstrate exec().
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

        // Build: UPDATE users SET age=? WHERE email=?
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
