// ============================================================================
// http_cache_app_debug.cpp — HTTP Cache (Debug + Vary)
// ----------------------------------------------------------------------------
// Run:
//   vix run examples/http_cache_app_debug.cpp
//
// Test:
//   curl -i "http://localhost:8080/api/users"
//   curl -i "http://localhost:8080/api/users"                 # cached
//   curl -i -H "Accept-Language: fr" "http://localhost:8080/api/users"
//   curl -i -H "Accept-Language: en" "http://localhost:8080/api/users"  # different cache key
//
// # 1) MISS
// curl -i "http://localhost:8080/api/users"

// # 2) HIT
// curl -i "http://localhost:8080/api/users"

// # 3) BYPASS (force origin)
// curl -i -H "x-vix-cache: bypass" "http://localhost:8080/api/users"
// Notes:
// - Adds debug header x-vix-cache-status (HIT/MISS/BYPASS depending on impl)
// - Demonstrates vary headers (Accept-Language)
// ============================================================================

#include <vix.hpp>
#include <vix/middleware/app/app_middleware.hpp>

using namespace vix;

static void register_routes(App &app)
{
    app.get("/", [](Request &, Response &res)
            { res.text("home (not cached)"); });

    app.get("/api/users", [](Request &req, Response &res)
            {
        // ✅ Request API you actually have:
        // - req.has_header(name)
        // - req.header(name) -> std::string
        const std::string lang =
            req.has_header("accept-language") ? req.header("accept-language") : "none";

        // ✅ Response API you actually have:
        // res.json(vix::json::kvs) or res.json({tokens...})
        res.status(200).json(vix::json::obj({
            "message", "users from origin",
            "accept_language", lang
        })); });
}

int main()
{
    App app;

    app.use("/api/", middleware::app::http_cache({
                         .ttl_ms = 30'000,
                         .allow_bypass = true,
                         .bypass_header = "x-vix-cache",
                         .bypass_value = "bypass",

                         // Create different cache entries per language header
                         .vary_headers = {"accept-language"},

                         // Useful for demo/learning
                         .add_debug_header = true,
                         .debug_header = "x-vix-cache-status",
                     }));

    register_routes(app);

    app.run(8080);
    return 0;
}
