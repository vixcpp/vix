# p2p-http

Architecture-first example for **Vix P2P** with an HTTP UI/API bridge.

This example is meant to show how to expose a P2P runtime through a small HTTP surface:
- for debugging
- for admin tooling
- for a lightweight UI (served locally)

It boots:
- a P2P node (`P2PRuntime`)
- an HTTP server (`App`)
- the built-in `p2p_http` routes under `/api/p2p`
- optional static UI pages (`index.html`, `connect.html`)

## Architecture

### Components

1) **P2P runtime**
- Owns the node
- Manages peer connections and message/ping lifecycle
- Produces status/metrics/logs that can be bridged to HTTP

2) **HTTP bridge**
- A thin adapter (`p2p_http`) that exposes P2P signals as HTTP routes
- Keeps your UI and tooling independent from the internal runtime

3) **UI pages (optional)**
- Pure static HTML served locally by the HTTP server
- Talks to `/api/p2p/*` using `fetch()`

### Data flow

1) User opens `http://127.0.0.1:5178/`
2) UI calls:
   - `GET /api/p2p/status`
   - `GET /api/p2p/peers`
3) User clicks “connect” or “ping”
4) UI calls:
   - `POST or GET /api/p2p/connect` (depending on your implementation)
   - `GET /api/p2p/ping`
5) Runtime performs P2P work, then the bridge returns JSON back to the UI

This is the pattern to keep:
- P2P logic stays in the runtime
- HTTP stays a thin control plane

## Run

```bash
vix run examples/p2p-ping.cpp
```

Then open:

- `http://127.0.0.1:5178/`
- `http://127.0.0.1:5178/connect`

Or test the API:

```bash
curl -i http://127.0.0.1:5178/api/p2p/status
curl -i http://127.0.0.1:5178/api/p2p/peers
curl -i http://127.0.0.1:5178/api/p2p/ping
```

## Example

### server.cpp

```cpp
// vix run server.cpp
// http://127.0.0.1:5178/

#include <vix.hpp>
#include <vix/console.hpp>
#include <vix/p2p/Node.hpp>
#include <vix/p2p/P2P.hpp>
#include <vix/p2p_http/P2PHttp.hpp>

using namespace vix;

int main()
{
  App app;

  // 1) P2P node config
  vix::p2p::NodeConfig cfg;
  cfg.node_id = "A";
  cfg.listen_port = 9001;

  // 2) Boot runtime
  auto node = vix::p2p::make_tcp_node(cfg);
  vix::p2p::P2PRuntime runtime(node);
  runtime.start();

  // 3) Bridge runtime to HTTP routes
  vix::p2p_http::P2PHttpOptions opt;
  opt.prefix = "/api/p2p";
  opt.enable_ping = true;
  opt.enable_status = true;
  opt.enable_peers = true;
  opt.enable_logs = true;
  opt.enable_live_logs = true;
  opt.stats_every_ms = 250;

  vix::p2p_http::registerRoutes(app, runtime, opt);

  // 4) Serve UI (optional)
  app.static_dir("./public");

  app.get("/", [](Request &, Response &res)
  {
    res.file("./public/index.html");
  });

  app.get("/connect", [](Request &, Response &res)
  {
    res.file("./public/connect.html");
  });

  // 5) Listen
  app.listen(5178, []()
  {
    console.info("UI API listening on", 5178);
  });

  // 6) Wait + shutdown
  app.wait();
  runtime.stop();
}
```

## UI files (in repo)

The HTML is intentionally kept out of this markdown because it is large.

- `index.html`:
  https://github.com/vixcpp/vix/blob/main/examples/p2p/public/index.html

- `connect.html`:
  https://github.com/vixcpp/vix/blob/main/examples/p2p/public/connect.html

## Notes

### Multi-node setup

To run multiple nodes, change:
- `cfg.node_id` (example: `"B"`, `"C"`)
- `cfg.listen_port` (example: `9002`, `9003`)
- the HTTP port in `app.listen(...)` (example: `5179`, `5180`)

Typical local layout:

- Node A: P2P `9001`, HTTP `5178`
- Node B: P2P `9002`, HTTP `5179`
- Node C: P2P `9003`, HTTP `5180`

### Endpoint names can differ

The exact API paths depend on your `p2p_http` implementation.
If your build exposes different endpoints, keep the same structure but adjust the URLs in:
- the `curl` commands
- your UI `fetch()` calls

### Security reminder

If you expose this beyond localhost:
- protect `/api/p2p/*` with an auth middleware (API key, token, etc.)
- restrict dangerous operations (connect, kick, shutdown) to admin only

