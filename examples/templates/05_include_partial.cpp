/**
 *
 *  @file 05_include_partial.cpp
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
//   vix run examples/templates/05_include_partial.cpp
//
// Then open:
//   http://localhost:8080
//
// Expected template files:
//   examples/templates/views/05_include/index.html
//   examples/templates/views/05_include/header.html

#include <vix.hpp>

using namespace vix;

int main()
{
  App app;
  app.templates("./examples/templates/views/05_include");

  app.get("/", [](Request &, Response &res)
          {
            vix::template_::Context ctx;
            ctx.set("title", "Template Include");
            ctx.set("user", "Gaspard");

            res.render("index.html", ctx); });

  app.run(8080);
}
