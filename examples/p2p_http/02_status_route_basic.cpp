/**
 *
 *  @file 02_status_route_basic.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.
 *  All rights reserved.
 *  https://github.com/vixcpp/vix
 *
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 *
 */
// Run:
//   vix run examples/p2p_http/02_status_route_basic.cpp
//
// Then:
//   curl http://127.0.0.1:8081/p2p/status

#include <iostream>
#include <memory>
#include <string_view>

#include <vix/app/App.hpp>
#include <vix/p2p/Node.hpp>
#include <vix/p2p/P2P.hpp>
#include <vix/p2p_http/P2PHttp.hpp>
#include <vix/p2p_http/P2PHttpOptions.hpp>
#include <vix/console.hpp>

namespace
{
  std::shared_ptr<vix::p2p::Node> make_node()
  {
    vix::p2p::NodeConfig cfg;
    cfg.node_id = "p2p-http-status";
    cfg.listen_port = 9102;
    cfg.on_log = [](std::string_view line)
    {
      std::cout << line << "\n";
    };

    return vix::p2p::make_tcp_node(cfg);
  }

  void run_example()
  {
    auto node = make_node();
    vix::p2p::P2PRuntime runtime(node);
    runtime.start();

    vix::p2p_http::P2PHttpOptions opt;
    opt.prefix = "/p2p";
    opt.enable_ping = false;
    opt.enable_status = true;
    opt.enable_logs = false;
    opt.enable_live_logs = false;
    opt.enable_peers = false;

    vix::App app;
    vix::p2p_http::registerRoutes(app, runtime, opt);

    std::cout << "p2p_http status example\n";
    std::cout << "GET  http://127.0.0.1:8081/p2p/status\n";

    app.listen(8081, []()
               { vix::console.info("API listening on", 8081); });

    runtime.stop();
    vix::p2p_http::shutdown_live_logs();
  }
}

int main()
{
  run_example();
  return 0;
}
