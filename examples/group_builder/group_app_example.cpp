/**
 *
 *  @file group_app_example.cpp — Groups + protect() demo (Vix.cpp)
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
// Run:
//   vix run group_app_example.cpp
//
// Test:
//   curl -i http://localhost:8080/
//   curl -i http://localhost:8080/api/public
//   curl -i http://localhost:8080/api/secure
//   curl -i -H "x-api-key: secret" http://localhost:8080/api/secure
//
// NOTE:
//   /api/admin/dashboard uses JWT + RBAC (admin role).
// ============================================================================

#include <iostream>
#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>

using namespace vix;

int main()
{
  App app;

  // Root
  app.get("/", [](Request &, Response &res)
          { res.send("Welcome. Try /api/public, /api/secure, /api/admin/dashboard"); });

  // GROUP: /api
  app.group("/api", [&](App::Group &api)
            {
        // Public API
        api.get("/public", [](Request &, Response &res)
        {
            res.send("Public API endpoint");
        });

        // Protect /api/secure with API key
        api.protect("/secure", middleware::app::api_key_dev("secret"));

        api.get("/secure", [](Request &req, Response &res)
        {
            auto &k = req.state<vix::middleware::auth::ApiKey>();
            res.json({
                "ok", true,
                "api_key", k.value
            });
        });

        // Nested group: /api/admin (JWT + RBAC)
        api.group("/admin", [&](App::Group &admin)
        {
            // Apply auth to the whole group
            admin.use(middleware::app::jwt_auth("dev_secret"));
            admin.use(middleware::app::rbac_admin()); // role=admin

            admin.get("/dashboard", [](Request &req, Response &res)
            {
                auto &authz = req.state<vix::middleware::auth::Authz>();
                res.json({
                    "ok", true,
                    "sub", authz.subject,
                    "role", "admin"
                });
            });
        }); });

  // Help
  std::cout
      << "Vix Groups example running:\n"
      << "  http://localhost:8080/\n"
      << "  http://localhost:8080/api/public\n"
      << "  http://localhost:8080/api/secure\n"
      << "  http://localhost:8080/api/admin/dashboard\n\n"
      << "API KEY:\n"
      << "  secret\n\n"
      << "Try:\n"
      << "  curl -i http://localhost:8080/api/public\n"
      << "  curl -i http://localhost:8080/api/secure\n"
      << "  curl -i -H \"x-api-key: secret\" http://localhost:8080/api/secure\n"
      << "  curl -i \"http://localhost:8080/api/secure?api_key=secret\"\n";

  app.run(8080);
  return 0;
}
