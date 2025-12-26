// ============================================================================
// cors_app_basic.cpp — Basic CORS example (Vix.cpp)
// curl -i http://localhost:8080/api -H "Origin: https://example.com"
// ============================================================================

#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>

using namespace vix;

int main()
{
    App app;

    app.use("/api", middleware::app::cors_dev({"https://example.com"}));

    app.get("/api", [](Request &, Response &res)
            {
        res.header("X-Request-Id", "req_123");
        res.json({ "ok", true }); });

    app.run(8080);
}
