/**
 *
 *  @file security_cors_csrf_server.cpp — CORS + CSRF (Vix.cpp)
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.  All rights reserved.
 *  https://github.com/vixcpp/vix
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 *
 */
// ----------------------------------------------------------------------------
// Goal:
//   - OPTIONS handled by CORS middleware (preflight)
//   - POST protected by CSRF (cookie token must match header token)
//   - Both middlewares only apply to "/api" prefix
//
// Run:
//   vix run security_cors_csrf_server.cpp
//
// Tests:
//
//   # Preflight (ALLOWED origin => 204 + CORS headers)
//   curl -i -X OPTIONS http://localhost:8080/api/update \
//     -H "Origin: https://example.com" \
//     -H "Access-Control-Request-Method: POST" \
//     -H "Access-Control-Request-Headers: Content-Type, X-CSRF-Token"
//
//   # Preflight (BLOCKED origin => 403)
//   curl -i -X OPTIONS http://localhost:8080/api/update \
//     -H "Origin: https://evil.com" \
//     -H "Access-Control-Request-Method: POST"
//
//   # Get CSRF cookie (sets csrf_token=abc)
//   curl -i -c cookies.txt http://localhost:8080/api/csrf \
//     -H "Origin: https://example.com"
//
//   # FAIL (missing header)
//   curl -i -b cookies.txt -X POST http://localhost:8080/api/update \
//     -H "Origin: https://example.com" \
//     -d "x=1"
//
//   # FAIL (wrong token)
//   curl -i -b cookies.txt -X POST http://localhost:8080/api/update \
//     -H "Origin: https://example.com" \
//     -H "X-CSRF-Token: wrong" \
//     -d "x=1"
//
//   # OK (correct token)
//   curl -i -b cookies.txt -X POST http://localhost:8080/api/update \
//     -H "Origin: https://example.com" \
//     -H "X-CSRF-Token: abc" \
//     -d "x=1"
// ============================================================================

#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>

using namespace vix;

int main()
{
  App app;

  // Apply on /api (order matters)
  middleware::app::protect_prefix(app, "/api",
                                  middleware::app::cors_dev({"https://example.com"}));

  // CSRF expects: cookie "csrf_token" and header "x-csrf-token" by default
  middleware::app::protect_prefix(app, "/api",
                                  middleware::app::csrf_dev("csrf_token", "x-csrf-token", false));
  // ou strict:
  // middleware::app::protect_prefix(app, "/api",
  //     middleware::app::csrf_strict_dev("csrf_token", "x-csrf-token"));

  // Routes
  app.get("/api/csrf", [](Request &, Response &res)
          {
        res.header("Set-Cookie", "csrf_token=abc; Path=/; SameSite=Lax");
        res.header("X-Request-Id", "req_123");
        res.json({ "csrf_token", "abc" }); });

  app.post("/api/update", [](Request &, Response &res)
           {
        res.header("X-Request-Id", "req_456");
        res.json({ "ok", true, "message", "CORS ✅ + CSRF ✅" }); });

  app.get("/", [](Request &, Response &res)
          { res.send("Welcome"); });

  app.run(8080);
}
