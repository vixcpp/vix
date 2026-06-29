/**
 *
 *  @file ui_pwa.cpp
 *  @author Gaspard Kirira
 *
 *  Vix UI PWA helpers example.
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

    const std::string pwa_meta =
        vix::ui::PwaMeta::mobile_app("Vix UI PWA", "#0f172a")
            .render();

    const std::string safe_area =
        vix::ui::SafeArea::all().render();

    auto view =
        vix::ui::View("pwa.html")
            .set_title("Vix UI PWA")
            .set("pwa_meta", pwa_meta)
            .set("safe_area", safe_area);

    res.ui(view); });

  app.get("/manifest.webmanifest", [](vix::Request &req, vix::Response &res)
          {
    (void)req;

    const std::string manifest =
        vix::ui::WebAppManifest::app("Vix UI PWA", "Vix UI")
            .set_description("A PWA metadata example powered by vix::ui.")
            .set_theme_color("#0f172a")
            .set_background_color("#0f172a")
            .render();

    res.ui(
        vix::ui::HtmlResponse::html(manifest)
            .set_content_type("application/manifest+json")); });

  app.run(8080);
  return 0;
}
