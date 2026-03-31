/**
 *
 *  @file includes.cpp
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
#include <string>

#include <vix/template/Engine.hpp>
#include <vix/template/StringLoader.hpp>

using namespace vix::template_;

int main()
{
  auto loader = std::make_shared<StringLoader>();

  // Simulate partials
  loader->set("header", "<header><h1>{{ title }}</h1></header>");
  loader->set("footer", "<footer><p>© {{ year }}</p></footer>");

  // Main template (manual include simulation)
  loader->set(
      "page",
      "{{ header }}\n"
      "<main>\n"
      "  <p>{{ content }}</p>\n"
      "</main>\n"
      "{{ footer }}");

  Engine engine(loader);

  // Render header separately
  Context header_ctx;
  header_ctx.set("title", "My Page");
  std::string header = engine.render("header", header_ctx).output;

  // Render footer separately
  Context footer_ctx;
  footer_ctx.set("year", 2026);
  std::string footer = engine.render("footer", footer_ctx).output;

  // Final page context
  Context page_ctx;
  page_ctx.set("header", header);
  page_ctx.set("footer", footer);
  page_ctx.set("content", "Welcome to the page!");

  auto result = engine.render("page", page_ctx);

  std::cout << result.output << '\n';

  return 0;
}
