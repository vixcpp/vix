/**
 *
 *  @file api_key_app_simple.cpp — API Key auth example (Vix.cpp)
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
//   vix run api_key_app_simple.cpp
//
// Test:
//   # Missing key -> 401
//   curl -i http://localhost:8080/secure
//
//   # Invalid key -> 403
//   curl -i -H "x-api-key: wrong" http://localhost:8080/secure
//   curl -i "http://localhost:8080/secure?api_key=wrong"
//
//   # Valid key -> 200
//   curl -i -H "x-api-key: secret" http://localhost:8080/secure
//   curl -i "http://localhost:8080/secure?api_key=secret"
// ============================================================================

#include <iostream>
#include <string>

#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>

using namespace vix;

static void print_help()
{
  std::cout
      << "Vix API Key example running:\n"
      << "  http://localhost:8080/\n"
      << "  http://localhost:8080/secure\n\n"
      << "Valid key:\n"
      << "  secret\n\n"
      << "Try:\n"
      << "  curl -i http://localhost:8080/secure\n"
      << "  curl -i -H \"x-api-key: wrong\" http://localhost:8080/secure\n"
      << "  curl -i -H \"x-api-key: secret\" http://localhost:8080/secure\n"
      << "  curl -i \"http://localhost:8080/secure?api_key=wrong\"\n"
      << "  curl -i \"http://localhost:8080/secure?api_key=secret\"\n\n";
}

int main()
{
  App app;

  // ---------------------------------------------------------------------
  // API key protection (preset)
  //  - Header: x-api-key
  //  - Query : ?api_key=
  //  - Allowed key: "secret"
  // ---------------------------------------------------------------------
  app.use("/secure", middleware::app::api_key_dev("secret"));
  // app.use(
  //     "/secure",
  //     middleware::app::api_key_auth({
  //         .header = "x-api-key",
  //         .query_param = "api_key",
  //         .allowed_keys = {"secret"},
  //     }));

  // ---------------------------------------------------------------------
  // Routes
  // ---------------------------------------------------------------------
  app.get("/", [](Request &, Response &res)
          { res.send(
                "API Key example:\n"
                "  /secure requires x-api-key: secret OR ?api_key=secret\n"); });

  app.get("/secure", [](Request &req, Response &res)
          {
         auto &key = req.state<vix::middleware::auth::ApiKey>();

        res.json({
            "ok", true,
            "api_key", key.value
        }); });

  print_help();
  app.run(8080);
  return 0;
}
