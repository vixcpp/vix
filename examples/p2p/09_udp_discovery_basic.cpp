/**
 *
 *  @file 09_udp_discovery_basic.cpp
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
//     vix run examples/p2p/09_udp_discovery_basic.cpp --run node-a 9201
//
//   Terminal 2:
//     vix run examples/p2p/09_udp_discovery_basic.cpp --run node-b 9202

#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <vix/p2p/Discovery.hpp>

namespace
{
  void sleep_ms(int ms)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
  }

  std::shared_ptr<vix::p2p::Discovery> make_discovery(const std::string &node_id,
                                                      std::uint16_t tcp_port)
  {
    vix::p2p::DiscoveryConfig cfg;
    cfg.self_node_id = node_id;
    cfg.self_tcp_port = tcp_port;
    cfg.discovery_port = 37020;
    cfg.mode = vix::p2p::DiscoveryMode::Broadcast;
    cfg.announce_interval_ms = 1000;
    cfg.seen_ttl_ms = 15000;
    cfg.connect_cooldown_ms = 4000;
    cfg.max_packet_bytes = 1024;

    auto on_peer = [](const vix::p2p::DiscoveryAnnouncement &a)
    {
      std::cout << "[discovered] "
                << "node_id=" << a.node_id
                << " host=" << a.host
                << " port=" << a.port
                << " transport=" << a.transport
                << "\n";
    };

    return vix::p2p::make_udp_discovery(cfg, on_peer);
  }

  void run_example(int argc, char **argv)
  {
    const std::string node_id = (argc >= 2) ? argv[1] : "node-a";
    const std::uint16_t tcp_port =
        (argc >= 3) ? static_cast<std::uint16_t>(std::stoi(argv[2])) : 9201;

    auto discovery = make_discovery(node_id, tcp_port);

    std::cout << "starting discovery\n";
    std::cout << "self node_id : " << node_id << "\n";
    std::cout << "self tcp_port: " << tcp_port << "\n";

    discovery->start();

    for (int i = 0; i < 10; ++i)
    {
      const auto snapshot = discovery->snapshot();
      std::cout << "snapshot size: " << snapshot.size() << "\n";
      sleep_ms(1000);
    }

    discovery->stop();
  }
}

int main(int argc, char **argv)
{
  run_example(argc, argv);
  return 0;
}
