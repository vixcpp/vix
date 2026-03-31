# Vix DB Production Guide

This guide explains how to use the Vix C++ DB module safely in
production environments.

It focuses on:

-   Connection pooling strategy
-   Transactions best practices
-   Error handling
-   Migrations strategy
-   Performance recommendations
-   Security considerations

# 1. Connection Pooling Strategy

## Recommended Settings

``` cpp
cfg.mysql.pool.min = 2;
cfg.mysql.pool.max = 16;
```

Guidelines:

-   Small apps: 2--4 connections
-   Medium apps: 8--16 connections
-   High traffic APIs: tune based on load testing

Why pooling matters:

-   Avoids connection creation overhead
-   Improves latency
-   Reduces database stress

Never create Database per request. Create it once at application
startup.

# 2. Transaction Best Practices

Always use RAII transactions:

``` cpp
Transaction tx(db.pool());
// work
tx.commit();
```

Rules:

-   Keep transactions short
-   Do not perform network calls inside transactions
-   Commit explicitly
-   Let rollback happen automatically on exceptions

# 3. Error Handling

Always wrap DB logic in try/catch:

``` cpp
try {
    Transaction tx(db.pool());
    // DB logic
    tx.commit();
} catch (const std::exception& e) {
    log_error(e.what());
}
```

Never expose raw DB errors to HTTP clients.

# 4. Migrations in Production

Strategy:

-   Use versioned migrations
-   Never modify old migrations
-   Always add new migration files

Recommended flow:

1.  Deploy code
2.  Run migrations
3.  Restart services if needed

Keep migrations idempotent.

# 5. Performance Optimization

Use prepared statements everywhere.

Bad:

``` cpp
"SELECT * FROM users WHERE id = " + id
```

Good:

``` cpp
st->bind(1, id);
```

Add proper DB indexes:

-   Index foreign keys
-   Index frequently filtered columns
-   Avoid over-indexing

Measure before optimizing.

# 6. Security Best Practices

-   Never store plaintext passwords
-   Use environment variables for DB credentials
-   Limit DB user privileges
-   Use TLS connections in production
-   Rotate credentials periodically

# 7. Deployment Checklist

Before production:

-   Pool size tuned
-   Indexes verified
-   Migrations tested
-   Error logging enabled
-   Backups configured
-   Monitoring enabled

# Final Advice

Database is critical infrastructure.

Treat it as:

-   Stateful
-   Sensitive
-   Performance-critical

Design carefully. Test under load. Monitor continuously.

Vix DB gives you control. Production reliability depends on your
discipline.

