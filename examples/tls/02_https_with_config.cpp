/**
 *
 * @file 02_https_with_config.cpp
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

#include <iostream>
#include <vix.hpp>

using namespace vix;

int main()
{
  config::Config cfg{".env"};

  std::cout << "SERVER_PORT=" << cfg.getServerPort() << '\n';
  std::cout << "SERVER_TLS_ENABLED="
            << (cfg.isTlsEnabled() ? "true" : "false") << '\n';

  App app;

  app.get("/", [](Request &, Response &res)
  {
    res.send("Hello from Vix HTTPS config example");
  });

  app.get("/health", [](Request &, Response &res)
  {
    res.send("OK");
  });

  app.run(cfg);

  return 0;
}
