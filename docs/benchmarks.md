# ⚡ Benchmarks (Updated — Dec 2025)

All benchmarks were executed using **wrk**  
`8 threads`, `800 connections`, for **30 seconds**, on the same machine:  
**Ubuntu 24.04 — Intel Xeon — C++20 optimized build — Logging disabled**

Results represent steady-state throughput on a simple `"OK"` endpoint.

---

## 🚀 Requests per second

| Framework                 | Requests/sec               | Avg Latency     | Transfer/sec   |
| ------------------------- | -------------------------- | --------------- | -------------- |
| ⭐ **Vix.cpp (v1.10.6)**  | **~98,942** _(pinned CPU)_ | **7.3–10.8 ms** | **~13.8 MB/s** |
| **Vix.cpp (default run)** | 81,300 – 81,400            | 9.7–10.8 ms     | ≈ 11.3 MB/s    |
| Go (Fiber)                | 81,336                     | 0.67 ms         | 10.16 MB/s     |
| **Deno**                  | ~48,868                    | 16.34 ms        | ~6.99 MB/s     |
| Node.js (Fastify)         | 4,220                      | 16.00 ms        | 0.97 MB/s      |
| PHP (Slim)                | 2,804                      | 16.87 ms        | 0.49 MB/s      |
| Crow (C++)                | 1,149                      | 41.60 ms        | 0.35 MB/s      |
| FastAPI (Python)          | 752                        | 63.71 ms        | 0.11 MB/s      |

> 🔥 **New record:** When pinned to a single core (`taskset -c 2`)  
> Vix.cpp reaches **~99k req/s**, surpassing Go and matching the fastest C++ microframeworks.

---

## 📝 Notes

### ✔ Why Vix.cpp reaches Go-level performance

- zero-cost abstractions
- custom ThreadPool tuned for HTTP workloads
- optimized HTTP pipeline
- fast-path routing
- Beast-based IO
- minimal memory allocations
- predictable threading model

---

## 🦕 Deno benchmark (reference)

```bash
$ wrk -t8 -c800 -d30s --latency http://127.0.0.1:8000
Requests/sec: 48,868.73
```

### ✔ Vix.cpp recommended benchmark mode

When benchmarking from inside the Vix.cpp repository (using the built-in example):

```bash
cd ~/vixcpp/vix
export VIX_LOG_LEVEL=critical
export VIX_LOG_ASYNC=false

# Run the optimized example server
vix run example main
```

Then, in another terminal:

```bash
wrk -t8 -c800 -d30s --latency http://127.0.0.1:8080/bench
```

If you want CPU pinning for more stable results:

```bash
taskset -c 2 ./build/main
wrk -t8 -c800 -d30s --latency http://127.0.0.1:8080/bench
```

#### 🏁 Result: ~98,942 req/s

✔ Fast-path routing gives +1–3%

## Use /fastbench to bypass RequestHandler overhead.

## 🧠 Takeaway

Vix.cpp provides **Go-level performance** with full C++ control and type safety.  
For deeper details, see [docs/architecture.md](./architecture.md) or run your own local tests.
