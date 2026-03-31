/**
 *
 *  @file 03_discovery_announce_json.cpp
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
//   vix run examples/p2p/03_discovery_announce_json.cpp

#include <iostream>
#include <optional>
#include <string>

#include <vix/p2p/messages/DiscoveryAnnounce.hpp>

namespace
{
  void print_parsed(const std::optional<vix::p2p::msg::DiscoveryAnnounce> &parsed)
  {
    if (!parsed)
    {
      std::cout << "parse failed\n";
      return;
    }

    std::cout << "parsed discovery announcement\n";
    std::cout << "node_id : " << parsed->node_id << "\n";
    std::cout << "tcp_port: " << parsed->tcp_port << "\n";
    std::cout << "ts_ms   : " << parsed->ts_ms << "\n";
    std::cout << "nonce   : " << parsed->nonce << "\n";
  }

  void run_example()
  {
    using namespace vix::p2p::msg;

    DiscoveryAnnounce announce;
    announce.node_id = "node-a";
    announce.tcp_port = 9001;
    announce.ts_ms = 1710000000000ULL;
    announce.nonce = 987654321ULL;
    announce.capabilities["proto"] = "1.0";
    announce.capabilities["transport"] = "tcp";

    const std::string json = announce.to_json();

    std::cout << "json payload\n";
    std::cout << json << "\n\n";

    const auto parsed = DiscoveryAnnounce::from_json(json);
    print_parsed(parsed);
  }
}

int main()
{
  run_example();
  return 0;
}
