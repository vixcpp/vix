/**
 *
 *  @file render_file.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.
 *  All rights reserved.
 *  https://github.com/vixcpp/vix
 *
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 *
 */
#include <iostream>
#include <memory>

#include <vix/template/Engine.hpp>
#include <vix/template/FileSystemLoader.hpp>

using namespace vix::template_;

int main()
{
  auto loader = std::make_shared<FileSystemLoader>(VIX_TEMPLATE_EXAMPLES_DIR);
  Engine engine(loader);

  Context ctx;
  ctx.set("name", "Gaspard");
  ctx.set("is_admin", true);

  Array items;
  items.emplace_back("Laptop");
  items.emplace_back("Phone");
  items.emplace_back("Book");
  ctx.set("items", items);

  auto result = engine.render("home.html", ctx);

  std::cout << result.output << '\n';

  return 0;
}
