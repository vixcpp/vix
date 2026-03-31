# Performance

Vix is designed for predictable, high-performance networking.

Performance in Vix comes from:

-   Explicit control
-   Minimal abstraction layers
-   No garbage collection
-   No hidden middleware chains

------------------------------------------------------------------------

## Minimal overhead model

A typical Vix request flow is:

Request → Route match → Handler → Response → End

There are no:

-   Reflection systems
-   Dynamic runtime inspection layers
-   Hidden serialization pipelines

Everything you write is what executes.

------------------------------------------------------------------------

## Minimal example

``` cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request&, Response& res) {
    res.json({"message", "Fast and explicit"});
  });

  app.run(8080);
}
```

The handler is a simple lambda. No extra runtime layers are involved.

------------------------------------------------------------------------

## Avoiding performance pitfalls

### 1. Avoid heavy allocations in hot paths

Instead of:

``` cpp
std::string large = some_expensive_operation();
res.json({"data", large});
```

Prefer precomputed or cached data when possible.

------------------------------------------------------------------------

### 2. Keep handlers small

Handlers should:

-   Parse input
-   Perform minimal logic
-   Send response

Move heavy logic into dedicated components.

------------------------------------------------------------------------

### 3. Avoid unnecessary JSON depth

Deeply nested objects increase serialization cost.

Prefer structured but reasonable JSON layouts.

------------------------------------------------------------------------

## Hardware considerations

For production workloads:

-   Use multi-core CPUs
-   Use fast storage (NVMe preferred)
-   Avoid memory pressure
-   Use proper OS tuning

Vix itself does not impose artificial limits. Hardware and architecture
choices matter.

------------------------------------------------------------------------

## Throughput mindset

To maximize throughput:

-   Minimize blocking operations
-   Avoid unnecessary copying
-   Reuse resources when possible
-   Keep I/O efficient

Example of lightweight route:

``` cpp
app.get("/ping", [](Request&, Response& res) {
  res.json({"message", "pong"});
});
```

This route has near-zero overhead.

------------------------------------------------------------------------

## Benchmarking rule

Always benchmark your real workload.

Synthetic benchmarks may not reflect:

-   Real traffic patterns
-   Database latency
-   External API delays

Measure what matters.

------------------------------------------------------------------------

## Philosophy

Performance in Vix is not magic.

It comes from:

-   Simplicity
-   Explicit behavior
-   Predictable execution
-   Modern C++ efficiency

If you write efficient C++, Vix stays efficient.

