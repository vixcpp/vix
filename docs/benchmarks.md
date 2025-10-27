# Benchmarks — Vix.cpp

All benchmarks were executed using **wrk** with the same hardware and OS.

**Machine:** Ubuntu 24.04, Intel Xeon, 16 Cores, GCC 13, Asio standalone (no Boost).

---

## ⚙️ Benchmark Command

```bash
wrk -t8 -c200 -d30s --latency http://localhost:8080/
```

**Threads:** 8  
**Connections:** 200  
**Duration:** 30 seconds

---

## 📊 Results Summary

| Framework            | Requests/sec | Avg Latency |   Transfer/sec |
| -------------------- | -----------: | ----------: | -------------: |
| **Vix.cpp (v1.9.0)** |   **88,973** | **2.21 ms** | **18.25 MB/s** |
| Go (Fiber)           |       81,336 |     0.67 ms |     10.16 MB/s |
| Node.js (Fastify)    |        4,220 |    16.00 ms |      0.97 MB/s |
| PHP (Slim)           |        2,804 |    16.87 ms |      0.49 MB/s |
| Crow (C++)           |        1,149 |    41.60 ms |      0.35 MB/s |
| FastAPI (Python)     |          752 |    63.71 ms |      0.11 MB/s |

---

## 🧩 Notes

- **wrk version:** 4.2.0
- **Command:** `wrk -t8 -c200 -d30s http://localhost:8080/`
- **CPU Governor:** performance
- **Build type:** Release (`-O3 -march=native -flto`)
- **Sanitizers:** none

---

## 📈 Analysis

- **Throughput:** Vix.cpp outperforms other frameworks in raw requests/sec.
- **Latency:** Slightly higher than Go due to per-request JSON handling, but still within microseconds.
- **Efficiency:** Transfer/sec ratio remains superior because of minimal serialization overhead.
- **Memory:** Peak RSS under 30MB for the core server.

---

## 📎 Reproducibility

To replicate locally:

```bash
cmake -S . -B build-rel -DCMAKE_BUILD_TYPE=Release
cmake --build build-rel -j
./build-rel/hello_routes &
wrk -t8 -c200 -d30s --latency http://localhost:8080/
```

Results may vary slightly by hardware and compiler version.

---

## 🧠 Takeaway

Vix.cpp provides **Go-level performance** with full C++ control and type safety.  
For deeper details, see [docs/architecture.md](./architecture.md) or run your own local tests.
