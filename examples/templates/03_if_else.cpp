/**
 *
 *  @file 03_if_else.cpp
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
//   vix run examples/templates/03_if_else.cpp
//
// Then open:
//   http://localhost:8080
//
// Expected template file:
//   examples/templates/views/03_if/index.html

#include <vix.hpp>

using namespace vix;

int main()
{
  App app;
  app.templates("./examples/templates/views/03_if");

  app.get("/", [](Request &, Response &res)
          {
            vix::template_::Context ctx;
            ctx.set("title", "Conditional Rendering");
            ctx.set("logged_in", true);
            ctx.set("user", "Gaspard");

            res.render("index.html", ctx); });

  app.run(8080);
}
