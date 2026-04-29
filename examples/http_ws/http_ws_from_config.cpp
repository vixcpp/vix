/**
 *
 * @file http_ws_from_config.cpp
 * @author Gaspard Kirira
 *
 * Copyright 2026, Gaspard Kirira. All rights reserved.
 * https://github.com/vixcpp/vix
 * Use of this source code is governed by a MIT license
 * that can be found in the License file.
 *
 * Vix.cpp
 *
 */

#include <iostream>
#include <string>

#include <vix.hpp>
#include <vix/websocket/AttachedRuntime.hpp>

int main()
{
  vix::config::Config cfg{".env"};

  std::cout << "SERVER_PORT=" << cfg.getServerPort() << '\n';
  std::cout << "SERVER_TLS_ENABLED="
            << (cfg.isTlsEnabled() ? "true" : "false") << '\n';

  vix::serve_http_and_ws(
      ".env",
      cfg.getServerPort(),
      [](auto &app, auto &ws)
      {
        app.get(
            "/",
            [](auto &, auto &res)
            {
              res.json({
                  {"message", "Hello from Vix HTTP + WebSocket config example"},
                  {"runtime", "Vix.cpp"},
              });
            });

        app.get(
            "/health",
            [](auto &, auto &res)
            {
              res.text("OK");
            });

        ws.on_typed_message(
            [&ws](
                auto &session,
                const std::string &type,
                const vix::json::kvs &payload)
            {
              (void)session;

              if (type == "chat.message")
              {
                ws.broadcast_json("chat.message", payload);
              }
            });
      });

  return 0;
}
