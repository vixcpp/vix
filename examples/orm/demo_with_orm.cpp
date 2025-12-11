#include <vix/orm/orm.hpp>
#include <iostream>

int main()
{
    using namespace vix::orm;

    try
    {
        ConnectionPool pool{make_mysql_factory("tcp://127.0.0.1:3306",
                                               "root",
                                               "",
                                               "vixdb")};

        UnitOfWork uow{pool};
        auto &conn = uow.conn();

        auto st = conn.prepare("SELECT 1");
        st->exec();

        std::cout << "ORM alive!\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << "ERR: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
