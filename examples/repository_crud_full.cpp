/**
 * @file repository_crud_full.cpp
 * @brief Example — Full CRUD except SELECT (pending ResultSet).
 */

#include <vix/orm/orm.hpp>
#include <iostream>
#include <cstdint>

struct User
{
    std::int64_t id{};
    std::string name;
    std::string email;
    int age{};
};

namespace Vix::orm
{
    template <>
    struct Mapper<User>
    {
        static User fromRow(const ResultRow &) { return {}; } // pending
        static std::vector<std::pair<std::string, std::any>> toInsertParams(const User &u)
        {
            return {{"name", u.name}, {"email", u.email}, {"age", u.age}};
        }
        static std::vector<std::pair<std::string, std::any>> toUpdateParams(const User &u)
        {
            return {{"name", u.name}, {"email", u.email}, {"age", u.age}};
        }
    };
} // namespace Vix::orm

int main(int argc, char **argv)
{
    using namespace Vix::orm;

    std::string host = (argc > 1 ? argv[1] : "tcp://127.0.0.1:3306");
    std::string user = (argc > 2 ? argv[2] : "root");
    std::string pass = (argc > 3 ? argv[3] : "");
    std::string db = (argc > 4 ? argv[4] : "vixdb");

    try
    {
        ConnectionPool pool{host, user, pass, db};
        BaseRepository<User> repo{pool, "users"};

        // Create
        std::int64_t id = static_cast<std::int64_t>(repo.create(User{0, "Bob", "gaspardkirira@example.com", 30}));
        std::cout << "[OK] create → id=" << id << "\n";

        // Update
        repo.updateById(id, User{id, "Adastra", "adastra@example.com", 31});
        std::cout << "[OK] update → id=" << id << "\n";

        // Delete
        repo.removeById(id);
        std::cout << "[OK] delete → id=" << id << "\n";

        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "[ERR] " << e.what() << "\n";
        return 1;
    }
}
