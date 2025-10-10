/**
 * @file migrate_init.cpp
 * @brief Example — Run schema migrations with MigrationsRunner.
 *
 * Build:
 *   cmake -S .. -B build -DVIX_ORM_BUILD_EXAMPLES=ON -DVIX_ORM_USE_MYSQL=ON
 *   cmake --build build -j && ./build/vix_orm_migrate_init
 */

#include <vix/orm/orm.hpp>
#include <iostream>

using namespace Vix::orm;

class CreateUsers : public Migration
{
public:
    std::string id() const override { return "2025_10_10_000001_create_users"; }
    void up(Connection &c) override
    {
        auto st = c.prepare(
            "CREATE TABLE IF NOT EXISTS users ("
            "  id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,"
            "  name  VARCHAR(120) NOT NULL,"
            "  email VARCHAR(190) NOT NULL,"
            "  age   INT NOT NULL,"
            "  PRIMARY KEY (id),"
            "  UNIQUE KEY uq_users_email(email)"
            ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;");
        st->exec();
    }
    void down(Connection &c) override
    {
        auto st = c.prepare("DROP TABLE IF EXISTS users");
        st->exec();
    }
};

class CreateOrders : public Migration
{
public:
    std::string id() const override { return "2025_10_10_000002_create_orders"; }
    void up(Connection &c) override
    {
        auto st = c.prepare(
            "CREATE TABLE IF NOT EXISTS orders ("
            "  id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,"
            "  user_id BIGINT UNSIGNED NOT NULL,"
            "  total DECIMAL(12,2) NOT NULL,"
            "  PRIMARY KEY (id),"
            "  INDEX ix_orders_user_id(user_id),"
            "  CONSTRAINT fk_orders_user FOREIGN KEY (user_id) REFERENCES users(id)"
            ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;");
        st->exec();
    }
    void down(Connection &c) override
    {
        auto st = c.prepare("DROP TABLE IF EXISTS orders");
        st->exec();
    }
};

int main(int argc, char **argv)
{
    std::string host = (argc > 1 ? argv[1] : "tcp://127.0.0.1:3306");
    std::string user = (argc > 2 ? argv[2] : "root");
    std::string pass = (argc > 3 ? argv[3] : "");
    std::string db = (argc > 4 ? argv[4] : "vixdb");

    try
    {
        // Direct connection (no pool needed just to migrate)
        auto raw = make_mysql_conn(host, user, pass, db);
        MySQLConnection conn{raw};

        MigrationsRunner runner{conn};
        CreateUsers m1;
        CreateOrders m2;
        runner.add(&m1);
        runner.add(&m2);
        runner.runAll();

        std::cout << "[OK] migrations applied\n";
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "[ERR] " << e.what() << "\n";
        return 1;
    }
}
