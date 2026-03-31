/**
 *
 *  @file loops_and_conditions.cpp
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
#include <vix/template/StringLoader.hpp>

using namespace vix::template_;

int main()
{
  auto loader = std::make_shared<StringLoader>();

  loader->set(
      "example",
      "{% if user %}\n"
      "User: {{ user }}\n"
      "{% endif %}\n"
      "\n"
      "Items:\n"
      "{% for item in items %}\n"
      "- {{ item }}\n"
      "{% endfor %}");

  Engine engine(loader);

  Context ctx;
  ctx.set("user", "Gaspard");

  Array items;
  items.emplace_back("Laptop");
  items.emplace_back("Phone");
  items.emplace_back("Book");
  ctx.set("items", items);

  auto result = engine.render("example", ctx);

  std::cout << result.output << '\n';

  return 0;
}
