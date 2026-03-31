# Router (core)

The **Vix Router** is a lightweight HTTP route matcher and dispatcher.

It is designed for:

-   High-performance method + path matching
-   Tree-based routing (segment by segment)
-   Path parameters (`/users/{id}`)
-   HEAD → GET fallback
-   Automatic 404 handling
-   Optional OpenAPI metadata
-   Heavy route scheduling hints

------------------------------------------------------------------------

## Design Philosophy

The router is:

-   Deterministic
-   Allocation-conscious
-   Tree-backed (not regex-based)
-   HTTP verb aware
-   Compatible with OpenAPI generation

It separates:

-   Routing (matching)
-   Handling (IRequestHandler)
-   Scheduling hints (`heavy` routes)
-   Documentation metadata (`RouteDoc`)

------------------------------------------------------------------------

## Basic Route Registration

``` cpp
router.add_route(
  http::verb::get,
  "/users",
  handler_ptr
);
```

You can also attach options and documentation:

``` cpp
router.add_route(
  http::verb::post,
  "/users/{id}",
  handler_ptr,
  RouteOptions{ .heavy = true },
  RouteDoc{
    .summary = "Update user",
    .description = "Updates user profile by ID"
  }
);
```

------------------------------------------------------------------------

## Route Tree Model

Internally, the router builds a **segment tree**.

Example:

    GET/users/{id}

Is stored as:

    root
     └── GET
          └── users
               └── *

Parameter segments are stored as `"*"` internally.

------------------------------------------------------------------------

## Path Parameters

A segment written as:

    /users/{id}

Is marked as:

-   `isParam = true`
-   `paramName = "id"`

The router matches dynamic segments safely without regex overhead.

------------------------------------------------------------------------

## Heavy Routes

Routes can be marked as:

``` cpp
RouteOptions{ .heavy = true }
```

This allows:

-   Dedicated executor scheduling
-   CPU/DB-intensive isolation
-   Runtime scheduling decisions

You can check at runtime:

``` cpp
router.is_heavy(request);
```

------------------------------------------------------------------------

## HEAD Fallback

If a `HEAD` request is received and no HEAD route exists:

-   The router automatically attempts to match `GET`
-   The response body is stripped
-   Content-Length is preserved

This follows standard HTTP semantics.

------------------------------------------------------------------------

## OPTIONS Handling

If no explicit OPTIONS route is defined:

-   Router returns `204 No Content`
-   Connection is closed
-   Empty body

------------------------------------------------------------------------

## 404 Handling

If no route matches:

-   Custom handler (if defined) is used
-   Otherwise, default JSON response:

``` json
{
  "error": "Route not found",
  "method": "GET",
  "path": "/unknown"
}
```

------------------------------------------------------------------------

## OpenAPI Integration

Each route can register metadata via `RouteDoc`:

-   summary
-   description
-   tags
-   request_body schema
-   responses
-   vendor extensions

You can retrieve all registered routes:

``` cpp
router.routes();
```

This enables:

-   OpenAPI generation
-   Runtime validation
-   Documentation tools

------------------------------------------------------------------------

## Matching Rules

Matching priority:

1.  Exact segment match
2.  Parameter match (`*`)
3.  Otherwise → no match

Method is part of the tree key (`GET/users` is different from
`POST/users`).

------------------------------------------------------------------------

## Guarantees

The Router guarantees:

-   Single-pass matching
-   No regex overhead
-   Deterministic behavior
-   Safe handling of HEAD and OPTIONS
-   Query string stripping during match
-   Clean separation between routing and execution

------------------------------------------------------------------------

## When To Use

Use Router when:

-   Building REST APIs
-   Generating OpenAPI specs
-   Managing heavy vs light routes
-   Requiring deterministic high-performance matching

------------------------------------------------------------------------

## Summary

The Vix Router is:

-   Tree-based
-   Verb-aware
-   Parameter-capable
-   Documentation-ready
-   Scheduling-aware

It forms the backbone of the Vix HTTP runtime.

