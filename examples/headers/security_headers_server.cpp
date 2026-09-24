/**
 *
 *  @file security_headers_server.cpp — Security headers middleware example (Vix.cpp)
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
//   - Apply security headers only on /api prefix
//   - Keep / public route without forced headers (demo)
//
// Run:
//   vix run security_headers_server.cpp
//
// Tests:
//   curl -i http://localhost:8080/api/ping
//   curl -i http://localhost:8080/
// ============================================================================

#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>

using namespace vix;

int main()
{
  App app;

  // 🔒 Apply security headers only on /api
  app.use("/api", middleware::app::security_headers_dev());

  app.get("/api/ping", [](Request &, Response &res)
          { res.json({"ok", true, "message", "headers applied ✅"}); });

  // Public route (no forced headers)
  app.get("/", [](Request &, Response &res)
          { res.send("public route"); });

  app.run(8080);
}
