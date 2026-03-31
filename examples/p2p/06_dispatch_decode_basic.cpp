/**
 *
 *  @file 06_dispatch_decode_basic.cpp
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
//   vix run examples/p2p/06_dispatch_decode_basic.cpp

#include <iostream>
#include <variant>

#include <vix/p2p/Protocol.hpp>
#include <vix/p2p/messages/Dispatch.hpp>
#include <vix/p2p/messages/Ping.hpp>
#include <vix/p2p/messages/WalAck.hpp>

namespace
{
  void run_ping_case()
  {
    using namespace vix::p2p;

    msg::Ping ping;
    ping.nonce = 123456;

    msg::AnyMessage any =
        msg::decode_payload_or_throw(MessageType::Ping, ping.encode());

    if (std::holds_alternative<msg::Ping>(any))
    {
      const auto &decoded = std::get<msg::Ping>(any);
      std::cout << "dispatch ping nonce: " << decoded.nonce << "\n";
    }
  }

  void run_wal_ack_case()
  {
    using namespace vix::p2p;

    msg::WalAck ack;
    ack.last_applied_seq = 77;

    msg::AnyMessage any =
        msg::decode_payload_or_throw(MessageType::WalAck, ack.encode());

    if (std::holds_alternative<msg::WalAck>(any))
    {
      const auto &decoded = std::get<msg::WalAck>(any);
      std::cout << "dispatch wal ack seq: " << decoded.last_applied_seq << "\n";
    }
  }

  void run_example()
  {
    run_ping_case();
    run_wal_ack_case();
  }
}

int main()
{
  run_example();
  return 0;
}
