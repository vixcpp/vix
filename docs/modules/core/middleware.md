# Middleware (core)

The middleware system in Vix provides a structured way to:

- intercept requests before they reach the handler
- share data across layers (auth, logging, rate limiting, etc.)
- standardize error handling
- manage cross-cutting concerns cleanly

It is built around five core concepts:

- `Context`
- `Services`
- `Next`
- `Hooks`
- `Result` / `Error`

This document explains how they fit together.

---

## 1. Context

`vix::mw::Context` is the object passed through the middleware pipeline.

It wraps:

- the current `Request`
- the current `ResponseWrapper`
- a shared `Services` container

You use it to:

- read request data
- write responses
- access shared services
- manage request-scoped state

Example usage inside middleware:

```cpp
void my_middleware(Context& ctx, Next next) {
  auto& req = ctx.req();
  auto& res = ctx.res();

  if (!req.has_header("Authorization")) {
    ctx.send_error(401, "unauthorized", "Missing token");
    return;
  }

  next(); // continue pipeline
}
```

The context is the single source of truth for the current request lifecycle.

---

## 2. Services

`vix::mw::Services` is a lightweight dependency container.

It allows registering shared services by type:

```cpp
services.provide(std::make_shared<MyDatabase>(...));
```

And retrieving them inside middleware or handlers:

```cpp
auto db = ctx.services().get<MyDatabase>();
```

Characteristics:

- type-safe
- shared ownership via `std::shared_ptr`
- no string keys
- minimal overhead

It is designed for runtime services such as:

- database connections
- configuration
- external clients
- caches

---

## 3. Request-Scoped State

Request-scoped data should not live in global services.

Use `RequestState` through `Context`:

```cpp
ctx.emplace_state<User>(User{42, "Ada"});
```

Later in the pipeline:

```cpp
auto& user = ctx.state<User>();
```

This is ideal for:

- authenticated user info
- correlation IDs
- rate-limit metadata
- validation results

State is strongly typed and stored using `std::any` internally.

---

## 4. Next (Continuation Control)

`vix::mw::Next` is a call-once continuation object.

It guarantees that the next middleware in the chain can only be invoked once.

```cpp
void middleware(Context& ctx, Next next) {
  // pre logic
  next(); // continue
  // post logic (optional)
}
```

Important properties:

- call-once semantics
- safe against double invocation
- explicit pipeline control

If `next()` is not called, the pipeline stops.

---

## 5. Hooks

`vix::mw::Hooks` provides lifecycle integration points:

- `on_begin`
- `on_end`
- `on_error`

Hooks can be merged using `merge_hooks(...)`.

Execution order:

- `on_begin`: forward order
- `on_end`: reverse order
- `on_error`: reverse order

This ensures predictable stacking behavior similar to nested middleware layers.

Hooks are ideal for:

- logging
- metrics
- tracing
- performance measurement

---

## 6. Result and Error

`vix::mw::Result<T>` is a typed result container that holds either:

- a value of type `T`
- an `Error`

`Error` contains:

- HTTP status
- machine-readable code
- human-readable message
- optional details map

Example:

```cpp
return vix::mw::fail(401, "unauthorized", "Invalid token");
```

Or:

```cpp
return vix::mw::ok(User{...});
```

This pattern encourages explicit error handling without exceptions.

`Result<void>` is supported for side-effect operations.

---

## 7. Standard Error Model

The middleware error model standardizes JSON error responses:

```json
{
  "status": 401,
  "code": "unauthorized",
  "message": "Invalid token",
  "details": {}
}
```

Helper constructors exist for common HTTP errors:

- `bad_request()`
- `unauthorized()`
- `forbidden()`
- `not_found()`
- `conflict()`
- `internal()`
- etc.

Errors are normalized to ensure valid status codes.

---

## 8. Typical Middleware Flow

1. Request arrives.
2. Context is created.
3. `on_begin` hooks execute.
4. Middleware chain runs.
5. Handler executes.
6. `on_end` hooks execute.
7. If an error occurs, `on_error` hooks run.

This guarantees structured lifecycle behavior.

---

## Design Philosophy

The Vix middleware layer is:

- explicit
- minimal
- strongly typed
- deterministic in order
- safe under composition

It avoids:

- hidden global state
- magic injection
- implicit execution order
- runtime reflection

The result is a predictable pipeline suitable for both small services and large structured systems.

---

## When to Use Middleware

Use middleware for:

- authentication
- authorization
- request validation
- rate limiting
- logging
- tracing
- CORS handling
- request enrichment

Avoid putting business logic in middleware. Keep it in handlers.

---

## Summary

The middleware system provides:

- structured request lifecycle control
- typed dependency injection
- typed request-scoped storage
- explicit continuation control
- standardized error handling

It is designed to remain simple while scaling to complex applications.

