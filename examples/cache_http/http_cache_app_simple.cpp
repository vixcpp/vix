/**
 *
 *  @file  http_cache_app_simple.cpp — HTTP Cache (Simple)
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
//   vix run examples/http_cache_app_simple.cpp
//
// Test:
//   curl -i "http://localhost:8080/api/users"
//   curl -i "http://localhost:8080/api/users"              # should be cached
//   curl -i -H "x-vix-cache: bypass" "http://localhost:8080/api/users"  # bypass
//
// Notes:
// - Caches GET /api/* for 30s
// - Bypass via header x-vix-cache: bypass or query api_key-style if supported
// ============================================================================

#include <vix.hpp>
#include <vix/middleware/app/app_middleware.hpp>

using namespace vix;

static void register_routes(App &app)
{
  app.get("/", [](Request &, Response &res)
          { res.text("home (not cached)"); });

  app.get("/api/users", [](Request &, Response &res)
          { res.text("users from origin"); });
}

int main()
{
  App app;

  // Cache GET requests under /api/*
  app.use("/api/", middleware::app::http_cache({
                       .ttl_ms = 30'000,
                       .allow_bypass = true,
                       .bypass_header = "x-vix-cache",
                       .bypass_value = "bypass",
                   }));

  register_routes(app);

  app.run(8080);
  return 0;
}
