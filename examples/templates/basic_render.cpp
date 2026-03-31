/**
 *
 *  @file basic_render.cpp
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
  // Create in-memory loader
  auto loader = std::make_shared<StringLoader>();

  // Register template
  loader->set("hello", "Hello {{ name }}!");

  // Create engine
  Engine engine(loader);

  // Build context
  Context ctx;
  ctx.set("name", "Gaspard");

  // Render template
  auto result = engine.render("hello", ctx);

  // Output result
  std::cout << result.output << std::endl;

  return 0;
}
