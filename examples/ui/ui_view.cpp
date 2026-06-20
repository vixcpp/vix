/**
 *
 * @file ui_view.cpp
 * @author Gaspard Kirira
 *
 * Copyright 2026, Gaspard Kirira.
 * All rights reserved.
 * https://github.com/vixcpp/vix
 *
 * Use of this source code is governed by a MIT license
 * that can be found in the License file.
 *
 * Vix.cpp
 *
 * Basic Vix UI View integration example.
 */

#include <vix/core.hpp>
#include <vix/ui/core/View.hpp>

int main()
{
  vix::App app;

  /*
   * Configure the template directory used by vix::view::TemplateView.
   *
   * This example expects:
   *
   *   examples/ui/templates/home.html
   */
  app.templates("examples/ui/templates");

  app.get("/", [](vix::http::Request &req,
                  vix::http::ResponseWrapper &res)
          {
    (void)req;

    auto view = vix::ui::View("home.html")
        .set_title("Vix UI View")
        .set("name", "Gaspard")
        .set("framework", "Vix.cpp")
        .set("module", "vix::ui");

    res.ui(view); });

  app.get("/about", [](vix::http::Request &req,
                       vix::http::ResponseWrapper &res)
          {
    (void)req;

    auto view = vix::ui::View("home.html")
        .set_title("About Vix UI")
        .set("name", "Builder")
        .set("framework", "Vix.cpp")
        .set("module", "server-rendered UI");

    res.ui(view); });

  app.run(8080);
  return 0;
}
