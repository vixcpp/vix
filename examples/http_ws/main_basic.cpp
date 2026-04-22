/**
 *
 *  @file examples/http_ws/main_basic.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.  All rights reserved.
 *  https://github.com/vixcpp/vix
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 *
 */

#include <string>

#include <vix.hpp>
#include <vix/websocket/AttachedRuntime.hpp>

int main()
{
  // Use default .env-based configuration and port 8080.
  vix::serve_http_and_ws(
      [](auto &app, auto &ws)
      {
        app.get(
            "/",
            [](auto &, auto &res)
            {
              res.json({
                  {"message", "Hello from Vix.cpp minimal example"},
                  {"framework", "Vix.cpp"},
              });
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
