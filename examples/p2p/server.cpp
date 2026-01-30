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
          { res.file("./public/index.html"); });

  app.get("/connect", [](Request &, Response &res)
          { res.file("./public/connect.html"); });

  app.listen(5178, [](const vix::utils::ServerReadyInfo &info)
             { console.info("UI API listening on", info.port); });

  app.wait();
  runtime.stop();
}
