/**
 *
 *  @file ui_live.cpp
 *  @author Gaspard Kirira
 *
 *  Vix UI live helpers example.
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

    const std::string flash =
        vix::ui::FlashMessage::success("The page was rendered from C++.")
            .set_title("Success")
            .set_dismissible(true)
            .render();

    const std::string toast =
        vix::ui::Toast::info("This is a UI toast.")
            .set_title("Vix UI")
            .render();

    const std::string fragment =
        vix::ui::Fragment::make("counter")
            .set_html("<strong>42</strong><span> live updates</span>")
            .render_wrapped();

    auto view =
        vix::ui::View("live.html")
            .set_title("Vix UI Live")
            .set("flash", flash)
            .set("toast", toast)
            .set("fragment", fragment);

    res.ui(view); });

  app.run(8080);
  return 0;
}
