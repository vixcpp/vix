/**
 *
 *  @file 06_filters_and_escape.cpp
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
// Run:
//   vix run examples/templates/06_filters_and_escape.cpp
//
// Then open:
//   http://localhost:8080
//
// Expected template file:
//   examples/templates/views/06_filters/index.html

#include <vix.hpp>

using namespace vix;

int main()
{
  App app;
  app.templates("./examples/templates/views/06_filters");

  app.get("/", [](Request &, Response &res)
          {
            vix::template_::Context ctx;
            ctx.set("title", "Filters and Escaping");
            ctx.set("name", "gaspard");
            ctx.set("html", "<b>safe?</b>");

            vix::template_::Array items;
            items.emplace_back("C++");
            items.emplace_back("HTTP");
            items.emplace_back("Templates");
            ctx.set("items", items);

            res.render("index.html", ctx); });

  app.run(8080);
}
