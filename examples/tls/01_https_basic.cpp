/**
 *
 * @file 01_https_basic.cpp
 * @author Gaspard Kirira
 *
 * Copyright 2026, Gaspard Kirira. All rights reserved.
 * https://github.com/vixcpp/vix
 * Use of this source code is governed by a MIT license
 * that can be found in the License file.
 *
 * Vix.cpp
 *
 */

#include <vix.hpp>

using namespace vix;

int main()
{
  config::Config cfg{".env"};

  App app;

  app.get("/", [](Request &, Response &res)
  {
    res.send("Hello from Vix HTTPS");
  });

  app.get("/bench", [](Request &, Response &res)
  {
    res.send("OK");
  });

  app.run(cfg);

  return 0;
}
