/**
 *
 *  @file examples/http_ws/main_chat.cpp
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
      [](vix::App &app, auto &ws)
      {
        app.get(
            "/",
            [](vix::Request &, vix::Response &res)
            {
              res.json({
                  {"name", "Vix Chat Example"},
                  {"description", "HTTP + WebSocket powered chat server"},
                  {"framework", "Vix.cpp"},
              });
            });

        app.get(
            "/health",
            [](auto &, auto &res)
            {
              res.json({
                  {"status", "ok"},
                  {"service", "chat"},
                  {"version", "1.0.0"},
              });
            });

        // WebSocket protocol:
        //
        // {
        //   "type": "chat.join" | "chat.message" | "chat.typing",
        //   "payload": {
        //     "user": "Alice",
        //     "text": "...",
        //     "room": "general"
        //   }
        // }

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
                      "A new user connected to the chat",
                  });
            });

        ws.on_typed_message(
            [&ws](
                auto &session,
                const std::string &type,
                const vix::json::kvs &payload)
            {
              (void)session;

              if (type == "chat.join")
              {
                const nlohmann::json json_payload =
                    vix::websocket::detail::ws_kvs_to_nlohmann(payload);

                const std::string user = json_payload.value("user", "anonymous");

                ws.broadcast_json(
                    "chat.system",
                    {
                        "user",
                        user,
                        "text",
                        user + " joined the chat",
                    });
              }
              else if (type == "chat.message")
              {
                ws.broadcast_json("chat.message", payload);
              }
              else if (type == "chat.typing")
              {
                ws.broadcast_json("chat.typing", payload);
              }
              else
              {
                ws.broadcast_json(
                    "chat.unknown",
                    {
                        "info",
                        "Unknown message type",
                        "type",
                        type,
                    });
              }
            });
      });

  return 0;
}
