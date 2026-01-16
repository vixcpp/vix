/**
 *
 *  @file main.cpp
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

using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request &, Response &res)
          {
            res.send("ok"); // light
          });

  app.get_heavy("/users", [](Request &, Response &res)
                {
    // DB query (heavy) -> executor
    res.send("users"); });

  app.get("/users/{id}", [](Request &req, Response &res)
          {
        auto id   = req.param("id");
        auto page = req.query_value("page", "1");

        res.json({
            "framework", "Vix.cpp",
            "id",        id,
            "page",      page
        }); });

  app.post("/echo/{id}", [](Request &req, Response &res)
           {
    const auto &body = req.json();

    res.json({
        "ok",    true,
        "echo",  body,
        "query", req.query(),
        "params", req.params()
    }); });

  app.run(8080);
}
