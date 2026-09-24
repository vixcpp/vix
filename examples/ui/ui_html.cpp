/**
 *
 *  @file ui_html.cpp
 *  @author Gaspard Kirira
 *
 *  Vix UI HTML helpers example.
 *
 */

#include <vix/core.hpp>
#include <vix/ui.hpp>

#include <string>

int main()
{
  vix::App app;

  app.get("/", [](vix::Request &req, vix::Response &res)
          {
    (void)req;

    vix::ui::HtmlAttrs card_attrs;
    card_attrs.set("class", "card");

    vix::ui::HtmlAttrs title_attrs;
    title_attrs.set("class", "title");

    const std::string card =
        vix::ui::Html::tag(
            "section",
            vix::ui::Html::tag(
                "h1",
                vix::ui::Html::text("Vix UI HTML"),
                title_attrs) +
                vix::ui::Html::tag(
                    "p",
                    vix::ui::Html::text(
                        "Small HTML fragments generated safely from C++.")),
            card_attrs);

    const std::string html =
        vix::ui::Html::doctype() +
        "<html lang=\"en\">"
        "<head>"
        "<meta charset=\"utf-8\">"
        "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
        "<title>Vix UI HTML</title>"
        "<style>"
        "body{margin:0;font-family:system-ui,sans-serif;background:#0f172a;color:#e5e7eb;}"
        "main{max-width:900px;margin:0 auto;padding:56px 24px;}"
        ".card{padding:32px;border-radius:24px;background:rgba(255,255,255,.08);border:1px solid rgba(255,255,255,.12);}"
        ".title{margin:0 0 12px;color:#fff;font-size:38px;}"
        "p{color:#94a3b8;font-size:18px;}"
        "</style>"
        "</head>"
        "<body><main>" +
        card +
        "</main></body></html>";

    res.ui(vix::ui::HtmlResponse::html(html)); });

  app.run(8080);
  return 0;
}
