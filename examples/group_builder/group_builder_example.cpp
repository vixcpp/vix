// ============================================================================
// group_builder_example.cpp — group() builder style (Vix.cpp)
// ----------------------------------------------------------------------------
// Run:
//   vix run group_builder_example.cpp
//
// Test:
//   curl -i http://localhost:8080/api/public
//   curl -i http://localhost:8080/api/secure
//   curl -i -H "x-api-key: secret" http://localhost:8080/api/secure
//   curl -i "http://localhost:8080/api/secure?api_key=secret"
// ============================================================================
#include <iostream>
#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>

using namespace vix;

int main()
{
    App app;

    // Create /api group
    auto api = app.group("/api");

    // Public endpoint
    api.get("/public", [](Request &, Response &res)
            { res.send("Public endpoint"); });

    // 🔐 Protect all following /api routes with API key (DEV preset)
    api.use(middleware::app::api_key_dev("secret"));

    // Secure endpoint
    api.get("/secure", [](Request &req, Response &res)
            {
        auto &k = req.state<vix::middleware::auth::ApiKey>();
        res.json({
            "ok", true,
            "api_key", k.value
        }); });

    std::cout
        << "Running:\n"
        << "  http://localhost:8080/api/public\n"
        << "  http://localhost:8080/api/secure\n";

    app.run(8080);
    return 0;
}
