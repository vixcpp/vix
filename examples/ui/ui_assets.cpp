/**
 *
 *  @file ui_assets.cpp
 *  @author Gaspard Kirira
 *
 *  Vix UI assets example.
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

    vix::ui::AssetManifest manifest;

    manifest
        .add_stylesheet("/assets/app.css")
        .add_script("/assets/app.js", vix::ui::AssetLoading::Deferred);

    const std::string html =
        "<!doctype html>"
        "<html lang=\"en\">"
        "<head>"
        "<meta charset=\"utf-8\">"
        "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
        "<title>Vix UI Assets</title>" +
        manifest.render() +
        "<style>"
        "body{margin:0;font-family:system-ui,sans-serif;background:#111827;color:#e5e7eb;}"
        "main{max-width:900px;margin:0 auto;padding:56px 24px;}"
        ".card{padding:32px;border-radius:24px;background:rgba(255,255,255,.08);border:1px solid rgba(255,255,255,.12);}"
        "h1{margin:0 0 12px;color:white;}"
        "code{color:#fbbf24;}"
        "</style>"
        "</head>"
        "<body>"
        "<main>"
        "<section class=\"card\">"
        "<h1>Vix UI Assets</h1>"
        "<p>AssetManifest can render stylesheets, scripts, images and fonts.</p>"
        "<p>Loaded resource: <code>/assets/app.css</code></p>"
        "</section>"
        "</main>"
        "</body>"
        "</html>";

    res.ui(vix::ui::HtmlResponse::html(html)); });

  app.run(8080);
  return 0;
}
