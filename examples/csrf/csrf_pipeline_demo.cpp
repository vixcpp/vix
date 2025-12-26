// ============================================================================
// csrf_pipeline_demo.cpp — CSRF pipeline demo (Vix.cpp)
// ----------------------------------------------------------------------------
// Run:
//   vix run csrf_pipeline_demo.cpp
// ============================================================================

#include <cassert>
#include <iostream>

#include <boost/beast/http.hpp>

#include <vix/middleware/pipeline.hpp>
#include <vix/middleware/security/csrf.hpp>

using namespace vix::middleware;

static vix::vhttp::RawRequest make_post(bool ok)
{
    namespace http = boost::beast::http;

    vix::vhttp::RawRequest req{http::verb::post, "/api/update", 11};
    req.set(http::field::host, "localhost");

    // Cookie + header must match
    req.set("Cookie", "csrf_token=abc");
    req.set("x-csrf-token", ok ? "abc" : "wrong");

    req.body() = "x=1";
    req.prepare_payload();
    return req;
}

int main()
{
    namespace http = boost::beast::http;

    // FAIL
    {
        auto raw = make_post(false);
        http::response<http::string_body> res;

        vix::vhttp::Request req(raw, {});
        vix::vhttp::ResponseWrapper w(res);

        HttpPipeline p;
        p.use(vix::middleware::security::csrf()); // MiddlewareFn(Context&, Next)

        int final_calls = 0;
        p.run(req, w, [&](Request &, Response &)
              {
            final_calls++;
            w.ok().text("OK"); });

        assert(final_calls == 0);
        assert(res.result_int() == 403);
    }

    // OK
    {
        auto raw = make_post(true);
        http::response<http::string_body> res;

        vix::vhttp::Request req(raw, {});
        vix::vhttp::ResponseWrapper w(res);

        HttpPipeline p;
        p.use(vix::middleware::security::csrf());

        int final_calls = 0;
        p.run(req, w, [&](Request &, Response &)
              {
            final_calls++;
            w.ok().text("OK"); });

        assert(final_calls == 1);
        assert(res.result_int() == 200);
    }

    std::cout << "[OK] csrf pipeline demo\n";
    return 0;
}
