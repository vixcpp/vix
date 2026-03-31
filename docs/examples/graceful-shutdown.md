# Graceful Shutdown (Vix.cpp)

Graceful shutdown ensures your server:

-   Stops accepting new connections
-   Finishes in-flight requests
-   Releases resources safely
-   Shuts down cleanly without corruption

Vix provides built-in support for safe shutdown using:

-   `run()`
-   `listen()`
-   `close()`
-   `wait()`
-   `request_stop_from_signal()`

# 1. Basic Blocking Mode (run)

The simplest mode:

``` cpp
#include <vix.hpp>
using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request&, Response& res) {
    res.json({"ok", true});
  });

  app.run(8080); // blocks until stopped
}
```

Press `Ctrl+C` to stop the server.

Vix will stop the server loop cleanly.

# 2. Background Mode (listen + wait)

Use this when you need more control.

``` cpp
#include <vix.hpp>
using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request&, Response& res) {
    res.send("Running...");
  });

  app.listen(8080, [](){
    console.log("Server is running on http://localhost:8080");
  });

  app.wait(); // wait until shutdown
}
```

# 3. Programmatic Shutdown

You can stop the server from inside the application:

``` cpp
app.get("/stop", [&](Request&, Response& res) {
  res.send("Stopping...");
  app.close();
});
```

`close()` requests graceful stop. Use `wait()` if running in background
mode.

# 4. Signal-Safe Shutdown (Production Pattern)

For production systems, handle SIGINT / SIGTERM:

``` cpp
#include <csignal>
#include <vix.hpp>

using namespace vix;

static App* g_app = nullptr;

void signal_handler(int)
{
  if (g_app)
    g_app->request_stop_from_signal();
}

int main()
{
  App app;
  g_app = &app;

  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  app.get("/", [](Request&, Response& res) {
    res.json({"ok", true});
  });

  app.run(8080);
}
```

This is safe to call inside signal handlers.

# 5. Shutdown Callback

You can run cleanup logic:

``` cpp
app.set_shutdown_callback([]() {
  vix::console.info("Server shutting down...");
});
```

Useful for:

-   Closing database connections
-   Flushing logs
-   Stopping background workers
-   Releasing file handles

# 6. Production Best Practices

When building serious systems:

-   Use `listen()` + `wait()`
-   Install signal handlers
-   Avoid long blocking tasks
-   Ensure DB pools close cleanly
-   Stop background threads before exit

# 7. Complete Production Example

``` cpp
#include <csignal>
#include <vix.hpp>

using namespace vix;

static App* g_app = nullptr;

void signal_handler(int)
{
  if (g_app)
    g_app->request_stop_from_signal();
}

int main()
{
  App app;
  g_app = &app;

  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  app.set_shutdown_callback([]() {
    vix::console.info("Cleaning resources...");
  });

  app.get("/", [](Request&, Response& res) {
    res.json({"message", "Graceful server running"});
  });

  app.listen(8080);
  app.wait();

  return 0;
}
```

# Summary

Graceful shutdown in Vix is:

-   Explicit
-   Signal-safe
-   Non-blocking
-   Production-ready

Use:

-   `run()` for simple apps
-   `listen()` + `wait()` for production
-   `request_stop_from_signal()` for signal handling
-   `set_shutdown_callback()` for cleanup

