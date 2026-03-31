/**
 *
 *  @file 10_bootstrap_http_basic.cpp
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
//   1) Start a small local registry that returns:
//      { "peers": [ { "host":"127.0.0.1", "tcp_port":9301, "node_id":"node-x" } ] }
//
//   2) Then run:
//      vix run examples/p2p/10_bootstrap_http_basic.cpp --run http://127.0.0.1:8080/peers

#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <vix/p2p/Bootstrap.hpp>

namespace
{
  void sleep_ms(int ms)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
  }

  std::shared_ptr<vix::p2p::Bootstrap> make_bootstrap(const std::string &url)
  {
    vix::p2p::BootstrapConfig cfg;
    cfg.self_node_id = "bootstrap-client";
    cfg.self_tcp_port = 9300;
    cfg.registry_url = url;
    cfg.mode = vix::p2p::BootstrapMode::PullOnly;
    cfg.poll_interval_ms = 2000;
    cfg.connect_cooldown_ms = 4000;
    cfg.max_http_bytes = 64 * 1024;
    cfg.connect_timeout_ms = 3000;
    cfg.request_timeout_ms = 5000;
    cfg.backoff_max_ms = 10000;
    cfg.max_peers_per_poll = 20;

    auto on_peer = [](const vix::p2p::BootstrapPeer &peer)
    {
      std::cout << "[bootstrap peer] "
                << "node_id=" << peer.node_id
                << " host=" << peer.host
                << " tcp_port=" << peer.tcp_port
                << " transport=" << peer.transport
                << "\n";
    };

    return vix::p2p::make_http_bootstrap(cfg, on_peer);
  }

  void run_example(int argc, char **argv)
  {
    const std::string url =
        (argc >= 2) ? argv[1] : "http://127.0.0.1:8080/peers";

    auto bootstrap = make_bootstrap(url);

    std::cout << "starting bootstrap\n";
    std::cout << "registry_url: " << url << "\n";

    bootstrap->start();

    for (int i = 0; i < 8; ++i)
    {
      const auto peers = bootstrap->snapshot();
      std::cout << "snapshot size: " << peers.size() << "\n";

      for (const auto &peer : peers)
      {
        std::cout << "  - "
                  << peer.node_id << " "
                  << peer.host << ":" << peer.tcp_port
                  << " via " << peer.transport << "\n";
      }

      sleep_ms(1500);
    }

    bootstrap->stop();
  }
}

int main(int argc, char **argv)
{
  run_example(argc, argv);
  return 0;
}
