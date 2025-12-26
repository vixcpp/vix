// ============================================================================
// rate_limit_pipeline_demo.cpp — Rate limit pipeline demo (Vix.cpp)
// ----------------------------------------------------------------------------
// Run:
//   vix run rate_limit_pipeline_demo.cpp
// ============================================================================
#include <cassert>
#include <iostream>
#include <memory>

#include <boost/beast/http.hpp>

#include <vix/middleware/pipeline.hpp>
#include <vix/middleware/security/rate_limit.hpp>

using namespace vix::middleware;

static vix::vhttp::RawRequest make_req()
{
    namespace http = boost::beast::http;
    vix::vhttp::RawRequest req{http::verb::get, "/api/x", 11};
    req.set(http::field::host, "localhost");
    req.set("x-forwarded-for", "1.2.3.4");
    req.prepare_payload();
    return req;
}

int main()
{
    namespace http = boost::beast::http;

    vix::middleware::security::RateLimitOptions opt{};
    opt.capacity = 2.0;
    opt.refill_per_sec = 0.0;
    opt.add_headers = true;

    HttpPipeline p;

    auto shared = std::make_shared<vix::middleware::security::RateLimiterState>();
    p.services().provide<vix::middleware::security::RateLimiterState>(shared);

    p.use(vix::middleware::security::rate_limit(opt));

    auto run_once = [&](http::response<http::string_body> &res)
    {
        auto raw = make_req();
        vix::vhttp::Request req(raw, {});
        vix::vhttp::ResponseWrapper w(res);

        p.run(req, w, [&](Request &, Response &)
              { w.ok().text("OK"); });
    };

    // 1) OK
    {
        http::response<http::string_body> res;
        run_once(res);
        assert(res.result_int() == 200);
        assert(res.body() == "OK");
        assert(!res["X-RateLimit-Limit"].empty());
        assert(!res["X-RateLimit-Remaining"].empty());
    }

    // 2) OK
    {
        http::response<http::string_body> res;
        run_once(res);
        assert(res.result_int() == 200);
        assert(res.body() == "OK");
    }

    // 3) BLOCKED
    {
        http::response<http::string_body> res;
        run_once(res);
        assert(res.result_int() == 429);
        assert(res.body().find("rate_limited") != std::string::npos);
        assert(!res["Retry-After"].empty());
        assert(res["X-RateLimit-Remaining"] == "0");
    }

    std::cout << "[OK] rate_limit pipeline demo\n";
    return 0;
}
