/**
 *
 *  @file examples/http/json_builders_routes.cpp
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
// ============================================================================
// GET /hello          -> {"message":"Hello, World!"}
// GET /users/{id}     -> {"user":{"id":"<id>","active":true}}
// GET /roles          -> {"roles":["admin","editor","viewer"]}
// ============================================================================

#include <vix.hpp>
#include <vix/json/Simple.hpp>

using namespace vix;
namespace J = vix::json;

int main()
{
  App app;

  // GET /hello -> {"message": "Hello, World!"}
  app.get("/hello", [](Request &, Response &res)
          { res.json({"message", "Hello, World!"}); });

  // GET /users/{id} -> {"user": {"id": "...", "active": true}}
  app.get("/users/{id}", [](Request &req, Response &res)
          {
        const std::string id = req.param("id");
        res.json({
            "user", J::obj({
                "id",     id,
                "active", true
            })
        }); });

  // GET /roles -> {"roles": ["admin", "editor", "viewer"]}
  app.get("/roles", [](Request &, Response &res)
          { res.json({"roles", J::array({"admin", "editor", "viewer"})}); });

  app.run(8080);
  return 0;
}
