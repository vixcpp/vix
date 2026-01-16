/**
 *
 *  @file examples/http/basic_get.cpp
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

  // GET /
  app.get("/", [](Request &, Response &res)
          { res.json({"framework", "Vix.cpp",
                      "message", "Hello from basic_get.cpp 👋"}); });

  // Start the HTTP server on port 8080
  app.run(8080);
  return 0;
}
