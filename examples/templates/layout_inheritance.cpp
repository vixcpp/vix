/**
 *
 *  @file layout_inheritance.cpp
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

  // Base layout
  loader->set(
      "base",
      "<!DOCTYPE html>\n"
      "<html>\n"
      "<head>\n"
      "  <meta charset=\"UTF-8\">\n"
      "  <title>{{ title }}</title>\n"
      "</head>\n"
      "<body>\n"
      "  <header><h1>{{ site_name }}</h1></header>\n"
      "  <main>\n"
      "    {{ content }}\n"
      "  </main>\n"
      "  <footer>© {{ year }} {{ site_name }}</footer>\n"
      "</body>\n"
      "</html>");

  // Child page content
  loader->set(
      "home_content",
      "<h2>Welcome {{ name }}</h2>\n"
      "{% if is_admin %}<p>You have admin access.</p>{% endif %}\n"
      "<ul>\n"
      "{% for item in items %}<li>{{ item }}</li>{% endfor %}\n"
      "</ul>");

  Engine engine(loader);

  // 1. Render child content
  Context content_ctx;
  content_ctx.set("name", "Gaspard");
  content_ctx.set("is_admin", true);

  Array items;
  items.emplace_back("Laptop");
  items.emplace_back("Phone");
  items.emplace_back("Book");
  content_ctx.set("items", items);

  std::string content = engine.render("home_content", content_ctx).output;

  // 2. Inject child content into base layout
  Context base_ctx;
  base_ctx.set("title", "Home");
  base_ctx.set("site_name", "Softadastra");
  base_ctx.set("year", 2026);
  base_ctx.set("content", content);

  auto result = engine.render("base", base_ctx);

  std::cout << result.output << '\n';

  return 0;
}
