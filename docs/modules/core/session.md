# Session (core)

The **Session** component represents a single client connection.

It is responsible for:

-   Reading HTTP requests
-   Enforcing limits (body size, timeouts)
-   Applying basic WAF checks
-   Dispatching to the Router
-   Handling heavy vs light routes
-   Writing HTTP responses
-   Managing keep-alive connections

------------------------------------------------------------------------

## Architecture Role

The execution chain is:

TCP Socket\
→ Session\
→ Router\
→ Handler\
→ Response

Each client connection owns one `Session` instance.

------------------------------------------------------------------------

## Responsibilities

### 1. Asynchronous Request Reading

Session uses Boost.Beast async read to:

-   Parse HTTP headers and body
-   Enforce body size limits
-   Handle connection resets safely

Maximum request body size (default):

10 MB

This protects the server from memory abuse.

------------------------------------------------------------------------

### 2. Timeout Protection

Each request starts a timer:

-   Configurable via `Config`
-   Prevents stalled connections
-   Automatically closes the socket if expired

This avoids slowloris-style attacks.

------------------------------------------------------------------------

### 3. Basic WAF (Web Application Firewall)

Session performs lightweight request filtering before routing.

Modes:

-   off
-   basic
-   strict

Checks include:

-   Suspicious SQL keywords (SELECT, UNION, DROP, etc.)
-   Script injection patterns
-   Body size validation
-   Target length validation
-   Control character rejection

In benchmark mode, WAF can be disabled.

This is not a full security layer, but a protective filter.

------------------------------------------------------------------------

## Light vs Heavy Routes

Session asks the router:

    router.is_heavy(request)

If route is NOT heavy:

-   Executed immediately in I/O thread

If route IS heavy:

-   Posted to configured executor
-   Prevents blocking I/O threads
-   Allows CPU/DB isolation

If executor is saturated:

-   Returns 503 Service Unavailable

This separation protects latency.

------------------------------------------------------------------------

## Keep-Alive Handling

If request uses keep-alive:

-   Session keeps socket open
-   Parses next request

If not:

-   Socket is gracefully closed

HEAD requests are handled properly.

------------------------------------------------------------------------

## Response Flow

All responses:

-   Are written asynchronously
-   Use strand to ensure serialized writes
-   Respect keep-alive headers
-   Automatically set connection field

Error responses are standardized.

------------------------------------------------------------------------

## Error Handling Guarantees

Session guarantees:

-   No uncaught exceptions escape async callbacks
-   Graceful shutdown on read/write errors
-   Proper socket close on failure
-   No double writes (strand protected)

------------------------------------------------------------------------

## Security Safeguards

Session protects against:

-   Oversized payloads
-   Malformed requests
-   Suspicious body patterns
-   Control character injection
-   Basic SQL/XSS injection attempts

It is a first defense layer, not a full firewall.

------------------------------------------------------------------------

## Thread Safety

-   Writes are serialized using `asio::strand`
-   Heavy route execution is offloaded to executor
-   No shared mutable state across sessions

Each Session is independent.

------------------------------------------------------------------------

## Performance Model

Fast path (light routes):

-   No thread hop
-   Direct router dispatch
-   Minimal overhead

Heavy path:

-   Executor dispatch
-   Safe isolation from I/O threads

Timeout and body limit checks are constant-time.

------------------------------------------------------------------------

## When To Use

Session is automatically used by HTTPServer.

You rarely instantiate it manually.

It exists to:

-   Encapsulate connection lifecycle
-   Protect I/O threads
-   Provide safe async boundaries

------------------------------------------------------------------------

## Summary

Session is the per-connection runtime engine of Vix.

It provides:

-   Async parsing
-   WAF filtering
-   Heavy route offloading
-   Timeout enforcement
-   Safe response writing
-   Keep-alive support

It forms the execution boundary between the network layer and
application logic.

