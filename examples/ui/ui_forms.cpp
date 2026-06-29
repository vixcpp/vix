/**
 *
 *  @file ui_forms.cpp
 *  @author Gaspard Kirira
 *
 *  Vix UI forms example.
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

    const std::string csrf =
        vix::ui::CsrfToken::make("demo-token").render();

    vix::ui::Field username = vix::ui::Field::text("username");
    username
        .set_value("gaspard")
        .set_placeholder("Username")
        .set_required(true);

    vix::ui::Field email = vix::ui::Field::email("email");
    email
        .set_placeholder("Email address")
        .set_required(true);

    vix::ui::Field country = vix::ui::Field::select("country");
    country
        .add_option("ug", "Uganda")
        .add_option("cd", "DRC")
        .add_option("rw", "Rwanda");

    const std::string html =
        "<!doctype html>"
        "<html lang=\"en\">"
        "<head>"
        "<meta charset=\"utf-8\">"
        "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
        "<title>Vix UI Forms</title>"
        "<style>"
        "body{margin:0;font-family:system-ui,sans-serif;background:#0f172a;color:#e5e7eb;}"
        "main{max-width:760px;margin:0 auto;padding:56px 24px;}"
        "form{display:grid;gap:16px;padding:32px;border-radius:24px;background:rgba(255,255,255,.08);border:1px solid rgba(255,255,255,.12);}"
        "input,select{width:100%;box-sizing:border-box;padding:12px 14px;border-radius:12px;border:1px solid rgba(255,255,255,.18);background:#111827;color:white;}"
        "button{padding:12px 16px;border:0;border-radius:12px;background:#f37726;color:white;font-weight:700;}"
        "h1{margin-top:0;color:white;}"
        "</style>"
        "</head>"
        "<body>"
        "<main>"
        "<form method=\"post\">"
        "<h1>Vix UI Forms</h1>" +
        csrf +
        username.render() +
        email.render() +
        country.render() +
        "<button type=\"submit\">Submit</button>"
        "</form>"
        "</main>"
        "</body>"
        "</html>";

    res.ui(vix::ui::HtmlResponse::html(html)); });

  app.run(8080);
  return 0;
}
