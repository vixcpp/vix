/**
 *
 *  @file jwt_gen.cpp — generate HS256 JWT tokens for rbac_app_simple.cpp (Vix.cpp)
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.  All rights reserved.
 *  https://github.com/vixcpp/vix
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 *
 */
// Build:
//   g++ -std=c++20 jwt_gen.cpp -lssl -lcrypto -O2 && ./a.out
//
// Prints:
//   TOKEN_OK        (admin + products:write)
//   TOKEN_NO_PERM   (admin but missing products:write)
//
// NOTE: secret must match rbac_app_simple.cpp (dev_secret).
// ============================================================================

#include <iostream>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <openssl/hmac.h>

static std::string b64url_encode(const unsigned char *data, size_t len)
{
  std::string b64;
  b64.resize(4 * ((len + 2) / 3));

  int out_len = EVP_EncodeBlock(
      reinterpret_cast<unsigned char *>(&b64[0]),
      data,
      static_cast<int>(len));

  b64.resize(static_cast<size_t>(out_len));

  for (char &c : b64)
  {
    if (c == '+')
      c = '-';
    else if (c == '/')
      c = '_';
  }
  while (!b64.empty() && b64.back() == '=')
    b64.pop_back();

  return b64;
}

static std::string hmac_sha256_b64url(std::string_view msg, std::string_view secret)
{
  unsigned int out_len = 0;
  unsigned char out[EVP_MAX_MD_SIZE];

  HMAC(EVP_sha256(),
       secret.data(),
       static_cast<int>(secret.size()),
       reinterpret_cast<const unsigned char *>(msg.data()),
       msg.size(),
       out,
       &out_len);

  return b64url_encode(out, static_cast<size_t>(out_len));
}

static std::string make_jwt_hs256(const nlohmann::json &payload, const std::string &secret)
{
  nlohmann::json header = {{"alg", "HS256"}, {"typ", "JWT"}};

  const std::string h = header.dump();
  const std::string p = payload.dump();

  const std::string h64 = b64url_encode(reinterpret_cast<const unsigned char *>(h.data()), h.size());
  const std::string p64 = b64url_encode(reinterpret_cast<const unsigned char *>(p.data()), p.size());

  const std::string signing = h64 + "." + p64;
  const std::string sig = hmac_sha256_b64url(signing, secret);

  return signing + "." + sig;
}

int main()
{
  const std::string secret = "dev_secret";

  nlohmann::json ok = {
      {"sub", "user123"},
      {"roles", {"admin"}},
      {"perms", {"products:write", "orders:read"}}};

  nlohmann::json no_perm = {
      {"sub", "user123"},
      {"roles", {"admin"}},
      {"perms", {"orders:read"}}};

  std::cout << "TOKEN_OK:\n"
            << make_jwt_hs256(ok, secret) << "\n\n";
  std::cout << "TOKEN_NO_PERM:\n"
            << make_jwt_hs256(no_perm, secret) << "\n";
  return 0;
}
