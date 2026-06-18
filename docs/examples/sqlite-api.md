# SQLite API

This example shows how to build a small Vix JSON API backed by SQLite.

It uses the Vix database module:

```cpp
#include <vix/db.hpp>
```

The example uses SQLite because it does not require a separate database server.

You will build:

```txt
GET    /api/health
GET    /api/products
GET    /api/products/{id}
POST   /api/products
PATCH  /api/products/{id}
DELETE /api/products/{id}
```

The database file is stored at:

```txt
storage/app.db
```

## Why SQLite

SQLite is a good default for examples, local tools, tests, prototypes, and local-first applications.

It gives you a real SQL database without requiring:

```txt
a database server
a network connection
a database daemon
a separate setup step
```

For this example, that means you can create a small API, store real rows, inspect the file, and reset the database by deleting it.

## Project structure

Create this structure:

```txt
sqlite_api_demo/
├── sqlite_api.cpp
└── storage/
```

Create the directory:

```bash
mkdir -p sqlite_api_demo/storage
cd sqlite_api_demo
touch sqlite_api.cpp
```

## Source

Open:

```txt
sqlite_api.cpp
```

Add:

```cpp
#include <exception>
#include <optional>
#include <string>
#include <vector>

#include <vix.hpp>
#include <vix/db.hpp>
#include <vix/json.hpp>
#include <vix/middleware.hpp>

using namespace vix;

struct Product
{
  long long id{};
  std::string name{};
  double price{};
  bool available{};
  std::string created_at{};
};

static vix::json::Json product_to_json(const Product &product)
{
  using namespace vix::json;

  return o(
    "id", product.id,
    "name", product.name,
    "price", product.price,
    "available", product.available,
    "created_at", product.created_at
  );
}

static void initialize_schema(vix::db::Database &db)
{
  db.exec(
    "CREATE TABLE IF NOT EXISTS products ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "name TEXT NOT NULL,"
    "price REAL NOT NULL,"
    "available INTEGER NOT NULL DEFAULT 1,"
    "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
    ")"
  );
}

static Product read_product(const vix::db::ResultRow &row)
{
  Product product;

  product.id = row.getInt64(0);
  product.name = row.getString(1);
  product.price = row.getDouble(2);
  product.available = row.getInt64(3) != 0;
  product.created_at = row.getString(4);

  return product;
}

static std::vector<Product> list_products(vix::db::Database &db)
{
  auto rows = db.query(
    "SELECT id, name, price, available, created_at "
    "FROM products "
    "ORDER BY id DESC"
  );

  std::vector<Product> products;

  while (rows->next())
  {
    products.push_back(read_product(rows->row()));
  }

  return products;
}

static std::optional<Product> find_product(
  vix::db::Database &db,
  long long id)
{
  auto rows = db.query(
    "SELECT id, name, price, available, created_at "
    "FROM products "
    "WHERE id = ?",
    id
  );

  if (!rows->next())
    return std::nullopt;

  return read_product(rows->row());
}

static Product create_product(
  vix::db::Database &db,
  const std::string &name,
  double price,
  bool available)
{
  db.exec(
    "INSERT INTO products (name, price, available) "
    "VALUES (?, ?, ?)",
    name,
    price,
    available
  );

  auto rows = db.query(
    "SELECT id, name, price, available, created_at "
    "FROM products "
    "WHERE id = last_insert_rowid()"
  );

  if (!rows->next())
    throw std::runtime_error("created product could not be loaded");

  return read_product(rows->row());
}

static bool update_product(
  vix::db::Database &db,
  long long id,
  const std::string &name,
  double price,
  bool available)
{
  const auto affected = db.exec(
    "UPDATE products "
    "SET name = ?, price = ?, available = ? "
    "WHERE id = ?",
    name,
    price,
    available,
    id
  );

  return affected > 0;
}

static bool delete_product(vix::db::Database &db, long long id)
{
  const auto affected = db.exec(
    "DELETE FROM products WHERE id = ?",
    id
  );

  return affected > 0;
}

static long long parse_id(const std::string &value)
{
  try
  {
    return std::stoll(value);
  }
  catch (...)
  {
    return 0;
  }
}

static void send_validation_error(
  Response &res,
  const std::string &field,
  const std::string &message)
{
  res.status(422).json({
    "ok", false,
    "error", message,
    "field", field
  });
}

static bool validate_product_input(
  Response &res,
  const std::optional<std::string> &name,
  double price)
{
  if (!name || name->empty())
  {
    send_validation_error(res, "name", "Missing required field");
    return false;
  }

  if (price <= 0.0)
  {
    send_validation_error(res, "price", "Price must be greater than zero");
    return false;
  }

  return true;
}

static void install_middleware(App &app)
{
  app.use("/api", middleware::app::recovery_dev());
  app.use("/api", middleware::app::request_id_dev());
  app.use("/api", middleware::app::timing_dev());
  app.use("/api", middleware::app::security_headers_dev());
  app.use("/api", middleware::app::rate_limit_dev());
  app.use("/api", middleware::app::body_limit_write_dev(1024 * 1024));

  app.use("/api/products", middleware::app::json_strict_dev(
    4096,
    false,
    true
  ));
}

static void register_routes(App &app, vix::db::Database &db)
{
  app.get("/", [](Request &, Response &res)
  {
    res.text("SQLite API example. Try /api/products.");
  });

  app.get("/api/health", [](Request &, Response &res)
  {
    res.json({
      "ok", true,
      "service", "sqlite-api"
    });
  });

  app.get("/api/products", [&db](Request &, Response &res)
  {
    using namespace vix::json;

    Json items = arr();

    for (const auto &product : list_products(db))
    {
      items.push_back(product_to_json(product));
    }

    res.json(o(
      "ok", true,
      "products", items
    ));
  });

  app.get("/api/products/{id}", [&db](Request &req, Response &res)
  {
    using namespace vix::json;

    const long long id = parse_id(req.param("id"));

    if (id <= 0)
    {
      res.status(400).json({
        "ok", false,
        "error", "Invalid product id"
      });
      return;
    }

    auto product = find_product(db, id);

    if (!product)
    {
      res.status(404).json({
        "ok", false,
        "error", "Product not found"
      });
      return;
    }

    res.json(o(
      "ok", true,
      "product", product_to_json(*product)
    ));
  });

  app.post("/api/products", [&db](Request &req, Response &res)
  {
    using namespace vix::json;

    auto &body =
      req.state<middleware::parsers::JsonBody>();

    auto name = get_opt<std::string>(body.value, "name");
    const double price = get_or<double>(body.value, "price", 0.0);
    const bool available = get_or<bool>(body.value, "available", true);

    if (!validate_product_input(res, name, price))
      return;

    Product product =
      create_product(db, *name, price, available);

    res.status(201).json(o(
      "ok", true,
      "product", product_to_json(product)
    ));
  });

  app.patch("/api/products/{id}", [&db](Request &req, Response &res)
  {
    using namespace vix::json;

    const long long id = parse_id(req.param("id"));

    if (id <= 0)
    {
      res.status(400).json({
        "ok", false,
        "error", "Invalid product id"
      });
      return;
    }

    auto &body =
      req.state<middleware::parsers::JsonBody>();

    auto name = get_opt<std::string>(body.value, "name");
    const double price = get_or<double>(body.value, "price", 0.0);
    const bool available = get_or<bool>(body.value, "available", true);

    if (!validate_product_input(res, name, price))
      return;

    const bool updated =
      update_product(db, id, *name, price, available);

    if (!updated)
    {
      res.status(404).json({
        "ok", false,
        "error", "Product not found"
      });
      return;
    }

    auto product = find_product(db, id);

    res.json(o(
      "ok", true,
      "product", product_to_json(*product)
    ));
  });

  app.del("/api/products/{id}", [&db](Request &req, Response &res)
  {
    const long long id = parse_id(req.param("id"));

    if (id <= 0)
    {
      res.status(400).json({
        "ok", false,
        "error", "Invalid product id"
      });
      return;
    }

    const bool deleted = delete_product(db, id);

    if (!deleted)
    {
      res.status(404).json({
        "ok", false,
        "error", "Product not found"
      });
      return;
    }

    res.json({
      "ok", true,
      "deleted", true
    });
  });
}

int main()
{
  try
  {
    auto db = vix::db::Database::sqlite("storage/app.db");

    initialize_schema(db);

    App app;

    install_middleware(app);
    register_routes(app, db);

    app.run(8080);
    return 0;
  }
  catch (const std::exception &e)
  {
    vix::print("sqlite api error:", e.what());
    return 1;
  }
}
```

## Build with SQLite support

This example uses the Vix database module with SQLite.

Build with:

```bash
vix build --with-sqlite
```

For a release build:

```bash
vix build --preset release --with-sqlite
```

Run with:

```bash
vix run --with-sqlite
```

If you use CMake directly, make sure your project links Vix with SQLite support enabled.

## Optional CMakeLists.txt

Create:

```txt
CMakeLists.txt
```

Add:

```cmake
cmake_minimum_required(VERSION 3.20)

project(sqlite_api_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(vix CONFIG REQUIRED)

add_executable(sqlite_api_demo
  sqlite_api.cpp
)

target_link_libraries(sqlite_api_demo
  PRIVATE
    vix::vix
)
```

If your installed Vix package exposes another target name, use the target name from your local installation.

## Run it

```bash
vix run --with-sqlite
```

The server listens on:

```txt
http://127.0.0.1:8080
```

The SQLite database is created at:

```txt
storage/app.db
```

## Test health

```bash
curl -i http://127.0.0.1:8080/api/health
```

Expected body:

```json
{
  "ok": true,
  "service": "sqlite-api"
}
```

## List products

```bash
curl -i http://127.0.0.1:8080/api/products
```

Expected body shape:

```json
{
  "ok": true,
  "products": []
}
```

If products already exist, the array contains rows from SQLite.

## Create a product

```bash
curl -i \
  -X POST http://127.0.0.1:8080/api/products \
  -H "Content-Type: application/json" \
  -d '{"name":"Laptop","price":999.99,"available":true}'
```

Expected status:

```txt
201 Created
```

Expected body shape:

```json
{
  "ok": true,
  "product": {
    "id": 1,
    "name": "Laptop",
    "price": 999.99,
    "available": true,
    "created_at": "..."
  }
}
```

## Get one product

```bash
curl -i http://127.0.0.1:8080/api/products/1
```

Expected body shape:

```json
{
  "ok": true,
  "product": {
    "id": 1,
    "name": "Laptop",
    "price": 999.99,
    "available": true,
    "created_at": "..."
  }
}
```

Missing product:

```bash
curl -i http://127.0.0.1:8080/api/products/999
```

Expected status:

```txt
404 Not Found
```

## Update a product

```bash
curl -i \
  -X PATCH http://127.0.0.1:8080/api/products/1 \
  -H "Content-Type: application/json" \
  -d '{"name":"Laptop Pro","price":1299.99,"available":true}'
```

Expected body shape:

```json
{
  "ok": true,
  "product": {
    "id": 1,
    "name": "Laptop Pro",
    "price": 1299.99,
    "available": true,
    "created_at": "..."
  }
}
```

## Delete a product

```bash
curl -i \
  -X DELETE http://127.0.0.1:8080/api/products/1
```

Expected body:

```json
{
  "ok": true,
  "deleted": true
}
```

Request it again:

```bash
curl -i http://127.0.0.1:8080/api/products/1
```

Expected status:

```txt
404 Not Found
```

## Test validation

Missing name:

```bash
curl -i \
  -X POST http://127.0.0.1:8080/api/products \
  -H "Content-Type: application/json" \
  -d '{"price":999.99}'
```

Expected status:

```txt
422 Unprocessable Entity
```

Invalid price:

```bash
curl -i \
  -X POST http://127.0.0.1:8080/api/products \
  -H "Content-Type: application/json" \
  -d '{"name":"Broken","price":0}'
```

Expected status:

```txt
422 Unprocessable Entity
```

Invalid JSON:

```bash
curl -i \
  -X POST http://127.0.0.1:8080/api/products \
  -H "Content-Type: application/json" \
  -d '{"name":}'
```

Expected status:

```txt
400 Bad Request
```

Wrong content type:

```bash
curl -i \
  -X POST http://127.0.0.1:8080/api/products \
  -H "Content-Type: text/plain" \
  -d '{"name":"Laptop","price":999.99}'
```

Expected status:

```txt
415 Unsupported Media Type
```

The JSON parser middleware rejects invalid HTTP input before the route handler runs.

The route handler validates business rules.

## What the database code does

The database opens with:

```cpp
auto db = vix::db::Database::sqlite("storage/app.db");
```

The schema is created with `exec()`:

```cpp
db.exec(
  "CREATE TABLE IF NOT EXISTS products ("
  "id INTEGER PRIMARY KEY AUTOINCREMENT,"
  "name TEXT NOT NULL,"
  "price REAL NOT NULL,"
  "available INTEGER NOT NULL DEFAULT 1,"
  "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
  ")"
);
```

Rows are inserted with bind parameters:

```cpp
db.exec(
  "INSERT INTO products (name, price, available) "
  "VALUES (?, ?, ?)",
  name,
  price,
  available
);
```

Rows are read with `query()`:

```cpp
auto rows = db.query(
  "SELECT id, name, price, available, created_at "
  "FROM products "
  "ORDER BY id DESC"
);
```

Then the result set is read forward:

```cpp
while (rows->next())
{
  const auto &row = rows->row();

  auto id = row.getInt64(0);
  auto name = row.getString(1);
  auto price = row.getDouble(2);
}
```

Column indexes start at `0`.

They follow the order of the `SELECT` list.

## Why bind parameters matter

Values are passed after the SQL string:

```cpp
db.exec(
  "DELETE FROM products WHERE id = ?",
  id
);
```

The first value binds to the first `?`.

Do not concatenate user input into SQL.

Bad:

```cpp
db.exec(
  "DELETE FROM products WHERE id = " + req.param("id")
);
```

Good:

```cpp
db.exec(
  "DELETE FROM products WHERE id = ?",
  id
);
```

Prepared statements keep SQL structure separate from values.

## Why `exec()` and `query()` are separate

Use `exec()` for SQL that does not return rows:

```txt
CREATE TABLE
INSERT
UPDATE
DELETE
DROP TABLE
ALTER TABLE
```

Use `query()` for SQL that returns rows:

```txt
SELECT
```

Example:

```cpp
db.exec(
  "UPDATE products SET name = ? WHERE id = ?",
  "Laptop Pro",
  1
);
```

and:

```cpp
auto rows = db.query(
  "SELECT id, name FROM products WHERE id = ?",
  1
);
```

This makes database code easy to read.

## Middleware stack

The example installs:

```cpp
app.use("/api", middleware::app::recovery_dev());
app.use("/api", middleware::app::request_id_dev());
app.use("/api", middleware::app::timing_dev());
app.use("/api", middleware::app::security_headers_dev());
app.use("/api", middleware::app::rate_limit_dev());
app.use("/api", middleware::app::body_limit_write_dev(1024 * 1024));

app.use("/api/products", middleware::app::json_strict_dev(
  4096,
  false,
  true
));
```

The order is intentional:

```txt
recovery
  catches unhandled exceptions

request id
  identifies requests

timing
  measures request duration

security headers
  hardens responses

rate limit
  protects the API

body limit
  rejects oversized request bodies

JSON parser
  parses request bodies for product routes
```

The database code stays in helper functions.

The reusable HTTP behavior stays in middleware.

## Why JSON parser is route-specific

The JSON parser is installed on:

```cpp
app.use("/api/products", middleware::app::json_strict_dev(...));
```

not globally on:

```cpp
app.use("/api", middleware::app::json_strict_dev(...));
```

That matters because:

```txt
GET /api/health
  does not need a JSON body

GET /api/products
  does not need a JSON body

POST /api/products
  needs a JSON body

PATCH /api/products/{id}
  needs a JSON body
```

Install parsers only where they make sense.

## Inspect the database

Use the SQLite CLI:

```bash
sqlite3 storage/app.db
```

Inside SQLite:

```sql
.tables
SELECT * FROM products;
```

Exit:

```txt
.quit
```

## Reset the database

Stop the server.

Remove the database files:

```bash
rm -f storage/app.db storage/app.db-wal storage/app.db-shm
```

Run the server again:

```bash
vix run --with-sqlite
```

The schema will be recreated automatically.

## Add WAL mode

For local APIs, SQLite WAL mode can be useful.

Add this after opening the database:

```cpp
db.exec("PRAGMA journal_mode = WAL");
db.exec("PRAGMA busy_timeout = 5000");
db.exec("PRAGMA foreign_keys = ON");
```

Example:

```cpp
auto db = vix::db::Database::sqlite("storage/app.db");

db.exec("PRAGMA journal_mode = WAL");
db.exec("PRAGMA busy_timeout = 5000");
db.exec("PRAGMA foreign_keys = ON");

initialize_schema(db);
```

These settings are useful for many SQLite applications:

```txt
journal_mode = WAL
  improves read/write behavior for many local workloads

busy_timeout = 5000
  waits briefly when the database is busy

foreign_keys = ON
  enforces foreign key constraints
```

## Add a transaction later

This example uses one SQL statement per write.

For multi-step writes, use a transaction.

Example shape:

```cpp
vix::db::Transaction tx(db.pool());

auto &conn = tx.conn();

auto insert = conn.prepare(
  "INSERT INTO products (name, price, available) VALUES (?, ?, ?)"
);

insert->bind(1, "Laptop");
insert->bind(2, 999.99);
insert->bind(3, true);
insert->exec();

tx.commit();
```

If `commit()` is not called, the transaction rolls back when it leaves scope.

Use transactions when several database operations must succeed together.

## Use configuration later

For an example, this is fine:

```cpp
auto db = vix::db::Database::sqlite("storage/app.db");
```

For a real project, move the database path into configuration.

Example `vix.json`:

```json
{
  "name": "SQLiteApi",
  "database": {
    "engine": "sqlite",
    "sqlite": {
      "path": "storage/app.db"
    },
    "storage": "storage",
    "migrations": "migrations"
  }
}
```

Example `.env`:

```dotenv
DATABASE_ENGINE=sqlite
DATABASE_SQLITE_PATH=storage/app.db
SERVER_PORT=8080
```

Use `vix.json` for project shape.

Use `.env` for runtime values.

## Production notes

This example is a good starting point, not a full production database layer.

For a production backend, add:

```txt
migrations
repositories
pagination
filtering
transactions for multi-step writes
authentication
authorization
structured logs
tests
```

For SQLite specifically:

```txt
keep writes short
avoid long write transactions
enable WAL when useful
set a busy timeout
store the database under storage/
do not commit runtime database files
```

A typical `.gitignore` should contain:

```txt
storage/*.db
storage/*.db-wal
storage/*.db-shm
```

## Complete test flow

Run:

```bash
vix run --with-sqlite
```

Health:

```bash
curl -i http://127.0.0.1:8080/api/health
```

Create:

```bash
curl -i \
  -X POST http://127.0.0.1:8080/api/products \
  -H "Content-Type: application/json" \
  -d '{"name":"Laptop","price":999.99,"available":true}'
```

List:

```bash
curl -i http://127.0.0.1:8080/api/products
```

Get one:

```bash
curl -i http://127.0.0.1:8080/api/products/1
```

Update:

```bash
curl -i \
  -X PATCH http://127.0.0.1:8080/api/products/1 \
  -H "Content-Type: application/json" \
  -d '{"name":"Laptop Pro","price":1299.99,"available":true}'
```

Delete:

```bash
curl -i \
  -X DELETE http://127.0.0.1:8080/api/products/1
```

Reset:

```bash
rm -f storage/app.db storage/app.db-wal storage/app.db-shm
```

## Summary

A small SQLite-backed Vix API follows this shape:

```txt
open database
create schema
install middleware
register routes
use exec() for writes
use query() for reads
return JSON responses
```

The core database entry point is:

```cpp
auto db = vix::db::Database::sqlite("storage/app.db");
```

Use `exec()` for writes:

```cpp
db.exec(
  "INSERT INTO products (name, price) VALUES (?, ?)",
  name,
  price
);
```

Use `query()` for reads:

```cpp
auto rows = db.query(
  "SELECT id, name, price FROM products"
);
```

Use Vix middleware to protect and prepare the API:

```cpp
app.use("/api", middleware::app::recovery_dev());
app.use("/api", middleware::app::security_headers_dev());
app.use("/api", middleware::app::body_limit_write_dev(1024 * 1024));
app.use("/api/products", middleware::app::json_strict_dev(4096));
```

This gives you a real Vix API with persistent SQLite storage and a clean modern C++ structure.
