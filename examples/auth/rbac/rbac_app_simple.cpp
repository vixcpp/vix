/**
 *
 *  @file rbac_app_simple.cpp — RBAC (roles + perms) example (Vix.cpp)
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
//   vix run rbac_app_simple.cpp
//
// Test:
//   curl -i http://localhost:8080/
//   curl -i http://localhost:8080/admin
//
// Valid token (admin + products:write):
//   curl -i -H "Authorization: Bearer <TOKEN_OK>" http://localhost:8080/admin
//
// Invalid token (admin but missing products:write):
//   curl -i -H "Authorization: Bearer <TOKEN_NO_PERM>" http://localhost:8080/admin
// ============================================================================

#include <iostream>
#include <string>

#include <vix.hpp>

#include <vix/middleware/app/adapter.hpp>
#include <vix/middleware/auth/jwt.hpp>
#include <vix/middleware/auth/rbac.hpp>

using namespace vix;

// HS256, secret=dev_secret
// payload: {"sub":"user123","roles":["admin"],"perms":["products:write","orders:read"]}
static const std::string TOKEN_OK =
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
    "eyJzdWIiOiJ1c2VyMTIzIiwicm9sZXMiOlsiYWRtaW4iXSwicGVybXMiOlsicHJvZHVjdHM6d3JpdGUiLCJvcmRlcnM6cmVhZCJdfQ."
    "w1y3nA2F1kq0oJ0x8wWc5wQx8zF4h2d6V7mYp0jYk3Q";

// HS256, secret=dev_secret
// payload: {"sub":"user123","roles":["admin"],"perms":["orders:read"]}  (missing products:write)
static const std::string TOKEN_NO_PERM =
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
    "eyJzdWIiOiJ1c2VyMTIzIiwicm9sZXMiOlsiYWRtaW4iXSwicGVybXMiOlsib3JkZXJzOnJlYWQiXX0."
    "qVqWmQmHf4yqPzvYzGf9m3jv9oGzW0Q8c8qkQkqkQkQ";

int main()
{
  App app;

  // 1) JWT auth (puts JwtClaims into request state)
  vix::middleware::auth::JwtOptions jwt_opt{};
  jwt_opt.secret = "dev_secret";
  jwt_opt.verify_exp = false;

  // 2) RBAC: build Authz from JwtClaims, then enforce rules
  vix::middleware::auth::RbacOptions rbac_opt{};
  rbac_opt.require_auth = true;
  rbac_opt.use_resolver = false; // keep the example simple

  auto jwt_mw = vix::middleware::app::adapt_ctx(vix::middleware::auth::jwt(jwt_opt));
  auto ctx_mw = vix::middleware::app::adapt_ctx(vix::middleware::auth::rbac_context(rbac_opt));
  auto role_mw = vix::middleware::app::adapt_ctx(vix::middleware::auth::require_role("admin"));
  auto perm_mw = vix::middleware::app::adapt_ctx(vix::middleware::auth::require_perm("products:write"));

  // Protect only /admin
  app.use(vix::middleware::app::when(
      [](const Request &req)
      { return req.path() == "/admin"; },
      std::move(jwt_mw)));
  app.use(vix::middleware::app::when(
      [](const Request &req)
      { return req.path() == "/admin"; },
      std::move(ctx_mw)));
  app.use(vix::middleware::app::when(
      [](const Request &req)
      { return req.path() == "/admin"; },
      std::move(role_mw)));
  app.use(vix::middleware::app::when(
      [](const Request &req)
      { return req.path() == "/admin"; },
      std::move(perm_mw)));

  // Public
  app.get("/", [](Request &, Response &res)
          { res.send("RBAC example: /admin requires role=admin + perm=products:write"); });

  // Protected
  app.get("/admin", [](Request &req, Response &res)
          {
        auto& authz = req.state<vix::middleware::auth::Authz>();

        res.json({
            "ok", true,
            "sub", authz.subject,
            "has_admin", authz.has_role("admin"),
            "has_products_write", authz.has_perm("products:write")
        }); });

  std::cout
      << "Vix RBAC example running:\n"
      << "  http://localhost:8080/\n"
      << "  http://localhost:8080/admin\n\n"
      << "TOKEN_OK:\n  " << TOKEN_OK << "\n\n"
      << "TOKEN_NO_PERM:\n  " << TOKEN_NO_PERM << "\n\n"
      << "Try:\n"
      << "  curl -i http://localhost:8080/admin\n"
      << "  curl -i -H \"Authorization: Bearer " << TOKEN_OK << "\" http://localhost:8080/admin\n"
      << "  curl -i -H \"Authorization: Bearer " << TOKEN_NO_PERM << "\" http://localhost:8080/admin\n";

  app.run(8080);
  return 0;
}
