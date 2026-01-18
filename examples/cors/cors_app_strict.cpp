/**
 *
 *  @file cors_app_strict.cpp — Strict CORS + controlled preflight (Vix.cpp)
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
// # allowed
// curl -i -X OPTIONS http://localhost:8080/api \
//   -H "Origin: https://example.com" \
//   -H "Access-Control-Request-Method: POST"
//
// # blocked
// curl -i -X OPTIONS http://localhost:8080/api \
//   -H "Origin: https://evil.com" \
//   -H "Access-Control-Request-Method: POST"
// ============================================================================

#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>

using namespace vix;

int main()
{
  App app;

  // 🔒 Apply CORS only on /api prefix
  app.use("/api", middleware::app::cors_dev({"https://example.com"}));

  // ✅ Explicit OPTIONS route (lets middleware answer preflight)
  app.options("/api", [](Request &, Response &res)
              {
        // Optional debug marker (only if this handler executes)
        res.header("X-OPTIONS-HIT", "1");
        res.status(204).send(); });

  app.get("/api", [](Request &, Response &res)
          {
        res.header("X-Request-Id", "req_123");
        res.json({ "ok", true }); });

  app.run(8080);
}
