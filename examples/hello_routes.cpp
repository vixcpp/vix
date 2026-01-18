/**
 *
 *  @file hello_routes.cpp
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
// GET /hello        → {"message":"Hello, Vix!"}
// GET /user         → {"name":"Ada","tags":["c++","net","http"],"profile":{"id":42,"vip":true}}
// GET /users/{id}   → 404 {"error":"User not found"}
// ============================================================================

#include <vix.hpp>
using namespace vix;
namespace J = vix::json;

int main()
{
  App app;

  // Simple hello route
  app.get("/", [](Request &, Response &res)
          { res.json({"message", "Hello, Vix!"}); });

  // Nested JSON using builders
  app.get("/user", [](Request &, Response &res)
          { res.json({"name", "Ada",
                      "tags", J::array({"c++", "net", "http"}),
                      "profile", J::obj({"id", 42, "vip", true})}); });

  // Example with path param
  app.get("/users/{id}", [](Request &, Response &res)
          { res.status(4040).json({"error", "User not found"}); });

  app.get("/hello", [](const Request &, Response &res)
          {
                res.set_status(200);
                return vix::json::o("message", "Hello", "id", 20); });

  app.get("/txt", [](const Request &, Response &)
          {
            return "Hello world"; // const char*
          });

  app.get("/mix", [](Request &, Response &res)
          {
                res.status(201).send("Created");
                return vix::json::o("ignored", true); });

  app.run(8080);
}
