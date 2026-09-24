/**
 *
 *  @file ui_view.cpp
 *  @author Gaspard Kirira
 *
 *  Vix UI view example.
 *
 */

#include <vix/core.hpp>
#include <vix/ui.hpp>

int main()
{
  vix::App app;

  app.templates("templates");

  app.get("/", [](vix::Request &req, vix::Response &res)
          {
    (void)req;

    auto view =
        vix::ui::View("view.html")
            .set_title("Vix UI View")
            .set("framework", "Vix.cpp")
            .set("module", "vix::ui")
            .set("description", "A server-rendered UI view backed by Vix templates.");

    res.ui(view); });

  app.run(8080);
  return 0;
}
