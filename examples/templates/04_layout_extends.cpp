/**
 *
 *  @file 04_layout_extends.cpp
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
//   vix run examples/templates/04_layout_extends.cpp
//
// Then open:
//   http://localhost:8080
//
// Expected template files:
//   examples/templates/views/04_extends/base.html
//   examples/templates/views/04_extends/index.html

#include <vix.hpp>

using namespace vix;

int main()
{
  App app;
  app.templates("./examples/templates/views/04_extends");

  app.get("/", [](Request &, Response &res)
          {
            vix::template_::Context ctx;
            ctx.set("title", "Template Inheritance");
            ctx.set("user", "Gaspard");

            res.render("index.html", ctx); });

  app.run(8080);
}
