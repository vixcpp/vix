/**
 *
 *  @file examples/http_crud/put_update_user.cpp
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

#include <vix.hpp>
#include <vix/json/Simple.hpp>
#include <string>

using namespace vix;
namespace J = vix::json;

int main()
{
  App app;

  // PUT /users/{id}
  app.put("/users/{id}", [](Request &req, Response &res)
          {
        const std::string id = req.param("id");

        try {
            // Parsing with nlohmann::json for input is fine (Vix supports it internally)
            auto body = json::Json::parse(req.body());

            const std::string name  = body.value("name",  "");
            const std::string email = body.value("email", "");
            const int age           = body.value("age",   0);

            res.json({
                "action", "update",
                "status", "updated",
                "user", J::obj({
                    "id",    id,
                    "name",  name,
                    "email", email,
                    "age",   static_cast<long long>(age)
                })
            });
        }
        catch (...) {
            res.status(400).json({
                "error", "Invalid JSON"
            });
        } });

  app.run(8080);
}
