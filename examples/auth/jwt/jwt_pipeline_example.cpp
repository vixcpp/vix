// ============================================================================
// jwt_example.cpp - Minimal JWT middleware example (Vix.cpp)
// ----------------------------------------------------------------------------
// Run:
//   vix run jwt_example.cpp
// ============================================================================

#include <cassert>
#include <iostream>
#include <string>

#include <vix/http/Request.hpp>
#include <vix/http/Response.hpp>
#include <vix/http/ResponseWrapper.hpp>
#include <vix/middleware/pipeline.hpp>
#include <vix/middleware/auth/jwt.hpp>

using namespace vix::middleware;

static vix::vhttp::Request make_req_with_bearer(const std::string &token)
{
  vix::vhttp::Request::HeaderMap headers;
  headers["Host"] = "localhost";
  headers["authorization"] = "Bearer " + token;

  return vix::vhttp::Request(
      "GET",
      "/secure",
      std::move(headers),
      "");
}

int main()
{
  const std::string secret = "dev_secret";

  // A valid HS256 token (payload: {"sub":"user123","roles":["admin"]})
  // signed with secret "dev_secret".
  const std::string token =
      "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
      "eyJzdWIiOiJ1c2VyMTIzIiwicm9sZXMiOlsiYWRtaW4iXX0."
      "3HK5b1sXMbxkjC3Tllwtcuzxm-1OI0D184Fuav0-XQo";

  HttpPipeline p;

  auth::JwtOptions opt{};
  opt.secret = secret;
  opt.verify_exp = false;
  p.use(auth::jwt(opt));

  auto req = make_req_with_bearer(token);

  vix::vhttp::Response raw_res;
  vix::vhttp::ResponseWrapper res(raw_res);

  p.run(req, res, [&](Request &r, Response &w)
        {
        auto &claims = r.state<auth::JwtClaims>();

        assert(claims.subject == "user123");
        assert(!claims.roles.empty());
        assert(claims.roles[0] == "admin");

        w.status(200).text("OK"); });

  assert(raw_res.status() == 200);
  assert(raw_res.body() == "OK");

  std::cout << "[OK] jwt example: subject=user123 role=admin\n";
  return 0;
}
