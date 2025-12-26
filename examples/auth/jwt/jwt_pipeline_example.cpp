// ============================================================================
// jwt_example.cpp — Minimal JWT middleware example (Vix.cpp)
// ----------------------------------------------------------------------------
// Run:
//   vix run jwt_example.cpp
// ============================================================================

#include <cassert>
#include <iostream>
#include <string>

#include <boost/beast/http.hpp>

#include <vix/middleware/pipeline.hpp>
#include <vix/middleware/auth/jwt.hpp>

using namespace vix::middleware;

static vix::vhttp::RawRequest make_req_with_bearer(std::string token)
{
    namespace http = boost::beast::http;

    vix::vhttp::RawRequest req{http::verb::get, "/secure", 11};
    req.set(http::field::host, "localhost");
    req.set("authorization", "Bearer " + token);
    return req;
}

int main()
{
    namespace http = boost::beast::http;

    // Same secret as the middleware config
    const std::string secret = "dev_secret";

    // A valid HS256 token (payload: {"sub":"user123","roles":["admin"]})
    // signed with secret "dev_secret".
    const std::string token =
        "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
        "eyJzdWIiOiJ1c2VyMTIzIiwicm9sZXMiOlsiYWRtaW4iXX0."
        "3HK5b1sXMbxkjC3Tllwtcuzxm-1OI0D184Fuav0-XQo";

    // 1) Build pipeline
    HttpPipeline p;

    // 2) Install JWT middleware
    auth::JwtOptions opt{};
    opt.secret = secret;
    opt.verify_exp = false; // keep example simple
    p.use(auth::jwt(opt));

    // 3) Run a request through the pipeline
    auto raw = make_req_with_bearer(token);
    http::response<http::string_body> res;

    vix::vhttp::Request req(raw, {});
    vix::vhttp::ResponseWrapper w(res);

    p.run(req, w, [&](Request &r, Response &)
          {
        // Claims are available after jwt() success
        auto &claims = r.state<auth::JwtClaims>();

        assert(claims.subject == "user123");
        assert(!claims.roles.empty());
        assert(claims.roles[0] == "admin");

        w.status(200).text("OK"); });

    assert(res.result_int() == 200);
    assert(res.body() == "OK");

    std::cout << "[OK] jwt example: subject=" << "user123" << " role=admin\n";
    return 0;
}
