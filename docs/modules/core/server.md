# HTTP Server Guide (vix::server::HTTPServer)

This guide explains how the Vix HTTP server works, how it connects to the Router and Executor, and how to run and stop it cleanly.

This guide matches the current code in:
- `vix/server/HTTPServer.hpp`
- `vix/server/HTTPServer.cpp`

---

## What HTTPServer is

`vix::server::HTTPServer` is an asynchronous HTTP server built on Boost.Asio and Boost.Beast.

It is responsible for:
- Binding a TCP acceptor on the configured port
- Accepting connections asynchronously
- Creating a `vix::session::Session` for each client socket
- Running the `io_context` on a pool of I/O threads
- Exposing a shared `vix::router::Router` used to dispatch requests
- Periodically reporting executor metrics

The server delegates request parsing and per-connection HTTP handling to `vix::session::Session`.

---

## Key dependencies

### Boost
- `boost::asio` provides the I/O event loop (`net::io_context`), TCP sockets, acceptor, and async accept.
- `boost::beast::http` provides HTTP request/response types.

### Vix components
- `vix::router::Router`: matches method + path and invokes the registered handler.
- `vix::executor::IExecutor`: schedules work (used by sessions and for timers/metrics).
- `vix::session::Session`: handles one client connection and drives HTTP reads/writes.

---

## Public API

### Constructor
```cpp
explicit HTTPServer(vix::config::Config &config,
                    std::shared_ptr<vix::executor::IExecutor> exec);
```

What it does:
- Stores references to `Config` and the executor
- Creates `io_context`
- Creates an empty `Router`
- Installs a default JSON 404 handler via `router_->setNotFoundHandler(...)`
  - Special case: if the request method is `HEAD`, it returns no body

### run()
Starts the server:
- Initializes and binds the acceptor if not yet open
- Starts the async accept loop
- Starts periodic metrics reporting
- Starts I/O worker threads that call `io_context_->run()`

Important:
- `run()` does not block. It starts threads and returns.

### start_accept()
Starts an async accept operation:
- Creates a `tcp::socket`
- Calls `acceptor_->async_accept(...)`
- On success, calls `handle_client(socket)`
- If stop is not requested, schedules the next accept

### calculate_io_thread_count()
Thread count policy:
- If `config_.getIOThreads()` is set and > 0, it forces that value
- Otherwise it uses `std::thread::hardware_concurrency()` (minimum 1)

### getRouter()
Returns the shared router:
```cpp
std::shared_ptr<vix::router::Router> getRouter();
```

Use this to register routes before calling `run()`.

### monitor_metrics()
Schedules a recurring timer using the executor:
- Every 5 seconds, reads `executor_->metrics()`
- Logs: pending, active, timed_out

This uses:
```cpp
vix::timers::interval(*executor_, std::chrono::seconds(5), ...);
```

### stop_async()
Requests shutdown:
- Sets `stop_requested_ = true`
- Closes acceptor (if open)
- Stops the io_context

This stops accepting new connections and causes `io_context_->run()` to return on I/O threads.

### stop_blocking()
Stops and blocks until clean:
- `executor_->wait_idle()`
- `join_threads()`

Note:
- In the current implementation, `stop_blocking()` does not call `stop_async()`.
  You typically call `stop_async()` first, then `stop_blocking()`.

### join_threads()
Joins all I/O worker threads.

### bound_port()
Returns the actual bound port:
- Useful when config port is `0` (ephemeral port)
- Returns `0` if not bound yet

---

## Internal flow

### 1) Acceptor initialization
`init_acceptor(port)`:
- `open()`
- `set_option(reuse_address=true)`
- `bind()`
- `listen()`
- stores the effective port in `bound_port_` using `local_endpoint().port()`

Special handling:
- If bind fails with address-in-use, a clearer exception message is thrown.

### 2) Accept loop
`start_accept()` calls `async_accept` and re-arms itself.

Stop logic:
- If `stop_requested_` is true, the loop stops re-arming accepts.

### 3) Session creation
When a socket is accepted:
- `handle_client(socket)` creates a `vix::session::Session`
- It passes:
  - socket
  - router
  - config
  - executor
- Calls `session->run()`

All request routing happens inside the session using the router.

### 4) I/O threads
`start_io_threads()`:
- Spawns N threads
- Each thread calls `io_context_->run()`

Linux only:
- `set_affinity(i)` pins each I/O thread to a CPU using `pthread_setaffinity_np`

---

## Typical usage

### Setup and run
Pseudo example:
```cpp
vix::config::Config cfg;
auto exec = vix::experimental::make_threadpool_executor(4, 32, 0);

vix::server::HTTPServer server(cfg, exec);

auto r = server.getRouter();
r->add_route(boost::beast::http::verb::get, "/health",
             std::make_shared<vix::vhttp::RequestHandler<...>>(...));

server.run();

// keep main alive, or wait on a signal
```

### Graceful shutdown
A typical pattern:
1) request stop
2) wait for threads to finish

```cpp
server.stop_async();
server.stop_blocking();
```

---

## Notes and gotchas

### Port validation
In `run()`:
- If port is not 0 and < 1024, or > 65535, it throws.

That means:
- privileged ports (<1024) are intentionally rejected unless the config uses port 0 or >=1024.

### HEAD requests
Not found handler:
- For `HEAD`, it does not write a body.

Router layer:
- The router already has special handling for `HEAD` by trying to match `GET` if no `HEAD` route exists.
- The not-found handler still treats HEAD separately.

### stop_blocking() does not stop
Current behavior:
- `stop_blocking()` only waits for the executor to become idle and joins threads.
- It does not close the acceptor or stop the io_context.
- Call `stop_async()` before `stop_blocking()` if you want the threads to exit.

### close_socket()
There is a helper to shutdown and close a socket safely, but it is not used inside `HTTPServer.cpp` right now.
Sessions likely manage their own socket lifecycle.

---

## How to extend

### Custom 404
Replace the default:
```cpp
server.getRouter()->setNotFoundHandler([](auto& req, auto& res) {
  // build your response
});
```

### Dedicated executors (heavy routes)
Heavy routes are detected and stored in `Router` nodes as `heavy`.
A session or dispatcher can use:
- `router.is_heavy(req)` to decide which executor to schedule on.

---

## Checklist for production

- Set `config_.getIOThreads()` explicitly if you need predictable I/O thread count.
- Make sure your executor is sized for your workload (CPU-bound vs I/O-bound).
- Always implement a signal handler to call `stop_async()` then `stop_blocking()`.
- Ensure logging level is appropriate (metrics logs currently use Debug).

