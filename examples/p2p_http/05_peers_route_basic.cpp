/**
 *
 *  @file 05_peers_route_basic.cpp
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
//   Terminal 1:
//     vix run examples/p2p_http/05_peers_route_basic.cpp --run target
//
//   Terminal 2:
//     vix run examples/p2p_http/05_peers_route_basic.cpp --run api
//
// Then:
//   1) POST /p2p/connect to connect to the target
//   2) GET  /p2p/peers

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

#include <vix/app/App.hpp>
#include <vix/p2p/Node.hpp>
#include <vix/p2p/P2P.hpp>
#include <vix/p2p_http/P2PHttp.hpp>
#include <vix/p2p_http/P2PHttpOptions.hpp>
#include <vix/console.hpp>

namespace
{
  constexpr std::uint16_t kTargetPort = 9211;
  constexpr std::uint16_t kApiNodePort = 9212;
  constexpr std::uint16_t kHttpPort = 8084;

  std::shared_ptr<vix::p2p::Node> make_node(const std::string &node_id, std::uint16_t port)
  {
    vix::p2p::NodeConfig cfg;
    cfg.node_id = node_id;
    cfg.listen_port = port;
    cfg.on_log = [](std::string_view line)
    {
      std::cout << line << "\n";
    };
    return vix::p2p::make_tcp_node(cfg);
  }

  void run_target()
  {
    auto node = make_node("peers-target", kTargetPort);
    node->start();

    std::cout << "target node running on tcp://127.0.0.1:" << kTargetPort << "\n";
    node->wait();
  }

  void run_api()
  {
    auto node = make_node("peers-api", kApiNodePort);
    vix::p2p::P2PRuntime runtime(node);
    runtime.start();

    vix::p2p_http::P2PHttpOptions opt;
    opt.prefix = "/p2p";
    opt.enable_ping = false;
    opt.enable_status = true;
    opt.enable_logs = false;
    opt.enable_live_logs = false;
    opt.enable_peers = true;

    vix::App app;
    vix::p2p_http::registerRoutes(app, runtime, opt);

    std::cout << "POST http://127.0.0.1:" << kHttpPort << "/p2p/connect\n";
    std::cout << R"(GET  http://127.0.0.1:8084/p2p/peers)" << "\n";
    std::cout << R"(payload: {"host":"127.0.0.1","port":9211,"scheme":"tcp"})" << "\n";

    app.listen(kHttpPort, []()
               { vix::console.info("API listening on", kHttpPort); });

    runtime.stop();
    vix::p2p_http::shutdown_live_logs();
  }

  void run_example(int argc, char **argv)
  {
    if (argc < 2)
    {
      std::cout << "usage:\n";
      std::cout << "  vix run examples/p2p_http/05_peers_route_basic.cpp --run target\n";
      std::cout << "  vix run examples/p2p_http/05_peers_route_basic.cpp --run api\n";
      return;
    }

    const std::string mode = argv[1];
    if (mode == "target")
    {
      run_target();
      return;
    }

    if (mode == "api")
    {
      run_api();
      return;
    }

    std::cout << "unknown mode: " << mode << "\n";
  }
}

int main(int argc, char **argv)
{
  run_example(argc, argv);
  return 0;
}
