/**
 * @file users_crud_internal.cpp
 * @brief Contributor documentation — internals & pedagogy for users_crud example.
 *
 * This file intentionally defines **no linker-visible symbols**. It documents
 * the rationale and teaching goals behind `examples/users_crud.cpp`, and serves
 * as a guide for contributors who evolve the example or the underlying ORM.
 *
 * ───────────────────────────────────────────────────────────────────────────
 *  Contents
 * ───────────────────────────────────────────────────────────────────────────
 *  1) Teaching goals of the example
 *  2) Minimal domain model & Mapper<T>
 *  3) Repository wiring & SQL safety
 *  4) Connection management (pool) & transactional hints
 *  5) Error handling policy
 *  6) Environment & schema pre-conditions
 *  7) Extending the example (roadmap)
 *
 * ----------------------------------------------------------------------------
 * 1) Teaching goals of the example
 * ----------------------------------------------------------------------------
 *  The example is deliberately small and focused:
 *    - Show how to define an entity struct (`User`).
 *    - Show how to specialize `Mapper<User>` with `toInsertParams` and `toUpdateParams`.
 *    - Demonstrate how to acquire a connection via `ConnectionPool`.
 *    - Demonstrate a simple `INSERT` with `BaseRepository<User>::create`.
 *
 *  It does NOT include:
 *    - Reading back rows (ResultSet adapter is introduced later).
 *    - Dynamic WHERE clauses, pagination, or joins (covered in advanced samples).
 *
 * ----------------------------------------------------------------------------
 * 2) Minimal domain model & Mapper<T>
 * ----------------------------------------------------------------------------
 *  The domain struct is a plain struct with public fields; no macros or intrusive
 *  base classes are required (optional `Entity` marker exists for clarity).
 *
 *  `Mapper<User>` responsibilities in the example:
 *    - `toInsertParams(const User&)`: list of (column, value) for INSERT.
 *    - `toUpdateParams(const User&)`: similar list for UPDATE.
 *    - `fromRow(const ResultRow&)`: intentionally left unimplemented here to keep
 *      the early example focused; implemented in later ResultSet-centric examples.
 *
 *  Invariants:
 *    - `toInsertParams` must exclude auto-increment `id`.
 *    - Column names must match the DB schema exactly (no escaping done by repo).
 *    - Values must be driver-supported `std::any` payloads (int, int64_t, unsigned,
 *      double, float, bool, const char*, std::string).
 *
 * ----------------------------------------------------------------------------
 * 3) Repository wiring & SQL safety
 * ----------------------------------------------------------------------------
 *  `BaseRepository<T>` constructs parameterized SQL:
 *    INSERT INTO <table> (col1,col2,...) VALUES (?, ?, ...)
 *  It binds parameters in the order returned by `Mapper<T>::toInsertParams`.
 *
 *  Security model:
 *    - All *values* are passed through `?` placeholders (safe binding).
 *    - Table and column identifiers are assumed trusted (application-owned).
 *
 * ----------------------------------------------------------------------------
 * 4) Connection management (pool) & transactional hints
 * ----------------------------------------------------------------------------
 *  The example uses `ConnectionPool` → `PooledConn` automatically via the repo.
 *  For multi-step atomic flows, contributors should encourage explicit
 *  transactional scopes:
 *
 *    @code
 *    Transaction tx(pool);
 *    auto& c = tx.conn();
 *    auto st = c.prepare("INSERT ...");
 *    // ...
 *    tx.commit();
 *    @endcode
 *
 *  or use `UnitOfWork` for grouping multiple repo calls:
 *
 *    @code
 *    UnitOfWork uow(pool);
 *    // repoA(uow.conn()).create(...);
 *    // repoB(uow.conn()).update(...);
 *    uow.commit();
 *    @endcode
 *
 * ----------------------------------------------------------------------------
 * 5) Error handling policy
 * ----------------------------------------------------------------------------
 *  - Driver failures propagate as `DBError` (see Errors.hpp).
 *  - The example catches `std::exception` at `main` and prints a concise error.
 *  - Contributors should avoid exposing secrets in error messages (no DSNs).
 *
 * ----------------------------------------------------------------------------
 * 6) Environment & schema pre-conditions
 * ----------------------------------------------------------------------------
 *  The example assumes:
 *    - A running MySQL instance (default host: `tcp://127.0.0.1:3306`).
 *    - Credentials passed via CLI or using defaults (root / empty password).
 *    - A database `vixdb` (or provided via argv) and a `users` table:
 *
 *      CREATE TABLE users (
 *        id BIGINT AUTO_INCREMENT PRIMARY KEY,
 *        name  VARCHAR(255) NOT NULL,
 *        email VARCHAR(255) NOT NULL,
 *        age   INT NOT NULL
 *      ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
 *
 *  Build & run:
 *    cmake -S .. -B build -DVIX_ORM_BUILD_EXAMPLES=ON -DVIX_ORM_USE_MYSQL=ON
 *    cmake --build build -j
 *    ./build/vix_orm_users tcp://127.0.0.1:3306 root "" vixdb
 *
 * ----------------------------------------------------------------------------
 * 7) Extending the example (roadmap)
 * ----------------------------------------------------------------------------
 *  Short term:
 *    - Implement `MySQLResultSet`/`MySQLResultRow` and switch the example to:
 *        auto u = repo.findById(id);
 *        std::cout << u->name << "\n";
 *    - Add an UPDATE and DELETE example.
 *
 *  Medium term:
 *    - Introduce `QueryBuilder` usage for WHERE clauses and pagination:
 *        qb.raw("SELECT id,name FROM users WHERE age >= ?").param(18);
 *    - Show `UnitOfWork` coordinating multiple repository actions.
 *
 *  Long term:
 *    - Demonstrate batch inserts (multi-values) and upserts (ON DUPLICATE KEY).
 *    - Add a migration-driven setup in examples (MigrationsRunner + migrations).
 *    - Provide optional logging via spdlog (timings, affected rows).
 */

// No includes or code needed; this TU is documentation-only.
