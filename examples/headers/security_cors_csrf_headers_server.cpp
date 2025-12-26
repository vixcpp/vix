// ============================================================================
// security_cors_csrf_headers_server.cpp — CORS + CSRF + Security Headers (Vix.cpp)
// ----------------------------------------------------------------------------
// Goal (realistic app):
//   - OPTIONS handled by CORS middleware (preflight)
//   - POST protected by CSRF (cookie token must match header token)
//   - Security headers added on ALL /api responses (including errors)
//
// Run:
//   vix run security_cors_csrf_headers_server.cpp
//
// Terminal tests (curl):
//
//   # 1) Preflight allowed (204 + CORS headers)
//   curl -i -X OPTIONS http://localhost:8080/api/update \
//     -H "Origin: https://example.com" \
//     -H "Access-Control-Request-Method: POST" \
//     -H "Access-Control-Request-Headers: Content-Type, X-CSRF-Token"
//
//   # 2) Preflight blocked (403)
//   curl -i -X OPTIONS http://localhost:8080/api/update \
//     -H "Origin: https://evil.com" \
//     -H "Access-Control-Request-Method: POST"
//
//   # 3) Get CSRF cookie (sets csrf_token=abc)
//   curl -i -c cookies.txt http://localhost:8080/api/csrf \
//     -H "Origin: https://example.com"
//
//   # 4) FAIL: missing CSRF header
//   curl -i -b cookies.txt -X POST http://localhost:8080/api/update \
//     -H "Origin: https://example.com" \
//     -d "x=1"
//
//   # 5) FAIL: wrong token
//   curl -i -b cookies.txt -X POST http://localhost:8080/api/update \
//     -H "Origin: https://example.com" \
//     -H "X-CSRF-Token: wrong" \
//     -d "x=1"
//
//   # 6) OK: correct token
//   curl -i -b cookies.txt -X POST http://localhost:8080/api/update \
//     -H "Origin: https://example.com" \
//     -H "X-CSRF-Token: abc" \
//     -d "x=1"
//
// Browser demo:
//   - Serve index.html via any static server on another port (ex: 5173)
//   - Open it and click buttons.
// ============================================================================
#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>

using namespace vix;

int main()
{
    App app;

    // Apply on ALL /api/*
    // Order matters: headers first, then CORS, then CSRF.
    app.use("/api", middleware::app::security_headers_dev()); // HSTS off by default
    app.use("/api", middleware::app::cors_dev({
                        "http://localhost:5173",
                        "http://0.0.0.0:5173",
                        "https://example.com" // for your curl tests
                    }));
    app.use("/api", middleware::app::csrf_dev("csrf_token", "x-csrf-token", false));

    // Explicit OPTIONS routes (lets CORS middleware answer preflight)
    app.options("/api/update", [](Request &, Response &res)
                { res.status(204).send(); });

    app.options("/api/csrf", [](Request &, Response &res)
                { res.status(204).send(); });

    // Routes
    app.get("/api/csrf", [](Request &, Response &res)
            {
        // For cross-origin cookie in browsers: HTTPS + SameSite=None; Secure
        // For local dev HTTP: SameSite=Lax is fine but cookie might not be sent cross-site.
        res.header("Set-Cookie", "csrf_token=abc; Path=/; SameSite=Lax");
        res.header("X-Request-Id", "req_csrf_1");
        res.json({"csrf_token", "abc"}); });

    app.post("/api/update", [](Request &, Response &res)
             {
        res.header("X-Request-Id", "req_update_1");
        res.json({"ok", true, "message", "CORS ✅ + CSRF ✅ + HEADERS ✅"}); });

    app.get("/", [](Request &, Response &res)
            { res.send("public route"); });

    app.run(8080);
}
