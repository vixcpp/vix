# Vix DB Quickstart (5 Minutes)

This is the fastest way to get started with the Vix C++ DB module.

Goal: connect, create table, insert, query.

## 1️⃣ Configure Database

``` cpp
#include <vix/db/db.hpp>
#include <iostream>

using namespace vix::db;

DbConfig make_cfg()
{
    DbConfig cfg;
    cfg.engine = Engine::MySQL;
    cfg.mysql.host = "tcp://127.0.0.1:3306";
    cfg.mysql.user = "root";
    cfg.mysql.password = "";
    cfg.mysql.database = "vixdb";
    cfg.mysql.pool.min = 1;
    cfg.mysql.pool.max = 4;
    return cfg;
}
```

## 2️⃣ Full Working Example

``` cpp
int main()
{
    Database db(make_cfg());

    Transaction tx(db.pool());

    // Create table
    tx.conn().prepare(
        "CREATE TABLE IF NOT EXISTS users ("
        "id BIGINT PRIMARY KEY AUTO_INCREMENT,"
        "name VARCHAR(255) NOT NULL,"
        "age INT NOT NULL)"
    )->exec();

    // Insert
    auto insert = tx.conn().prepare(
        "INSERT INTO users (name, age) VALUES (?, ?)"
    );
    insert->bind(1, std::string("Alice"));
    insert->bind(2, static_cast<std::int64_t>(22));
    insert->exec();

    // Query
    auto query = tx.conn().prepare(
        "SELECT id, name, age FROM users"
    );

    auto rs = query->query();
    while (rs->next())
    {
        const auto& row = rs->row();
        std::cout
            << row.getInt64(0) << " "
            << row.getString(1) << " "
            << row.getInt64(2) << "\n";
    }

    tx.commit();
    return 0;
}
```

## 3️⃣ What You Just Used

Database\
Connection Pool\
Prepared Statements\
Transaction (RAII)

## 🎯 That's it.

You now have a working database setup in Vix C++.

Next step: - Add migrations - Add error handling - Integrate with your
HTTP routes

