# ORM Examples

This page is a guide to the Vix ORM module with minimal, explicit examples.

Assumptions:
- MySQL is running locally
- Database: `vixdb`
- Table `users(id, name, email, age)` exists

Run any example with:

```bash
vix run examples/orm/<file>.cpp
```

## Concepts

Vix ORM is built on top of the `vix::db` layer and keeps everything explicit:

- Connection pools
- Transactions (RAII)
- Prepared statements
- Small helpers for mapping rows to C++ types

Nothing is hidden. SQL stays visible.

---

## 1) Connection pool + basic insert (transaction)

```cpp
#include <vix/orm/orm.hpp>
#include <iostream>

using namespace vix::orm;

int main()
{
  auto factory = make_mysql_factory(
    "tcp://127.0.0.1:3306",
    "root",
    "",
    "vixdb"
  );

  PoolConfig cfg;
  cfg.min = 1;
  cfg.max = 8;

  ConnectionPool pool{factory, cfg};
  pool.warmup();

  try
  {
    Transaction tx(pool);
    auto &c = tx.conn();

    auto st = c.prepare("INSERT INTO users(name,email,age) VALUES(?,?,?)");
    st->bind(1, "Alice");
    st->bind(2, "alice@example.com");
    st->bind(3, 25);
    st->exec();

    tx.commit();
    std::cout << "Inserted\n";
    return 0;
  }
  catch (const std::exception &e)
  {
    std::cerr << e.what() << "\n";
    return 1;
  }
}
```

Notes:
- `Transaction` rolls back automatically if you do not call `commit()`.
- Binding is positional and 1-based (1, 2, 3...).

---

## 2) QueryBuilder UPDATE

Use `QueryBuilder` when you want to build SQL with parameters, while keeping parameters separate from the SQL string.

```cpp
#include <vix/orm/orm.hpp>
#include <iostream>

using namespace vix::orm;

int main()
{
  auto factory = make_mysql_factory(
    "tcp://127.0.0.1:3306",
    "root",
    "",
    "vixdb"
  );

  ConnectionPool pool{factory, {1, 8}};
  pool.warmup();

  try
  {
    QueryBuilder qb;
    qb.raw("UPDATE users SET age=? WHERE email=?")
      .param(29)
      .param(std::string("alice@example.com"));

    PooledConn pc(pool);
    auto st = pc.get().prepare(qb.sql());

    const auto &ps = qb.params();
    for (std::size_t i = 0; i < ps.size(); ++i)
      st->bind(i + 1, ps[i]);

    std::cout << "Affected rows: " << st->exec() << "\n";
    return 0;
  }
  catch (const std::exception &e)
  {
    std::cerr << e.what() << "\n";
    return 1;
  }
}
```

---

## 3) Repository CRUD

To use the repository, define:
- Your entity struct
- A `Mapper<T>` specialization (row to entity, entity to params)

```cpp
#include <vix/orm/orm.hpp>
#include <iostream>
#include <string>

struct User
{
  std::int64_t id{};
  std::string name;
  std::string email;
  int age{};
};

namespace vix::orm
{
  template <>
  struct Mapper<User>
  {
    static User fromRow(const ResultRow &row)
    {
      User u{};
      u.id = row.getInt64Or(0, 0);
      u.name = row.getStringOr(1, "");
      u.email = row.getStringOr(2, "");
      u.age = static_cast<int>(row.getInt64Or(3, 0));
      return u;
    }

    static std::vector<std::pair<std::string, std::any>>
    toInsertParams(const User &u)
    {
      return {
        {"name", u.name},
        {"email", u.email},
        {"age", u.age},
      };
    }

    static std::vector<std::pair<std::string, std::any>>
    toUpdateParams(const User &u)
    {
      return {
        {"name", u.name},
        {"email", u.email},
        {"age", u.age},
      };
    }
  };
}

int main()
{
  using namespace vix::orm;

  auto factory = make_mysql_factory(
    "tcp://127.0.0.1:3306",
    "root",
    "",
    "vixdb"
  );

  ConnectionPool pool{factory, {1, 8}};
  pool.warmup();

  BaseRepository<User> repo{pool, "users"};

  try
  {
    auto id = repo.create(User{0, "Bob", "bob@example.com", 30});
    std::cout << "Created id=" << id << "\n";

    repo.updateById(static_cast<std::int64_t>(id),
                    User{static_cast<std::int64_t>(id), "Bobby", "bob@example.com", 31});

    if (auto u = repo.findById(static_cast<std::int64_t>(id)))
      std::cout << "Found: " << u->name << "\n";

    repo.removeById(static_cast<std::int64_t>(id));
    std::cout << "Deleted\n";
    return 0;
  }
  catch (const std::exception &e)
  {
    std::cerr << e.what() << "\n";
    return 1;
  }
}
```

---

## 4) UnitOfWork

When you want a short transactional scope without writing SQL in every place, use `UnitOfWork`.

```cpp
#include <vix/orm/orm.hpp>
#include <iostream>

using namespace vix::orm;

int main()
{
  auto factory = make_mysql_factory(
    "tcp://127.0.0.1:3306",
    "root",
    "",
    "vixdb"
  );

  ConnectionPool pool{factory, {1, 8}};
  pool.warmup();

  try
  {
    UnitOfWork uow{pool};
    auto &c = uow.conn();

    c.prepare("INSERT INTO users(name,email,age) VALUES(?,?,?)")
      ->bind(1, "Charlie")
      ->bind(2, "charlie@example.com")
      ->bind(3, 22)
      ->exec();

    uow.commit();
    std::cout << "Committed\n";
    return 0;
  }
  catch (const std::exception &e)
  {
    std::cerr << e.what() << "\n";
    return 1;
  }
}
```

---

## Common errors

- Cannot connect: verify host, port, user, password, and database name
- Missing table: create the table or run migrations first
- Bind mismatch: bind integers as `std::int64_t` when in doubt

Vix ORM stays minimal: explicit SQL, predictable transactions, clear failures.


