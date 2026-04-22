/**
 *
 *  @file examples/http_ws/main_runtime.cpp
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
  vix::serve_http_and_ws(
      ".env",
      8080,
      [](auto &app, auto &ws)
      {
        app.get(
            "/",
            [](auto &, auto &res)
            {
              res.json({
                  {"framework", "Vix.cpp"},
                  {"mode", "runtime"},
                  {"message", "HTTP + WebSocket via serve_http_and_ws()"},
              });
            });

        app.get(
            "/hello/{name}",
            [](vix::Request &req, vix::Response &res)
            {
              res.json({
                  {"greeting", "Hello " + req.param("name")},
                  {"powered_by", "Vix.cpp"},
              });
            });

        ws.on_open(
            [&ws](auto &session)
            {
              (void)session;

              ws.broadcast_json(
                  "chat.system",
                  {
                      "user",
                      "server",
                      "text",
                      "Welcome to the runtime example",
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
              else if (type == "runtime.ping")
              {
                ws.broadcast_json(
                    "runtime.pong",
                    {
                        "status",
                        "ok",
                    });
              }
            });
      });

  return 0;
}
