// ============================================================================
// ip_filter_server.cpp — IP filter middleware example (Vix.cpp)
// ----------------------------------------------------------------------------
// Goal:
//   - Protect /api/* using ip_filter()
//   - Client IP extracted from X-Forwarded-For (first value)
//   - Demonstrate deny + allow behavior
//
// Run:
//   vix run ip_filter_server.cpp
//
// Tests:
//
//   # Public route (no middleware)
//   curl -i http://localhost:8080/
//
//   # Allowed IP (in allow list)
//   curl -i http://localhost:8080/api/hello -H "X-Forwarded-For: 10.0.0.1"
//
//   # Not allowed (not in allow list)
//   curl -i http://localhost:8080/api/hello -H "X-Forwarded-For: 1.2.3.4"
//
//   # Denied explicitly (deny wins if you configure both)
//   curl -i http://localhost:8080/api/hello -H "X-Forwarded-For: 9.9.9.9"
//
//   # X-Forwarded-For with chain: "client, proxy1, proxy2"
//   curl -i http://localhost:8080/api/hello -H "X-Forwarded-For: 10.0.0.1, 127.0.0.1"
// ============================================================================

// ============================================================================
// ip_filter_server.cpp — IP filter middleware example (Vix.cpp)
// ----------------------------------------------------------------------------
// Run:
//   vix run ip_filter_server.cpp
// ============================================================================
#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>

using namespace vix;

int main()
{
    App app;

    // Apply on /api/*
    app.use("/api", middleware::app::ip_filter_allow_deny_dev(
                        "x-forwarded-for",
                        {"10.0.0.1", "127.0.0.1"}, // allow
                        {"9.9.9.9"},               // deny (priority)
                        true                       // fallback to x-real-ip, etc.
                        ));

    // Routes
    app.get("/", [](Request &, Response &res)
            { res.send("public route"); });

    app.get("/api/hello", [](Request &req, Response &res)
            { res.json({"ok", true,
                        "message", "Hello from /api/hello",
                        "x_forwarded_for", req.header("x-forwarded-for"),
                        "x_real_ip", req.header("x-real-ip")}); });

    app.run(8080);
}
