# 🧩 Vix ORM — Modern C++ Object-Relational Mapper

**Vix ORM** is the official database abstraction module for the **Vix.cpp Framework** — a high-performance C++ backend ecosystem inspired by FastAPI, Vue.js, and modern data engineering practices.

It provides a clean, type-safe, and connection-pooled interface to interact with **MySQL** and **SQLite**, designed with:

- Modern C++20 features
- Connection pooling
- Transaction management
- Query builders
- Repository & Mapper pattern
- Extensible driver interface for multiple databases

---

## 🚀 Features

| Category                  | Description                                                   |
| ------------------------- | ------------------------------------------------------------- |
| **Drivers**               | MySQL (via MySQL Connector/C++) and SQLite (optional)         |
| **Connection Pool**       | Thread-safe, min/max configurable pool with automatic release |
| **Transactions**          | RAII-based commit/rollback handling                           |
| **Query Builder**         | Fluent SQL construction with type-safe parameter binding      |
| **Repository Pattern**    | Generic data repositories for CRUD operations                 |
| **Migration System**      | Lightweight migration runner for schema evolution             |
| **Header-Only Utilities** | `Mapper<T>`, `QueryBuilder`, `Transaction`, and helpers       |
| **Integration-Ready**     | Used internally by `Vix::core` and `Vix::orm` modules         |

---

## 🏗️ Project Structure

```swift
vixcpp/orm/
├── CMakeLists.txt
├── include/vix/orm/
│   ├── ConnectionPool.hpp
│   ├── Drivers.hpp
│   ├── Entity.hpp
│   ├── Errors.hpp
│   ├── Mapper.hpp
│   ├── Migration.hpp
│   ├── MigrationsRunner.hpp
│   ├── MySQLDriver.hpp
│   ├── orm.hpp
│   ├── QueryBuilder.hpp
│   ├── Repository.hpp
│   ├── Transaction.hpp
│   ├── UnitOfWork.hpp
├── src/
│   ├── ConnectionPool.cpp
│   ├── MigrationsRunner.cpp
│   ├── MySQLDriver.cpp
│   ├── QueryBuilder.cpp
│   ├── Repository.cpp
│   ├── Transaction.cpp
├── examples/
│   └── users_crud.cpp
└── cmake/
    ├── VixOrmConfig.cmake.in
    └── MySQLCppConnAlias.cmake
```

---

## ⚙️ Build Instructions

### 1️⃣ Prerequisites

#### On Ubuntu/Debian:

```bash
sudo apt update
sudo apt install -y cmake g++ pkg-config libspdlog-dev libsqlite3-dev libmysqlcppconn-dev
```

#### Optional (manual MySQL Connector)

If not installed via APT:

```bash
export MYSQLCPPCONN_ROOT=/opt/mysql-connector-cpp
```

### 2️⃣ Configure and Build

```bash
mkdir -p build && cd build
cmake -S .. -B . \
  -DCMAKE_BUILD_TYPE=Release \
  -DVIX_ORM_BUILD_EXAMPLES=ON \
  -DVIX_ORM_USE_MYSQL=ON \
  -DVIX_ORM_USE_SQLITE=OFF
cmake --build . -j
```

Optional: Debug build with sanitizers

```bash
cmake -S .. -B . \
  -DCMAKE_BUILD_TYPE=Debug \
  -DVIX_ORM_USE_MYSQL=ON
cmake --build . -j
```

# 🧠 Usage Example

Example: Simple CRUD (examples/users_crud.cpp)

```cpp
#include <vix/orm/orm.hpp>
#include <iostream>

struct User {
    std::int64_t id{};
    std::string name;
    std::string email;
    int age{};
};

namespace Vix::orm {
template<> struct Mapper<User> {
    static std::vector<std::pair<std::string, std::any>> toInsertParams(const User& u) {
        return {{"name", u.name}, {"email", u.email}, {"age", u.age}};
    }
    static std::vector<std::pair<std::string, std::any>> toUpdateParams(const User& u) {
        return {{"name", u.name}, {"email", u.email}, {"age", u.age}};
    }
};
}

int main() {
    using namespace Vix::orm;
    try {
        ConnectionPool pool{"tcp://127.0.0.1:3306", "root", "", "vixdb"};
        BaseRepository<User> users{pool, "users"};
        auto id = users.create(User{0, "Alice", "alice@example.com", 28});
        std::cout << "[OK] Insert user → id=" << id << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[ERR] " << e.what() << std::endl;
    }
}
```

# Example Database Schema

```sql
CREATE DATABASE IF NOT EXISTS vixdb;
USE vixdb;
CREATE TABLE IF NOT EXISTS users (
  id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  name  VARCHAR(120) NOT NULL,
  email VARCHAR(190) NOT NULL,
  age   INT NOT NULL,
  PRIMARY KEY (id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

# Run:

```bash
./examples/vix_orm_users
# or
./examples/vix_orm_users "tcp://127.0.0.1:3306" root "<password>" vixdb
```

# Expected output:

```bash
[OK] Insert user → id=1
```

# Module Overview

```markdown
### 🧩 Module Overview

| **Component**             | **Role / Responsibility**                                                       |
| ------------------------- | ------------------------------------------------------------------------------- |
| `Drivers.hpp`             | Abstract base interface for all database drivers.                               |
| `MySQLDriver.hpp/.cpp`    | Concrete implementation for MySQL using Connector/C++.                          |
| `ConnectionPool.hpp/.cpp` | Manages reusable, thread-safe database connections.                             |
| `Repository.hpp`          | Provides generic CRUD operations for mapped entities.                           |
| `Mapper.hpp`              | Template specialization for mapping C++ entities to database tables and fields. |
| `Transaction.hpp`         | RAII helper to manage transactions (begin/commit/rollback) safely.              |
| `QueryBuilder.hpp`        | Fluent interface to construct SQL queries with parameter binding support.       |
| `MigrationsRunner.hpp`    | Discovers and runs migration classes implementing the `Migration` base class.   |
| `Migration.hpp`           | Base class for defining database schema migrations in a structured way.         |
```

# 🧱 Architecture

```lua
+-------------------+         +-------------------+
|   Application     | ----->  |   Repository<T>   |
+-------------------+         +-------------------+
          |                           |
          | uses Mapper<T>            |
          v                           v
  +-------------------+       +-------------------+
  |   ConnectionPool  | <-->  |   MySQLConnection |
  +-------------------+       +-------------------+
          |                           |
          v                           v
  +-------------------+       +-------------------+
  |  MySQL Driver     |       |   QueryBuilder    |
  +-------------------+       +-------------------+
```

# 🧩 Example Migration

```cpp
#include <vix/orm/Migration.hpp>

class CreateProductsTable : public Vix::orm::Migration {
public:
  std::string id() const override { return "2025_10_01_create_products"; }

  void up(Vix::orm::Connection& c) override {
      auto st = c.prepare("CREATE TABLE products (id INT AUTO_INCREMENT PRIMARY KEY, name VARCHAR(255))");
      st->exec();
  }

  void down(Vix::orm::Connection& c) override {
      auto st = c.prepare("DROP TABLE IF EXISTS products");
      st->exec();
  }
};
```

# 🧰 Installation & Integration (for Vix.cpp)

If you’re integrating this as a submodule in the main Vix.cpp project:

```cmake
# In vix/CMakeLists.txt
add_subdirectory(modules/orm)
target_link_libraries(vix_core PRIVATE vix_orm)
```

# 🧪 Testing (optional)

Enable unit tests:

```bash
cmake -S .. -B . -DBUILD_TESTS=ON
ctest --output-on-failure
```

# 🤝 Contributing

Contributions are welcome!
Please follow the steps below:

1. Fork the repository.
2. Create a new branch: git checkout -b feat/your-feature.
3. Commit changes with clear messages.
4. Run all builds/tests before pushing.
5. Submit a Pull Request with a concise explanation.

## License

MIT License – see [LICENSE](../../LICENSE) for details.

💡 Credits

Developed as part of the Vix.cpp Project
by Softadastra -> https://softadastra.com
— bringing high-performance C++ to the modern web.

# 🌟 Next Steps

. Implement ResultSet/ResultRow adapter for read operations
. Add PostgreSQL driver
. Expand QueryBuilder with JOIN, ORDER, and WHERE chaining
. Provide Vix CLI integration (vix orm:make:migration)
. Vix ORM — Designed for performance, built for clarity, and ready for the next generation of C++ backends.

Vix ORM — Designed for performance, built for clarity, and ready for the next generation of C++ backends.
