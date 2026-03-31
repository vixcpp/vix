/**
 *
 *  @file 01_envelope_and_framing_basic.cpp
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
//   vix run examples/p2p/01_envelope_and_framing_basic.cpp

#include <cstdint>
#include <iostream>
#include <vector>

#include <vix/p2p/Framing.hpp>
#include <vix/p2p/Protocol.hpp>
#include <vix/p2p/framing/LengthPrefixVarint.hpp>
#include <vix/p2p/messages/Envelope.hpp>
#include <vix/p2p/messages/Pack.hpp>
#include <vix/p2p/messages/Ping.hpp>

namespace
{
  void print_outgoing(const vix::p2p::Envelope &env)
  {
    std::cout << "outgoing envelope\n";
    std::cout << "type   : " << vix::p2p::to_string(env.type) << "\n";
    std::cout << "msg_id : " << env.msg_id << "\n";
    std::cout << "flags  : " << env.flags << "\n";
  }

  void run_example()
  {
    using namespace vix::p2p;

    msg::Ping ping;
    ping.nonce = 42;

    Envelope outgoing = pack::make_envelope(MessageType::Ping, ping);
    print_outgoing(outgoing);

    const std::vector<std::uint8_t> envelope_bytes = outgoing.encode();
    std::cout << "encoded envelope bytes: " << envelope_bytes.size() << "\n";

    framing::LengthPrefixVarint framer;
    Frame frame = framer.encode(envelope_bytes);

    std::cout << "framed bytes          : " << frame.bytes.size() << "\n";

    FrameDecodeResult decoded = framer.decode(frame.bytes);
    if (decoded.frames.empty())
    {
      std::cout << "decode failed: no frame\n";
      return;
    }

    Envelope incoming = Envelope::decode_or_throw(decoded.frames.front().bytes);

    std::cout << "decoded envelope\n";
    std::cout << "type   : " << to_string(incoming.type) << "\n";
    std::cout << "msg_id : " << incoming.msg_id << "\n";
    std::cout << "flags  : " << incoming.flags << "\n";

    msg::Ping roundtrip = msg::Ping::decode_or_throw(incoming.payload);
    std::cout << "decoded ping\n";
    std::cout << "nonce  : " << roundtrip.nonce << "\n";
    std::cout << "remaining bytes: " << decoded.remaining.size() << "\n";
  }
}

int main()
{
  run_example();
  return 0;
}
