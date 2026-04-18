/**
 *
 *  @file 09_heavy_admin_hook.cpp
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
//   vix run examples/p2p_http/09_heavy_admin_hook.cpp
//
// Then:
//   curl -i -X POST http://127.0.0.1:8088/p2p/admin/hook -H "x-auth-token: admin"

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
    cfg.node_id = "p2p-http-heavy-admin";
    cfg.listen_port = 9109;
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
    opt.enable_status = false;
    opt.enable_logs = false;
    opt.enable_live_logs = false;
    opt.enable_peers = false;

    opt.auth_legacy = [](vix::http::Request &req, vix::http::ResponseWrapper &res) -> bool
    {
      const auto token = req.header("x-auth-token");
      if (token == "admin")
        return true;

      res.status(401).json(vix::json::obj({"ok", false,
                                           "error", "unauthorized",
                                           "hint", "expected x-auth-token: admin"}));
      return false;
    };

    vix::App app;
    vix::p2p_http::registerRoutes(app, runtime, opt);

    std::cout << "heavy admin hook example\n";
    std::cout << "POST http://127.0.0.1:8088/p2p/admin/hook\n";
    std::cout << "Check response headers for x-vix-route-heavy: 1\n";

    app.listen(8088, []()
               { vix::console.info("API listening on", 8088); });

    runtime.stop();
    vix::p2p_http::shutdown_live_logs();
  }
}

int main()
{
  run_example();
  return 0;
}
