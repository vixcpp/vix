# p2p-ping

Minimal example for **Vix P2P** with an HTTP UI/API bridge.

This example boots:
- a P2P node (`P2PRuntime`)
- an HTTP server (`App`)
- the built-in `p2p_http` routes under `/api/p2p`

All code is kept inside `main()` on purpose.

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

  vix::p2p::NodeConfig cfg;
  cfg.node_id = "A";
  cfg.listen_port = 9001;

  auto node = vix::p2p::make_tcp_node(cfg);
  vix::p2p::P2PRuntime runtime(node);
  runtime.start();

  vix::p2p_http::P2PHttpOptions opt;
  opt.prefix = "/api/p2p";
  opt.enable_ping = true;
  opt.enable_status = true;
  opt.enable_peers = true;
  opt.enable_logs = true;
  opt.enable_live_logs = true;
  opt.stats_every_ms = 250;

  vix::p2p_http::registerRoutes(app, runtime, opt);

  app.static_dir("./public");
  app.get("/", [](Request &, Response &res)
  {
    res.file("./public/index.html");
  });

  app.get("/connect", [](Request &, Response &res)
  {
    res.file("./public/connect.html");
  });

  app.listen(5178, []()
  {
    console.info("UI API listening on", 5178);
  });

  app.wait();
  runtime.stop();
}
```

## Notes

- If you run multiple nodes, change:
  - `cfg.node_id` (example: "B", "C")
  - `cfg.listen_port` (example: 9002, 9003)
  - the HTTP port in `app.listen(...)` (example: 5179, 5180)

- The exact API paths depend on your `p2p_http` implementation.
  If your build exposes different endpoints, keep the same structure but adjust the URLs in the curl commands.


