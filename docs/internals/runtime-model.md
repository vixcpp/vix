# Runtime Model

The Vix runtime model has two sides: developer runtime (what happens when you run commands) and application runtime (what happens when the compiled app is running).

```txt
command → runtime decision → build or direct compile → application starts
→ event loop / server loop → shutdown
```

## Runtime layers

```txt
┌─────────────────────────────────────┐
│ CLI runtime                         │
│ command parsing, mode detection      │
└─────────────────────────────────────┘
                ↓
┌─────────────────────────────────────┐
│ Build runtime                       │
│ direct compile, cache, CMake fallback│
└─────────────────────────────────────┘
                ↓
┌─────────────────────────────────────┐
│ Application runtime                 │
│ main(), App, routes, services        │
└─────────────────────────────────────┘
                ↓
┌─────────────────────────────────────┐
│ Server/runtime modules              │
│ HTTP, WebSocket, async, sync, p2p    │
└─────────────────────────────────────┘
                ↓
┌─────────────────────────────────────┐
│ Operating system                    │
│ process, files, sockets, signals     │
└─────────────────────────────────────┘
```

## Developer runtime

```txt
vix command → read arguments → detect mode → resolve build strategy
→ compile or build → run binary → capture and format errors
```

## Mode detection

| Mode     | Trigger                         | Optimized for                          |
|----------|---------------------------------|----------------------------------------|
| Script   | A single `.cpp` file.           | Learning, examples, and small tools.   |
| Project  | `CMakeLists.txt`, `vix.json`, or `src/`. | Real multi-file applications. |
| Manifest | An `app.vix` file.              | Stable application descriptions.       |

## Runtime arguments vs compiler flags

```bash
--    = compiler or linker flags
--run = arguments passed to your program

vix run main.cpp -- -O2 -DNDEBUG      # compiler flags
vix run main.cpp --run server          # runtime args
vix run main.cpp --run client 127.0.0.1 9101  # correct
vix run main.cpp -- client 127.0.0.1 9101     # wrong
```

## Application runtime

```cpp
int main()
{
  App app;                              // create App
  configure_middlewares(app);           // register middleware before routes
  register_system_routes(app);
  register_user_routes(app);
  app.run(8080);                        // start server — blocks until shutdown
  return 0;
}
```

## Request lifecycle

```txt
client → socket read → HTTP parse → Request object → middleware chain
→ route match → handler → Response object → HTTP write → client
```

## Middleware lifecycle

```txt
request → middleware 1 → middleware 2 → middleware 3 → route handler
```

```cpp
next();    // continue request
// or
res.status(401).json({
  "ok", false,
  "error", "unauthorized"
});
return;    // stop request
```

## Database runtime

```txt
HTTP request → route → service → repository → database → JSON response
```

## WebSocket runtime

```txt
HTTP server starts → WebSocket server starts → client connects
→ on_open → typed messages → on_close → shutdown
```

```cpp
vix::config::Config cfg{".env"};
auto executor = std::make_shared<vix::executor::RuntimeExecutor>(1u);
vix::App app{executor};
vix::websocket::Server ws{cfg, executor};
vix::run_http_and_ws(app, ws, executor, cfg);
```

## Async runtime model

```txt
create task → post coroutine handle → run scheduler → resume task
→ await timers or I/O → stop context
```

Rule: **a task starts suspended and must be scheduled.**

```cpp
// Timer — never block the event loop thread
co_await ctx.timers().sleep_for(std::chrono::milliseconds(100));

// CPU pool — offload heavy work
int value = co_await ctx.cpu_pool().submit([]() { return heavy_work(); });
```

## Sync runtime model

```txt
operation created → persisted in outbox → worker tick → transport send → done or retry
```

Outbox lifecycle:

```txt
Pending → Inflight → Done
               ↓ (retryable)
          Retry scheduled → Pending later
               ↓ (permanent)
             Failed
```

## P2P runtime model

```txt
node starts → listen socket opens → peer connects → handshake starts
→ handshake completes → peer connected → messages flow → node stops
```

## Sync + P2P together

```txt
local write → WAL → outbox → P2P message → remote peer → ack → convergence
```

Sync decides what must be delivered. P2P decides how nodes connect and exchange messages.

## Shutdown model

```txt
initialize → start → serve or process → stop → cleanup
```

```cpp
// Async runtime signal handling
auto &sig = ctx.signals();
sig.add(SIGINT);
sig.add(SIGTERM);
sig.on_signal([&](int) { ctx.stop(); });
```

## Production runtime model

```txt
systemd → Vix binary → 127.0.0.1:8080 → Nginx → HTTPS users
```

```ini
[Service]
Type=simple
WorkingDirectory=/opt/vix-app
ExecStart=/opt/vix-app/bin/app
Restart=always
Environment=SERVER_PORT=8080
Environment=SERVER_TLS_ENABLED=false
```

## Runtime design principles

1. **Fast feedback:** simple files run quickly.
2. **Application-first:** think about the application, not build details.
3. **Explicit lifecycle:** start, run, tick, stop, wait, and shutdown.
4. **Recoverability:** persisted state survives failures.
5. **Normal native deployment:** deploy as a binary with systemd and Nginx.

## Common mistakes

```bash
# Wrong — runtime args as compiler flags
vix run main.cpp -- server
# Correct
vix run main.cpp --run server

# Wrong — routes registered after server starts
app.run(8080);
app.get("/health", handler);
# Correct
app.get("/health", handler);
app.run(8080);
```

## What you should remember

Developer runtime:
- commands,
- mode detection,
- build pipeline,
- diagnostics.

Application runtime:
`App → routes → middleware → server loop → modules → shutdown`.

The core idea: **Vix makes the development loop direct, while the application remains a normal explicit C++ runtime.**

Next: [Direct Compile](/internals/direct-compile)
