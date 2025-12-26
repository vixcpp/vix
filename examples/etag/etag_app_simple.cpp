// ============================================================================
// etag_app_simple.cpp — ETag middleware example (Vix.cpp)
// ----------------------------------------------------------------------------
// Run:
//   vix run etag_app_simple.cpp
//
// Test:
//   curl -i http://localhost:8080/x
//   curl -i -H 'If-None-Match: <etag>' http://localhost:8080/x
//   curl -I http://localhost:8080/x
// ============================================================================

#include <iostream>
#include <string>
#include <vix.hpp>

#include <vix/middleware/app/adapter.hpp>
#include <vix/middleware/performance/etag.hpp>

using namespace vix;

static void print_help()
{
    std::cout
        << "Vix ETag example running:\n"
        << "  http://localhost:8080/x\n\n"
        << "Try:\n"
        << "  curl -i http://localhost:8080/x\n"
        << "  curl -i -H 'If-None-Match: <etag>' http://localhost:8080/x\n"
        << "  curl -I http://localhost:8080/x\n";
}

int main()
{
    App app;

    // Install ETag middleware globally
    auto mw = vix::middleware::app::adapt_ctx(
        vix::middleware::performance::etag({.weak = true,
                                            .add_cache_control_if_missing = false,
                                            .min_body_size = 1}));
    app.use(std::move(mw));

    app.head("/x", [](Request &, Response &res)
             { res.status(200); });

    print_help();
    app.run(8080);
    return 0;
}
