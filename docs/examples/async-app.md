# Async App

This example shows how to use Vix Async inside a Vix application.

Use this when you want an HTTP app that also runs background async work.

The goal is to keep the model clear:

```txt id="mmkz1h"
vix::App
  handles HTTP requests

vix::async::core::io_context
  runs async background work

shared application state
  connects background work to HTTP routes

cancellation
  stops the worker cleanly
```

This is different from a standalone background worker.

`background-task.md` shows an async worker by itself.

This page shows async work running next to an HTTP application.

## What this example builds

The app exposes:

```txt id="nyqi29"
GET /api/health
GET /api/status
POST /api/refresh
```

At the same time, an async worker runs in the background every second.

The worker updates shared state:

```txt id="qvoa7j"
tick count
last job result
running flag
```

The HTTP routes can read that state.

## Project structure

Create:

```txt id="ak4mqh"
async_app_demo/
└── async_app.cpp
```

Create the file:

```bash id="ufayyi"
mkdir async_app_demo
cd async_app_demo
touch async_app.cpp
```

## Source

Open:

```txt id="zj1rrd"
async_app.cpp
```

Add:

```cpp id="txntkb"
#include <atomic>
#include <chrono>
#include <csignal>
#include <exception>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <vix.hpp>
#include <vix/async.hpp>
#include <vix/middleware.hpp>
#include <vix/print.hpp>

using namespace std::chrono_literals;

struct AppState
{
  std::mutex mutex{};
  int tick_count{0};
  int last_result{0};
  bool worker_running{false};
};

static int compute_background_result(int tick)
{
  return tick * 10;
}

static void set_worker_running(
  const std::shared_ptr<AppState> &state,
  bool running)
{
  std::lock_guard<std::mutex> lock(state->mutex);
  state->worker_running = running;
}

static void update_background_state(
  const std::shared_ptr<AppState> &state,
  int tick,
  int result)
{
  std::lock_guard<std::mutex> lock(state->mutex);

  state->tick_count = tick;
  state->last_result = result;
  state->worker_running = true;
}

static vix::async::core::task<void> background_refresh_worker(
  vix::async::core::io_context &ctx,
  std::shared_ptr<AppState> state,
  vix::async::core::cancel_token token)
{
  int tick = 0;

  set_worker_running(state, true);

  while (!token.is_cancelled())
  {
    ++tick;

    try
    {
      const int result = co_await ctx.cpu_pool().submit([tick]()
      {
        return compute_background_result(tick);
      }, token);

      update_background_state(state, tick, result);

      vix::print("background refresh tick =", tick);
    }
    catch (const std::exception &ex)
    {
      if (token.is_cancelled())
        break;

      vix::eprint("background refresh error:", ex.what());
    }

    try
    {
      co_await ctx.timers().sleep_for(1s, token);
    }
    catch (const std::exception &)
    {
      if (token.is_cancelled())
        break;
    }
  }

  set_worker_running(state, false);

  vix::print("background refresh worker stopped");

  co_return;
}

static vix::async::core::task<void> stop_async_on_signal(
  vix::async::core::io_context &ctx,
  vix::async::core::cancel_source &source)
{
  ctx.signals().add(SIGINT);
  ctx.signals().add(SIGTERM);

  const int signal = co_await ctx.signals().async_wait();

  vix::print("shutdown signal received =", signal);

  source.request_cancel();

  co_await ctx.timers().sleep_for(100ms);

  ctx.stop();

  co_return;
}

static void install_middleware(vix::App &app)
{
  app.use("/api", vix::middleware::app::recovery_dev());
  app.use("/api", vix::middleware::app::request_id_dev());
  app.use("/api", vix::middleware::app::timing_dev());
  app.use("/api", vix::middleware::app::security_headers_dev());
  app.use("/api", vix::middleware::app::rate_limit_dev());
}

static void register_routes(
  vix::App &app,
  const std::shared_ptr<AppState> &state,
  vix::async::core::io_context &async_ctx)
{
  app.get("/", [](vix::Request &, vix::Response &res)
  {
    res.text("Async App example. Try /api/status.");
  });

  app.get("/api/health", [](vix::Request &, vix::Response &res)
  {
    res.json({
      "ok", true,
      "service", "async-app"
    });
  });

  app.get("/api/status", [state](vix::Request &, vix::Response &res)
  {
    int tick_count = 0;
    int last_result = 0;
    bool worker_running = false;

    {
      std::lock_guard<std::mutex> lock(state->mutex);

      tick_count = state->tick_count;
      last_result = state->last_result;
      worker_running = state->worker_running;
    }

    res.json({
      "ok", true,
      "worker_running", worker_running,
      "tick_count", tick_count,
      "last_result", last_result
    });
  });

  app.post("/api/refresh", [state, &async_ctx](vix::Request &, vix::Response &res)
  {
    async_ctx.post([state]()
    {
      std::lock_guard<std::mutex> lock(state->mutex);

      state->tick_count += 1;
      state->last_result = state->tick_count * 100;
      state->worker_running = true;
    });

    res.status(202).json({
      "ok", true,
      "queued", true
    });
  });
}

static void run_async_runtime(
  vix::async::core::io_context &ctx)
{
  ctx.run();
  ctx.shutdown();
}

int main()
{
  auto state = std::make_shared<AppState>();

  vix::async::core::io_context async_ctx;
  vix::async::core::cancel_source cancel_source;

  auto worker =
    background_refresh_worker(
      async_ctx,
      state,
      cancel_source.token()
    );

  auto signal =
    stop_async_on_signal(
      async_ctx,
      cancel_source
    );

  std::move(worker).start(async_ctx.get_scheduler());
  std::move(signal).start(async_ctx.get_scheduler());

  std::thread async_thread([&async_ctx]()
  {
    run_async_runtime(async_ctx);
  });

  vix::App app;

  install_middleware(app);
  register_routes(app, state, async_ctx);

  app.run(8080);

  cancel_source.request_cancel();
  async_ctx.stop();

  if (async_thread.joinable())
    async_thread.join();

  return 0;
}
```

## Run it

Run:

```bash id="gz5f5l"
vix run async_app.cpp
```

The HTTP server listens on:

```txt id="texvql"
http://127.0.0.1:8080
```

The async worker starts in a separate runtime thread.

## Test health

```bash id="xq9w38"
curl -i http://127.0.0.1:8080/api/health
```

Expected body:

```json id="begh7f"
{
  "ok": true,
  "service": "async-app"
}
```

## Test background status

```bash id="sl9pjs"
curl -i http://127.0.0.1:8080/api/status
```

Expected body shape:

```json id="mdw6gr"
{
  "ok": true,
  "worker_running": true,
  "tick_count": 3,
  "last_result": 30
}
```

Call it again after a few seconds:

```bash id="tvki8p"
curl -i http://127.0.0.1:8080/api/status
```

You should see `tick_count` and `last_result` changing.

## Trigger one background update from HTTP

```bash id="ix3mcs"
curl -i \
  -X POST http://127.0.0.1:8080/api/refresh
```

Expected status:

```txt id="hh2du2"
202 Accepted
```

Expected body:

```json id="b5947i"
{
  "ok": true,
  "queued": true
}
```

This route does not run the work directly inside the HTTP handler.

It schedules work into the async runtime:

```cpp id="hgjwb7"
async_ctx.post([state]()
{
  std::lock_guard<std::mutex> lock(state->mutex);

  state->tick_count += 1;
  state->last_result = state->tick_count * 100;
  state->worker_running = true;
});
```

That keeps the handler small.

## Why use a separate async runtime thread

`vix::App::run(...)` blocks while the HTTP server is running.

`io_context::run()` also blocks while async work is running.

So this example runs the async context in another thread:

```cpp id="y7kv1s"
std::thread async_thread([&async_ctx]()
{
  run_async_runtime(async_ctx);
});
```

The HTTP app runs on the main thread:

```cpp id="k6kapl"
app.run(8080);
```

The model is:

```txt id="c4hfjx"
main thread
  HTTP server

async thread
  background worker
  timers
  CPU pool tasks
  signal watcher
```

This keeps the example simple and explicit.

## Shared state

The HTTP routes and async worker share:

```cpp id="gpwclq"
std::shared_ptr<AppState>
```

The state is protected by a mutex:

```cpp id="xeimqa"
struct AppState
{
  std::mutex mutex{};
  int tick_count{0};
  int last_result{0};
  bool worker_running{false};
};
```

The worker writes the state:

```cpp id="y7vj3y"
update_background_state(state, tick, result);
```

The HTTP route reads the state:

```cpp id="r937ms"
std::lock_guard<std::mutex> lock(state->mutex);

tick_count = state->tick_count;
last_result = state->last_result;
worker_running = state->worker_running;
```

Use this pattern for small shared status.

For larger systems, prefer a service class.

## Better structure for bigger apps

For bigger applications, do not keep everything in one file.

Use a service:

```txt id="yr6t65"
AsyncRuntime
  owns io_context
  owns cancel_source
  owns thread
  starts workers
  stops workers

AppState
  owns shared state

RouteRegistry
  registers HTTP routes

AppBootstrap
  wires everything together
```

The single-file example is useful for learning.

A production app should move this logic into classes.

## Why the worker uses timers

The worker waits with:

```cpp id="ixhr3i"
co_await ctx.timers().sleep_for(1s, token);
```

This is better than:

```cpp id="x9ij6d"
std::this_thread::sleep_for(1s);
```

because the coroutine sleep does not block the scheduler thread.

The async runtime can still process posted callbacks and other async work.

## Why the worker uses the CPU pool

The worker offloads work to:

```cpp id="at5llk"
ctx.cpu_pool().submit(...)
```

Example:

```cpp id="dvufyo"
const int result = co_await ctx.cpu_pool().submit([tick]()
{
  return compute_background_result(tick);
}, token);
```

Use the CPU pool when the work may be:

```txt id="nd5jhn"
CPU-heavy
blocking
synchronous
slow enough to affect the scheduler
```

For tiny work, direct computation is fine.

For real backend work, avoid blocking the scheduler thread.

## Cancellation

The app creates:

```cpp id="yy7i36"
vix::async::core::cancel_source cancel_source;
```

The worker receives:

```cpp id="fgcoyw"
cancel_source.token()
```

When the app shuts down, cancellation is requested:

```cpp id="mfqoh5"
cancel_source.request_cancel();
```

The worker checks:

```cpp id="q08mwr"
while (!token.is_cancelled())
```

and cancellable waits use the token:

```cpp id="r1xrza"
co_await ctx.timers().sleep_for(1s, token);
```

The rule is:

```txt id="k1yu8s"
cancel_source requests shutdown
cancel_token lets tasks observe shutdown
```

## Signal handling

The async runtime listens for:

```cpp id="aniv8c"
SIGINT
SIGTERM
```

with:

```cpp id="m49qp3"
ctx.signals().add(SIGINT);
ctx.signals().add(SIGTERM);
```

Then waits:

```cpp id="nyscgo"
const int signal = co_await ctx.signals().async_wait();
```

When a signal is received:

```cpp id="lm1i7e"
source.request_cancel();
```

This is useful for terminal shutdown:

```txt id="puc7tf"
Ctrl+C
```

and process managers:

```txt id="fhxijy"
SIGTERM
```

## Shutdown flow

The shutdown flow is:

```txt id="svcwa8"
Ctrl+C
  -> signal task wakes up
  -> cancellation requested
  -> worker observes cancellation
  -> async context stops
  -> async thread joins
  -> process exits
```

The code also handles the case where `app.run(...)` returns normally:

```cpp id="i67y3h"
cancel_source.request_cancel();
async_ctx.stop();

if (async_thread.joinable())
  async_thread.join();
```

This ensures the async thread is not left running.

## HTTP handlers should not block

Avoid this inside a route:

```cpp id="ath55m"
app.post("/api/refresh", [](Request &, Response &res)
{
  expensive_work();

  res.json({
    "ok", true
  });
});
```

Prefer scheduling the work:

```cpp id="jn9vn0"
app.post("/api/refresh", [&async_ctx, state](Request &, Response &res)
{
  async_ctx.post([state]()
  {
    // background update
  });

  res.status(202).json({
    "ok", true,
    "queued", true
  });
});
```

This makes the route respond quickly.

## When to use this pattern

Use this pattern when the app needs:

```txt id="f9nmci"
periodic refresh
background cleanup
cache warming
sync loop
metrics collection
queue polling
file watching
maintenance jobs
```

Examples:

```txt id="btzbo6"
refresh product recommendations every minute
clean expired sessions every 10 minutes
sync local data with remote storage
send queued notifications
calculate metrics snapshots
```

## When not to use this pattern

Do not use this pattern for everything.

For a simple HTTP API with no background work, you do not need `vix::async`.

For one-off immediate work inside a handler, regular C++ code may be enough.

Use async when there is a real reason:

```txt id="ka37ep"
periodic work
delayed work
background coordination
cancellable long-running task
offloaded blocking work
signal-driven shutdown
```

## Common mistakes

### Blocking the HTTP handler

Wrong:

```cpp id="fnmffy"
app.post("/api/refresh", [](Request &, Response &res)
{
  std::this_thread::sleep_for(std::chrono::seconds(5));
  res.json({"ok", true});
});
```

Correct:

```cpp id="vgzjyd"
app.post("/api/refresh", [&async_ctx](Request &, Response &res)
{
  async_ctx.post([]()
  {
    // background work
  });

  res.status(202).json({
    "ok", true,
    "queued", true
  });
});
```

### Sharing state without synchronization

Wrong:

```cpp id="u1f0sa"
state->tick_count += 1;
```

from multiple threads without protection.

Correct:

```cpp id="ob6hpl"
std::lock_guard<std::mutex> lock(state->mutex);
state->tick_count += 1;
```

### Forgetting to stop the async runtime

If the async thread is running, stop it before exiting:

```cpp id="z8u3mz"
cancel_source.request_cancel();
async_ctx.stop();

if (async_thread.joinable())
  async_thread.join();
```

### Calling long work directly in the scheduler

Wrong:

```cpp id="dsdkmt"
async_ctx.post([]()
{
  expensive_work();
});
```

Better for expensive work:

```cpp id="q6xe1t"
co_await ctx.cpu_pool().submit([]()
{
  return expensive_work();
});
```

### Installing JSON parser globally

If a route does not need JSON, do not force it through the JSON parser.

Good:

```cpp id="ex1o7o"
app.use("/api/products", middleware::app::json_strict_dev(4096));
```

Risky:

```cpp id="yr06l3"
app.use("/api", middleware::app::json_strict_dev(4096));
```

Keep parsers route-specific.

## Production notes

For production apps, consider wrapping the async runtime.

A clean class could own:

```txt id="wze36e"
io_context
cancel_source
thread
worker task startup
shutdown logic
```

Example shape:

```cpp id="rfqhvp"
class AsyncRuntime
{
public:
  void start();
  void stop();

  vix::async::core::io_context &context();

private:
  vix::async::core::io_context ctx_;
  vix::async::core::cancel_source cancel_;
  std::thread thread_;
};
```

Then `AppBootstrap` can look like:

```txt id="guktql"
create AppState
create AsyncRuntime
start AsyncRuntime
create App
register middleware
register routes
run App
stop AsyncRuntime
```

This keeps `main()` clean.

## Complete test flow

Run:

```bash id="yxv8qx"
vix run async_app.cpp
```

Check status:

```bash id="s4dkia"
curl -i http://127.0.0.1:8080/api/status
```

Wait two seconds.

Check again:

```bash id="b4v947"
curl -i http://127.0.0.1:8080/api/status
```

Trigger manual refresh:

```bash id="xch5se"
curl -i \
  -X POST http://127.0.0.1:8080/api/refresh
```

Check status again:

```bash id="vrd834"
curl -i http://127.0.0.1:8080/api/status
```

Stop with:

```txt id="hiipef"
Ctrl+C
```

## Summary

Use `examples/background-task.md` for a standalone worker.

Use this page when async runs inside an application.

The core shape is:

```cpp id="n5yh28"
vix::async::core::io_context async_ctx;
vix::async::core::cancel_source cancel_source;

auto worker =
  background_refresh_worker(
    async_ctx,
    state,
    cancel_source.token()
  );

std::move(worker).start(async_ctx.get_scheduler());

std::thread async_thread([&async_ctx]()
{
  async_ctx.run();
  async_ctx.shutdown();
});

vix::App app;

register_routes(app, state, async_ctx);

app.run(8080);

cancel_source.request_cancel();
async_ctx.stop();
async_thread.join();
```

The mental model is:

```txt id="ra2buw"
HTTP handlers
  should stay fast

async runtime
  runs background work

shared state
  connects background work to HTTP routes

cancellation
  stops the worker cleanly
```

This gives you a practical base for using Vix Async inside real Vix applications.
