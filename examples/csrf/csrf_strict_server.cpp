/**
 *
 *  @file csrf_strict_server.cpp — CSRF middleware example (Vix.cpp)
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
// Run:
//   vix run csrf_strict_server.cpp
//
// Test:
//   # 1) Get token (cookie)
//   curl -i -c cookies.txt http://localhost:8080/api/csrf
//
//   # 2) FAIL: missing header
//   curl -i -b cookies.txt -X POST http://localhost:8080/api/update -d "x=1"
//
//   # 3) FAIL: wrong token
//   curl -i -b cookies.txt -X POST http://localhost:8080/api/update \
//     -H "x-csrf-token: wrong" -d "x=1"
//
//   # 4) OK: header token matches cookie token
//   curl -i -b cookies.txt -X POST http://localhost:8080/api/update \
//     -H "x-csrf-token: abc" -d "x=1"
// ============================================================================

#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>

using namespace vix;

int main()
{
  App app;

  app.use("/api", middleware::app::csrf_dev());

  app.get("/api/csrf", [](Request &, Response &res)
          {
        res.header("Set-Cookie", "csrf_token=abc; Path=/; SameSite=Lax");
        res.json({ "csrf_token", "abc" }); });

  app.post("/api/update", [](Request &, Response &res)
           { res.json({"ok", true, "message", "CSRF passed ✅"}); });

  app.run(8080);
}
