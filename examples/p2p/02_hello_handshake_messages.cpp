/**
 *
 *  @file 02_hello_handshake_messages.cpp
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
//   vix run examples/p2p/02_hello_handshake_messages.cpp

#include <cstdint>
#include <iostream>
#include <vector>

#include <vix/p2p/messages/Hello.hpp>
#include <vix/p2p/messages/HelloAck.hpp>
#include <vix/p2p/messages/HelloFinish.hpp>

namespace
{
  std::vector<std::uint8_t> make_key(std::uint8_t seed, std::size_t n = 32)
  {
    std::vector<std::uint8_t> out(n);
    for (std::size_t i = 0; i < n; ++i)
      out[i] = static_cast<std::uint8_t>(seed + i);
    return out;
  }

  void run_hello()
  {
    using namespace vix::p2p::msg;

    Hello hello;
    hello.nonce_a = 1001;
    hello.ts_ms = 1710000000000ULL;
    hello.node_id = "node-a";
    hello.capabilities["proto"] = "1.0";
    hello.capabilities["transport"] = "tcp";
    hello.public_key = make_key(10);

    const auto bytes = hello.encode();
    const Hello decoded = Hello::decode_or_throw(bytes);

    std::cout << "hello\n";
    std::cout << "node_id      : " << decoded.node_id << "\n";
    std::cout << "nonce_a      : " << decoded.nonce_a << "\n";
    std::cout << "ts_ms        : " << decoded.ts_ms << "\n";
    std::cout << "capabilities : " << decoded.capabilities.size() << "\n";
    std::cout << "public_key   : " << decoded.public_key.size() << " bytes\n";
  }

  void run_hello_ack()
  {
    using namespace vix::p2p::msg;

    HelloAck ack;
    ack.nonce_a = 1001;
    ack.nonce_b = 2002;
    ack.public_key = make_key(50);

    const auto bytes = ack.encode();
    const HelloAck decoded = HelloAck::decode_or_throw(bytes);

    std::cout << "hello_ack\n";
    std::cout << "nonce_a    : " << decoded.nonce_a << "\n";
    std::cout << "nonce_b    : " << decoded.nonce_b << "\n";
    std::cout << "public_key : " << decoded.public_key.size() << " bytes\n";
  }

  void run_hello_finish()
  {
    using namespace vix::p2p::msg;

    HelloFinish finish;
    finish.nonce_a = 1001;
    finish.nonce_b = 2002;
    finish.signature = make_key(90, 64);

    const auto bytes = finish.encode();
    const HelloFinish decoded = HelloFinish::decode_or_throw(bytes);

    std::cout << "hello_finish\n";
    std::cout << "nonce_a   : " << decoded.nonce_a << "\n";
    std::cout << "nonce_b   : " << decoded.nonce_b << "\n";
    std::cout << "signature : " << decoded.signature.size() << " bytes\n";
  }

  void run_example()
  {
    run_hello();
    std::cout << "\n";
    run_hello_ack();
    std::cout << "\n";
    run_hello_finish();
  }
}

int main()
{
  run_example();
  return 0;
}
