/**
 * @file users_crud.cpp
 * @brief Example — Basic CRUD operations with Vix ORM.
 *
 * This example demonstrates how to use the Vix ORM core classes
 * (`ConnectionPool`, `BaseRepository`, and `Mapper<T>`) to perform
 * a simple `INSERT` on a `users` table.
 *
 * ───────────────────────────────────────────────────────────────────────────
 *  Requirements
 * ───────────────────────────────────────────────────────────────────────────
 *  - MySQL server accessible on localhost or custom TCP host.
 *  - A table `users` with columns:
 *
 *    ```sql
 *    CREATE TABLE users (
 *      id BIGINT AUTO_INCREMENT PRIMARY KEY,
 *      name VARCHAR(255) NOT NULL,
 *      email VARCHAR(255) NOT NULL,
 *      age INT NOT NULL
 *    );
 *    ```
 *
 *  - Compiled with:
 *    ```bash
 *    cmake -S .. -B build -DVIX_ORM_BUILD_EXAMPLES=ON -DVIX_ORM_USE_MYSQL=ON
 *    cmake --build build -j
 *    ./build/vix_orm_users tcp://127.0.0.1:3306 root password vixdb
 *    ```
 *
 * ───────────────────────────────────────────────────────────────────────────
 *  Example flow
 * ───────────────────────────────────────────────────────────────────────────
 *  1. Define a C++ struct `User` representing the table.
 *  2. Provide a `Mapper<User>` specialization describing how to
 *     convert a `User` to SQL insert/update parameters.
 *  3. Use a `ConnectionPool` to manage MySQL connections.
 *  4. Create a `BaseRepository<User>` and call `create()`.
 *
 * ───────────────────────────────────────────────────────────────────────────
 *  Output example
 * ───────────────────────────────────────────────────────────────────────────
 *  ```
 *  [OK] Insert user → id=42
 *  ```
 */

#include <vix/orm/orm.hpp>
#include <iostream>
#include <string>

// ============================================================================
// Entity definition
// ----------------------------------------------------------------------------
// Represents a single row in the `users` table.
// ============================================================================
struct User
{
    std::int64_t id{};
    std::string name;
    std::string email;
    int age{};
};

// ============================================================================
// Mapper specialization for User
// ----------------------------------------------------------------------------
// Defines how to convert between `User` objects and SQL parameters.
// Note: reading (fromRow) is omitted here since ResultSet adapter is pending.
// ============================================================================
namespace Vix::orm
{
    template <>
    struct Mapper<User>
    {
        static User fromRow(const ResultRow &)
        {
            // Not implemented yet (ResultSet adapter pending)
            return {};
        }

        static std::vector<std::pair<std::string, std::any>> toInsertParams(const User &u)
        {
            return {
                {"name", u.name},
                {"email", u.email},
                {"age", u.age}};
        }

        static std::vector<std::pair<std::string, std::any>> toUpdateParams(const User &u)
        {
            return {
                {"name", u.name},
                {"email", u.email},
                {"age", u.age}};
        }
    };
} // namespace Vix::orm

// ============================================================================
// Entry point
// ----------------------------------------------------------------------------
// Demonstrates connecting, creating a repository, and inserting a record.
// ============================================================================
int main(int argc, char **argv)
{
    using namespace Vix::orm;

    // ------------------------------------------------------------------------
    // 1. Retrieve DB credentials (defaults or CLI arguments)
    // ------------------------------------------------------------------------
    std::string host = (argc > 1 ? argv[1] : "tcp://127.0.0.1:3306");
    std::string user = (argc > 2 ? argv[2] : "root");
    std::string pass = (argc > 3 ? argv[3] : "");
    std::string db = (argc > 4 ? argv[4] : "vixdb");

    try
    {
        // --------------------------------------------------------------------
        // 2. Initialize a connection pool
        // --------------------------------------------------------------------
        ConnectionPool pool{host, user, pass, db};

        // --------------------------------------------------------------------
        // 3. Create repository for `users`
        // --------------------------------------------------------------------
        BaseRepository<User> users{pool, "users"};

        // --------------------------------------------------------------------
        // 4. Insert a user (CREATE)
        // --------------------------------------------------------------------
        std::uint64_t id = users.create(User{
            0,
            "Gaspard",
            "gaspardkirira@outlook.com",
            28});

        // --------------------------------------------------------------------
        // 5. Display success result
        // --------------------------------------------------------------------
        std::cout << "[OK] Insert user → id=" << id << "\n";
        return 0;
    }
    catch (const std::exception &e)
    {
        // --------------------------------------------------------------------
        // 6. Error handling (DB connection or execution failure)
        // --------------------------------------------------------------------
        std::cerr << "[ERR] " << e.what() << "\n";
        return 1;
    }
}
