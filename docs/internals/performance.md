# Performance

Performance in Vix is not only about raw speed.
It is about keeping the runtime fast, predictable, observable, and safe under sustained load.

```txt
measure → identify bottleneck → optimize carefully → measure again
```

## Benchmark mode

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DVIX_BENCH_MODE=ON
cmake --build build -j
```

`VIX_BENCH_MODE=ON` removes development overhead (extra logs, debug checks, diagnostics) to measure the runtime path more clearly.
Always benchmark release builds.

## HTTP benchmark shape

```cpp
#include <vix.hpp>
using namespace vix;

int main()
{
  App app;

  app.get("/bench", [](Request &, Response &res) {
    res.text("OK");
  });

  app.get("/json", [](Request &, Response &res){
    res.json({
      "ok", true,
      "framework", "Vix.cpp",
      "route", "json"
    });
  });

  app.get("/health", [](Request &, Response &res) {
    res.json({"ok", true});
  });

  app.run(8080);

  return 0;
}
```

```bash
wrk -t8 -c800 -d30s http://127.0.0.1:8080/bench
wrk -t8 -c800 -d30s http://127.0.0.1:8080/json
```

Use separate routes for separate questions — `/bench` for raw HTTP, `/json` for serialization, `/db/users` for database path.

## Important metrics

| Metric       | What it tells you                         |
|--------------|-------------------------------------------|
| Requests/sec | Measures request throughput.              |
| Avg latency  | Shows the average response time.          |
| P99 latency  | Shows tail latency for 99% of requests.   |
| Error count  | Counts failures during the load test.     |
| Stability    | Shows behavior under sustained load.      |

P99 matters because real users feel tail latency. High throughput with bad P99 is not a good result.

## v2.4 vs v2.5 results

| Version | Requests/sec | Avg Latency  | P99 Latency  |
|---------|------------- |------------- |------------- |
| v2.4    | ~66k to 68k  | ~13 to 20 ms | ~17 to 50 ms |
| v2.5    | ~98k         | ~8.47 ms     | ~14.01 ms    |

v2.5 improves throughput, average latency, tail latency, and runtime stability on the `/bench` route.

## Performance philosophy

**Measure before optimizing.** Do not optimize because code "looks slow". The hot path — the code executed for every request — is where small improvements matter most.

## Hot path areas

```txt
socket read → HTTP parsing → Request creation → route matching → middleware chain
→ handler call → response generation → socket write
```

## Route matching

```cpp
// Correct — specific routes before wildcard
app.get("/users/search", search_handler);
app.get("/users/{id}", user_handler);
app.get("/*", fallback_handler);

// Wrong — wildcard catches everything first
app.get("/*", fallback_handler);
app.get("/users/{id}", user_handler);
```

## Middleware overhead

Apply middleware only where needed:

```cpp
app.use("/api", api_middleware);       // prefix only
app.protect("/admin", admin_auth);     // prefix only
```

Avoid global middleware that does expensive work on every route.

## Logging in benchmarks

```cpp
// Wrong — measures terminal I/O, not HTTP performance
app.get("/bench", [](Request &, Response &res) { std::cout << "request\n"; res.text("OK"); });

// Correct
app.get("/bench", [](Request &, Response &res) { res.text("OK"); });
```

## JSON overhead

JSON serialization costs CPU.
Use plain text for raw HTTP benchmarks, JSON for realistic API benchmarks — compare them separately.

## WAF and security overhead

```dotenv
WAF_MODE=basic
WAF_MAX_TARGET_LEN=4096
WAF_MAX_BODY_BYTES=1048576
```

WAF adds protection but also work on the hot path.
`VIX_BENCH_MODE=ON` gives a cleaner measurement.

## Memory and file descriptors

```bash
# Check file descriptor limit
ulimit -n

# Increase for high-concurrency services
# In systemd service file:
# LimitNOFILE=65535
```

Watch for:
- unbounded buffers,
- memory leaks,
- cache growth,
- unreleased resources.

## Benchmarking context

Always document:

```txt
- Vix version,
- build type,
- benchmark mode,
- machine,
- CPU,
- RAM,
- OS,
- benchmark tool,
- threads,
- connections,
- duration,
- route tested,

whether Nginx was used, whether TLS was used
```

```txt
direct:          wrk → Vix (measures runtime overhead)
through proxy:   wrk → Nginx → Vix (measures production behavior)
loopback:        127.0.0.1 (removes network variability)
```

## Optimization workflow

```txt
1. Choose one benchmark route
2. Run baseline
3. Change one thing
4. Run again
5. Compare throughput and P99
6. Keep change only if it improves without hurting correctness
```

## Performance and correctness

**Never sacrifice correctness for benchmark numbers.** Do not remove required validation, security checks, error handling, durability guarantees, or proper shutdown to make a number look bigger.

For reliability modules such as Sync, correctness is the performance baseline.

## Common mistakes

```bash
# Wrong — benchmarking debug build
cmake -DCMAKE_BUILD_TYPE=Debug

# Wrong — ignoring errors in results
# High req/s with many errors is not a good result

# Wrong — comparing different routes as if equal
# /bench and /db/users measure different things

# Wrong — publishing numbers without context
# Always include environment details
```

## What you should remember

```txt
measure → optimize one thing → measure again
```

Key metrics: Requests/sec, Avg latency, P99 latency, errors, stability.

Concrete numbers are stronger than marketing claims.

The core idea: **performance is not a slogan — it is measured behavior under clear conditions.**

Next: [Design Decisions](/internals/design-decisions)
