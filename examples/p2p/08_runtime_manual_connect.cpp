/**
 *
 *  @file 08_runtime_manual_connect.cpp
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
//     vix run examples/p2p/08_runtime_manual_connect.cpp --run server
//
//   Terminal 2:
//     vix run examples/p2p/08_runtime_manual_connect.cpp --run client 127.0.0.1 9101

#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <vix/p2p/Node.hpp>
#include <vix/p2p/P2P.hpp>
#include <vix/p2p/Peer.hpp>

namespace
{
  constexpr std::uint16_t kDefaultPort = 9101;

  void sleep_ms(int ms)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
  }

  std::shared_ptr<vix::p2p::Node> make_node(const std::string &node_id,
                                            std::uint16_t port)
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

  void run_server()
  {
    auto node = make_node("server-node", kDefaultPort);

    std::cout << "starting server on port " << kDefaultPort << "\n";
    node->start();

    for (int i = 0; i < 10; ++i)
    {
      const auto stats = node->stats();
      std::cout << "server stats"
                << " peers_total=" << stats.peers_total
                << " peers_connected=" << stats.peers_connected
                << " handshakes_started=" << stats.handshakes_started
                << " handshakes_completed=" << stats.handshakes_completed
                << "\n";
      sleep_ms(1000);
    }

    node->stop();
  }

  void run_client(const std::string &host, std::uint16_t port)
  {
    auto node = make_node("client-node", 0);
    vix::p2p::P2PRuntime runtime(node);

    runtime.start();

    vix::p2p::PeerEndpoint ep;
    ep.host = host;
    ep.port = port;
    ep.scheme = "tcp";

    const bool started = runtime.connect(ep);
    std::cout << "manual connect started: " << (started ? "true" : "false") << "\n";

    for (int i = 0; i < 8; ++i)
    {
      const auto stats = runtime.stats();
      std::cout << "client stats"
                << " peers_total=" << stats.peers_total
                << " peers_connected=" << stats.peers_connected
                << " handshakes_started=" << stats.handshakes_started
                << " handshakes_completed=" << stats.handshakes_completed
                << "\n";
      sleep_ms(1000);
    }

    runtime.stop();
  }

  void run_example(int argc, char **argv)
  {
    if (argc < 2)
    {
      std::cout << "usage:\n";
      std::cout << "  server\n";
      std::cout << "  client <host> <port>\n";
      return;
    }

    const std::string mode = argv[1];

    if (mode == "server")
    {
      run_server();
      return;
    }

    if (mode == "client")
    {
      const std::string host = (argc >= 3) ? argv[2] : "127.0.0.1";
      const std::uint16_t port =
          (argc >= 4) ? static_cast<std::uint16_t>(std::stoi(argv[3])) : kDefaultPort;

      run_client(host, port);
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
