/**
 *
 *  @file 02_loop_features.cpp
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
//   vix run examples/templates/02_loop_features.cpp
//
// Then open:
//   http://localhost:8080
//
// Expected template file:
//   examples/templates/views/02_loop/index.html

#include <vix.hpp>

using namespace vix;

int main()
{
  App app;
  app.templates("./examples/templates/views/02_loop");

  app.get("/", [](Request &, Response &res)
          {
            vix::template_::Context ctx;
            ctx.set("title", "Template Features");

            vix::template_::Array features;
            features.emplace_back("Blazing fast C++");
            features.emplace_back("Template engine built-in");
            features.emplace_back("Modern Jinja-style syntax");
            features.emplace_back("Simple integration with App");

            ctx.set("features", features);

            res.render("index.html", ctx); });

  app.run(8080);
}
