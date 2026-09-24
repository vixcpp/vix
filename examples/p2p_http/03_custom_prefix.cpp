/**
 *
 *  @file 03_custom_prefix.cpp
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
//   vix run examples/p2p_http/03_custom_prefix.cpp
//
// Then:
//   curl http://127.0.0.1:8082/control/ping
//   curl http://127.0.0.1:8082/control/status

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
    cfg.node_id = "p2p-http-prefix";
    cfg.listen_port = 9103;
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
    opt.prefix = "/control";
    opt.enable_ping = true;
    opt.enable_status = true;
    opt.enable_logs = false;
    opt.enable_live_logs = false;
    opt.enable_peers = false;

    vix::App app;
    vix::p2p_http::registerRoutes(app, runtime, opt);

    std::cout << "custom prefix example\n";
    std::cout << "GET  http://127.0.0.1:8082/control/ping\n";
    std::cout << "GET  http://127.0.0.1:8082/control/status\n";

    app.listen(8082, []()
               { vix::console.info("API listening on", 8082); });

    runtime.stop();
    vix::p2p_http::shutdown_live_logs();
  }
}

int main()
{
  run_example();
  return 0;
}
