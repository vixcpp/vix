/**
 *
 *  @file examples/http_ws/main_minimal.cpp
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

#include <vix.hpp>
#include <vix/websocket/AttachedRuntime.hpp>

using namespace vix;

int main()
{
  // Use default config path "config/config.json" and port 8080
  vix::serve_http_and_ws([](auto &app, auto &ws)
                         {
        // Minimal HTTP route
        app.get("/", [](auto&, auto& res) {
            res.json({
                "message",   "Hello from Vix.cpp minimal example 👋",
                "framework", "Vix.cpp"
            });
        });

        // Minimal WebSocket handler: log and echo chat.message
        ws.on_typed_message(
            [&ws](auto& session,
                  const std::string& type,
                  const vix::json::kvs& payload)
            {
                (void)session;

                if (type == "chat.message") {
                    ws.broadcast_json("chat.message", payload);
                }
            }); });

  return 0;
}
