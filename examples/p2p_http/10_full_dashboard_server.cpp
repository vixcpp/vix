/**
 *
 *  @file 10_full_dashboard_server.cpp
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
//   vix run examples/p2p_http/10_full_dashboard_server.cpp
//
// Useful endpoints:
//   GET  /p2p/ping
//   GET  /p2p/status
//   GET  /p2p/peers
//   GET  /p2p/logs
//   POST /p2p/connect
//   POST /p2p/admin/hook
//
// Example connect payload:
//   {"host":"127.0.0.1","port":9110,"scheme":"tcp"}

#include <iostream>
#include <memory>
#include <string_view>

#include <vix/app/App.hpp>
#include <vix/http/RequestHandler.hpp>
#include <vix/json/json.hpp>
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
    cfg.node_id = "p2p-http-dashboard";
    cfg.listen_port = 9110;
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
    opt.enable_ping = true;
    opt.enable_status = true;
    opt.enable_logs = true;
    opt.enable_live_logs = true;
    opt.enable_peers = true;
    opt.stats_every_ms = 1000;

    opt.auth_legacy = [](vix::vhttp::Request &req, vix::vhttp::ResponseWrapper &res) -> bool
    {
      const auto token = req.header("x-auth-token");
      if (token == "dashboard-admin")
        return true;

      res.status(401).json(vix::json::obj({"ok", false,
                                           "error", "unauthorized",
                                           "hint", "expected x-auth-token: dashboard-admin"}));
      return false;
    };

    opt.log_sink = [](std::string_view line)
    {
      std::cout << "[p2p_http] " << line << "\n";
    };

    vix::App app;
    vix::p2p_http::registerRoutes(app, runtime, opt);

    std::cout << "full dashboard server\n";
    std::cout << "GET  http://127.0.0.1:8089/p2p/ping\n";
    std::cout << "GET  http://127.0.0.1:8089/p2p/status\n";
    std::cout << "GET  http://127.0.0.1:8089/p2p/peers\n";
    std::cout << "GET  http://127.0.0.1:8089/p2p/logs\n";
    std::cout << "POST http://127.0.0.1:8089/p2p/connect\n";
    std::cout << "POST http://127.0.0.1:8089/p2p/admin/hook\n";
    std::cout << "Admin token: x-auth-token: dashboard-admin\n";

    app.listen(8089, []()
               { vix::console.info("API listening on", 8089); });

    runtime.stop();
    vix::p2p_http::shutdown_live_logs();
  }
}

int main()
{
  run_example();
  return 0;
}
