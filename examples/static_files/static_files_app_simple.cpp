/**
 *
 *  @file static_files_app_simple.cpp
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

#include <vix/middleware/static_dir_bridge.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>

using namespace vix;

static std::filesystem::path source_dir()
{
  return std::filesystem::path(__FILE__).parent_path();
}

int main()
{
  std::filesystem::path root = source_dir() / "public";
  std::filesystem::create_directories(root);

  {
    std::ofstream(root / "index.html") << "<h1>OK</h1>";
  }

  {
    std::ofstream(root / "hello.txt") << "hello";
  }

  vix::middleware::register_static_dir();

  App app;

  app.static_dir(
      root,
      "/",
      "index.html",
      true,
      "public, max-age=3600",
      true);

  app.get("/api/ping", [](Request &, Response &res)
          { res.json({"ok", true}); });

  std::cout << "Static root: " << root << "\n";
  app.run(8080);
}
