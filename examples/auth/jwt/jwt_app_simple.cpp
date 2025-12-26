// ============================================================================
// jwt_app_simple.cpp — JWT middleware (App) super simple
// ----------------------------------------------------------------------------
// Run:
//   vix run jwt_app_simple.cpp
//
// Test:
//   curl -i http://localhost:8080/
//   curl -i http://localhost:8080/secure
//
// Valid token (HS256, secret=dev_secret):
//   TOKEN="eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJ1c2VyMTIzIiwicm9sZXMiOlsiYWRtaW4iXX0.3HK5b1sXMbxkjC3Tllwtcuzxm-1OI0D184Fuav0-XQo"
//
//   curl -i -H "Authorization: Bearer $TOKEN" http://localhost:8080/secure
// ============================================================================
#include <iostream>
#include <string>

#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>

using namespace vix;

static const std::string kToken =
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
    "eyJzdWIiOiJ1c2VyMTIzIiwicm9sZXMiOlsiYWRtaW4iXX0."
    "3HK5b1sXMbxkjC3Tllwtcuzxm-1OI0D184Fuav0-XQo"; // HS256, secret=dev_secret

int main()
{
    App app;

    // 🔐 Protect ONLY /secure (dev preset: verify_exp = false)
    app.use("/secure", middleware::app::jwt_dev("dev_secret"));

    app.get("/", [](Request &, Response &res)
            { res.send(
                  "JWT example:\n"
                  "  GET /secure requires Bearer token.\n"
                  "\n"
                  "Try:\n"
                  "  curl -i http://localhost:8080/secure\n"
                  "  curl -i -H \"Authorization: Bearer <TOKEN>\" http://localhost:8080/secure\n"); });

    app.get("/secure", [](Request &req, Response &res)
            {
                auto &claims = req.state<vix::middleware::auth::JwtClaims>();
                res.json({"ok", true,
                          "sub", claims.subject,
                          "roles", claims.roles});
                res.status(501).json({"ok", false,
                                      "error", "JWT middleware not enabled (VIX_ENABLE_JWT)"}); });

    std::cout
        << "Vix JWT example running:\n"
        << "  http://localhost:8080/\n"
        << "  http://localhost:8080/secure\n\n"
        << "Use this token:\n"
        << "  " << kToken << "\n\n"
        << "Test:\n"
        << "  curl -i -H \"Authorization: Bearer " << kToken
        << "\" http://localhost:8080/secure\n";

    app.run(8080);
    return 0;
}
