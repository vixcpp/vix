/**
 *
 *  @file  http_cache_app_custom_cache.cpp — HTTP Cache (Custom Cache Injection)
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
//   vix run examples/http_cache_app_custom_cache.cpp
//
// Test:
//   curl -i "http://localhost:8080/api/slow"
//   curl -i "http://localhost:8080/api/slow"   # second call should be cached
//
// Notes:
// - Injects a custom Cache instance (MemoryStore + CachePolicy)
// - Still uses App-level middleware adapter (no RawRequest exposed)
// ============================================================================

#include <chrono>
#include <thread>

#include <vix.hpp>
#include <vix/middleware/app/http_cache.hpp>

using namespace vix;

static void register_routes(App &app)
{
  app.get("/api/slow", [](Request &, Response &res)
          {
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        res.text("slow response (origin)"); });

  app.get("/", [](Request &, Response &res)
          { res.text("home (not cached)"); });
}

int main()
{
  App app;

  // Build a default cache instance (MemoryStore + policy) with ttl
  auto cache = middleware::app::make_default_cache({
      .ttl_ms = 30'000,
  });

  // Install middleware using injected cache
  app.use("/api/", middleware::app::http_cache_mw({
                       .prefix = "/api/",
                       .only_get = true,
                       .ttl_ms = 30'000,

                       .allow_bypass = true,
                       .bypass_header = "x-vix-cache",
                       .bypass_value = "bypass",

                       .vary_headers = {},
                       .cache = cache,

                       .add_debug_header = true,
                       .debug_header = "x-vix-cache-status",
                   }));

  register_routes(app);

  app.run(8080);
  return 0;
}
