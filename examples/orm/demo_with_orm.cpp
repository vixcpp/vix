#include <vix/orm/orm.hpp>
#include <iostream>

int main()
{
    using namespace vix::orm;

    try
    {
        ConnectionPool pool{"tcp://127.0.0.1:3306", "root", "", "vixdb"};
        auto &conn = UnitOfWork{pool}.conn(); // simple ping
        auto st = conn.prepare("SELECT 1");   // exec() OK même sans ResultSet adapter
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
