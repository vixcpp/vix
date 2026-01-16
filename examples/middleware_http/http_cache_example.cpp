/**
 *
 *  @file examples/middleware_http/http_cache_example.cpp
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
#include <vix/middleware/app/app_middleware.hpp>

using namespace vix;

int main()
{
  App app;

  app.use("/api",
          middleware::app::http_cache(
              {.ttl_ms = 30'000,
               .allow_bypass = true,
               .bypass_header = "x-vix-cache",
               .bypass_value = "bypass"}));

  app.get("/api/users", [](Request &req, Response &res)
          { res.json({"ok", true,
                      "page", req.query_value("page", "1"),
                      "source", "origin"}); });

  app.get("/", [](Request, Response res)
          { res.send("Welcome !"); });

  app.get("/hello", [](Request, Response res)
          { res.status(200).send("Hello, World"); });

  app.run(8080);
}
