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

## 🧱 Core Concepts

### 1️⃣ DbConfig — The Database Configuration Model

```cpp
DbConfig cfg;
cfg.engine = DbEngine::MySQL;

cfg.mysql.host = "tcp://127.0.0.1:3306";
cfg.mysql.user = "root";
cfg.mysql.password = "";
cfg.mysql.database = "vixdb";
cfg.mysql.pool = {1, 8};
```

### 2️⃣ Database — High-level database entry point

```cpp
Database db{cfg};
auto& pool = db.pool();
```

### 3️⃣ ConnectionPool

```cpp
PooledConn pc(pool);
auto& c = pc.get();

auto st = c.prepare("SELECT 1");
st->exec();
```

### 4️⃣ Mapper<T>

```cpp
template<>
struct Mapper<User> {
    static auto toInsertParams(const User& u) {
        return std::vector<std::pair<std::string, std::any>>{
            {"name", u.name},
            {"email", u.email},
            {"age", u.age}
        };
    }
};
```

### 5️⃣ Repository<T>

```cpp
Repository<User> users{db.pool(), "users"};
auto id = users.create(User{"Alice", "alice@example.com", 27});
```

### 6️⃣ Transactions / UnitOfWork

```cpp
UnitOfWork uow{db.pool()};
auto& c = uow.conn();

c.prepare("INSERT INTO logs(msg) VALUES(?)")
    ->bind(1, "hello")->exec();

uow.commit();
```

### 7️⃣ QueryBuilder

```cpp
QueryBuilder qb;
qb.raw("UPDATE users SET age=? WHERE email=?")
  .param(29)
  .param("zoe@example.com");
```

### 🧰 Usage Examples

#### ✔️ Insert with Repository

```cpp
DbConfig cfg = make_db_config_from_vix_config(vix::config::Config::getInstance());
Database db{cfg};

Repository<User> users{db.pool(), "users"};
auto id = users.create({0, "Gaspard", "gaspard@example.com", 28});
std::cout << "User inserted with id=" << id << "\n";
```

### ✔️ UnitOfWork with Automatic Rollback

```cpp
UnitOfWork uow{db.pool()};
auto& c = uow.conn();

// Insert user
c.prepare("INSERT INTO users(name,email,age) VALUES(?,?,?)")
    ->bind(1, "Alice")
    ->bind(2, "alice@example.com")
    ->bind(3, 27)
    ->exec();

auto userId = c.lastInsertId();

// Insert order
c.prepare("INSERT INTO orders(user_id,total) VALUES(?,?)")
    ->bind(1, userId)
    ->bind(2, 150.0)
    ->exec();

uow.commit();
```

### ✔️ QueryBuilder UPDATE

```cpp
QueryBuilder qb;
qb.raw("UPDATE users SET age=? WHERE email=?")
  .param(30)
  .param("mina@example.com");

PooledConn pc(db.pool());
auto st = pc.get().prepare(qb.sql());

// bind params
const auto& ps = qb.params();
for (size_t i = 0; i < ps.size(); ++i)
    st->bind(i + 1, ps[i]);
st->exec();
```

### ✔️ Migrations Example

```cpp
class CreateUsers : public Migration {
public:
std::string id() const override { return "2025_10_10_create_users"; }

    void up(Connection& c) override {
        c.prepare(
            "CREATE TABLE IF NOT EXISTS users("
            " id BIGINT PRIMARY KEY AUTO_INCREMENT,"
            " name VARCHAR(120),"
            " email VARCHAR(190),"
            " age INT )")->exec();
    }

};
```

### Running:

```cpp
auto raw = make_mysql_conn(host, user, pass, db);
MySQLConnection conn{raw};

MigrationsRunner runner{conn};
CreateUsers m1;
runner.add(&m1);
runner.runAll();
```

### 🧠 Usage Example

Example: Simple CRUD (examples/users_crud.cpp)

```cpp
DbConfig cfg = make_db_config_from_vix_config(
    vix::config::Config::getInstance()
);

Database db{cfg};

Repository<User> users{db.pool(), "users"};
auto id = users.create({0, "Gaspard", "gaspard@example.com", 28});

std::cout << "User inserted with id=" << id << "\n";
```

### ✔️ UnitOfWork Transaction Example

```cpp
UnitOfWork uow{db.pool()};
auto& c = uow.conn();

// Insert user
c.prepare("INSERT INTO users(name,email,age) VALUES(?,?,?)")
    ->bind(1, "Alice")
    ->bind(2, "alice@example.com")
    ->bind(3, 27)
    ->exec();

auto userId = c.lastInsertId();

// Insert order
c.prepare("INSERT INTO orders(user_id,total) VALUES(?,?)")
    ->bind(1, userId)
    ->bind(2, 150.0)
    ->exec();

uow.commit();
```

### ✔️ QueryBuilder UPDATE Example

```cpp
QueryBuilder qb;
qb.raw("UPDATE users SET age=? WHERE email=?")
  .param(29)
  .param("zoe@example.com");

PooledConn pc(db.pool());
auto st = pc.get().prepare(qb.sql());

const auto& params = qb.params();
for (size_t i = 0; i < params.size(); ++i)
    st->bind(i + 1, params[i]);

auto affected = st->exec();
```

### ✔️ Migrations Example

```cpp
class CreateUsers : public Migration {
public:
    std::string id() const override { return "2025_10_10_create_users"; }

    void up(Connection& c) override {
        c.prepare(
            "CREATE TABLE IF NOT EXISTS users("
            " id BIGINT AUTO_INCREMENT PRIMARY KEY,"
            " name VARCHAR(120),"
            " email VARCHAR(190),"
            " age INT)"
        )->exec();
    }

    void down(Connection& c) override {
        c.prepare("DROP TABLE IF EXISTS users")->exec();
    }
};
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
App / Services
       |
       v
+---------------------+
|     Repository<T>   |
+---------------------+
      | uses Mapper<T>
      v
+---------------------+
|    UnitOfWork       |
|  Transaction/Commit |
+---------------------+
      v
+---------------------+
|   ConnectionPool    |
+---------------------+
      v
+---------------------+
|   Driver (MySQL)    |
+---------------------+
```

### 🧪 Testing

```bash
cmake -S . -B build -DVIX_ORM_BUILD_TESTS=ON
ctest --output-on-failure
```

# 🤝 Contributing

Contributions are welcome!
Please follow the steps below:

1. Fork the repository.
2. Create a new branch: git checkout -b feat/your-feature.
3. Follow C++20 style & Vix coding standards
4. Commit changes with clear messages.
5. Run all builds/tests before pushing.
6. Submit a Pull Request with a concise explanation.

## License

MIT License © Softadastra / Gaspard Kirira
See LICENSE file for full details.

## 🌟 Next Steps (Roadmap)

1. ResultSet / Row adapter (typed reads)
2. Full SQLite driver
3. PostgreSQL driver
4. Advanced QueryBuilder (JOIN, ORDER, GROUP…)
5. CLI integration: vix orm:make:migration
6. Relationship API (has_many, belongs_to)
7. Validation layer for entities
