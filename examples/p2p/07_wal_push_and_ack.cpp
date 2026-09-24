/**
 *
 *  @file 07_wal_push_and_ack.cpp
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
//   vix run examples/p2p/07_wal_push_and_ack.cpp

#include <cstdint>
#include <iostream>
#include <vector>

#include <vix/p2p/messages/OutboxPull.hpp>
#include <vix/p2p/messages/WalAck.hpp>
#include <vix/p2p/messages/WalPush.hpp>

namespace
{
  std::vector<std::uint8_t> make_fake_wal_bytes()
  {
    return {
        0x10, 0x20, 0x30, 0x40,
        0x50, 0x60, 0x70, 0x80};
  }

  void run_wal_push()
  {
    using namespace vix::p2p::msg;

    WalPush push;
    push.seq_begin = 10;
    push.seq_end = 12;
    push.wal_bytes = make_fake_wal_bytes();

    const auto bytes = push.encode();
    const WalPush decoded = WalPush::decode_or_throw(bytes);

    std::cout << "wal_push\n";
    std::cout << "seq_begin : " << decoded.seq_begin << "\n";
    std::cout << "seq_end   : " << decoded.seq_end << "\n";
    std::cout << "wal_bytes : " << decoded.wal_bytes.size() << " bytes\n";
  }

  void run_wal_ack()
  {
    using namespace vix::p2p::msg;

    WalAck ack;
    ack.last_applied_seq = 12;

    const auto bytes = ack.encode();
    const WalAck decoded = WalAck::decode_or_throw(bytes);

    std::cout << "wal_ack\n";
    std::cout << "last_applied_seq: " << decoded.last_applied_seq << "\n";
  }

  void run_outbox_pull()
  {
    using namespace vix::p2p::msg;

    OutboxPull pull;
    pull.target_node_id = "node-b";
    pull.max_items = 64;

    const auto bytes = pull.encode();
    const OutboxPull decoded = OutboxPull::decode_or_throw(bytes);

    std::cout << "outbox_pull\n";
    std::cout << "target_node_id: " << decoded.target_node_id << "\n";
    std::cout << "max_items     : " << decoded.max_items << "\n";
  }

  void run_example()
  {
    run_wal_push();
    std::cout << "\n";
    run_wal_ack();
    std::cout << "\n";
    run_outbox_pull();
  }
}

int main()
{
  run_example();
  return 0;
}
