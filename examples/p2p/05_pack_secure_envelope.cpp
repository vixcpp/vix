/**
 *
 *  @file 05_pack_secure_envelope.cpp
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
//   vix run examples/p2p/05_pack_secure_envelope.cpp

#include <array>
#include <cstdint>
#include <iostream>
#include <span>
#include <vector>

#include <vix/p2p/Crypto.hpp>
#include <vix/p2p/Protocol.hpp>
#include <vix/p2p/messages/Envelope.hpp>
#include <vix/p2p/messages/Pack.hpp>
#include <vix/p2p/messages/Ping.hpp>

namespace
{
  std::vector<std::uint8_t> make_session_key()
  {
    std::vector<std::uint8_t> key(32);
    for (std::size_t i = 0; i < key.size(); ++i)
      key[i] = static_cast<std::uint8_t>(0xA0 + i);
    return key;
  }

  void run_example()
  {
    using namespace vix::p2p;

    msg::Ping ping;
    ping.nonce = 999;

    const std::vector<std::uint8_t> plaintext = ping.encode();
    const std::vector<std::uint8_t> session_key = make_session_key();

    NullCrypto crypto;
    Envelope env = pack::make_envelope_secure(
        MessageType::Ping,
        plaintext,
        session_key,
        crypto,
        1);

    std::cout << "secure envelope\n";
    std::cout << "type      : " << to_string(env.type) << "\n";
    std::cout << "msg_id    : " << env.msg_id << "\n";
    std::cout << "encrypted : "
              << (has_flag(env.flags, EnvelopeFlag::Encrypted) ? "true" : "false")
              << "\n";
    std::cout << "payload   : " << env.payload.size() << " bytes\n";

    const auto aad = pack::make_aad(env);
    const std::vector<std::uint8_t> decrypted = crypto.aead_decrypt(
        session_key,
        std::span<const std::uint8_t>(env.nonce.data(), env.nonce.size()),
        aad,
        env.payload,
        std::span<const std::uint8_t>(env.tag.data(), env.tag.size()));

    if (decrypted.empty())
    {
      std::cout << "decrypt failed\n";
      return;
    }

    const msg::Ping roundtrip = msg::Ping::decode_or_throw(decrypted);
    std::cout << "decrypted ping nonce: " << roundtrip.nonce << "\n";
  }
}

int main()
{
  run_example();
  return 0;
}
