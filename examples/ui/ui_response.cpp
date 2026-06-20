/**
 *
 * @file ui_response.cpp
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
 * Basic Vix UI HtmlResponse integration example.
 */

#include <vix/core.hpp>

#include <vix/ui/html/HtmlResponse.hpp>

int main()
{
  vix::App app;

  app.get("/", [](vix::http::Request &req,
                  vix::http::ResponseWrapper &res)
          {
    (void)req;

    auto html = vix::ui::HtmlResponse::html(
        "<!doctype html>"
        "<html>"
        "<head>"
        "<meta charset=\"utf-8\">"
        "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
        "<title>Vix UI Response</title>"
        "<style>"
        "body{font-family:system-ui,sans-serif;margin:40px;background:#0f172a;color:#f8fafc;}"
        ".card{max-width:720px;padding:28px;border:1px solid #334155;border-radius:16px;background:#111827;}"
        "h1{margin-top:0;color:#38bdf8;}"
        "code{background:#020617;padding:3px 6px;border-radius:6px;}"
        "</style>"
        "</head>"
        "<body>"
        "<main class=\"card\">"
        "<h1>Hello from Vix UI</h1>"
        "<p>This page is returned with <code>res.ui(HtmlResponse)</code>.</p>"
        "<p>The core response wrapper receives a UI HTML response and converts it into a native Vix HTTP response.</p>"
        "</main>"
        "</body>"
        "</html>");

    res.ui(html); });

  app.run(8080);
  return 0;
}
