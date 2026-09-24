/**
 *
 *  @file ui_dashboard.cpp
 *  @author Gaspard Kirira
 *
 *  Vix UI dashboard example.
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

    vix::ui::AssetManifest assets;

    assets
        .add_stylesheet("/assets/app.css")
        .add_script("/assets/app.js", vix::ui::AssetLoading::Deferred);

    const std::string flash =
        vix::ui::FlashMessage::success("Vix UI dashboard is running.")
            .set_title("Ready")
            .set_dismissible(true)
            .render();

    const std::string stats =
        vix::ui::Fragment::make("stats")
            .set_html(
                "<div class=\"stats\">"
                "<article><strong>12</strong><span>Routes</span></article>"
                "<article><strong>4</strong><span>Modules</span></article>"
                "<article><strong>1</strong><span>UI layer</span></article>"
                "</div>")
            .render();

    const std::string toast =
        vix::ui::Toast::info("Server-rendered UI from C++.")
            .set_title("Vix UI")
            .render();

    auto view =
        vix::ui::View("dashboard.html")
            .set_title("Vix UI Dashboard")
            .set("assets", assets.render())
            .set("flash", flash)
            .set("stats", stats)
            .set("toast", toast)
            .set("framework", "Vix.cpp")
            .set("module", "vix::ui");

    res.ui(view); });

  app.run(8080);
  return 0;
}
