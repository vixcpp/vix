/**
 *
 *  @file examples/http_crud/delete_user.cpp
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

  // DELETE /users/{id}
  app.del("/users/{id}", [](Request &req, Response &res)
          {
        const std::string id = req.param("id");

        // In a real app you'd remove the resource from DB or memory here
        res.json({
            "action",  "delete",
            "status",  "deleted",
            "user_id", id
        }); });

  app.run(8080);
  return 0;
}
