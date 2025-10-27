# ORM Overview — Vix.cpp

The **Vix ORM** is a lightweight, modern C++20 data layer built for **performance**, **type safety**, and **simplicity**.  
It follows **Repository** and **Unit of Work** patterns, offers a **QueryBuilder**, and supports **MySQL** and **SQLite** drivers.

> Use it standalone or as part of the full Vix.cpp stack. Everything is modular and opt‑in.

---

## ✨ Goals

- **Zero‑surprise APIs** — predictable CRUD with strong typing.
- **High throughput** — connection pooling, prepared statements, minimal copies.
- **Composable** — repositories wrap the builder/driver; transactions via RAII.
- **Portable** — pure C++20 with thin adapters for MySQL / SQLite.

---

## 🧱 Architecture (High‑Level)

```
+-------------------+       +-----------------+
|   Your Services   |       |    Migrations   |
|  (Handlers etc.)  |       |  (optional)     |
+---------+---------+       +--------+--------+
          |                           |
          v                           v
+-------------------+       +-----------------+
|   UnitOfWork      |<----->|  Repository<T>  |
|  (tracks changes) |       |  CRUD, queries  |
+---------+---------+       +--------+--------+
          |                           |
          v                           v
+-------------------+       +-----------------+
|   QueryBuilder    |------>|   Driver (DB)   |
|  (fluent SQL)     |       | MySQL / SQLite  |
+-------------------+       +-----------------+
                ^                   |
                |                   v
           +----+-------------------+----+
           |     ConnectionPool (thread‑safe) |
           +----------------------------------+
```

---

## 🔌 Drivers

| Driver  | Status      | Dependency            | Notes                                      |
| ------- | ----------- | --------------------- | ------------------------------------------ |
| MySQL   | ✅ Stable   | `mysql-connector-cpp` | Prepared statements, transactions, pooling |
| SQLite3 | ⚙️ Optional | `libsqlite3-dev`      | Lightweight, file‑backed DB (experimental) |

Enable in CMake:

```bash
# MySQL only (default)
cmake -S . -B build -DVIX_ORM_USE_MYSQL=ON -DVIX_ORM_USE_SQLITE=OFF

# MySQL + SQLite
cmake -S . -B build -DVIX_ORM_USE_MYSQL=ON -DVIX_ORM_USE_SQLITE=ON
```

---

## 🚀 Quick Start

```cpp
#include <vix/orm/orm.hpp>
using namespace Vix::orm;

struct User {
    int id{};
    std::string name;
    std::string email;
};

int main() {
    // 1) Create a connection pool (MySQL example)
    auto pool = ConnectionPool::create_mysql(
        "tcp://127.0.0.1:3306", "root", "pass", "vixdb",
        /*pool_size=*/8);

    // 2) Use a repository
    Repository<User> repo(pool);

    // 3) Basic CRUD
    User u{0, "Gaspard", "gaspard@example.com"};
    repo.insert(u);              // INSERT and set u.id
    auto users = repo.find_all(); // SELECT *

    u.email = "gkirira@example.com";
    repo.update(u);              // UPDATE
    repo.remove(u.id);           // DELETE
}
```

---

## 🧩 Repository Pattern

Repositories hide raw SQL and expose intent‑based operations:

```cpp
Repository<User> users(pool);

auto all   = users.find_all();
auto alice = users.find_by_id(42);
users.insert(User{0, "Alice", "alice@x.test"});
users.update(/* ... */);
users.remove(42);
```

### Custom Queries

```cpp
auto actives = users.where([](auto& qb){
    qb.select("id","name","email")
      .from("users")
      .where("active = ?").bind(true)
      .order_by("created_at DESC")
      .limit(50);
});
```

---

## 🧮 QueryBuilder (Fluent SQL)

```cpp
QueryBuilder qb;
auto sql = qb.select("*").from("users")
             .where("email = ? AND age >= ?")
             .bind("alice@test.com")
             .bind(18)
             .order_by("id DESC")
             .limit(10)
             .to_string();
// Execute via driver or repository
```

- **Safety**: placeholders + `.bind()` → avoids SQL injection.
- **Composability**: conditional clauses, pagination helpers, batch inserts (where supported).

---

## 🔒 Transactions & Unit of Work

```cpp
UnitOfWork uow(pool);
auto& users = uow.repository<User>();

User u{0,"Ada","ada@test.com"};
users.insert(u);

// more repositories / changes…

uow.commit();   // all or nothing
// uow.rollback() automatically on destruction if not committed
```

- **RAII semantics** prevent half‑commits on exceptions.
- Cross‑repository consistency without manual transaction wiring.

---

## 🧳 Connection Pooling

```cpp
auto pool = ConnectionPool::create_mysql(uri, user, pass, db,
    /*pool_size=*/std::thread::hardware_concurrency(),
    ConnectionPool::Options{
        .connect_timeout_ms = 2000,
        .idle_timeout_ms    = 30'000,
        .max_lifetime_ms    = 300'000
    });
```

- Thread‑safe lease/return.
- Optional health‑checks and idle culling.
- Tune pool size by workload (throughput vs memory).

---

## 🛠️ Migrations (Optional)

- Versioned `up.sql` / `down.sql` files.
- `MigrationsRunner` applies pending versions atomically.
- Store current version in a dedicated table (e.g., `_migrations`).

```cpp
MigrationsRunner mgr(pool, "./migrations");
mgr.up();     // apply all pending
// mgr.down(1);  // rollback one step (example)
```

---

## 🧯 Error Handling

All DB errors surface as **`DBError`** with rich context:

```cpp
try {
    repo.insert(u);
} catch (const DBError& e) {
    // e.driver(), e.code(), e.message()
    std::cerr << "DBError[" << e.driver() << ":" << e.code()
              << "] " << e.message() << "\n";
}
```

Integrates cleanly with the HTTP layer to return consistent JSON errors.

---

## 📈 Performance Tips

- Prefer **prepared statements** and reuse them.
- Keep transactions **short‑lived**; batch logically.
- Use **connection pooling** sized to your concurrency.
- Avoid needless JSON → entity → JSON round‑trips in hot paths.
- Consider **LTO/`-O3`/`-march=native`** for production builds.

---

## 🔭 Next

- **Repository guide** — mapping, projections, partial updates.
- **QueryBuilder cookbook** — complex filters, joins, aggregations.
- **Examples** — see `docs/orm/examples.md` for runnable demos.
