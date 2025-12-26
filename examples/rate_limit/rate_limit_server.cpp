// ============================================================================
// rate_limit_server.cpp — Rate limit server demo (Vix.cpp)
// ----------------------------------------------------------------------------
// Run:
//   vix run rate_limit_server.cpp
//
// Endpoints:
//   GET /              (public)
//   GET /api/ping      (rate limited)
// ============================================================================
#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>

using namespace vix;

int main()
{
    App app;

    // burst=5, refill=0 => easy to trigger
    app.use("/api", middleware::app::rate_limit_custom_dev(5.0, 0.0));

    app.get("/", [](Request &, Response &res)
            { res.send("public route"); });

    app.get("/api/ping", [](Request &req, Response &res)
            { res.json({"ok", true, "msg", "pong", "xff", req.header("x-forwarded-for")}); });

    app.run(8080);
}
